#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/utils/general.hpp>

using namespace geode::prelude;

constexpr float MIN_FPS = 1.f;// es como me va mi aiphon 99, 60 fps en un celular caché es una mierda
constexpr float MAX_FPS = 100000.f;//100k fps es mucho de hecho
// explotara los telefonos de gama baja

bool fpsBypassEnabled = false; 
float fpsBypassValue = 60.f;

// Aplica el intervalo de frames
void updateFPSBypass() {
    auto* gm = GameManager::get();
    float targetFPS = std::clamp(fpsBypassValue, MIN_FPS, MAX_FPS);
    gm->m_customFPSTarget = targetFPS;

    float interval = 1.f / (fpsBypassEnabled ? targetFPS : 60.f);
    cocos2d::CCDirector::sharedDirector()->setAnimationInterval(interval);
}

$on_mod(Loaded) {
    fpsBypassEnabled = Mod::get()->getSettingValue<bool>("enable-fps-bypass");
    fpsBypassValue = Mod::get()->getSettingValue<float>("fps-bypass");

    updateFPSBypass();

    listenForSettingChanges("enable-fps-bypass", [](bool enabled) {
        fpsBypassEnabled = enabled;
        updateFPSBypass();
    });

    listenForSettingChanges("fps-bypass", [](float value) {
        fpsBypassValue = value;
        updateFPSBypass();
    });
}
