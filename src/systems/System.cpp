#include "systems/System.hpp"

#include "core/Logger.hpp"

#include <string>
#include <utility>

System::System(std::string name) : _name(std::move(name)) {
    Logger::write('[' + _name + " System] Starting");
}

System::~System() {
    Logger::write('[' + _name + " System] Shutdown");
}

void System::init() {
    Logger::write('[' + _name + " System] Initialized");
}
