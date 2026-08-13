#pragma once

enum class EventType {
    EscapePressed,
    WindowCloseRequested,
};

struct Event {
    EventType type;
};

class EventListener {
public:
    virtual ~EventListener() = default;
    virtual void onEvent(const Event& event) = 0;
};
