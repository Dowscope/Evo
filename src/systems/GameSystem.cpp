#include "systems/GameSystem.hpp"

#include "persistence/Persistence.hpp"
#include "systems/ScreenSystem.hpp"

#include <charconv>
#include <stdexcept>
#include <string>

GameSystem::GameSystem() : System("Game") {}

GameSystem::~GameSystem() {
    if (_persistence != nullptr) {
        _saveState();
    }
}

void GameSystem::init() {
    if (_renderTarget == nullptr || _persistence == nullptr) {
        throw std::runtime_error(
            "GameSystem requires registered rendering and persistence"
        );
    }
    if (const auto savedCount = _persistence->get("game.update_count")) {
        const auto [end, error] = std::from_chars(
            savedCount->data(),
            savedCount->data() + savedCount->size(),
            _updateCount
        );
        if (error != std::errc{} || end != savedCount->data() + savedCount->size()) {
            _updateCount = 0;
        }
    }
    _createLand();
    System::init();
}

void GameSystem::registerRenderTarget(RenderTarget& renderTarget) {
    _renderTarget = &renderTarget;
}

void GameSystem::registerPersistence(Persistence& persistence) {
    _persistence = &persistence;
}

void GameSystem::update() {
    ++_updateCount;
    constexpr auto checkpointInterval = std::chrono::seconds(5);
    const auto now = std::chrono::steady_clock::now();
    if (now - _lastCheckpoint >= checkpointInterval) {
        _saveState();
        _persistence->checkpoint();
        _lastCheckpoint = now;
    }
}

void GameSystem::render() {
    _renderTarget->render(_land);
}

void GameSystem::_saveState() {
    _persistence->set("game.update_count", std::to_string(_updateCount));
}

void GameSystem::_createLand() {
    constexpr float size = 5.0F;
    constexpr glm::vec3 dirt{0.38F, 0.20F, 0.08F};
    _land.vertices = {
        {{-size, 0.0F, -size}, dirt},
        {{ size, 0.0F, -size}, dirt},
        {{ size, 0.0F,  size}, dirt},
        {{-size, 0.0F,  size}, dirt},
    };
    _land.indices = {0, 1, 2, 2, 3, 0};
}
