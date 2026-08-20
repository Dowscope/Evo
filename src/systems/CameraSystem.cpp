#include "systems/CameraSystem.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace {
constexpr int leftMouseButton = 0;
constexpr int middleMouseButton = 2;
}

CameraSystem::CameraSystem() : System("Camera") {}

void CameraSystem::init() {
    System::init();
}

void CameraSystem::setViewport(int width, int height) {
    _viewportWidth = std::max(width, 1);
    _viewportHeight = std::max(height, 1);
}

CameraFrame CameraSystem::frame() const {
    const float yaw = glm::radians(_yaw);
    const float pitch = glm::radians(_pitch);
    const glm::vec3 direction{
        std::cos(pitch) * std::cos(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::sin(yaw),
    };
    const glm::vec3 position = _target + direction * _distance;

    CameraFrame result;
    result.view = glm::lookAt(position, _target, {0.0F, 1.0F, 0.0F});
    result.projection = glm::perspective(
        glm::radians(45.0F),
        static_cast<float>(_viewportWidth) /
            static_cast<float>(_viewportHeight),
        0.1F,
        1000.0F
    );
    result.projection[1][1] *= -1.0F;
    return result;
}

void CameraSystem::onEvent(const Event& event) {
    switch (event.type) {
    case EventType::MouseMoved:
        if (_hasMousePosition) {
            const double deltaX = event.x - _lastMouseX;
            const double deltaY = event.y - _lastMouseY;
            if (_orbiting) {
                _orbit(deltaX, deltaY);
            }
            if (_panning) {
                _pan(deltaX, deltaY);
            }
        }
        _lastMouseX = event.x;
        _lastMouseY = event.y;
        _hasMousePosition = true;
        break;
    case EventType::MouseButtonPressed:
        _orbiting = event.button == leftMouseButton || _orbiting;
        _panning = event.button == middleMouseButton || _panning;
        break;
    case EventType::MouseButtonReleased:
        if (event.button == leftMouseButton) {
            _orbiting = false;
        }
        if (event.button == middleMouseButton) {
            _panning = false;
        }
        break;
    case EventType::MouseScrolled:
        _zoom(event.y);
        break;
    case EventType::ViewportResized:
        setViewport(static_cast<int>(event.x), static_cast<int>(event.y));
        break;
    default:
        break;
    }
}

void CameraSystem::_orbit(double deltaX, double deltaY) {
    _yaw += static_cast<float>(deltaX) * 0.25F;
    _pitch = std::clamp(
        _pitch - static_cast<float>(deltaY) * 0.25F,
        5.0F,
        89.0F
    );
}

void CameraSystem::_pan(double deltaX, double deltaY) {
    const float yaw = glm::radians(_yaw);
    const glm::vec3 right{-std::sin(yaw), 0.0F, std::cos(yaw)};
    const glm::vec3 forward{-std::cos(yaw), 0.0F, -std::sin(yaw)};
    const float scale = _distance * 0.0015F;
    _target += right * static_cast<float>(-deltaX) * scale;
    _target += forward * static_cast<float>(deltaY) * scale;
}

void CameraSystem::_zoom(double amount) {
    _distance = std::clamp(
        _distance * (1.0F - static_cast<float>(amount) * 0.1F),
        2.0F,
        500.0F
    );
}
