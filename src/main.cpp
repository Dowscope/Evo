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

#include <cstdlib>
#include <exception>
#include <memory>

namespace {

std::unique_ptr<EventSystem> eventSystem;
std::unique_ptr<TimeSystem> timeSystem;
std::unique_ptr<ChunkSimulationSystem> chunkSimulationSystem;
std::unique_ptr<SunSystem> sunSystem;
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
    cameraSystem = std::make_unique<CameraSystem>();
    saveSystem = std::make_unique<SaveSystem>("Data/evo.save");
    screenSystem = std::make_unique<ScreenSystem>(config.window);
    gameSystem = std::make_unique<GameSystem>(config.world);

    eventSystem->init();
    timeSystem->init();
    chunkSimulationSystem->init();
    sunSystem->init();
    cameraSystem->init();
    saveSystem->init();
    screenSystem->init();

    eventSystem->registerSource(*screenSystem);
    eventSystem->registerListener(*screenSystem);
    eventSystem->registerListener(*cameraSystem);
    screenSystem->registerCamera(*cameraSystem);
    screenSystem->registerClock(*timeSystem);
    gameSystem->registerRenderTarget(*screenSystem);
    gameSystem->registerPersistence(*saveSystem);
    gameSystem->registerClock(*timeSystem);
    gameSystem->registerChunkSimulation(*chunkSimulationSystem);
    gameSystem->registerSunSimulation(*sunSystem);
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
    sunSystem.reset();
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
