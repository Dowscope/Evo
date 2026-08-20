#pragma once

#include "core/ApplicationConfig.hpp"
#include "simulation/SunSimulation.hpp"
#include "systems/System.hpp"

class SunSystem final : public System, public SunSimulation {
public:
    explicit SunSystem(WorldConfig config);

    void init() override;
    void updateSun(Sun& sun, const TimeFrame& time) override;

private:
    WorldConfig _config;
};
