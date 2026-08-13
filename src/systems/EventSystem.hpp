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
    void queue(Event event);

    std::vector<EventSource*> sources_;
    std::vector<EventListener*> listeners_;
    std::vector<Event> pendingEvents_;
};
