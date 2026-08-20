#include "systems/TerrainMeshSystem.hpp"

#include "ecs/components/TerrainComponents.hpp"

#include <algorithm>
#include <array>

TerrainMeshSystem::TerrainMeshSystem(SoilThermalConfig soilConfig)
    : System("Terrain Mesh"), _soilConfig(soilConfig) {}

void TerrainMeshSystem::init() {
    System::init();
}

bool TerrainMeshSystem::updateTerrainMesh(
    const WorldConfig& config,
    const Registry& registry,
    std::span<Chunk> chunks,
    std::span<const Entity> terrainEntities,
    Land& land
) {
    bool dirty = false;
    for (const Chunk& chunk : chunks) {
        dirty = dirty || chunk.terrainMeshDirty;
    }
    if (!dirty) {
        return false;
    }
    const std::uint32_t cellsX = config.chunkSize * config.chunksX;
    const std::uint32_t cellsZ = config.chunkSize * config.chunksZ;
    const std::uint32_t pointsX = cellsX + 1;
    const std::uint32_t pointsZ = cellsZ + 1;
    const float halfWidth = static_cast<float>(cellsX) *
                            config.cellSizeMeters * 0.5F;
    const float halfDepth = static_cast<float>(cellsZ) *
                            config.cellSizeMeters * 0.5F;
    constexpr std::array<glm::vec3, 4> soilLayerColors{
        glm::vec3{0.25F, 0.12F, 0.045F},
        glm::vec3{0.36F, 0.19F, 0.065F},
        glm::vec3{0.43F, 0.27F, 0.105F},
        glm::vec3{0.31F, 0.22F, 0.145F},
    };
    constexpr glm::vec3 bottomDirt{0.18F, 0.13F, 0.085F};
    const auto terrainColor = [](float height) {
        const float variation = std::clamp(height * 0.12F, -0.055F, 0.055F);
        return glm::vec3{
            0.42F + variation,
            0.23F + variation * 0.65F,
            0.09F + variation * 0.25F,
        };
    };

    land.vertices.clear();
    land.indices.clear();
    land.vertices.reserve(
        pointsX * pointsZ +
        (cellsX + cellsZ) * 8 * soilLayerColors.size() + pointsX * pointsZ
    );
    land.indices.reserve(
        cellsX * cellsZ * 6 +
        (cellsX + cellsZ) * 12 * soilLayerColors.size() +
        cellsX * cellsZ * 6
    );
    for (std::uint32_t row = 0; row < pointsZ; ++row) {
        const float z = -halfDepth + static_cast<float>(row) * config.cellSizeMeters;
        for (std::uint32_t column = 0; column < pointsX; ++column) {
            const float x = -halfWidth +
                            static_cast<float>(column) * config.cellSizeMeters;
            const float height = _vertexElevation(
                config, registry, terrainEntities, column, row
            );
            const float temperature = _vertexTemperature(
                config, registry, terrainEntities, column, row
            );
            land.vertices.push_back({
                {x, height, z}, terrainColor(height), temperature
            });
        }
    }
    for (std::uint32_t row = 0; row < cellsZ; ++row) {
        for (std::uint32_t column = 0; column < cellsX; ++column) {
            const std::uint32_t topLeft = row * pointsX + column;
            const std::uint32_t topRight = topLeft + 1;
            const std::uint32_t bottomLeft = topLeft + pointsX;
            const std::uint32_t bottomRight = bottomLeft + 1;
            land.indices.insert(land.indices.end(), {
                topLeft, topRight, bottomRight,
                bottomRight, bottomLeft, topLeft,
            });
        }
    }

    const auto addSoilWall = [this, &land, &soilLayerColors](
        glm::vec3 first,
        glm::vec3 second
    ) {
        float upperDepthMeters = 0.0F;
        for (std::size_t layer = 0; layer < soilLayerColors.size(); ++layer) {
            const float lowerDepthMeters = upperDepthMeters +
                _soilConfig.layerThicknessMeters[layer];
            const std::uint32_t start =
                static_cast<std::uint32_t>(land.vertices.size());
            land.vertices.insert(land.vertices.end(), {
                {{first.x, first.y - upperDepthMeters, first.z},
                 soilLayerColors[layer]},
                {{second.x, second.y - upperDepthMeters, second.z},
                 soilLayerColors[layer]},
                {{second.x, second.y - lowerDepthMeters, second.z},
                 soilLayerColors[layer]},
                {{first.x, first.y - lowerDepthMeters, first.z},
                 soilLayerColors[layer]},
            });
            land.indices.insert(land.indices.end(), {
                start, start + 1, start + 2,
                start + 2, start + 3, start,
            });
            upperDepthMeters = lowerDepthMeters;
        }
    };
    for (std::uint32_t cell = 0; cell < cellsX; ++cell) {
        const float low = -halfWidth + static_cast<float>(cell) * config.cellSizeMeters;
        const float high = low + config.cellSizeMeters;
        addSoilWall(
            {low, _vertexElevation(config, registry, terrainEntities, cell, cellsZ), halfDepth},
            {high, _vertexElevation(config, registry, terrainEntities, cell + 1, cellsZ), halfDepth}
        );
        addSoilWall(
            {high, _vertexElevation(config, registry, terrainEntities, cell + 1, 0), -halfDepth},
            {low, _vertexElevation(config, registry, terrainEntities, cell, 0), -halfDepth}
        );
    }
    for (std::uint32_t cell = 0; cell < cellsZ; ++cell) {
        const float low = -halfDepth + static_cast<float>(cell) * config.cellSizeMeters;
        const float high = low + config.cellSizeMeters;
        addSoilWall(
            {-halfWidth, _vertexElevation(config, registry, terrainEntities, 0, cell + 1), high},
            {-halfWidth, _vertexElevation(config, registry, terrainEntities, 0, cell), low}
        );
        addSoilWall(
            {halfWidth, _vertexElevation(config, registry, terrainEntities, cellsX, cell), low},
            {halfWidth, _vertexElevation(config, registry, terrainEntities, cellsX, cell + 1), high}
        );
    }
    float totalSoilDepthMeters = 0.0F;
    for (const float thicknessMeters : _soilConfig.layerThicknessMeters) {
        totalSoilDepthMeters += thicknessMeters;
    }
    const std::uint32_t bottomStart =
        static_cast<std::uint32_t>(land.vertices.size());
    for (std::uint32_t row = 0; row < pointsZ; ++row) {
        const float z = -halfDepth +
            static_cast<float>(row) * config.cellSizeMeters;
        for (std::uint32_t column = 0; column < pointsX; ++column) {
            const float x = -halfWidth +
                static_cast<float>(column) * config.cellSizeMeters;
            const float bottomElevation = _vertexElevation(
                config, registry, terrainEntities, column, row
            ) - totalSoilDepthMeters;
            land.vertices.push_back({{x, bottomElevation, z}, bottomDirt});
        }
    }
    for (std::uint32_t row = 0; row < cellsZ; ++row) {
        for (std::uint32_t column = 0; column < cellsX; ++column) {
            const std::uint32_t topLeft =
                bottomStart + row * pointsX + column;
            const std::uint32_t topRight = topLeft + 1;
            const std::uint32_t bottomLeft = topLeft + pointsX;
            const std::uint32_t bottomRight = bottomLeft + 1;
            land.indices.insert(land.indices.end(), {
                topLeft, bottomRight, topRight,
                bottomRight, topLeft, bottomLeft,
            });
        }
    }
    ++land.revision;
    for (Chunk& chunk : chunks) {
        chunk.terrainMeshDirty = false;
    }
    return true;
}

float TerrainMeshSystem::_vertexElevation(
    const WorldConfig& config,
    const Registry& registry,
    std::span<const Entity> terrainEntities,
    std::uint32_t vertexX,
    std::uint32_t vertexZ
) {
    const std::uint32_t totalCellsX = config.chunkSize * config.chunksX;
    const std::uint32_t totalCellsZ = config.chunkSize * config.chunksZ;
    float elevation = 0.0F;
    std::uint32_t samples = 0;
    for (int offsetZ = -1; offsetZ <= 0; ++offsetZ) {
        for (int offsetX = -1; offsetX <= 0; ++offsetX) {
            const int cellX = static_cast<int>(vertexX) + offsetX;
            const int cellZ = static_cast<int>(vertexZ) + offsetZ;
            if (cellX >= 0 && cellZ >= 0 &&
                cellX < static_cast<int>(totalCellsX) &&
                cellZ < static_cast<int>(totalCellsZ)) {
                const Entity entity = terrainEntities[
                    static_cast<std::uint32_t>(cellZ) * totalCellsX +
                    static_cast<std::uint32_t>(cellX)
                ];
                elevation += registry.get<Elevation>(entity).meters;
                ++samples;
            }
        }
    }
    return elevation / static_cast<float>(samples);
}

float TerrainMeshSystem::_vertexTemperature(
    const WorldConfig& config,
    const Registry& registry,
    std::span<const Entity> terrainEntities,
    std::uint32_t vertexX,
    std::uint32_t vertexZ
) {
    const std::uint32_t totalCellsX = config.chunkSize * config.chunksX;
    const std::uint32_t totalCellsZ = config.chunkSize * config.chunksZ;
    float temperature = 0.0F;
    std::uint32_t samples = 0;
    for (int offsetZ = -1; offsetZ <= 0; ++offsetZ) {
        for (int offsetX = -1; offsetX <= 0; ++offsetX) {
            const int cellX = static_cast<int>(vertexX) + offsetX;
            const int cellZ = static_cast<int>(vertexZ) + offsetZ;
            if (cellX >= 0 && cellZ >= 0 &&
                cellX < static_cast<int>(totalCellsX) &&
                cellZ < static_cast<int>(totalCellsZ)) {
                const Entity entity = terrainEntities[
                    static_cast<std::uint32_t>(cellZ) * totalCellsX +
                    static_cast<std::uint32_t>(cellX)
                ];
                temperature += registry.get<SurfaceTemperature>(entity).celsius;
                ++samples;
            }
        }
    }
    return temperature / static_cast<float>(samples);
}
