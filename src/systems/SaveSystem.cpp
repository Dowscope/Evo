#include "systems/SaveSystem.hpp"

#include "core/Logger.hpp"

#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

SaveSystem::SaveSystem(std::filesystem::path savePath)
    : System("Save"), savePath_(std::move(savePath)) {}

SaveSystem::~SaveSystem() {
    try {
        flush();
    } catch (const std::exception& error) {
        Logger::error(std::string("Failed to save persistent data: ") +
                      error.what());
    }
}

void SaveSystem::init() {
    load();
    System::init();
}

std::optional<std::string> SaveSystem::get(std::string_view key) const {
    const auto value = values_.find(std::string(key));
    if (value == values_.end()) {
        return std::nullopt;
    }
    return value->second;
}

void SaveSystem::set(std::string key, std::string value) {
    values_.insert_or_assign(std::move(key), std::move(value));
    dirty_ = true;
}

void SaveSystem::checkpoint() {
    flush();
}

void SaveSystem::flush() {
    if (!dirty_) {
        return;
    }

    const std::filesystem::path parent = savePath_.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::filesystem::path temporaryPath = savePath_;
    temporaryPath += ".tmp";
    {
        std::ofstream output(temporaryPath, std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Unable to create " +
                                     temporaryPath.string());
        }
        for (const auto& [key, value] : values_) {
            output << key << '=' << value << '\n';
        }
        if (!output) {
            throw std::runtime_error("Unable to write " +
                                     temporaryPath.string());
        }
    }

    std::error_code error;
    std::filesystem::rename(temporaryPath, savePath_, error);
    if (error) {
        std::filesystem::remove(savePath_, error);
        error.clear();
        std::filesystem::rename(temporaryPath, savePath_, error);
    }
    if (error) {
        throw std::runtime_error("Unable to replace save file: " +
                                 error.message());
    }
    dirty_ = false;
}

void SaveSystem::load() {
    std::ifstream input(savePath_);
    if (!input) {
        return;
    }

    std::string line;
    while (std::getline(input, line)) {
        const auto separator = line.find('=');
        if (separator != std::string::npos) {
            values_.insert_or_assign(
                line.substr(0, separator),
                line.substr(separator + 1)
            );
        }
    }
}
