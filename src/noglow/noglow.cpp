#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../SettingsState.hpp"

using namespace geode::prelude;

// Definición de la variable global
bool noGlowEnabled = false;

namespace firulais::hacks::Level {

    class $modify(NoGlowPLHook, PlayLayer) {
    public:
        void addObject(GameObject* obj) {
            if (::noGlowEnabled) {
                obj->m_hasNoGlow = true;
            }
            PlayLayer::addObject(obj);
        }
    };

}

$on_mod(Loaded) {
    noGlowEnabled = geode::Mod::get()->getSettingValue<bool>("noglow");
    listenForSettingChanges<bool>("noglow", [](bool enable) {
        noGlowEnabled = enable;
    });
}
