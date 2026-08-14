#pragma once

#include "events/Event.hpp"
#include "rendering/Camera.hpp"
#include "systems/System.hpp"

#include <glm/vec3.hpp>

class CameraSystem final : public System,
                           public Camera,
                           public EventListener {
public:
    CameraSystem();

    void init() override;
    void setViewport(int width, int height) override;
    [[nodiscard]] CameraFrame frame() const override;
    void onEvent(const Event& event) override;

private:
    void _orbit(double deltaX, double deltaY);
    void _pan(double deltaX, double deltaY);
    void _zoom(double amount);

    int _viewportWidth = 1280;
    int _viewportHeight = 720;
    glm::vec3 _target{0.0F, 0.0F, 0.0F};
    float _yaw = 45.0F;
    float _pitch = 35.0F;
    float _distance = 12.0F;
    double _lastMouseX = 0.0;
    double _lastMouseY = 0.0;
    bool _hasMousePosition = false;
    bool _orbiting = false;
    bool _panning = false;
};
