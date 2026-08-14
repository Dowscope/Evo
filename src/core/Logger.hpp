#pragma once

#include <string_view>

class Logger final {
public:
    Logger() = delete;

    static void write(std::string_view message);
    static void info(std::string_view message);
    static void warning(std::string_view message);
    static void error(std::string_view message);

private:
    static void _writeLevel(std::string_view level, std::string_view message);
};
