#pragma once

#include "events/Event.hpp"

#include <functional>

class EventSource {
public:
    using EventCallback = std::function<void(Event)>;

    virtual ~EventSource() = default;
    virtual void setEventCallback(EventCallback callback) = 0;
    virtual void pollEvents() = 0;
};
