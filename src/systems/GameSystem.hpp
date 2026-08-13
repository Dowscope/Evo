#pragma once

#include "systems/System.hpp"

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
    void saveState();

    RenderTarget* renderTarget_ = nullptr;
    Persistence* persistence_ = nullptr;
    std::uint64_t updateCount_ = 0;
    std::chrono::steady_clock::time_point lastCheckpoint_ =
        std::chrono::steady_clock::now();
};
