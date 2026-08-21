#pragma once

#include "core/ApplicationConfig.hpp"
#include "simulation/ChunkSimulation.hpp"
#include "simulation/SurfaceTemperatureSimulation.hpp"
#include "simulation/SurfaceTemperatureStatistics.hpp"

#include <vector>

class TemperatureModule final : public ChunkTickSystem,
                                public SurfaceTemperatureSimulation,
                                public SurfaceTemperatureStatistics {
public:
    TemperatureModule(
        WorldConfig worldConfig,
        ClimateConfig climateConfig,
        SoilThermalConfig soilConfig
    );

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
    WorldConfig _worldConfig;
    ClimateConfig _climateConfig;
    SoilThermalConfig _soilConfig;
    Sun _sun;
    AtmosphereState _atmosphere;
    std::vector<float> _directSolarExposure;
    float _averageSurfaceTemperatureCelsius = 15.0F;
};
