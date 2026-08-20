#pragma once

#include "systems/System.hpp"
#include "rendering/Scene.hpp"
#include "core/ApplicationConfig.hpp"

#include <chrono>
#include <cstdint>

class Persistence;
class RenderTarget;

class GameSystem final : public System {
public:
    explicit GameSystem(WorldConfig config);
    ~GameSystem() override;

    void init() override;
    void registerRenderTarget(RenderTarget& renderTarget);
    void registerPersistence(Persistence& persistence);
    void update();
    void render();

private:
    void _saveState();
    void _createLand();
    void _updateSun(float elapsedSeconds);
    void _resolveWorldSeed();

    RenderTarget* _renderTarget = nullptr;
    Persistence* _persistence = nullptr;
    WorldConfig _config;
    Land _land;
    Sun _sun;
    float _sunAngle = 0.0F;
    std::uint64_t _updateCount = 0;
    std::chrono::steady_clock::time_point _lastUpdate =
        std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point _lastCheckpoint =
        std::chrono::steady_clock::now();
};
