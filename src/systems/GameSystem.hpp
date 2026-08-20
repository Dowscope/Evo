#pragma once

#include "systems/System.hpp"
#include "rendering/Scene.hpp"
#include "core/ApplicationConfig.hpp"
#include "ecs/Entity.hpp"
#include "ecs/Registry.hpp"
#include "world/Chunk.hpp"

#include <cstdint>
#include <vector>

class Persistence;
class RenderTarget;
class Clock;
class ChunkSimulation;
class SunSimulation;
class TerrainGeneration;
class TerrainMeshing;
class SurfaceTemperatureSimulation;

class GameSystem final : public System {
public:
    explicit GameSystem(WorldConfig config);
    ~GameSystem() override;

    void init() override;
    void registerRenderTarget(RenderTarget& renderTarget);
    void registerPersistence(Persistence& persistence);
    void registerClock(Clock& clock);
    void registerChunkSimulation(ChunkSimulation& simulation);
    void registerSunSimulation(SunSimulation& simulation);
    void registerTerrainGeneration(TerrainGeneration& generation);
    void registerTerrainMeshing(TerrainMeshing& meshing);
    void registerSurfaceTemperature(
        SurfaceTemperatureSimulation& surfaceTemperature
    );
    void update();
    void render();

private:
    void _saveState();
    void _resolveWorldSeed();

    RenderTarget* _renderTarget = nullptr;
    Persistence* _persistence = nullptr;
    Clock* _clock = nullptr;
    ChunkSimulation* _chunkSimulation = nullptr;
    SunSimulation* _sunSimulation = nullptr;
    TerrainGeneration* _terrainGeneration = nullptr;
    TerrainMeshing* _terrainMeshing = nullptr;
    SurfaceTemperatureSimulation* _surfaceTemperature = nullptr;
    WorldConfig _config;
    Registry _registry;
    std::vector<Chunk> _chunks;
    std::vector<Entity> _terrainEntities;
    Land _land;
    Sun _sun;
    std::uint64_t _updateCount = 0;
    double _nextCheckpointRealTimeSeconds = 5.0;
};
