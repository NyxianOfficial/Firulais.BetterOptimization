#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../SettingsState.hpp"

using namespace geode::prelude;

// Definición de la variable global
bool novehicleparticles = false;

namespace firulais::optimizations::Player {

    void hideVehicleParticles(GJBaseGameLayer* bgl) {
        if (!::novehicleparticles) return;
        
        auto hide_particles = [](PlayerObject* player) {
            if (!player) return;
            player->m_playerGroundParticles->setVisible(false);
            player->m_ufoClickParticles->setVisible(false);
            player->m_dashParticles->setVisible(false);
            player->m_robotBurstParticles->setVisible(false);
            player->m_trailingParticles->setVisible(false);
            player->m_shipClickParticles->setVisible(false);
            player->m_vehicleGroundParticles->setVisible(false);
            player->m_landParticles0->setVisible(false);
            player->m_landParticles1->setVisible(false);
            player->m_swingBurstParticles1->setVisible(false);
            player->m_swingBurstParticles2->setVisible(false);
        };

        hide_particles(bgl->m_player1);
        hide_particles(bgl->m_player2);
    }

    class $modify(NoVehicleParticlesMod, GJBaseGameLayer) {
        void createPlayer() {
            GJBaseGameLayer::createPlayer();
            hideVehicleParticles(this);
        }
    };
}

$on_mod(Loaded) {
	novehicleparticles = geode::Mod::get()->getSettingValue<bool>("novehicleparticles");
	listenForSettingChanges("novehicleparticles", [](bool enable) {
		novehicleparticles = enable;
	});
}
