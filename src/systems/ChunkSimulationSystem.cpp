#include "systems/ChunkSimulationSystem.hpp"

#include <stdexcept>

ChunkSimulationSystem::ChunkSimulationSystem()
    : System("Chunk Simulation") {}

void ChunkSimulationSystem::init() {
    System::init();
}

void ChunkSimulationSystem::registerTickSystem(ChunkTickSystem& system) {
    for (const ChunkTickSystem* registered : _tickSystems) {
        if (registered == &system) {
            throw std::runtime_error("Chunk tick system is already registered");
        }
    }
    _tickSystems.push_back(&system);
}

void ChunkSimulationSystem::simulate(
    Registry& registry,
    std::span<Chunk> chunks,
    const TimeFrame& time
) {
    for (std::uint32_t step = 0; step < time.fixedSteps; ++step) {
        for (ChunkTickSystem* system : _tickSystems) {
            for (Chunk& chunk : chunks) {
                if (chunk.simulationLevel == SimulationLevel::Active) {
                    system->updateChunk(registry, chunk, time.fixedStepSeconds);
                }
            }
        }
        for (ChunkTickSystem* system : _tickSystems) {
            system->collectBoundaryTransfers(registry, chunks);
        }
        for (ChunkTickSystem* system : _tickSystems) {
            system->applyBoundaryTransfers(registry, chunks);
        }
        for (Chunk& chunk : chunks) {
            if (chunk.simulationLevel == SimulationLevel::Active) {
                ++chunk.completedTicks;
            }
        }
    }
}
