#include "systems/SunSystem.hpp"

#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>

SunSystem::SunSystem(WorldConfig config)
    : System("Sun"), _config(config) {}

void SunSystem::init() {
    System::init();
}

void SunSystem::updateSun(Sun& sun, const TimeFrame& time) {
    const float worldWidth = static_cast<float>(
        _config.chunkSize * std::max(_config.chunksX, _config.chunksZ)
    ) * _config.cellSizeMeters;
    const float orbitRadius = worldWidth * 0.75F;
    const float orbitHeight = worldWidth * 0.12F;
    const float sunAngle = static_cast<float>(time.dayProgress) *
                           2.0F * glm::pi<float>();
    sun.position = {
        std::cos(sunAngle) * orbitRadius,
        orbitHeight + std::sin(sunAngle) * orbitRadius,
        0.0F,
    };
    sun.intensity = std::clamp(
        (sun.position.y + worldWidth * 0.03F) / (worldWidth * 0.20F),
        0.0F,
        1.0F
    );
}
