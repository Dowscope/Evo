#pragma once

#include "simulation/TerrainMeshing.hpp"
#include "systems/System.hpp"

class TerrainMeshSystem final : public System, public TerrainMeshing {
public:
    explicit TerrainMeshSystem(SoilThermalConfig soilConfig);

    void init() override;
    bool updateTerrainMesh(
        const WorldConfig& config,
        const Registry& registry,
        std::span<Chunk> chunks,
        std::span<const Entity> terrainEntities,
        Land& land
    ) override;

private:
    [[nodiscard]] static float _vertexElevation(
        const WorldConfig& config,
        const Registry& registry,
        std::span<const Entity> terrainEntities,
        std::uint32_t vertexX,
        std::uint32_t vertexZ
    );
    [[nodiscard]] static float _vertexTemperature(
        const WorldConfig& config,
        const Registry& registry,
        std::span<const Entity> terrainEntities,
        std::uint32_t vertexX,
        std::uint32_t vertexZ
    );

    SoilThermalConfig _soilConfig;
};
