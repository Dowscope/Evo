#pragma once

#include "ecs/Registry.hpp"
#include "time/Clock.hpp"
#include "world/Chunk.hpp"

#include <span>

class ChunkTickSystem {
public:
    virtual ~ChunkTickSystem() = default;
    virtual void beginTick(
        Registry& /* registry */,
        std::span<Chunk> /* chunks */,
        double /* fixedStepSeconds */
    ) {}
    virtual void updateChunk(
        Registry& registry,
        Chunk& chunk,
        std::span<Chunk> chunks,
        double fixedStepSeconds
    ) = 0;
    virtual void collectBoundaryTransfers(
        Registry& registry,
        std::span<Chunk> chunks
    ) = 0;
    virtual void applyBoundaryTransfers(
        Registry& registry,
        std::span<Chunk> chunks
    ) = 0;
};

class ChunkSimulation {
public:
    virtual ~ChunkSimulation() = default;
    virtual void simulate(
        Registry& registry,
        std::span<Chunk> chunks,
        const TimeFrame& time
    ) = 0;
};
