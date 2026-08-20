#pragma once

#include "rendering/Scene.hpp"

class SurfaceTemperatureSimulation {
public:
    virtual ~SurfaceTemperatureSimulation() = default;
    virtual void setSunState(const Sun& sun) = 0;
};
