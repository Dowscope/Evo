#pragma once

enum class EventType {
    EscapePressed,
    TemperatureOverlayToggled,
    WindowCloseRequested,
    MouseMoved,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseScrolled,
    ViewportResized,
};

struct Event {
    EventType type;
    double x = 0.0;
    double y = 0.0;
    int button = -1;
};

class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void onEvent(const Event& event) = 0;
};
