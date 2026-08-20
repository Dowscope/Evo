#include "systems/AtmosphereSystem.hpp"

#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>

AtmosphereSystem::AtmosphereSystem(AtmosphereConfig config)
    : System("Atmosphere"), _config(config) {}

void AtmosphereSystem::init() {
    System::init();
}

void AtmosphereSystem::updateAtmosphere(const TimeFrame& time) {
    const float hour = static_cast<float>(time.dayProgress * 24.0);
    float temperatureProgress = 0.0F;
    if (hour >= _config.minimumTemperatureHour &&
        hour < _config.maximumTemperatureHour) {
        temperatureProgress =
            (hour - _config.minimumTemperatureHour) /
            (_config.maximumTemperatureHour - _config.minimumTemperatureHour);
        _state.airTemperatureCelsius = glm::mix(
            _config.minimumAirTemperatureCelsius,
            _config.maximumAirTemperatureCelsius,
            _smoothTransition(temperatureProgress)
        );
    } else {
        const float hoursSinceMaximum = hour >= _config.maximumTemperatureHour
            ? hour - _config.maximumTemperatureHour
            : hour + 24.0F - _config.maximumTemperatureHour;
        const float coolingDurationHours =
            24.0F - _config.maximumTemperatureHour +
            _config.minimumTemperatureHour;
        temperatureProgress = hoursSinceMaximum / coolingDurationHours;
        _state.airTemperatureCelsius = glm::mix(
            _config.maximumAirTemperatureCelsius,
            _config.minimumAirTemperatureCelsius,
            _smoothTransition(temperatureProgress)
        );
    }
    _state.effectiveSkyTemperatureCelsius =
        _state.airTemperatureCelsius +
        _config.clearSkyTemperatureOffsetCelsius;
}

const AtmosphereState& AtmosphereSystem::atmosphere() const {
    return _state;
}

float AtmosphereSystem::_smoothTransition(float progress) {
    return 0.5F - 0.5F * std::cos(glm::pi<float>() * progress);
}
