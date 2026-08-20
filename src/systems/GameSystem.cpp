#include "systems/GameSystem.hpp"

#include "core/Logger.hpp"
#include "persistence/Persistence.hpp"
#include "simulation/ChunkSimulation.hpp"
#include "simulation/SunSimulation.hpp"
#include "simulation/TerrainGeneration.hpp"
#include "simulation/TerrainMeshing.hpp"
#include "systems/ScreenSystem.hpp"
#include "time/Clock.hpp"

#include <charconv>
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
        _chunkSimulation == nullptr || _sunSimulation == nullptr ||
        _terrainGeneration == nullptr || _terrainMeshing == nullptr) {
        throw std::runtime_error(
            "GameSystem is missing one or more registered dependencies"
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
    _terrainGeneration->generateTerrain(
        _config, _registry, _chunks, _terrainEntities
    );
    _terrainMeshing->updateTerrainMesh(
        _config, _registry, _chunks, _terrainEntities, _land
    );
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

void GameSystem::registerTerrainGeneration(TerrainGeneration& generation) {
    _terrainGeneration = &generation;
}

void GameSystem::registerTerrainMeshing(TerrainMeshing& meshing) {
    _terrainMeshing = &meshing;
}

void GameSystem::update() {
    const TimeFrame& time = _clock->frame();
    _sunSimulation->updateSun(_sun, time);
    _chunkSimulation->simulate(_registry, _chunks, time);
    _terrainMeshing->updateTerrainMesh(
        _config, _registry, _chunks, _terrainEntities, _land
    );
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
