#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;
};
struct Land {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

struct CameraFrame {
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
};

struct Scene {
    const Land* land = nullptr;
    CameraFrame camera;
};
