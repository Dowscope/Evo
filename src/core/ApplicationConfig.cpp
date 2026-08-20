#include "core/ApplicationConfig.hpp"

#include <charconv>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

template <typename Number>
Number parseNumber(std::string_view value, std::string_view key) {
    Number number{};
    const auto [end, error] = std::from_chars(
        value.data(),
        value.data() + value.size(),
        number
    );
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::runtime_error(
            "Invalid numeric config value for " + std::string(key)
        );
    }
    return number;
}

std::string parseString(std::string_view value, std::string_view key) {
    if (value.size() < 2 || value.front() != '"' || value.back() != '"') {
        throw std::runtime_error(
            "Invalid string config value for " + std::string(key)
        );
    }
    std::string result;
    result.reserve(value.size() - 2);
    bool escaping = false;
    for (const char character : value.substr(1, value.size() - 2)) {
        if (escaping) {
            if (character != '"' && character != '\\') {
                throw std::runtime_error(
                    "Unsupported escape in config value for " + std::string(key)
                );
            }
            result.push_back(character);
            escaping = false;
        } else if (character == '\\') {
            escaping = true;
        } else {
            result.push_back(character);
        }
    }
    if (escaping) {
        throw std::runtime_error(
            "Invalid escape in config value for " + std::string(key)
        );
    }
    return result;
}

} // namespace

ApplicationConfig ConfigLoader::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open config file: " + path.string());
    }

    ApplicationConfig config;
    std::string line;
    std::size_t lineNumber = 0;
    bool opened = false;
    bool closed = false;
    while (std::getline(input, line)) {
        ++lineNumber;
        std::string_view content = trim(line);
        if (content.empty()) {
            continue;
        }
        if (!opened && content == "{") {
            opened = true;
            continue;
        }
        if (opened && content == "}") {
            closed = true;
            continue;
        }
        if (!opened || closed) {
            throw std::runtime_error(
                "Malformed JSON config at line " + std::to_string(lineNumber)
            );
        }
        if (content.back() == ',') {
            content = trim(content.substr(0, content.size() - 1));
        }
        const auto separator = content.find(':');
        if (separator == std::string_view::npos) {
            throw std::runtime_error(
                "Malformed JSON config at line " + std::to_string(lineNumber)
            );
        }
        const std::string key = parseString(trim(content.substr(0, separator)), "key");
        const std::string_view value = trim(content.substr(separator + 1));

        if (key == "window.title") {
            config.window.title = parseString(value, key);
        } else if (key == "window.width") {
            config.window.width = parseNumber<int>(value, key);
        } else if (key == "window.height") {
            config.window.height = parseNumber<int>(value, key);
        } else if (key == "network.address") {
            config.network.address = parseString(value, key);
        } else if (key == "network.port") {
            config.network.port = parseNumber<std::uint16_t>(value, key);
        } else if (key == "world.seed") {
            config.world.seed = parseNumber<std::uint64_t>(value, key);
        } else if (key == "world.chunk_size") {
            config.world.chunkSize = parseNumber<std::uint32_t>(value, key);
        } else if (key == "world.chunks_x") {
            config.world.chunksX = parseNumber<std::uint32_t>(value, key);
        } else if (key == "world.chunks_z") {
            config.world.chunksZ = parseNumber<std::uint32_t>(value, key);
        } else if (key == "world.cell_size_meters") {
            config.world.cellSizeMeters = parseNumber<float>(value, key);
        } else if (key == "time.fixed_step_seconds") {
            config.time.fixedStepSeconds = parseNumber<double>(value, key);
        } else if (key == "time.scale") {
            config.time.timeScale = parseNumber<double>(value, key);
        } else if (key == "time.day_length_seconds") {
            config.time.dayLengthSeconds = parseNumber<double>(value, key);
        } else if (key == "climate.initial_surface_temperature_celsius") {
            config.climate.initialSurfaceTemperatureCelsius =
                parseNumber<float>(value, key);
        } else if (key == "climate.solar_irradiance_w_m2") {
            config.climate.solarIrradianceWattsPerSquareMeter =
                parseNumber<float>(value, key);
        } else if (key == "climate.surface_absorptivity") {
            config.climate.surfaceAbsorptivity = parseNumber<float>(value, key);
        } else if (key == "climate.surface_heat_capacity_j_m2_k") {
            config.climate.surfaceHeatCapacityJoulesPerSquareMeterKelvin =
                parseNumber<float>(value, key);
        } else if (key == "climate.surface_heat_transfer_w_m2_k") {
            config.climate.surfaceHeatTransferWattsPerSquareMeterKelvin =
                parseNumber<float>(value, key);
        } else if (key == "climate.surface_emissivity") {
            config.climate.surfaceEmissivity = parseNumber<float>(value, key);
        } else if (key == "atmosphere.minimum_air_temperature_celsius") {
            config.atmosphere.minimumAirTemperatureCelsius =
                parseNumber<float>(value, key);
        } else if (key == "atmosphere.maximum_air_temperature_celsius") {
            config.atmosphere.maximumAirTemperatureCelsius =
                parseNumber<float>(value, key);
        } else if (key == "atmosphere.minimum_temperature_hour") {
            config.atmosphere.minimumTemperatureHour =
                parseNumber<float>(value, key);
        } else if (key == "atmosphere.maximum_temperature_hour") {
            config.atmosphere.maximumTemperatureHour =
                parseNumber<float>(value, key);
        } else if (key == "atmosphere.clear_sky_temperature_offset_celsius") {
            config.atmosphere.clearSkyTemperatureOffsetCelsius =
                parseNumber<float>(value, key);
        } else if (key == "soil.initial_temperature_celsius") {
            config.soil.initialTemperatureCelsius =
                parseNumber<float>(value, key);
        } else if (key == "soil.deep_ground_temperature_celsius") {
            config.soil.deepGroundTemperatureCelsius =
                parseNumber<float>(value, key);
        } else if (key == "soil.thermal_conductivity_w_m_k") {
            config.soil.thermalConductivityWattsPerMeterKelvin =
                parseNumber<float>(value, key);
        } else if (key == "soil.volumetric_heat_capacity_j_m3_k") {
            config.soil.volumetricHeatCapacityJoulesPerCubicMeterKelvin =
                parseNumber<float>(value, key);
        } else if (key == "soil.surface_conductance_w_m2_k") {
            config.soil.surfaceConductanceWattsPerSquareMeterKelvin =
                parseNumber<float>(value, key);
        } else if (key == "soil.layer_1_thickness_m") {
            config.soil.layerThicknessMeters[0] = parseNumber<float>(value, key);
        } else if (key == "soil.layer_2_thickness_m") {
            config.soil.layerThicknessMeters[1] = parseNumber<float>(value, key);
        } else if (key == "soil.layer_3_thickness_m") {
            config.soil.layerThicknessMeters[2] = parseNumber<float>(value, key);
        } else if (key == "soil.layer_4_thickness_m") {
            config.soil.layerThicknessMeters[3] = parseNumber<float>(value, key);
        } else if (key == "soil.deep_boundary_depth_m") {
            config.soil.deepBoundaryDepthMeters =
                parseNumber<float>(value, key);
        }
    }

    if (!opened || !closed) {
        throw std::runtime_error("JSON config must contain one object");
    }
    if (config.window.title.empty() || config.window.width <= 0 ||
        config.window.height <= 0) {
        throw std::runtime_error("Window configuration is invalid");
    }
    if (config.world.chunkSize < 2 || config.world.chunkSize > 64) {
        throw std::runtime_error("world.chunk_size must be between 2 and 64");
    }
    if (config.world.chunksX == 0 || config.world.chunksX > 64 ||
        config.world.chunksZ == 0 || config.world.chunksZ > 64) {
        throw std::runtime_error(
            "world chunk counts must each be between 1 and 64"
        );
    }
    if (config.world.cellSizeMeters < 0.1F ||
        config.world.cellSizeMeters > 100.0F) {
        throw std::runtime_error(
            "world.cell_size_meters must be between 0.1 and 100"
        );
    }
    if (!std::isfinite(config.time.fixedStepSeconds) ||
        config.time.fixedStepSeconds <= 0.0 ||
        config.time.fixedStepSeconds > 3600.0) {
        throw std::runtime_error(
            "time.fixed_step_seconds must be greater than 0 and at most 3600"
        );
    }
    if (!std::isfinite(config.time.timeScale) || config.time.timeScale < 0.0 ||
        config.time.timeScale > 100000.0) {
        throw std::runtime_error("time.scale must be between 0 and 100000");
    }
    if (!std::isfinite(config.time.dayLengthSeconds) ||
        config.time.dayLengthSeconds <= 0.0) {
        throw std::runtime_error("time.day_length_seconds must be positive");
    }
    if (!std::isfinite(config.climate.initialSurfaceTemperatureCelsius) ||
        config.climate.initialSurfaceTemperatureCelsius < -100.0F ||
        config.climate.initialSurfaceTemperatureCelsius > 100.0F) {
        throw std::runtime_error(
            "climate.initial_surface_temperature_celsius must be between -100 and 100"
        );
    }
    if (!std::isfinite(config.climate.solarIrradianceWattsPerSquareMeter) ||
        config.climate.solarIrradianceWattsPerSquareMeter < 0.0F ||
        config.climate.solarIrradianceWattsPerSquareMeter > 2000.0F) {
        throw std::runtime_error(
            "climate.solar_irradiance_w_m2 must be between 0 and 2000"
        );
    }
    if (!std::isfinite(config.climate.surfaceAbsorptivity) ||
        config.climate.surfaceAbsorptivity < 0.0F ||
        config.climate.surfaceAbsorptivity > 1.0F) {
        throw std::runtime_error(
            "climate.surface_absorptivity must be between 0 and 1"
        );
    }
    if (!std::isfinite(
            config.climate.surfaceHeatCapacityJoulesPerSquareMeterKelvin
        ) || config.climate.surfaceHeatCapacityJoulesPerSquareMeterKelvin <= 0.0F) {
        throw std::runtime_error(
            "climate.surface_heat_capacity_j_m2_k must be positive"
        );
    }
    if (!std::isfinite(
            config.climate.surfaceHeatTransferWattsPerSquareMeterKelvin
        ) || config.climate.surfaceHeatTransferWattsPerSquareMeterKelvin < 0.0F) {
        throw std::runtime_error(
            "climate.surface_heat_transfer_w_m2_k must be non-negative"
        );
    }
    if (!std::isfinite(config.climate.surfaceEmissivity) ||
        config.climate.surfaceEmissivity < 0.0F ||
        config.climate.surfaceEmissivity > 1.0F) {
        throw std::runtime_error(
            "climate.surface_emissivity must be between 0 and 1"
        );
    }
    if (!std::isfinite(config.atmosphere.minimumAirTemperatureCelsius) ||
        !std::isfinite(config.atmosphere.maximumAirTemperatureCelsius) ||
        config.atmosphere.minimumAirTemperatureCelsius < -100.0F ||
        config.atmosphere.maximumAirTemperatureCelsius > 100.0F ||
        config.atmosphere.minimumAirTemperatureCelsius >=
            config.atmosphere.maximumAirTemperatureCelsius) {
        throw std::runtime_error(
            "atmosphere air temperatures must be ordered between -100 and 100"
        );
    }
    if (!std::isfinite(config.atmosphere.minimumTemperatureHour) ||
        !std::isfinite(config.atmosphere.maximumTemperatureHour) ||
        config.atmosphere.minimumTemperatureHour < 0.0F ||
        config.atmosphere.minimumTemperatureHour >= 24.0F ||
        config.atmosphere.maximumTemperatureHour < 0.0F ||
        config.atmosphere.maximumTemperatureHour >= 24.0F ||
        config.atmosphere.minimumTemperatureHour >=
            config.atmosphere.maximumTemperatureHour) {
        throw std::runtime_error(
            "atmosphere temperature hours must be ordered within [0, 24)"
        );
    }
    if (!std::isfinite(config.atmosphere.clearSkyTemperatureOffsetCelsius) ||
        config.atmosphere.minimumAirTemperatureCelsius +
            config.atmosphere.clearSkyTemperatureOffsetCelsius <= -273.15F) {
        throw std::runtime_error(
            "atmosphere clear-sky temperature must remain above absolute zero"
        );
    }
    if (!std::isfinite(config.soil.initialTemperatureCelsius) ||
        config.soil.initialTemperatureCelsius < -100.0F ||
        config.soil.initialTemperatureCelsius > 100.0F) {
        throw std::runtime_error(
            "soil.initial_temperature_celsius must be between -100 and 100"
        );
    }
    if (!std::isfinite(config.soil.deepGroundTemperatureCelsius) ||
        config.soil.deepGroundTemperatureCelsius < -100.0F ||
        config.soil.deepGroundTemperatureCelsius > 100.0F) {
        throw std::runtime_error(
            "soil.deep_ground_temperature_celsius must be between -100 and 100"
        );
    }
    if (!std::isfinite(config.soil.thermalConductivityWattsPerMeterKelvin) ||
        config.soil.thermalConductivityWattsPerMeterKelvin < 0.0F) {
        throw std::runtime_error(
            "soil.thermal_conductivity_w_m_k must be non-negative"
        );
    }
    if (!std::isfinite(
            config.soil.volumetricHeatCapacityJoulesPerCubicMeterKelvin
        ) || config.soil.volumetricHeatCapacityJoulesPerCubicMeterKelvin <= 0.0F) {
        throw std::runtime_error(
            "soil.volumetric_heat_capacity_j_m3_k must be positive"
        );
    }
    if (!std::isfinite(config.soil.surfaceConductanceWattsPerSquareMeterKelvin) ||
        config.soil.surfaceConductanceWattsPerSquareMeterKelvin < 0.0F) {
        throw std::runtime_error(
            "soil.surface_conductance_w_m2_k must be non-negative"
        );
    }
    float modeledSoilDepthMeters = 0.0F;
    for (const float thicknessMeters : config.soil.layerThicknessMeters) {
        if (!std::isfinite(thicknessMeters) || thicknessMeters <= 0.0F) {
            throw std::runtime_error("soil layer thicknesses must be positive");
        }
        modeledSoilDepthMeters += thicknessMeters;
    }
    if (!std::isfinite(config.soil.deepBoundaryDepthMeters) ||
        config.soil.deepBoundaryDepthMeters <= modeledSoilDepthMeters) {
        throw std::runtime_error(
            "soil.deep_boundary_depth_m must be below all modeled soil layers"
        );
    }
    return config;
}
