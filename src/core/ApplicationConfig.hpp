#pragma once

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
    std::uint32_t gridSize = 32;
};

struct ApplicationConfig {
    WindowConfig window;
    NetworkConfig network;
    WorldConfig world;
};

class ConfigLoader final {
public:
    ConfigLoader() = delete;

    [[nodiscard]] static ApplicationConfig load(
        const std::filesystem::path& path
    );
};
