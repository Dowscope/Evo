#include "weather/TemperatureModule.hpp"

#include "ecs/components/TerrainComponents.hpp"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace {

constexpr float kelvinOffset = 273.15F;
constexpr float stefanBoltzmannWattsPerSquareMeterKelvinFourth =
    5.670374419e-8F;

} // namespace

TemperatureModule::TemperatureModule(
    WorldConfig worldConfig,
    ClimateConfig climateConfig,
    SoilThermalConfig soilConfig
) : _worldConfig(worldConfig),
    _climateConfig(climateConfig),
    _soilConfig(soilConfig) {}

void TemperatureModule::setSunState(const Sun& sun) {
    _sun = sun;
}

void TemperatureModule::setAtmosphereState(
    const AtmosphereState& atmosphere
) {
    _atmosphere = atmosphere;
}

float TemperatureModule::averageSurfaceTemperatureCelsius() const {
    return _averageSurfaceTemperatureCelsius;
}

void TemperatureModule::beginTick(
    Registry& registry,
    std::span<Chunk> chunks,
    double /* fixedStepSeconds */
) {
    const std::uint32_t cellsX =
        _worldConfig.chunkSize * _worldConfig.chunksX;
    const std::uint32_t cellsZ =
        _worldConfig.chunkSize * _worldConfig.chunksZ;
    std::vector<Entity> entities(cellsX * cellsZ, nullEntity);
    for (const Chunk& chunk : chunks) {
        for (const Entity entity : chunk.terrainCells) {
            const GridPosition position = registry.get<GridPosition>(entity);
            entities[position.z * cellsX + position.x] = entity;
        }
    }
    _directSolarExposure.assign(cellsX * cellsZ, 1.0F);

    const float horizontalSunLength = std::hypot(
        _sun.direction.x,
        _sun.direction.z
    );
    if (_sun.direction.y <= 0.0F || horizontalSunLength <= 1.0e-5F) {
        return;
    }
    const float rayX = _sun.direction.x / horizontalSunLength;
    const float rayZ = _sun.direction.z / horizontalSunLength;
    const float verticalRisePerMeter =
        _sun.direction.y / horizontalSunLength;
    const float sampleStepMeters = _worldConfig.cellSizeMeters * 0.5F;
    const float maximumDistanceMeters = std::hypot(
        static_cast<float>(cellsX),
        static_cast<float>(cellsZ)
    ) * _worldConfig.cellSizeMeters;
    constexpr float surfaceToleranceMeters = 0.02F;

    for (std::uint32_t z = 0; z < cellsZ; ++z) {
        for (std::uint32_t x = 0; x < cellsX; ++x) {
            const Entity entity = entities[z * cellsX + x];
            if (entity == nullEntity) {
                continue;
            }
            const float originElevation =
                registry.get<Elevation>(entity).meters;
            for (float distanceMeters = sampleStepMeters;
                 distanceMeters <= maximumDistanceMeters;
                 distanceMeters += sampleStepMeters) {
                const float sampleGridX = static_cast<float>(x) + 0.5F +
                    rayX * distanceMeters / _worldConfig.cellSizeMeters;
                const float sampleGridZ = static_cast<float>(z) + 0.5F +
                    rayZ * distanceMeters / _worldConfig.cellSizeMeters;
                const int sampleX = static_cast<int>(std::floor(sampleGridX));
                const int sampleZ = static_cast<int>(std::floor(sampleGridZ));
                if (sampleX < 0 || sampleZ < 0 ||
                    sampleX >= static_cast<int>(cellsX) ||
                    sampleZ >= static_cast<int>(cellsZ)) {
                    break;
                }
                if (sampleX == static_cast<int>(x) &&
                    sampleZ == static_cast<int>(z)) {
                    continue;
                }
                const Entity sampleEntity = entities[
                    static_cast<std::uint32_t>(sampleZ) * cellsX +
                    static_cast<std::uint32_t>(sampleX)
                ];
                if (sampleEntity == nullEntity) {
                    continue;
                }
                const float sampleCenterDistanceMeters =
                    ((static_cast<float>(sampleX) - static_cast<float>(x)) *
                         rayX +
                     (static_cast<float>(sampleZ) - static_cast<float>(z)) *
                         rayZ) * _worldConfig.cellSizeMeters;
                const float rayElevation = originElevation +
                    sampleCenterDistanceMeters * verticalRisePerMeter;
                if (registry.get<Elevation>(sampleEntity).meters >
                    rayElevation + surfaceToleranceMeters) {
                    _directSolarExposure[z * cellsX + x] = 0.0F;
                    break;
                }
            }
        }
    }
}

void TemperatureModule::updateChunk(
    Registry& registry,
    Chunk& chunk,
    std::span<Chunk> /* chunks */,
    double fixedStepSeconds
) {
    bool changed = false;
    for (const Entity entity : chunk.terrainCells) {
        const Slope slope = registry.get<Slope>(entity);
        const Aspect aspect = registry.get<Aspect>(entity);
        const float slopeRadians = glm::radians(slope.degrees);
        glm::vec3 normal{0.0F, 1.0F, 0.0F};
        if (aspect.defined) {
            normal = {
                std::sin(slopeRadians) * std::cos(aspect.radians),
                std::cos(slopeRadians),
                std::sin(slopeRadians) * std::sin(aspect.radians),
            };
        }
        const float incidence = std::max(glm::dot(normal, _sun.direction), 0.0F);
        const GridPosition position = registry.get<GridPosition>(entity);
        const std::uint32_t cellsX =
            _worldConfig.chunkSize * _worldConfig.chunksX;
        const float directExposure = _directSolarExposure.empty()
            ? 1.0F
            : _directSolarExposure[position.z * cellsX + position.x];
        const float diffuseIncidence = std::max(_sun.direction.y, 0.0F) *
            (0.5F + 0.5F * normal.y);
        const float solarIncidence =
            (1.0F - _climateConfig.diffuseSolarFraction) * incidence *
                directExposure +
            _climateConfig.diffuseSolarFraction * diffuseIncidence;
        const float absorbedSolarWattsPerSquareMeter =
            _climateConfig.solarIrradianceWattsPerSquareMeter *
            _climateConfig.surfaceAbsorptivity * _sun.intensity * solarIncidence;
        SurfaceTemperature& temperature = registry.get<SurfaceTemperature>(entity);
        const float sensibleHeatLossWattsPerSquareMeter =
            _climateConfig.surfaceHeatTransferWattsPerSquareMeterKelvin *
            (temperature.celsius - _atmosphere.airTemperatureCelsius);
        const float surfaceKelvin = temperature.celsius + kelvinOffset;
        const float skyKelvin =
            _atmosphere.effectiveSkyTemperatureCelsius + kelvinOffset;
        const float longwaveHeatLossWattsPerSquareMeter =
            _climateConfig.surfaceEmissivity *
            stefanBoltzmannWattsPerSquareMeterKelvinFourth *
            (std::pow(surfaceKelvin, 4.0F) - std::pow(skyKelvin, 4.0F));
        SoilTemperatureProfile& soil =
            registry.get<SoilTemperatureProfile>(entity);
        const auto previousSoilTemperatures = soil.celsius;
        const float conductiveHeatLossWattsPerSquareMeter =
            _soilConfig.surfaceConductanceWattsPerSquareMeterKelvin *
            (temperature.celsius - previousSoilTemperatures[0]);
        const float netEnergyJoulesPerSquareMeter =
            (absorbedSolarWattsPerSquareMeter - sensibleHeatLossWattsPerSquareMeter -
             longwaveHeatLossWattsPerSquareMeter -
             conductiveHeatLossWattsPerSquareMeter) *
            static_cast<float>(fixedStepSeconds);
        const float changeCelsius = netEnergyJoulesPerSquareMeter /
            _climateConfig.surfaceHeatCapacityJoulesPerSquareMeterKelvin;
        temperature.celsius += changeCelsius;

        std::array<float, SoilTemperatureProfile::layerCount>
            soilNetPowerWattsPerSquareMeter{};
        soilNetPowerWattsPerSquareMeter[0] =
            conductiveHeatLossWattsPerSquareMeter;
        for (std::size_t layer = 0;
             layer + 1 < SoilTemperatureProfile::layerCount;
             ++layer) {
            const float centerDistanceMeters =
                (_soilConfig.layerThicknessMeters[layer] +
                 _soilConfig.layerThicknessMeters[layer + 1]) * 0.5F;
            const float downwardHeatFluxWattsPerSquareMeter =
                _soilConfig.thermalConductivityWattsPerMeterKelvin /
                centerDistanceMeters *
                (previousSoilTemperatures[layer] -
                 previousSoilTemperatures[layer + 1]);
            soilNetPowerWattsPerSquareMeter[layer] -=
                downwardHeatFluxWattsPerSquareMeter;
            soilNetPowerWattsPerSquareMeter[layer + 1] +=
                downwardHeatFluxWattsPerSquareMeter;
        }

        float modeledDepthMeters = 0.0F;
        for (const float thicknessMeters : _soilConfig.layerThicknessMeters) {
            modeledDepthMeters += thicknessMeters;
        }
        constexpr std::size_t deepestLayer =
            SoilTemperatureProfile::layerCount - 1;
        const float deepestCenterMeters = modeledDepthMeters -
            _soilConfig.layerThicknessMeters[deepestLayer] * 0.5F;
        const float deepBoundaryDistanceMeters =
            _soilConfig.deepBoundaryDepthMeters - deepestCenterMeters;
        const float deepHeatLossWattsPerSquareMeter =
            _soilConfig.thermalConductivityWattsPerMeterKelvin /
            deepBoundaryDistanceMeters *
            (previousSoilTemperatures[deepestLayer] -
             _soilConfig.deepGroundTemperatureCelsius);
        soilNetPowerWattsPerSquareMeter[deepestLayer] -=
            deepHeatLossWattsPerSquareMeter;

        for (std::size_t layer = 0;
             layer < SoilTemperatureProfile::layerCount;
             ++layer) {
            const float arealHeatCapacityJoulesPerSquareMeterKelvin =
                _soilConfig.volumetricHeatCapacityJoulesPerCubicMeterKelvin *
                _soilConfig.layerThicknessMeters[layer];
            soil.celsius[layer] = previousSoilTemperatures[layer] +
                soilNetPowerWattsPerSquareMeter[layer] *
                static_cast<float>(fixedStepSeconds) /
                arealHeatCapacityJoulesPerSquareMeterKelvin;
        }
        changed = changed || std::abs(changeCelsius) > 1.0e-5F;
    }
    chunk.terrainMeshDirty = chunk.terrainMeshDirty || changed;
}

void TemperatureModule::collectBoundaryTransfers(
    Registry& registry,
    std::span<Chunk> chunks
) {
    double sumCelsius = 0.0;
    std::uint64_t cellCount = 0;
    for (const Chunk& chunk : chunks) {
        for (const Entity entity : chunk.terrainCells) {
            sumCelsius += registry.get<SurfaceTemperature>(entity).celsius;
            ++cellCount;
        }
    }
    if (cellCount > 0) {
        _averageSurfaceTemperatureCelsius = static_cast<float>(
            sumCelsius / static_cast<double>(cellCount)
        );
    }
}

void TemperatureModule::applyBoundaryTransfers(
    Registry& /* registry */,
    std::span<Chunk> /* chunks */
) {}
