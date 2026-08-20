#pragma once

#include "simulation/Atmosphere.hpp"
#include "time/Clock.hpp"

class AtmosphereSimulation {
public:
    virtual ~AtmosphereSimulation() = default;
    virtual void updateAtmosphere(const TimeFrame& time) = 0;
    [[nodiscard]] virtual const AtmosphereState& atmosphere() const = 0;
};
