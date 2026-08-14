#include "systems/EventSystem.hpp"

#include "events/EventSource.hpp"

#include <utility>

EventSystem::EventSystem() : System("Event") {}

void EventSystem::init() {
    System::init();
}

void EventSystem::registerSource(EventSource& source) {
    _sources.push_back(&source);
    source.setEventCallback([this](Event event) { _queue(event); });
}

void EventSystem::registerListener(EventListener& listener) {
    _listeners.push_back(&listener);
}

void EventSystem::update() {
    for (EventSource* source : _sources) {
        source->pollEvents();
    }

    std::vector<Event> events = std::move(_pendingEvents);
    _pendingEvents.clear();
    for (const Event& event : events) {
        for (EventListener* listener : _listeners) {
            listener->onEvent(event);
        }
    }
}

void EventSystem::_queue(Event event) {
    _pendingEvents.push_back(event);
}
