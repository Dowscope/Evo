#pragma once

#include "ecs/Entity.hpp"

#include <cstdint>
#include <vector>

enum class SimulationLevel {
    Active,
    Nearby,
    Distant,
    Dormant,
};

struct Chunk {
    std::int32_t x = 0;
    std::int32_t z = 0;
    SimulationLevel simulationLevel = SimulationLevel::Active;
    std::uint64_t completedTicks = 0;
    std::vector<Entity> terrainCells;
};
