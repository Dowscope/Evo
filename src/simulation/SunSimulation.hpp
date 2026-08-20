#pragma once

#include "rendering/Scene.hpp"
#include "time/Clock.hpp"

class SunSimulation {
public:
    virtual ~SunSimulation() = default;
    virtual void updateSun(Sun& sun, const TimeFrame& time) = 0;
};
