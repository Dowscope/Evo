#pragma once

#include "simulation/TerrainGeneration.hpp"
#include "systems/System.hpp"

class TerrainGenerationSystem final : public System, public TerrainGeneration {
public:
    TerrainGenerationSystem(
        ClimateConfig climateConfig,
        SoilThermalConfig soilConfig
    );

    void init() override;
    void generateTerrain(
        WorldConfig config,
        Registry& registry,
        std::vector<Chunk>& chunks,
        std::vector<Entity>& terrainEntities
    ) override;

private:
    [[nodiscard]] static float _terrainHeight(
        float x,
        float z,
        std::uint64_t seed
    );

    ClimateConfig _climateConfig;
    SoilThermalConfig _soilConfig;
};
