#pragma once

#include <cstdint>

struct TimeFrame {
    double realDeltaSeconds = 0.0;
    double realTimeSeconds = 0.0;
    double simulationDeltaSeconds = 0.0;
    double simulationTimeSeconds = 0.0;
    double fixedStepSeconds = 0.1;
    std::uint32_t fixedSteps = 0;
    std::uint64_t day = 1;
    double dayProgress = 0.0;
};

class Clock {
public:
    virtual ~Clock() = default;
    [[nodiscard]] virtual const TimeFrame& frame() const = 0;
};
