#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

struct WindowConfig {
    std::string title = "EVO";
    int width = 1280;
    int height = 720;
};

struct NetworkConfig {
    std::string address = "127.0.0.1";
    std::uint16_t port = 0;
};

struct WorldConfig {
    std::uint64_t seed = 2847593;
    std::uint32_t chunkSize = 16;
    std::uint32_t chunksX = 2;
    std::uint32_t chunksZ = 2;
    float cellSizeMeters = 1.0F;
};

struct TimeConfig {
    double fixedStepSeconds = 60.0;
    double timeScale = 1440.0;
    double dayLengthSeconds = 86400.0;
};

struct ClimateConfig {
    float initialSurfaceTemperatureCelsius = 15.0F;
    float solarIrradianceWattsPerSquareMeter = 1000.0F;
    float surfaceAbsorptivity = 0.75F;
    float surfaceHeatCapacityJoulesPerSquareMeterKelvin = 200000.0F;
    float surfaceHeatTransferWattsPerSquareMeterKelvin = 10.0F;
    float surfaceEmissivity = 0.95F;
};

struct AtmosphereConfig {
    float minimumAirTemperatureCelsius = 10.0F;
    float maximumAirTemperatureCelsius = 22.0F;
    float minimumTemperatureHour = 6.0F;
    float maximumTemperatureHour = 15.0F;
    float clearSkyTemperatureOffsetCelsius = -20.0F;
};

struct SoilThermalConfig {
    float initialTemperatureCelsius = 15.0F;
    float deepGroundTemperatureCelsius = 12.0F;
    float thermalConductivityWattsPerMeterKelvin = 1.0F;
    float volumetricHeatCapacityJoulesPerCubicMeterKelvin = 2000000.0F;
    float surfaceConductanceWattsPerSquareMeterKelvin = 5.0F;
    std::array<float, 4> layerThicknessMeters{0.1F, 0.2F, 0.5F, 1.2F};
    float deepBoundaryDepthMeters = 3.0F;
};

struct ApplicationConfig {
    WindowConfig window;
    NetworkConfig network;
    WorldConfig world;
    TimeConfig time;
    ClimateConfig climate;
    AtmosphereConfig atmosphere;
    SoilThermalConfig soil;
};

class ConfigLoader final {
public:
    ConfigLoader() = delete;

    [[nodiscard]] static ApplicationConfig load(
        const std::filesystem::path& path
    );
};
