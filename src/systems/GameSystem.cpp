#include "systems/GameSystem.hpp"

#include "persistence/Persistence.hpp"
#include "core/Logger.hpp"
#include "ecs/components/TerrainComponents.hpp"
#include "simulation/ChunkSimulation.hpp"
#include "simulation/SunSimulation.hpp"
#include "systems/ScreenSystem.hpp"
#include "time/Clock.hpp"

#include <charconv>
#include <algorithm>
#include <cmath>
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
    if (_renderTarget == nullptr || _persistence == nullptr || _clock == nullptr ||
        _chunkSimulation == nullptr || _sunSimulation == nullptr) {
        throw std::runtime_error(
            "GameSystem requires rendering, persistence, clock, chunk, and sun dependencies"
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
    _createTerrainEntities();
    _createLand();
    System::init();
}

void GameSystem::registerRenderTarget(RenderTarget& renderTarget) {
    _renderTarget = &renderTarget;
}

void GameSystem::registerPersistence(Persistence& persistence) {
    _persistence = &persistence;
}

void GameSystem::registerClock(Clock& clock) {
    _clock = &clock;
}

void GameSystem::registerChunkSimulation(ChunkSimulation& simulation) {
    _chunkSimulation = &simulation;
}

void GameSystem::registerSunSimulation(SunSimulation& simulation) {
    _sunSimulation = &simulation;
}

void GameSystem::update() {
    const TimeFrame& time = _clock->frame();
    _sunSimulation->updateSun(_sun, time);
    _chunkSimulation->simulate(_registry, _chunks, time);
    _updateCount += time.fixedSteps;
    constexpr double checkpointIntervalSeconds = 5.0;
    if (time.realTimeSeconds >= _nextCheckpointRealTimeSeconds) {
        _saveState();
        _persistence->checkpoint();
        _nextCheckpointRealTimeSeconds =
            (std::floor(time.realTimeSeconds / checkpointIntervalSeconds) + 1.0) *
            checkpointIntervalSeconds;
    }
}

void GameSystem::render() {
    _renderTarget->render(_land, _sun);
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

void GameSystem::_createTerrainEntities() {
    const std::uint32_t totalCellsX = _config.chunkSize * _config.chunksX;
    const std::uint32_t totalCellsZ = _config.chunkSize * _config.chunksZ;
    const float halfWidth = static_cast<float>(totalCellsX) *
                            _config.cellSizeMeters * 0.5F;
    const float halfDepth = static_cast<float>(totalCellsZ) *
                            _config.cellSizeMeters * 0.5F;
    _terrainEntities.assign(totalCellsX * totalCellsZ, nullEntity);
    _chunks.reserve(_config.chunksX * _config.chunksZ);

    for (std::uint32_t chunkZ = 0; chunkZ < _config.chunksZ; ++chunkZ) {
        for (std::uint32_t chunkX = 0; chunkX < _config.chunksX; ++chunkX) {
            Chunk chunk{
                .x = static_cast<std::int32_t>(chunkX),
                .z = static_cast<std::int32_t>(chunkZ),
            };
            chunk.terrainCells.reserve(_config.chunkSize * _config.chunkSize);
            for (std::uint32_t localZ = 0; localZ < _config.chunkSize; ++localZ) {
                for (std::uint32_t localX = 0; localX < _config.chunkSize; ++localX) {
                    const std::uint32_t globalX = chunkX * _config.chunkSize + localX;
                    const std::uint32_t globalZ = chunkZ * _config.chunkSize + localZ;
                    const float worldX = -halfWidth +
                        (static_cast<float>(globalX) + 0.5F) * _config.cellSizeMeters;
                    const float worldZ = -halfDepth +
                        (static_cast<float>(globalZ) + 0.5F) * _config.cellSizeMeters;
                    const Entity entity = _registry.create();
                    _registry.emplace<GridPosition>(entity, globalX, globalZ);
                    _registry.emplace<ChunkPosition>(
                        entity, chunkX, chunkZ, localX, localZ
                    );
                    _registry.emplace<Elevation>(
                        entity, _terrainHeight(worldX, worldZ)
                    );
                    chunk.terrainCells.push_back(entity);
                    _terrainEntities[globalZ * totalCellsX + globalX] = entity;
                }
            }
            _chunks.push_back(std::move(chunk));
        }
    }
}

float GameSystem::_terrainHeight(float x, float z) const {
    const auto hash = [seed = _config.seed](
        int gridX,
        int gridZ,
        std::uint64_t octave
    ) {
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
        const float nearA = std::lerp(
            hash(x0, z0, octave), hash(x0 + 1, z0, octave), smoothX
        );
        const float nearB = std::lerp(
            hash(x0, z0 + 1, octave),
            hash(x0 + 1, z0 + 1, octave),
            smoothX
        );
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
}

float GameSystem::_vertexElevation(
    std::uint32_t vertexX,
    std::uint32_t vertexZ
) const {
    const std::uint32_t totalCellsX = _config.chunkSize * _config.chunksX;
    const std::uint32_t totalCellsZ = _config.chunkSize * _config.chunksZ;
    float elevation = 0.0F;
    std::uint32_t samples = 0;
    for (int offsetZ = -1; offsetZ <= 0; ++offsetZ) {
        for (int offsetX = -1; offsetX <= 0; ++offsetX) {
            const int cellX = static_cast<int>(vertexX) + offsetX;
            const int cellZ = static_cast<int>(vertexZ) + offsetZ;
            if (cellX >= 0 && cellZ >= 0 &&
                cellX < static_cast<int>(totalCellsX) &&
                cellZ < static_cast<int>(totalCellsZ)) {
                const Entity entity = _terrainEntities[
                    static_cast<std::uint32_t>(cellZ) * totalCellsX +
                    static_cast<std::uint32_t>(cellX)
                ];
                elevation += _registry.get<Elevation>(entity).meters;
                ++samples;
            }
        }
    }
    return elevation / static_cast<float>(samples);
}

void GameSystem::_createLand() {
    const std::uint32_t cellsX = _config.chunkSize * _config.chunksX;
    const std::uint32_t cellsZ = _config.chunkSize * _config.chunksZ;
    const std::uint32_t pointsX = cellsX + 1;
    const std::uint32_t pointsZ = cellsZ + 1;
    const float halfWidth = static_cast<float>(cellsX) *
                            _config.cellSizeMeters * 0.5F;
    const float halfDepth = static_cast<float>(cellsZ) *
                            _config.cellSizeMeters * 0.5F;
    const float bottom = -std::max(1.25F, _config.cellSizeMeters * 1.25F);
    constexpr glm::vec3 sideDirt{0.28F, 0.13F, 0.045F};
    constexpr glm::vec3 bottomDirt{0.20F, 0.085F, 0.025F};
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
    _land.vertices.reserve(pointsX * pointsZ + (cellsX + cellsZ) * 8 + 4);
    _land.indices.reserve(cellsX * cellsZ * 6 + (cellsX + cellsZ) * 12 + 6);

    for (std::uint32_t row = 0; row < pointsZ; ++row) {
        const float z = -halfDepth +
                        static_cast<float>(row) * _config.cellSizeMeters;
        for (std::uint32_t column = 0; column < pointsX; ++column) {
            const float x = -halfWidth +
                            static_cast<float>(column) * _config.cellSizeMeters;
            const float height = _vertexElevation(column, row);
            _land.vertices.push_back({{x, height, z}, terrainColor(height)});
        }
    }
    for (std::uint32_t row = 0; row < cellsZ; ++row) {
        for (std::uint32_t column = 0; column < cellsX; ++column) {
            const std::uint32_t topLeft = row * pointsX + column;
            const std::uint32_t topRight = topLeft + 1;
            const std::uint32_t bottomLeft = topLeft + pointsX;
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
    for (std::uint32_t cell = 0; cell < cellsX; ++cell) {
        const float low = -halfWidth +
                          static_cast<float>(cell) * _config.cellSizeMeters;
        const float high = low + _config.cellSizeMeters;
        addSoilWall({low, _vertexElevation(cell, cellsZ), halfDepth},
                    {high, _vertexElevation(cell + 1, cellsZ), halfDepth});
        addSoilWall({high, _vertexElevation(cell + 1, 0), -halfDepth},
                    {low, _vertexElevation(cell, 0), -halfDepth});
    }
    for (std::uint32_t cell = 0; cell < cellsZ; ++cell) {
        const float low = -halfDepth +
                          static_cast<float>(cell) * _config.cellSizeMeters;
        const float high = low + _config.cellSizeMeters;
        addSoilWall({-halfWidth, _vertexElevation(0, cell + 1), high},
                    {-halfWidth, _vertexElevation(0, cell), low});
        addSoilWall({halfWidth, _vertexElevation(cellsX, cell), low},
                    {halfWidth, _vertexElevation(cellsX, cell + 1), high});
    }

    const std::uint32_t bottomStart = static_cast<std::uint32_t>(_land.vertices.size());
    _land.vertices.insert(_land.vertices.end(), {
        {{-halfWidth, bottom, halfDepth}, bottomDirt},
        {{halfWidth, bottom, halfDepth}, bottomDirt},
        {{halfWidth, bottom, -halfDepth}, bottomDirt},
        {{-halfWidth, bottom, -halfDepth}, bottomDirt},
    });
    _land.indices.insert(_land.indices.end(), {
        bottomStart, bottomStart + 1, bottomStart + 2,
        bottomStart + 2, bottomStart + 3, bottomStart,
    });
}
