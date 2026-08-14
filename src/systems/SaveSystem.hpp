#pragma once

#include "persistence/Persistence.hpp"
#include "systems/System.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

class SaveSystem final : public System, public Persistence {
public:
    explicit SaveSystem(std::filesystem::path savePath);
    ~SaveSystem() override;

    void init() override;
    [[nodiscard]] std::optional<std::string> get(
        std::string_view key
    ) const override;
    void set(std::string key, std::string value) override;
    void checkpoint() override;
    void flush();

private:
    void _load();

    std::filesystem::path _savePath;
    std::unordered_map<std::string, std::string> _values;
    bool _dirty = false;
};
