#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

// Click Buffer
// You press early -> we remember the press for N physics frames.
// The first frame the player is touching an orb/ring that has not been
// activated yet, we re-send the click so it lands on that exact frame.

class $modify(BufferGJBGL, GJBaseGameLayer) {
	struct Fields {
		int framesLeft[2] = {0, 0}; // remaining buffer per player
		bool held[2] = {false, false}; // is the jump button still down
		bool injecting = false; // stops our own click from re-arming the buffer
	};

	bool isLivePlayLayer() {
		return PlayLayer::get() && static_cast<GJBaseGameLayer*>(PlayLayer::get()) == this;
	}

	void handleButton(bool down, int button, bool isPlayer1) {
		GJBaseGameLayer::handleButton(down, button, isPlayer1);

		if (button != 1) return; // 1 = jump
		if (m_fields->injecting) return;
		if (!Mod::get()->getSettingValue<bool>("enabled")) return;
		if (!this->isLivePlayLayer()) return;

		int i = isPlayer1 ? 0 : 1;
		if (down) {
			m_fields->held[i] = true;
			m_fields->framesLeft[i] = static_cast<int>(Mod::get()->getSettingValue<int64_t>("window"));
		}
		else {
			m_fields->held[i] = false;
			m_fields->framesLeft[i] = 0;
		}
	}

	bool touchingFreshRing(PlayerObject* player, bool isPlayer1) {
		if (!player || !player->m_touchingRings) return false;
		for (auto* obj : CCArrayExt<GameObject*>(player->m_touchingRings)) {
			bool used = isPlayer1 ? obj->m_hasBeenActivated : obj->m_hasBeenActivatedP2;
			if (!used) return true;
		}
		return false;
	}

	void processCommands(float dt) {
		if (Mod::get()->getSettingValue<bool>("enabled") && this->isLivePlayLayer()) {
			for (int i = 0; i < 2; i++) {
				if (!m_fields->held[i] || m_fields->framesLeft[i] <= 0) continue;

				bool isP1 = (i == 0);
				auto* player = isP1 ? m_player1 : m_player2;

				if (this->touchingFreshRing(player, isP1)) {
					// re-click on this exact frame
					m_fields->injecting = true;
					GJBaseGameLayer::handleButton(false, 1, isP1);
					GJBaseGameLayer::handleButton(true, 1, isP1);
					m_fields->injecting = false;
					m_fields->framesLeft[i] = 0;
				}
				else {
					m_fields->framesLeft[i]--;
				}
			}
		}
		GJBaseGameLayer::processCommands(dt);
	}
};

// Keeps assisted runs from submitting.
class $modify(BufferPlayLayer, PlayLayer) {
	void levelComplete() {
		if (Mod::get()->getSettingValue<bool>("enabled")
			&& Mod::get()->getSettingValue<bool>("block-completion")) {
			m_isTestMode = true;
		}
		PlayLayer::levelComplete();
	}
};
