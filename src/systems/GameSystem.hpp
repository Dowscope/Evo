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
    void update();
    void render();

private:
    void _saveState();
    void _createTerrainEntities();
    void _createLand();
    void _resolveWorldSeed();
    [[nodiscard]] float _terrainHeight(float x, float z) const;
    [[nodiscard]] float _vertexElevation(
        std::uint32_t vertexX,
        std::uint32_t vertexZ
    ) const;

    RenderTarget* _renderTarget = nullptr;
    Persistence* _persistence = nullptr;
    Clock* _clock = nullptr;
    ChunkSimulation* _chunkSimulation = nullptr;
    SunSimulation* _sunSimulation = nullptr;
    WorldConfig _config;
    Registry _registry;
    std::vector<Chunk> _chunks;
    std::vector<Entity> _terrainEntities;
    Land _land;
    Sun _sun;
    std::uint64_t _updateCount = 0;
    double _nextCheckpointRealTimeSeconds = 5.0;
};
