#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

constexpr float MIN_TPS = 0.f;
constexpr float MAX_TPS = 100000.f;

#ifdef GEODE_IS_MACOS
#define REQUIRE_PATCH
#endif

// Variables globales para el estado del TPS bypass
bool tpsBypassEnabled = false;
int tpsValue = 240;

// Almacena punteros a los parches propiedad del mod
std::vector<Patch*> activePatchPointers;

#ifdef REQUIRE_PATCH
// Función para convertir el valor de TPS a bytes para el parche
#ifdef GEODE_IS_INTEL_MAC
template <typename T>
[[nodiscard]] static std::vector<uint8_t> TPStoBytes(int tps) {
    if constexpr (std::is_same_v<T, float>) {
        return geode::toBytes(1.f / static_cast<float>(tps));
    } else if constexpr (std::is_same_v<T, double>) {
        return geode::toBytes<double>(1.0 / static_cast<double>(tps));
    } else {
        static_assert(alwaysFalse<T>, "TPStoBytes only supports float and double");
    }
}
#elif defined(GEODE_IS_ARM_MAC)
template <typename T>
[[nodiscard]] static std::vector<uint8_t> TPStoBytes(int tps) {
    if constexpr (std::is_same_v<T, float>) {
        auto bytes = assembler::arm64::mov_float(
            assembler::arm64::Register::w9,
            1.f / static_cast<float>(tps)
        );
        return std::vector(bytes.begin(), bytes.end());
    } else if constexpr (std::is_same_v<T, double>) {
        auto bytes = assembler::arm64::mov_double(
            assembler::arm64::Register::x9,
            1.0 / static_cast<double>(tps)
        );
        return std::vector(bytes.begin(), bytes.end());
    } else {
        static_assert(alwaysFalse<T>, "TPStoBytes only supports float and double");
    }
}
#endif
#endif

// Función para actualizar los parches de TPS
void updateTPSPatches(bool enabled, int tps) {
    // Limpiar parches antiguos
    auto mod = Mod::get();
    for (auto patch : activePatchPointers) {
        mod->disownPatch(patch);
    }
    activePatchPointers.clear();

    #ifdef REQUIRE_PATCH
    if (enabled) {
        auto base = reinterpret_cast<uint8_t*>(geode::base::get());
        auto moduleSize = utils::getBaseSize();
        geode::log::debug("TPSBypass: base = 0x{:X}", (uintptr_t)base);
        geode::log::debug("TPSBypass: moduleSize = 0x{:X}", moduleSize);

        using namespace sinaps::mask;

        #ifdef GEODE_IS_INTEL_MAC
        // En x86, necesitamos reemplazar dos valores que se usan en GJBaseGameLayer::getModifiedDelta
        auto floatAddr = sinaps::find<dword<0x3B888889>>(base, moduleSize);
        auto doubleAddr = sinaps::find<qword<0x3F71111120000000>>(base, moduleSize);
        #elif defined(GEODE_IS_ARM_MAC)
        // En ARM, es un poco complicado, ya que el valor no se almacena como una constante
        auto floatAddr = sinaps::find<"29 11 91 52 09 71 A7 72">(base, moduleSize);
        auto doubleAddr = sinaps::find<"09 00 A4 D2 29 22 C2 F2 29 EE E7 F2">(base, moduleSize);
        #endif

        if (floatAddr == -1 || doubleAddr == -1) {
            geode::log::error("TPSBypass: failed to find addresses: float = 0x{:X}, double = 0x{:X}", floatAddr, doubleAddr);
            return;
        }

        geode::log::debug("TPSBypass: floatAddr = 0x{:X}", floatAddr);
        geode::log::debug("TPSBypass: doubleAddr = 0x{:X}", doubleAddr);

        auto patch1Res = mod->patch(floatAddr + base, TPStoBytes<float>(tps));
        if (!patch1Res) {
            geode::log::error("TPSBypass: failed to patch float address: {}", patch1Res.unwrapErr());
            return;
        }
        activePatchPointers.push_back(patch1Res.unwrap());

        auto patch2Res = mod->patch(doubleAddr + base, TPStoBytes<double>(tps));
        if (!patch2Res) {
            geode::log::error("TPSBypass: failed to patch double address: {}", patch2Res.unwrapErr());
            return;
        }
        activePatchPointers.push_back(patch2Res.unwrap());
    }
    #endif
}

// Registrar el TPS Bypass
$on_mod(Loaded) {
    tpsBypassEnabled = Mod::get()->getSettingValue<bool>("enable-tps-bypass");
    tpsValue = Mod::get()->getSettingValue<int>("tps-bypass");

    updateTPSPatches(tpsBypassEnabled, tpsValue);

    listenForSettingChanges("enable-tps-bypass", [](bool enabled) {
        tpsBypassEnabled = enabled;
        updateTPSPatches(tpsBypassEnabled, tpsValue);
    });

    listenForSettingChanges("tps-bypass", [](int value) {
        tpsValue = value;
        updateTPSPatches(tpsBypassEnabled, tpsValue);
    });
}

class $modify(TPSBypassGJBGLHook, GJBaseGameLayer) {
    struct Fields {
        double m_extraDelta = 0.0;
        float m_realDelta = 0.0;
        bool m_shouldHide = false;
        bool m_shouldBufferPostUpdate = false;
        bool m_isEditor = utils::get<LevelEditorLayer>() != nullptr;
        bool m_shouldBreak = false;
        bool m_postRestart = false;
    };

    float getCustomDelta(float dt, float tps, bool applyExtraDelta = true) {
        auto spt = 1.f / tps;

        if (applyExtraDelta && m_resumeTimer > 0) {
            --m_resumeTimer;
            dt = 0.f;
        }

        auto totalDelta = dt + m_extraDelta;
        auto timestep = std::min(m_gameState.m_timeWarp, 1.f) * spt;
        auto steps = std::round(totalDelta / timestep);
        auto newDelta = steps * timestep;
        if (applyExtraDelta) m_extraDelta = totalDelta - newDelta;
        return static_cast<float>(newDelta);
    }

    #ifndef REQUIRE_PATCH
    float getModifiedDelta(float dt) {
        return getCustomDelta(dt, static_cast<float>(tpsValue));
    }
    #endif

    bool shouldContinue(const Fields* fields) const {
        if (!fields->m_isEditor) return true;

        // in editor, player hitbox is removed from section when it dies,
        // so we need to check if it's still there
        return !fields->m_shouldBreak;
    }

    void update(float dt) override {
        auto fields = m_fields.self();

        // to prevent the lag after restarting the level, we will call just skip this iteration (and reset the spillover delta)
        if (fields->m_postRestart) {
            fields->m_postRestart = false;
            fields->m_extraDelta = 0.0;
            return;
        }

        // If TPS bypass is not enabled, call the original update function
        if (!tpsBypassEnabled) {
            GJBaseGameLayer::update(dt);
            return;
        }

        fields->m_extraDelta += dt;
        fields->m_shouldBreak = false;

        // store current frame delta for later use in updateVisibility
        fields->m_realDelta = getCustomDelta(fields->m_extraDelta, 240.f, false);

        auto newTPS = static_cast<float>(tpsValue);

        // extra divisor, to make the iterations smoother
        auto divisor = std::max(1.0, newTPS / 240.0);

        auto newDelta = 1.0 / newTPS / divisor;

        if (fields->m_extraDelta >= newDelta) {
            // call original update several times, until the extra delta is less than the new delta
            size_t steps = fields->m_extraDelta / newDelta;
            fields->m_extraDelta -= steps * newDelta;

            auto start = utils::getTimestamp();
            auto ms = dt * 1000;
            fields->m_shouldHide = true;
            while (steps > 1 && shouldContinue(fields)) {
                GJBaseGameLayer::update(newDelta);
                --steps;

                // if the update took too long, break out of the loop
                auto end = utils::getTimestamp();
                if (end - start > ms) {
                    // re-add the remaining delta
                    fields->m_extraDelta += steps * newDelta;
                    fields->m_shouldHide = false;
                    goto END; // don't feel like adding a bool to skip the last update
                }
            }
            fields->m_shouldHide = false;

            if (shouldContinue(fields)) {
                // call one last time with the remaining delta
                GJBaseGameLayer::update(newDelta * steps);
            }

            END: if (fields->m_shouldBufferPostUpdate) {
                fields->m_shouldBufferPostUpdate = false;
                reinterpret_cast<PlayLayer*>(this)->postUpdate(fields->m_realDelta);
            }
        }
    }
};

inline TPSBypassGJBGLHook::Fields* getFields(GJBaseGameLayer* self) {
    return reinterpret_cast<TPSBypassGJBGLHook*>(self)->m_fields.self();
}

// Skip some functions to make the game run faster during extra updates period

class $modify(TPSBypassPLHook, PlayLayer) {
    // PlayLayer postUpdate handles practice mode checkpoints, labels and also calls updateVisibility
    void postUpdate(float dt) override {
        auto fields = getFields(this);
        if (fields->m_shouldHide) {
            fields->m_shouldBufferPostUpdate = true;
            return;
        }

        fields->m_shouldBufferPostUpdate = false;
        PlayLayer::postUpdate(fields->m_realDelta);
    }

    void resetLevel() {
        auto fields = getFields(this);
        fields->m_postRestart = true;
        PlayLayer::resetLevel();
    }

    // we also would like to fix the percentage calculation, which uses constant 240 TPS to determine the progress
    int calculationFix() {
        auto timestamp = m_level->m_timestamp;
        auto currentProgress = m_gameState.m_currentProgress;
        // this is only an issue for 2.2+ levels (with TPS greater than 240)
        if (timestamp > 0 && tpsBypassEnabled && tpsValue > 240) {
            // recalculate m_currentProgress based on the actual time passed
            auto progress = utils::getActualProgress(this);
            m_gameState.m_currentProgress = timestamp * progress / 100.f;
        }
        return currentProgress;
    }

    void updateProgressbar() {
        auto currentProgress = calculationFix();
        PlayLayer::updateProgressbar();
        m_gameState.m_currentProgress = currentProgress;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) override {
        auto currentProgress = calculationFix();
        PlayLayer::destroyPlayer(player, object);
        m_gameState.m_currentProgress = currentProgress;
    }
    
    void levelComplete() {
        auto oldTimestamp = m_gameState.m_unkUint2;
        if (tpsBypassEnabled && tpsValue > 240) {
            auto ticks = static_cast<uint32_t>(std::round(m_gameState.m_levelTime * 240));
            m_gameState.m_unkUint2 = ticks;
        }
        PlayLayer::levelComplete();
        m_gameState.m_unkUint2 = oldTimestamp;
    }
};

class $modify(TPSBypassLELHook, LevelEditorLayer) {
    void postUpdate(float dt) override {
        auto fields = getFields(this);
        if (fields->m_shouldHide && !m_player1->m_maybeIsColliding && !m_player2->m_maybeIsColliding) return;
        fields->m_shouldBreak = m_player1->m_maybeIsColliding || m_player2->m_maybeIsColliding;
        LevelEditorLayer::postUpdate(fields->m_realDelta);
    }
}
