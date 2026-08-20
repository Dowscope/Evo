#include "systems/SurfaceTemperatureSystem.hpp"

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

SurfaceTemperatureSystem::SurfaceTemperatureSystem(
    ClimateConfig climateConfig,
    SoilThermalConfig soilConfig
) : System("Surface Temperature"),
    _climateConfig(climateConfig),
    _soilConfig(soilConfig) {}

void SurfaceTemperatureSystem::init() {
    System::init();
}

void SurfaceTemperatureSystem::setSunState(const Sun& sun) {
    _sun = sun;
}

void SurfaceTemperatureSystem::setAtmosphereState(
    const AtmosphereState& atmosphere
) {
    _atmosphere = atmosphere;
}

float SurfaceTemperatureSystem::averageSurfaceTemperatureCelsius() const {
    return _averageSurfaceTemperatureCelsius;
}

void SurfaceTemperatureSystem::updateChunk(
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
        const float absorbedSolarWattsPerSquareMeter =
            _climateConfig.solarIrradianceWattsPerSquareMeter *
            _climateConfig.surfaceAbsorptivity * _sun.intensity * incidence;
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

void SurfaceTemperatureSystem::collectBoundaryTransfers(
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

void SurfaceTemperatureSystem::applyBoundaryTransfers(
    Registry& /* registry */,
    std::span<Chunk> /* chunks */
) {}
