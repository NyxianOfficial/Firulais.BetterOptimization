#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#ifndef GEODE_IS_WINDOWS
#include <Geode/modify/GameManager.hpp>
#else
#include <Geode/modify/PlayerObject.hpp>
#endif

#include "../SettingsState.hpp"

using namespace geode::prelude;

namespace firulais::hacks {
    inline void disableParticles(GJBaseGameLayer* gjbgl) {
        if (!gjbgl) return;

        if (gjbgl->m_unk3238)
            gjbgl->m_unk3238->setVisible(false);

        if (gjbgl->m_particlesDict) {
            for (const auto& [_, array] : geode::cocos::CCDictionaryExt<gd::string, cocos2d::CCArray*>(gjbgl->m_particlesDict)) {
                for (auto* p : geode::cocos::CCArrayExt<cocos2d::CCParticleSystemQuad*>(array))
                    p->setVisible(false);
            }
        }

        if (gjbgl->m_claimedParticles) {
            for (const auto& [_, array] : geode::cocos::CCDictionaryExt<gd::string, cocos2d::CCArray*>(gjbgl->m_claimedParticles)) {
                for (auto* p : geode::cocos::CCArrayExt<cocos2d::CCParticleSystemQuad*>(array))
                    p->setVisible(false);
            }
        }

        if (gjbgl->m_unclaimedParticles) {
            for (auto* p : geode::cocos::CCArrayExt<cocos2d::CCParticleSystemQuad*>(gjbgl->m_unclaimedParticles))
                p->setVisible(false);
        }

        if (gjbgl->m_customParticles) {
            for (const auto& [_, array] : geode::cocos::CCDictionaryExt<gd::string, cocos2d::CCArray*>(gjbgl->m_customParticles)) {
                for (auto* p : geode::cocos::CCArrayExt<cocos2d::CCParticleSystemQuad*>(array))
                    p->setVisible(false);
            }
        }
    }
}

class $modify(NoParticlesHook, GJBaseGameLayer) {
    cocos2d::CCParticleSystemQuad* spawnParticle(char const* plist, int zOrder, cocos2d::tCCPositionType positionType, cocos2d::CCPoint position) {
        return nullptr;
    }

#ifndef GEODE_IS_WINDOWS
    void playSpeedParticle(float speed) {
        auto gm = GameManager::sharedState();
        bool original = gm->m_performanceMode;
        gm->m_performanceMode = true;
        GJBaseGameLayer::playSpeedParticle(speed);
        gm->m_performanceMode = original;
    }
#endif

    void update(float dt) {
        GJBaseGameLayer::update(dt);
        if (noParticlesEnabled)
            firulais::hacks::disableParticles(this);
    }
};

#ifdef GEODE_IS_WINDOWS
class $modify(NoParticlesWinHook, PlayerObject) {
    void updateTimeMod(float p0, bool p1) {
        auto gm = GameManager::sharedState();
        bool original = gm->m_performanceMode;
        gm->m_performanceMode = true;
        PlayerObject::updateTimeMod(p0, p1);
        gm->m_performanceMode = original;
    }
};
#endif

// Definición de la variable global
bool noParticlesEnabled = false;

$on_mod(Loaded) {
	noParticlesEnabled = geode::Mod::get()->getSettingValue<bool>("noparticles");
	listenForSettingChanges("noparticles", [](bool enable) {
		noParticlesEnabled = enable;
	});
}
