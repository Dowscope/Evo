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

struct ApplicationConfig {
    WindowConfig window;
    NetworkConfig network;
};

class ConfigLoader final {
public:
    ConfigLoader() = delete;

    [[nodiscard]] static ApplicationConfig load(
        const std::filesystem::path& path
    );
};
