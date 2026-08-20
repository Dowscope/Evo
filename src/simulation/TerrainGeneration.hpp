#pragma once

#include "core/ApplicationConfig.hpp"
#include "ecs/Registry.hpp"
#include "world/Chunk.hpp"

#include <vector>

class TerrainGeneration {
public:
    virtual ~TerrainGeneration() = default;
    virtual void generateTerrain(
        WorldConfig config,
        Registry& registry,
        std::vector<Chunk>& chunks,
        std::vector<Entity>& terrainEntities
    ) = 0;
};
