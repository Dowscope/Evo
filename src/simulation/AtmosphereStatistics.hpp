#pragma once

#include "simulation/Atmosphere.hpp"

class AtmosphereStatistics {
public:
    virtual ~AtmosphereStatistics() = default;
    [[nodiscard]] virtual const AtmosphereState& atmosphere() const = 0;
};
