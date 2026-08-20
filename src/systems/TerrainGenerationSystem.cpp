#include "systems/TerrainGenerationSystem.hpp"

#include "ecs/components/TerrainComponents.hpp"

#include <cmath>

TerrainGenerationSystem::TerrainGenerationSystem(
    ClimateConfig climateConfig,
    SoilThermalConfig soilConfig
) : System("Terrain Generation"),
    _climateConfig(climateConfig),
    _soilConfig(soilConfig) {}

void TerrainGenerationSystem::init() {
    System::init();
}

void TerrainGenerationSystem::generateTerrain(
    WorldConfig config,
    Registry& registry,
    std::vector<Chunk>& chunks,
    std::vector<Entity>& terrainEntities
) {
    const std::uint32_t totalCellsX = config.chunkSize * config.chunksX;
    const std::uint32_t totalCellsZ = config.chunkSize * config.chunksZ;
    const float halfWidth = static_cast<float>(totalCellsX) *
                            config.cellSizeMeters * 0.5F;
    const float halfDepth = static_cast<float>(totalCellsZ) *
                            config.cellSizeMeters * 0.5F;
    terrainEntities.assign(totalCellsX * totalCellsZ, nullEntity);
    chunks.clear();
    chunks.reserve(config.chunksX * config.chunksZ);

    for (std::uint32_t chunkZ = 0; chunkZ < config.chunksZ; ++chunkZ) {
        for (std::uint32_t chunkX = 0; chunkX < config.chunksX; ++chunkX) {
            Chunk chunk{
                .x = static_cast<std::int32_t>(chunkX),
                .z = static_cast<std::int32_t>(chunkZ),
            };
            chunk.terrainCells.reserve(config.chunkSize * config.chunkSize);
            for (std::uint32_t localZ = 0; localZ < config.chunkSize; ++localZ) {
                for (std::uint32_t localX = 0; localX < config.chunkSize; ++localX) {
                    const std::uint32_t globalX = chunkX * config.chunkSize + localX;
                    const std::uint32_t globalZ = chunkZ * config.chunkSize + localZ;
                    const float worldX = -halfWidth +
                        (static_cast<float>(globalX) + 0.5F) * config.cellSizeMeters;
                    const float worldZ = -halfDepth +
                        (static_cast<float>(globalZ) + 0.5F) * config.cellSizeMeters;
                    const Entity entity = registry.create();
                    registry.emplace<GridPosition>(entity, globalX, globalZ);
                    registry.emplace<ChunkPosition>(
                        entity, chunkX, chunkZ, localX, localZ
                    );
                    registry.emplace<Elevation>(
                        entity, _terrainHeight(worldX, worldZ, config.seed)
                    );
                    registry.emplace<Slope>(entity);
                    registry.emplace<Aspect>(entity);
                    registry.emplace<Drainage>(entity);
                    registry.emplace<SurfaceTemperature>(
                        entity, _climateConfig.initialSurfaceTemperatureCelsius
                    );
                    registry.emplace<SoilTemperatureProfile>(
                        entity,
                        std::array<float, SoilTemperatureProfile::layerCount>{
                            _soilConfig.initialTemperatureCelsius,
                            _soilConfig.initialTemperatureCelsius,
                            _soilConfig.initialTemperatureCelsius,
                            _soilConfig.initialTemperatureCelsius,
                        }
                    );
                    chunk.terrainCells.push_back(entity);
                    terrainEntities[globalZ * totalCellsX + globalX] = entity;
                }
            }
            chunks.push_back(std::move(chunk));
        }
    }
}

float TerrainGenerationSystem::_terrainHeight(
    float x,
    float z,
    std::uint64_t seed
) {
    const auto hash = [seed](int gridX, int gridZ, std::uint64_t octave) {
        std::uint64_t value = seed + octave * 0x9e3779b97f4a7c15ULL;
        value ^= static_cast<std::uint64_t>(gridX) * 0xbf58476d1ce4e5b9ULL;
        value ^= static_cast<std::uint64_t>(gridZ) * 0x94d049bb133111ebULL;
        value ^= value >> 30U;
        value *= 0xbf58476d1ce4e5b9ULL;
        value ^= value >> 27U;
        value *= 0x94d049bb133111ebULL;
        value ^= value >> 31U;
        return static_cast<float>(value & 0xffffU) / 32767.5F - 1.0F;
    };
    const auto noise = [&hash](float sampleX, float sampleZ, std::uint64_t octave) {
        const int x0 = static_cast<int>(std::floor(sampleX));
        const int z0 = static_cast<int>(std::floor(sampleZ));
        const float localX = sampleX - static_cast<float>(x0);
        const float localZ = sampleZ - static_cast<float>(z0);
        const float smoothX = localX * localX * (3.0F - 2.0F * localX);
        const float smoothZ = localZ * localZ * (3.0F - 2.0F * localZ);
        const float nearA = std::lerp(
            hash(x0, z0, octave), hash(x0 + 1, z0, octave), smoothX
        );
        const float nearB = std::lerp(
            hash(x0, z0 + 1, octave),
            hash(x0 + 1, z0 + 1, octave),
            smoothX
        );
        return std::lerp(nearA, nearB, smoothZ);
    };
    float height = 0.0F;
    float amplitude = 0.68F;
    float frequency = 0.18F;
    for (std::uint64_t octave = 0; octave < 4; ++octave) {
        height += noise(x * frequency, z * frequency, octave) * amplitude;
        frequency *= 2.05F;
        amplitude *= 0.48F;
    }
    return height;
}
