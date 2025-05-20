#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../SettingsState.hpp"

using namespace geode::prelude;

// el portal espejo es muy malo y feo para mi y
//el gameplay es muy horrible por eso recomiendo quitarlo

// Definición de la variable global
bool nomirror = false;

namespace firulais::hacks::Level {
    
    class $modify(NoMirrorMod, GJBaseGameLayer) {
        void toggleFlipped(bool flipped, bool idk) {
            if (::nomirror) return;
            GJBaseGameLayer::toggleFlipped(flipped, idk);
        }
    };
}

$on_mod(Loaded) {
	nomirror = geode::Mod::get()->getSettingValue<bool>("nomirror");
	listenForSettingChanges("nomirror", [](bool enable) {
		nomirror = enable;
	});
}


