#pragma once

#include "simulation/ChunkSimulation.hpp"
#include "systems/System.hpp"

#include <vector>

class ChunkSimulationSystem final : public System, public ChunkSimulation {
public:
    ChunkSimulationSystem();

    void init() override;
    void registerTickSystem(ChunkTickSystem& system);
    void simulate(
        Registry& registry,
        std::span<Chunk> chunks,
        const TimeFrame& time
    ) override;

private:
    std::vector<ChunkTickSystem*> _tickSystems;
};
