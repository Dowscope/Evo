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
        config.time.fixedStepSeconds > 10.0) {
        throw std::runtime_error(
            "time.fixed_step_seconds must be greater than 0 and at most 10"
        );
    }
    if (!std::isfinite(config.time.timeScale) || config.time.timeScale < 0.0 ||
        config.time.timeScale > 1000.0) {
        throw std::runtime_error("time.scale must be between 0 and 1000");
    }
    if (!std::isfinite(config.time.dayLengthSeconds) ||
        config.time.dayLengthSeconds <= 0.0) {
        throw std::runtime_error("time.day_length_seconds must be positive");
    }
    return config;
}
