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
    const float sunAngle = static_cast<float>(time.dayProgress) *
                           2.0F * glm::pi<float>() - glm::half_pi<float>();
    sun.direction = glm::normalize(glm::vec3{
        std::cos(sunAngle),
        std::sin(sunAngle),
        0.0F,
    });
    sun.position = {
        sun.direction.x * orbitRadius,
        sun.direction.y * orbitRadius,
        sun.direction.z * orbitRadius,
    };
    sun.intensity = std::clamp(
        (sun.direction.y + 0.10F) / 0.30F,
        0.0F,
        1.0F
    );
}
