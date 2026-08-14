#include "systems/SaveSystem.hpp"

#include "core/Logger.hpp"

#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

SaveSystem::SaveSystem(std::filesystem::path savePath)
    : System("Save"), _savePath(std::move(savePath)) {}

SaveSystem::~SaveSystem() {
    try {
        flush();
    } catch (const std::exception& error) {
        Logger::error(std::string("Failed to save persistent data: ") +
                      error.what());
    }
}

void SaveSystem::init() {
    _load();
    System::init();
}

std::optional<std::string> SaveSystem::get(std::string_view key) const {
    const auto value = _values.find(std::string(key));
    if (value == _values.end()) {
        return std::nullopt;
    }
    return value->second;
}

void SaveSystem::set(std::string key, std::string value) {
    _values.insert_or_assign(std::move(key), std::move(value));
    _dirty = true;
}

void SaveSystem::checkpoint() {
    flush();
}

void SaveSystem::flush() {
    if (!_dirty) {
        return;
    }

    const std::filesystem::path parent = _savePath.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::filesystem::path temporaryPath = _savePath;
    temporaryPath += ".tmp";
    {
        std::ofstream output(temporaryPath, std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Unable to create " +
                                     temporaryPath.string());
        }
        for (const auto& [key, value] : _values) {
            output << key << '=' << value << '\n';
        }
        if (!output) {
            throw std::runtime_error("Unable to write " +
                                     temporaryPath.string());
        }
    }

    std::error_code error;
    std::filesystem::rename(temporaryPath, _savePath, error);
    if (error) {
        std::filesystem::remove(_savePath, error);
        error.clear();
        std::filesystem::rename(temporaryPath, _savePath, error);
    }
    if (error) {
        throw std::runtime_error("Unable to replace save file: " +
                                 error.message());
    }
    _dirty = false;
}

void SaveSystem::_load() {
    std::ifstream input(_savePath);
    if (!input) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator != std::string::npos) {
            _values.insert_or_assign(
                line.substr(0, separator),
                line.substr(separator + 1)
            );
        }
    }
}
