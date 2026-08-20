#include "systems/TerrainAnalysisSystem.hpp"

#include "ecs/components/TerrainComponents.hpp"

#include <array>
#include <cmath>
#include <limits>

TerrainAnalysisSystem::TerrainAnalysisSystem(WorldConfig config)
    : System("Terrain Analysis"), _config(config) {}

void TerrainAnalysisSystem::init() {
    System::init();
}

void TerrainAnalysisSystem::updateChunk(
    Registry& registry,
    Chunk& chunk,
    std::span<Chunk> chunks,
    double /* fixedStepSeconds */
) {
    if (!chunk.terrainAnalysisDirty) {
        return;
    }
    const std::uint32_t totalCellsX = _config.chunkSize * _config.chunksX;
    const std::uint32_t totalCellsZ = _config.chunkSize * _config.chunksZ;
    constexpr float radiansToDegrees = 57.29577951308232F;
    constexpr float twoPi = 6.283185307179586F;
    constexpr std::array<std::array<int, 2>, 8> neighbors{{
        {{-1, -1}}, {{0, -1}}, {{1, -1}}, {{-1, 0}},
        {{1, 0}}, {{-1, 1}}, {{0, 1}}, {{1, 1}},
    }};

    for (const Entity entity : chunk.terrainCells) {
        const GridPosition position = registry.get<GridPosition>(entity);
        const float center = registry.get<Elevation>(entity).meters;
        const float left = _elevationAt(
            registry, chunks, position.x > 0 ? position.x - 1 : position.x, position.z
        );
        const float right = _elevationAt(
            registry, chunks,
            position.x + 1 < totalCellsX ? position.x + 1 : position.x,
            position.z
        );
        const float back = _elevationAt(
            registry, chunks, position.x, position.z > 0 ? position.z - 1 : position.z
        );
        const float front = _elevationAt(
            registry, chunks, position.x,
            position.z + 1 < totalCellsZ ? position.z + 1 : position.z
        );
        const float xDistance = _config.cellSizeMeters *
            ((position.x > 0 && position.x + 1 < totalCellsX) ? 2.0F : 1.0F);
        const float zDistance = _config.cellSizeMeters *
            ((position.z > 0 && position.z + 1 < totalCellsZ) ? 2.0F : 1.0F);
        const float gradientX = (right - left) / xDistance;
        const float gradientZ = (front - back) / zDistance;
        const float gradientMagnitude = std::hypot(gradientX, gradientZ);
        registry.get<Slope>(entity).degrees =
            std::atan(gradientMagnitude) * radiansToDegrees;
        Aspect& aspectComponent = registry.get<Aspect>(entity);
        aspectComponent.defined = gradientMagnitude > 1.0e-6F;
        aspectComponent.radians = 0.0F;
        if (aspectComponent.defined) {
            aspectComponent.radians = std::atan2(-gradientZ, -gradientX);
            if (aspectComponent.radians < 0.0F) {
                aspectComponent.radians += twoPi;
            }
        }

        Entity downhill = nullEntity;
        float greatestGradient = 0.0F;
        float selectedDrop = 0.0F;
        for (const auto& offset : neighbors) {
            const int neighborX = static_cast<int>(position.x) + offset[0];
            const int neighborZ = static_cast<int>(position.z) + offset[1];
            if (neighborX < 0 || neighborZ < 0 ||
                neighborX >= static_cast<int>(totalCellsX) ||
                neighborZ >= static_cast<int>(totalCellsZ)) {
                continue;
            }
            const Entity candidate = _entityAt(
                chunks,
                static_cast<std::uint32_t>(neighborX),
                static_cast<std::uint32_t>(neighborZ)
            );
            const float drop = center - registry.get<Elevation>(candidate).meters;
            const bool diagonal = offset[0] != 0 && offset[1] != 0;
            const float distance = _config.cellSizeMeters *
                                   (diagonal ? std::sqrt(2.0F) : 1.0F);
            const float downhillGradient = drop / distance;
            if (downhillGradient > greatestGradient) {
                greatestGradient = downhillGradient;
                selectedDrop = drop;
                downhill = candidate;
            }
        }
        Drainage& drainage = registry.get<Drainage>(entity);
        drainage.downhillNeighbor = downhill;
        drainage.elevationDropMeters = selectedDrop;
        drainage.localDepression = downhill == nullEntity;
    }
    chunk.terrainAnalysisDirty = false;
}

void TerrainAnalysisSystem::collectBoundaryTransfers(
    Registry& /* registry */,
    std::span<Chunk> /* chunks */
) {}

void TerrainAnalysisSystem::applyBoundaryTransfers(
    Registry& /* registry */,
    std::span<Chunk> /* chunks */
) {}

Entity TerrainAnalysisSystem::_entityAt(
    std::span<Chunk> chunks,
    std::uint32_t x,
    std::uint32_t z
) const {
    const std::uint32_t chunkX = x / _config.chunkSize;
    const std::uint32_t chunkZ = z / _config.chunkSize;
    const std::uint32_t localX = x % _config.chunkSize;
    const std::uint32_t localZ = z % _config.chunkSize;
    const Chunk& chunk = chunks[chunkZ * _config.chunksX + chunkX];
    return chunk.terrainCells[localZ * _config.chunkSize + localX];
}

float TerrainAnalysisSystem::_elevationAt(
    const Registry& registry,
    std::span<Chunk> chunks,
    std::uint32_t x,
    std::uint32_t z
) const {
    return registry.get<Elevation>(_entityAt(chunks, x, z)).meters;
}
