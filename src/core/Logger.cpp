#include "core/Logger.hpp"

#include <iostream>
#include <mutex>

namespace {

std::mutex outputMutex;

void writeToConsole(std::string_view message) {
    std::cout << message << std::endl;
}

} // namespace

void Logger::write(std::string_view message) {
    const std::scoped_lock lock(outputMutex);
    writeToConsole(message);
}

void Logger::info(std::string_view message) {
    _writeLevel("INFO", message);
}

void Logger::warning(std::string_view message) {
    _writeLevel("WARNING", message);
}

void Logger::error(std::string_view message) {
    _writeLevel("ERROR", message);
}

void Logger::_writeLevel(std::string_view level, std::string_view message) {
    const std::scoped_lock lock(outputMutex);
    std::cout << '[' << level << "] " << message << std::endl;
}
