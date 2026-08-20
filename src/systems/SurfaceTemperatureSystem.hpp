#pragma once

#include "core/ApplicationConfig.hpp"
#include "simulation/ChunkSimulation.hpp"
#include "simulation/SurfaceTemperatureSimulation.hpp"
#include "simulation/SurfaceTemperatureStatistics.hpp"
#include "systems/System.hpp"

class SurfaceTemperatureSystem final : public System,
                                       public ChunkTickSystem,
                                       public SurfaceTemperatureSimulation,
                                       public SurfaceTemperatureStatistics {
public:
    SurfaceTemperatureSystem(
        ClimateConfig climateConfig,
        SoilThermalConfig soilConfig
    );

    void init() override;
    void setSunState(const Sun& sun) override;
    [[nodiscard]] float averageSurfaceTemperatureCelsius() const override;
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
    ClimateConfig _climateConfig;
    SoilThermalConfig _soilConfig;
    Sun _sun;
    float _averageSurfaceTemperatureCelsius = 15.0F;
};
