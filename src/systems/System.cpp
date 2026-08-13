#include "systems/System.hpp"

#include "core/Logger.hpp"

#include <string>
#include <utility>

System::System(std::string name) : name_(std::move(name)) {
    Logger::write('[' + name_ + " System] Starting");
}

System::~System() {
    Logger::write('[' + name_ + " System] Shutdown");
}

void System::init() {
    Logger::write('[' + name_ + " System] Initialized");
}
