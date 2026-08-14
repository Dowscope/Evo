#pragma once

#include "rendering/Scene.hpp"

class Camera {
public:
    virtual ~Camera() = default;
    virtual void setViewport(int width, int height) = 0;
    [[nodiscard]] virtual CameraFrame frame() const = 0;
};
