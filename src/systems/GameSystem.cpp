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
    constexpr float depth = 1.0F;
    constexpr glm::vec3 topDirt{0.42F, 0.23F, 0.09F};
    constexpr glm::vec3 sideDirt{0.28F, 0.13F, 0.045F};
    constexpr glm::vec3 bottomDirt{0.20F, 0.085F, 0.025F};
    _land.vertices = {
        // Top
        {{-size, 0.0F, -size}, topDirt},
        {{ size, 0.0F, -size}, topDirt},
        {{ size, 0.0F,  size}, topDirt},
        {{-size, 0.0F,  size}, topDirt},
        // Front
        {{-size, 0.0F,  size}, sideDirt},
        {{ size, 0.0F,  size}, sideDirt},
        {{ size, -depth, size}, sideDirt},
        {{-size, -depth, size}, sideDirt},
        // Back
        {{ size, 0.0F, -size}, sideDirt},
        {{-size, 0.0F, -size}, sideDirt},
        {{-size, -depth, -size}, sideDirt},
        {{ size, -depth, -size}, sideDirt},
        // Left
        {{-size, 0.0F, -size}, sideDirt},
        {{-size, 0.0F,  size}, sideDirt},
        {{-size, -depth, size}, sideDirt},
        {{-size, -depth, -size}, sideDirt},
        // Right
        {{ size, 0.0F,  size}, sideDirt},
        {{ size, 0.0F, -size}, sideDirt},
        {{ size, -depth, -size}, sideDirt},
        {{ size, -depth, size}, sideDirt},
        // Bottom
        {{-size, -depth, size}, bottomDirt},
        {{ size, -depth, size}, bottomDirt},
        {{ size, -depth, -size}, bottomDirt},
        {{-size, -depth, -size}, bottomDirt},
    };
    _land.indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20,
    };
}
