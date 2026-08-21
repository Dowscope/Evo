#include "core/Logger.hpp"
#include "core/ApplicationConfig.hpp"
#include "systems/EventSystem.hpp"
#include "systems/CameraSystem.hpp"
#include "systems/GameSystem.hpp"
#include "systems/ScreenSystem.hpp"
#include "systems/SaveSystem.hpp"
#include "systems/TimeSystem.hpp"
#include "systems/ChunkSimulationSystem.hpp"
#include "systems/SunSystem.hpp"
#include "systems/TerrainAnalysisSystem.hpp"
#include "systems/TerrainGenerationSystem.hpp"
#include "systems/TerrainMeshSystem.hpp"
#include "systems/WeatherSystem.hpp"
#include "systems/AtmosphereSystem.hpp"

#include <cstdlib>
#include <exception>
#include <memory>

namespace {

std::unique_ptr<EventSystem> eventSystem;
std::unique_ptr<TimeSystem> timeSystem;
std::unique_ptr<ChunkSimulationSystem> chunkSimulationSystem;
std::unique_ptr<SunSystem> sunSystem;
std::unique_ptr<AtmosphereSystem> atmosphereSystem;
std::unique_ptr<WeatherSystem> weatherSystem;
std::unique_ptr<TerrainAnalysisSystem> terrainAnalysisSystem;
std::unique_ptr<TerrainGenerationSystem> terrainGenerationSystem;
std::unique_ptr<TerrainMeshSystem> terrainMeshSystem;
std::unique_ptr<CameraSystem> cameraSystem;
std::unique_ptr<ScreenSystem> screenSystem;
std::unique_ptr<GameSystem> gameSystem;
std::unique_ptr<SaveSystem> saveSystem;

void init() {
    const ApplicationConfig config = ConfigLoader::load("config/config.json");

    eventSystem = std::make_unique<EventSystem>();
    timeSystem = std::make_unique<TimeSystem>(config.time);
    chunkSimulationSystem = std::make_unique<ChunkSimulationSystem>();
    sunSystem = std::make_unique<SunSystem>(config.world);
    atmosphereSystem = std::make_unique<AtmosphereSystem>(config.atmosphere);
    weatherSystem = std::make_unique<WeatherSystem>(
        config.world,
        config.climate,
        config.soil
    );
    terrainAnalysisSystem = std::make_unique<TerrainAnalysisSystem>(config.world);
    terrainGenerationSystem = std::make_unique<TerrainGenerationSystem>(
        config.climate,
        config.soil
    );
    terrainMeshSystem = std::make_unique<TerrainMeshSystem>(config.soil);
    cameraSystem = std::make_unique<CameraSystem>();
    saveSystem = std::make_unique<SaveSystem>("Data/evo.save");
    screenSystem = std::make_unique<ScreenSystem>(
        config.window,
        config.statsWindow
    );
    gameSystem = std::make_unique<GameSystem>(config.world);

    eventSystem->init();
    timeSystem->init();
    chunkSimulationSystem->init();
    sunSystem->init();
    atmosphereSystem->init();
    weatherSystem->init();
    terrainAnalysisSystem->init();
    terrainGenerationSystem->init();
    terrainMeshSystem->init();
    cameraSystem->init();
    saveSystem->init();
    screenSystem->init();

    eventSystem->registerSource(*screenSystem);
    eventSystem->registerListener(*screenSystem);
    eventSystem->registerListener(*cameraSystem);
    screenSystem->registerCamera(*cameraSystem);
    screenSystem->registerClock(*timeSystem);
    screenSystem->registerTemperatureStatistics(*weatherSystem);
    screenSystem->registerAtmosphereStatistics(*atmosphereSystem);
    chunkSimulationSystem->registerTickSystem(*terrainAnalysisSystem);
    chunkSimulationSystem->registerTickSystem(*weatherSystem);
    gameSystem->registerRenderTarget(*screenSystem);
    gameSystem->registerPersistence(*saveSystem);
    gameSystem->registerClock(*timeSystem);
    gameSystem->registerChunkSimulation(*chunkSimulationSystem);
    gameSystem->registerSunSimulation(*sunSystem);
    gameSystem->registerAtmosphereSimulation(*atmosphereSystem);
    gameSystem->registerTerrainGeneration(*terrainGenerationSystem);
    gameSystem->registerTerrainMeshing(*terrainMeshSystem);
    gameSystem->registerSurfaceTemperature(*weatherSystem);
    gameSystem->init();
}

void update() {
    timeSystem->update();
    eventSystem->update();
    gameSystem->update();
}

void render() {
    gameSystem->render();
}

void gameLoop() {
    while (!screenSystem->shouldClose()) {
        update();
        render();
    }
}

void shutdown() {
    gameSystem.reset();
    if (saveSystem != nullptr) {
        saveSystem->flush();
    }
    screenSystem.reset();
    weatherSystem.reset();
    terrainMeshSystem.reset();
    terrainGenerationSystem.reset();
    terrainAnalysisSystem.reset();
    sunSystem.reset();
    atmosphereSystem.reset();
    chunkSimulationSystem.reset();
    cameraSystem.reset();
    saveSystem.reset();
    timeSystem.reset();
    eventSystem.reset();
}

} // namespace

int main() {
    try {
        init();
        gameLoop();
        shutdown();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        Logger::error(error.what());
        shutdown();
        return EXIT_FAILURE;
    }
}
