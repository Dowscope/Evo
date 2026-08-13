#include "systems/EventSystem.hpp"

#include "events/EventSource.hpp"

#include <utility>

EventSystem::EventSystem() : System("Event") {}

void EventSystem::init() {
    System::init();
}

void EventSystem::registerSource(EventSource& source) {
    sources_.push_back(&source);
    source.setEventCallback([this](Event event) { queue(event); });
}

void EventSystem::registerListener(EventListener& listener) {
    listeners_.push_back(&listener);
}

void EventSystem::update() {
    for (EventSource* source : sources_) {
        source->pollEvents();
    }

    std::vector<Event> events = std::move(pendingEvents_);
    pendingEvents_.clear();
    for (const Event& event : events) {
        for (EventListener* listener : listeners_) {
            listener->onEvent(event);
        }
    }
}

void EventSystem::queue(Event event) {
    pendingEvents_.push_back(event);
}
