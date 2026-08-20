#pragma once

#include <cstdint>

struct GridPosition {
    std::uint32_t x = 0;
    std::uint32_t z = 0;
};

struct ChunkPosition {
    std::uint32_t chunkX = 0;
    std::uint32_t chunkZ = 0;
    std::uint32_t localX = 0;
    std::uint32_t localZ = 0;
};

struct Elevation {
    float meters = 0.0F;
};
