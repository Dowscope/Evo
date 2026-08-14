#pragma once

#include "events/Event.hpp"
#include "systems/System.hpp"

#include <vector>

class EventSource;

class EventSystem final : public System {
public:
    EventSystem();

    void init() override;
    void registerSource(EventSource& source);
    void registerListener(EventListener& listener);
    void update();

private:
    void _queue(Event event);

    std::vector<EventSource*> _sources;
    std::vector<EventListener*> _listeners;
    std::vector<Event> _pendingEvents;
};
