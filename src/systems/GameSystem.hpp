#pragma once

#include "systems/System.hpp"
#include "rendering/Scene.hpp"

#include <chrono>
#include <cstdint>

class Persistence;
class RenderTarget;

class GameSystem final : public System {
public:
    GameSystem();
    ~GameSystem() override;

    void init() override;
    void registerRenderTarget(RenderTarget& renderTarget);
    void registerPersistence(Persistence& persistence);
    void update();
    void render();

private:
    void _saveState();
    void _createLand();

    RenderTarget* _renderTarget = nullptr;
    Persistence* _persistence = nullptr;
    Land _land;
    std::uint64_t _updateCount = 0;
    std::chrono::steady_clock::time_point _lastCheckpoint =
        std::chrono::steady_clock::now();
};
