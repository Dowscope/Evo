#include "systems/WeatherSystem.hpp"

WeatherSystem::WeatherSystem(
    WorldConfig worldConfig,
    ClimateConfig climateConfig,
    SoilThermalConfig soilConfig
) : System("Weather"),
    _temperatureModule(worldConfig, climateConfig, soilConfig) {}

void WeatherSystem::init() {
    System::init();
}

void WeatherSystem::setSunState(const Sun& sun) {
    _temperatureModule.setSunState(sun);
}

void WeatherSystem::setAtmosphereState(const AtmosphereState& atmosphere) {
    _temperatureModule.setAtmosphereState(atmosphere);
}

float WeatherSystem::averageSurfaceTemperatureCelsius() const {
    return _temperatureModule.averageSurfaceTemperatureCelsius();
}

void WeatherSystem::beginTick(
    Registry& registry,
    std::span<Chunk> chunks,
    double fixedStepSeconds
) {
    _temperatureModule.beginTick(registry, chunks, fixedStepSeconds);
}

void WeatherSystem::updateChunk(
    Registry& registry,
    Chunk& chunk,
    std::span<Chunk> chunks,
    double fixedStepSeconds
) {
    _temperatureModule.updateChunk(
        registry, chunk, chunks, fixedStepSeconds
    );
}

void WeatherSystem::collectBoundaryTransfers(
    Registry& registry,
    std::span<Chunk> chunks
) {
    _temperatureModule.collectBoundaryTransfers(registry, chunks);
}

void WeatherSystem::applyBoundaryTransfers(
    Registry& registry,
    std::span<Chunk> chunks
) {
    _temperatureModule.applyBoundaryTransfers(registry, chunks);
}
