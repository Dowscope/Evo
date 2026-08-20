#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
    float surfaceTemperatureCelsius = 15.0F;
};
struct Land {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::uint64_t revision = 0;
};

struct Sun {
    glm::vec3 position{0.0F, 6.0F, 8.0F};
    glm::vec3 direction{0.0F, 1.0F, 0.0F};
    float intensity = 1.0F;
};

struct CameraFrame {
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
};

struct Scene {
    const Land* land = nullptr;
    Sun sun;
    CameraFrame camera;
    bool temperatureOverlay = false;
};
