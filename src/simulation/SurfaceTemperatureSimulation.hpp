#pragma once

#include "rendering/Scene.hpp"
#include "simulation/Atmosphere.hpp"

class SurfaceTemperatureSimulation {
public:
    virtual ~SurfaceTemperatureSimulation() = default;
    virtual void setSunState(const Sun& sun) = 0;
    virtual void setAtmosphereState(const AtmosphereState& atmosphere) = 0;
};
