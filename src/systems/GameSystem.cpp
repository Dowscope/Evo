#include "systems/GameSystem.hpp"

#include "persistence/Persistence.hpp"
#include "core/Logger.hpp"
#include "systems/ScreenSystem.hpp"

#include <charconv>
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <random>
#include <stdexcept>
#include <string>

GameSystem::GameSystem(WorldConfig config)
    : System("Game"), _config(config) {}

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
    _resolveWorldSeed();
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
    const auto now = std::chrono::steady_clock::now();
    const float elapsedSeconds = std::chrono::duration<float>(now - _lastUpdate).count();
    _lastUpdate = now;
    _updateSun(elapsedSeconds);
    ++_updateCount;
    constexpr auto checkpointInterval = std::chrono::seconds(5);
    if (now - _lastCheckpoint >= checkpointInterval) {
        _saveState();
        _persistence->checkpoint();
        _lastCheckpoint = now;
    }
}

void GameSystem::render() {
    _renderTarget->render(_land, _sun);
}

void GameSystem::_updateSun(float elapsedSeconds) {
    constexpr float orbitRadius = 9.0F;
    constexpr float orbitHeight = 3.0F;
    constexpr float radiansPerSecond = 0.20F;
    _sunAngle = std::fmod(
        _sunAngle + elapsedSeconds * radiansPerSecond,
        2.0F * glm::pi<float>()
    );
    _sun.position = {
        std::cos(_sunAngle) * orbitRadius,
        orbitHeight + std::sin(_sunAngle) * orbitRadius,
        0.0F,
    };
    _sun.intensity = std::clamp((_sun.position.y + 1.0F) / 6.0F, 0.0F, 1.0F);
}

void GameSystem::_saveState() {
    _persistence->set("game.update_count", std::to_string(_updateCount));
    _persistence->set("world.last_seed", std::to_string(_config.seed));
}

void GameSystem::_resolveWorldSeed() {
    if (_config.seed == 0) {
        std::random_device random;
        _config.seed =
            (static_cast<std::uint64_t>(random()) << 32U) |
            static_cast<std::uint64_t>(random());
        if (_config.seed == 0) {
            _config.seed = 1;
        }
    }
    _persistence->set("world.last_seed", std::to_string(_config.seed));
    _persistence->checkpoint();
    Logger::info("World seed: " + std::to_string(_config.seed));
}

void GameSystem::_createLand() {
    constexpr float size = 5.0F;
    constexpr float bottom = -1.25F;
    const std::uint32_t cellsPerSide = _config.gridSize;
    const std::uint32_t pointsPerSide = cellsPerSide + 1;
    const float spacing = (size * 2.0F) / static_cast<float>(cellsPerSide);
    constexpr glm::vec3 sideDirt{0.28F, 0.13F, 0.045F};
    constexpr glm::vec3 bottomDirt{0.20F, 0.085F, 0.025F};

    const auto terrainHeight = [seed = _config.seed](float x, float z) {
        const auto hash = [seed](int gridX, int gridZ, std::uint64_t octave) {
            std::uint64_t value = seed + octave * 0x9e3779b97f4a7c15ULL;
            value ^= static_cast<std::uint64_t>(gridX) * 0xbf58476d1ce4e5b9ULL;
            value ^= static_cast<std::uint64_t>(gridZ) * 0x94d049bb133111ebULL;
            value ^= value >> 30U;
            value *= 0xbf58476d1ce4e5b9ULL;
            value ^= value >> 27U;
            value *= 0x94d049bb133111ebULL;
            value ^= value >> 31U;
            return static_cast<float>(value & 0xffffU) / 32767.5F - 1.0F;
        };
        const auto noise = [&hash](float sampleX, float sampleZ, std::uint64_t octave) {
            const int x0 = static_cast<int>(std::floor(sampleX));
            const int z0 = static_cast<int>(std::floor(sampleZ));
            const float localX = sampleX - static_cast<float>(x0);
            const float localZ = sampleZ - static_cast<float>(z0);
            const float smoothX = localX * localX * (3.0F - 2.0F * localX);
            const float smoothZ = localZ * localZ * (3.0F - 2.0F * localZ);
            const float nearA = std::lerp(hash(x0, z0, octave),
                                          hash(x0 + 1, z0, octave), smoothX);
            const float nearB = std::lerp(hash(x0, z0 + 1, octave),
                                          hash(x0 + 1, z0 + 1, octave), smoothX);
            return std::lerp(nearA, nearB, smoothZ);
        };
        float height = 0.0F;
        float amplitude = 0.55F;
        float frequency = 0.18F;
        for (std::uint64_t octave = 0; octave < 4; ++octave) {
            height += noise(x * frequency, z * frequency, octave) * amplitude;
            frequency *= 2.05F;
            amplitude *= 0.48F;
        }
        return height;
    };
    const auto terrainColor = [](float height) {
        const float variation = std::clamp(height * 0.12F, -0.055F, 0.055F);
        return glm::vec3{
            0.42F + variation,
            0.23F + variation * 0.65F,
            0.09F + variation * 0.25F,
        };
    };

    _land.vertices.clear();
    _land.indices.clear();
    _land.vertices.reserve(pointsPerSide * pointsPerSide + cellsPerSide * 16 + 4);
    _land.indices.reserve(cellsPerSide * cellsPerSide * 6 + cellsPerSide * 24 + 6);

    for (std::uint32_t row = 0; row < pointsPerSide; ++row) {
        const float z = -size + static_cast<float>(row) * spacing;
        for (std::uint32_t column = 0; column < pointsPerSide; ++column) {
            const float x = -size + static_cast<float>(column) * spacing;
            const float height = terrainHeight(x, z);
            _land.vertices.push_back({{x, height, z}, terrainColor(height)});
        }
    }
    for (std::uint32_t row = 0; row < cellsPerSide; ++row) {
        for (std::uint32_t column = 0; column < cellsPerSide; ++column) {
            const std::uint32_t topLeft = row * pointsPerSide + column;
            const std::uint32_t topRight = topLeft + 1;
            const std::uint32_t bottomLeft = topLeft + pointsPerSide;
            const std::uint32_t bottomRight = bottomLeft + 1;
            _land.indices.insert(_land.indices.end(), {
                topLeft, topRight, bottomRight,
                bottomRight, bottomLeft, topLeft,
            });
        }
    }

    const auto addSoilWall = [this, bottom](glm::vec3 first, glm::vec3 second) {
        const std::uint32_t start = static_cast<std::uint32_t>(_land.vertices.size());
        _land.vertices.insert(_land.vertices.end(), {
            {first, sideDirt},
            {second, sideDirt},
            {{second.x, bottom, second.z}, bottomDirt},
            {{first.x, bottom, first.z}, bottomDirt},
        });
        _land.indices.insert(_land.indices.end(), {
            start, start + 1, start + 2,
            start + 2, start + 3, start,
        });
    };
    for (std::uint32_t cell = 0; cell < cellsPerSide; ++cell) {
        const float low = -size + static_cast<float>(cell) * spacing;
        const float high = low + spacing;
        addSoilWall({low, terrainHeight(low, size), size},
                    {high, terrainHeight(high, size), size});
        addSoilWall({high, terrainHeight(high, -size), -size},
                    {low, terrainHeight(low, -size), -size});
        addSoilWall({-size, terrainHeight(-size, high), high},
                    {-size, terrainHeight(-size, low), low});
        addSoilWall({size, terrainHeight(size, low), low},
                    {size, terrainHeight(size, high), high});
    }

    const std::uint32_t bottomStart = static_cast<std::uint32_t>(_land.vertices.size());
    _land.vertices.insert(_land.vertices.end(), {
        {{-size, bottom, size}, bottomDirt},
        {{size, bottom, size}, bottomDirt},
        {{size, bottom, -size}, bottomDirt},
        {{-size, bottom, -size}, bottomDirt},
    });
    _land.indices.insert(_land.indices.end(), {
        bottomStart, bottomStart + 1, bottomStart + 2,
        bottomStart + 2, bottomStart + 3, bottomStart,
    });
}
