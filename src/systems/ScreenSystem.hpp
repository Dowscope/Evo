#pragma once

#include "events/Event.hpp"
#include "events/EventSource.hpp"
#include "core/ApplicationConfig.hpp"
#include "systems/System.hpp"

#include <memory>

class RenderSystem;
struct GLFWwindow;

class RenderTarget {
public:
    virtual ~RenderTarget() = default;
    virtual void render() = 0;
};

class ScreenSystem final : public System,
                           public EventSource,
                           public EventListener,
                           public RenderTarget {
public:
    explicit ScreenSystem(WindowConfig config);
    ~ScreenSystem() override;

    void init() override;
    void setEventCallback(EventCallback callback) override;
    void pollEvents() override;
    void onEvent(const Event& event) override;
    void render() override;

    [[nodiscard]] bool shouldClose() const;

private:
    static void handleKey(
        GLFWwindow* window,
        int key,
        int scanCode,
        int action,
        int modifiers
    );
    static void handleWindowClose(GLFWwindow* window);

    void emit(Event event) const;

    GLFWwindow* window_ = nullptr;
    WindowConfig config_;
    EventCallback eventCallback_;
    std::unique_ptr<RenderSystem> renderer_;
};
