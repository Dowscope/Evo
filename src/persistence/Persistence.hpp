#pragma once

#include <optional>
#include <string>
#include <string_view>

class Persistence {
public:
    virtual ~Persistence() = default;

    [[nodiscard]] virtual std::optional<std::string> get(
        std::string_view key
    ) const = 0;
    virtual void set(std::string key, std::string value) = 0;
    virtual void checkpoint() = 0;
};
