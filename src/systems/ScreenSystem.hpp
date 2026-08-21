#pragma once

#include "events/Event.hpp"
#include "events/EventSource.hpp"
#include "core/ApplicationConfig.hpp"
#include "rendering/Scene.hpp"
#include "systems/System.hpp"

#include <memory>

class RenderSystem;
class Camera;
class Clock;
class SurfaceTemperatureStatistics;
class AtmosphereStatistics;
struct GLFWwindow;

class RenderTarget {
public:
    virtual ~RenderTarget() = default;
    virtual void render(const Land& land, const Sun& sun) = 0;
};

class ScreenSystem final : public System,
                           public EventSource,
                           public EventListener,
                           public RenderTarget {
public:
    ScreenSystem(WindowConfig config, StatsWindowConfig statsConfig);
    ~ScreenSystem() override;

    void init() override;
    void setEventCallback(EventCallback callback) override;
    void pollEvents() override;
    void onEvent(const Event& event) override;
    void render(const Land& land, const Sun& sun) override;
    void registerCamera(Camera& camera);
    void registerClock(Clock& clock);
    void registerTemperatureStatistics(
        SurfaceTemperatureStatistics& statistics
    );
    void registerAtmosphereStatistics(AtmosphereStatistics& statistics);

    [[nodiscard]] bool shouldClose() const;

private:
    static void _handleKey(
        GLFWwindow* window,
        int key,
        int scanCode,
        int action,
        int modifiers
    );
    static void _handleWindowClose(GLFWwindow* window);
    static void _handleCursor(GLFWwindow* window, double x, double y);
    static void _handleMouseButton(
        GLFWwindow* window,
        int button,
        int action,
        int modifiers
    );
    static void _handleScroll(GLFWwindow* window, double x, double y);
    static void _handleFramebufferSize(GLFWwindow* window, int width, int height);

    void _emit(Event event) const;
    void _renderStatsWindow();
    void _positionStatsWindow();

    GLFWwindow* _window = nullptr;
    GLFWwindow* _statsWindow = nullptr;
    WindowConfig _config;
    StatsWindowConfig _statsConfig;
    EventCallback _eventCallback;
    std::unique_ptr<RenderSystem> _renderer;
    Camera* _camera = nullptr;
    Clock* _clock = nullptr;
    SurfaceTemperatureStatistics* _temperatureStatistics = nullptr;
    AtmosphereStatistics* _atmosphereStatistics = nullptr;
    bool _temperatureOverlay = false;
};
