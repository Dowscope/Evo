#pragma once

#include "core/ApplicationConfig.hpp"
#include "simulation/ChunkSimulation.hpp"
#include "systems/System.hpp"

class TerrainAnalysisSystem final : public System, public ChunkTickSystem {
public:
    explicit TerrainAnalysisSystem(WorldConfig config);

    void init() override;
    void updateChunk(
        Registry& registry,
        Chunk& chunk,
        std::span<Chunk> chunks,
        double fixedStepSeconds
    ) override;
    void collectBoundaryTransfers(
        Registry& registry,
        std::span<Chunk> chunks
    ) override;
    void applyBoundaryTransfers(
        Registry& registry,
        std::span<Chunk> chunks
    ) override;

private:
    [[nodiscard]] Entity _entityAt(
        std::span<Chunk> chunks,
        std::uint32_t x,
        std::uint32_t z
    ) const;
    [[nodiscard]] float _elevationAt(
        const Registry& registry,
        std::span<Chunk> chunks,
        std::uint32_t x,
        std::uint32_t z
    ) const;

    WorldConfig _config;
};
