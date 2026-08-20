#pragma once

#include "core/ApplicationConfig.hpp"
#include "simulation/AtmosphereSimulation.hpp"
#include "simulation/AtmosphereStatistics.hpp"
#include "systems/System.hpp"

class AtmosphereSystem final : public System,
                               public AtmosphereSimulation,
                               public AtmosphereStatistics {
public:
    explicit AtmosphereSystem(AtmosphereConfig config);

    void init() override;
    void updateAtmosphere(const TimeFrame& time) override;
    [[nodiscard]] const AtmosphereState& atmosphere() const override;

private:
    [[nodiscard]] static float _smoothTransition(float progress);

    AtmosphereConfig _config;
    AtmosphereState _state;
};
