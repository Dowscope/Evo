#pragma once

#include "simulation/ChunkSimulation.hpp"
#include "simulation/SurfaceTemperatureSimulation.hpp"
#include "simulation/SurfaceTemperatureStatistics.hpp"
#include "systems/System.hpp"
#include "weather/TemperatureModule.hpp"

class WeatherSystem final : public System,
                            public SurfaceTemperatureSimulation,
                            public SurfaceTemperatureStatistics,
                            public ChunkTickSystem {
public:
    WeatherSystem(
        WorldConfig worldConfig,
        ClimateConfig climateConfig,
        SoilThermalConfig soilConfig
    );

    void init() override;
    void setSunState(const Sun& sun) override;
    void setAtmosphereState(const AtmosphereState& atmosphere) override;
    [[nodiscard]] float averageSurfaceTemperatureCelsius() const override;
    void beginTick(
        Registry& registry,
        std::span<Chunk> chunks,
        double fixedStepSeconds
    ) override;
    void updateChunk(
        Registry& registry,
        Chunk& chunk,
        std::span<Chunk> chunks,
        double fixedStepSeconds
    ) override;
    void collectBoundaryTransfers(
        Registry& registry,
        std::span<Chunk> chunks
    ) override;
    void applyBoundaryTransfers(
        Registry& registry,
        std::span<Chunk> chunks
    ) override;

private:
    TemperatureModule _temperatureModule;
};
