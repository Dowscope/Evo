#pragma once

#include "ecs/Entity.hpp"

#include <array>
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

struct Slope {
    float degrees = 0.0F;
};

struct Aspect {
    float radians = 0.0F;
    bool defined = false;
};

struct Drainage {
    Entity downhillNeighbor = nullEntity;
    float elevationDropMeters = 0.0F;
    bool localDepression = false;
};

struct SurfaceTemperature {
    float celsius = 15.0F;
};

struct SoilTemperatureProfile {
    static constexpr std::size_t layerCount = 4;
    std::array<float, layerCount> celsius{15.0F, 15.0F, 15.0F, 15.0F};
};
