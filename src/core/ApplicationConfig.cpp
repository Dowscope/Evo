#include "core/ApplicationConfig.hpp"

#include <charconv>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

std::string_view trim(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

template <typename Number>
Number parseNumber(std::string_view value, std::string_view key) {
    Number number{};
    const auto [end, error] = std::from_chars(
        value.data(),
        value.data() + value.size(),
        number
    );
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::runtime_error("Invalid numeric config value for " +
                                 std::string(key));
    }
    return number;
}

} // namespace

ApplicationConfig ConfigLoader::load(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Unable to open config file: " + path.string());
    }

    ApplicationConfig config;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const std::string_view content = trim(line);
        if (content.empty() || content.front() == '#') {
            continue;
        }

        const auto separator = content.find('=');
        if (separator == std::string_view::npos) {
            throw std::runtime_error("Malformed config line " +
                                     std::to_string(lineNumber));
        }

        const std::string_view key = trim(content.substr(0, separator));
        const std::string_view value = trim(content.substr(separator + 1));

        if (key == "window.title") {
            config.window.title = value;
        } else if (key == "window.width") {
            config.window.width = parseNumber<int>(value, key);
        } else if (key == "window.height") {
            config.window.height = parseNumber<int>(value, key);
        } else if (key == "network.address") {
            config.network.address = value;
        } else if (key == "network.port") {
            config.network.port = parseNumber<std::uint16_t>(value, key);
        }
    }

    if (config.window.title.empty() || config.window.width <= 0 ||
        config.window.height <= 0) {
        throw std::runtime_error("Window configuration is invalid");
    }
    return config;
}
