#pragma once

#include "core/ApplicationConfig.hpp"
#include "ecs/Registry.hpp"
#include "rendering/Scene.hpp"
#include "world/Chunk.hpp"

#include <span>

class TerrainMeshing {
public:
    virtual ~TerrainMeshing() = default;
    virtual bool updateTerrainMesh(
        const WorldConfig& config,
        const Registry& registry,
        std::span<Chunk> chunks,
        std::span<const Entity> terrainEntities,
        Land& land
    ) = 0;
};
