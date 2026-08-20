#include "systems/TimeSystem.hpp"

#include <cmath>
#include <stdexcept>

TimeSystem::TimeSystem(TimeConfig config)
    : System("Time"), _config(config), _timeScale(config.timeScale) {}

void TimeSystem::init() {
    _lastFrame = std::chrono::steady_clock::now();
    _frame.fixedStepSeconds = _config.fixedStepSeconds;
    System::init();
}

void TimeSystem::update() {
    const auto now = std::chrono::steady_clock::now();
    _frame.realDeltaSeconds = std::chrono::duration<double>(
        now - _lastFrame
    ).count();
    _lastFrame = now;
    _frame.realTimeSeconds += _frame.realDeltaSeconds;

    _frame.simulationDeltaSeconds = _paused
        ? 0.0
        : _frame.realDeltaSeconds * _timeScale;
    _frame.simulationTimeSeconds += _frame.simulationDeltaSeconds;
    _frame.day = static_cast<std::uint64_t>(std::floor(
        _frame.simulationTimeSeconds / _config.dayLengthSeconds
    )) + 1;
    _frame.dayProgress = std::fmod(
        _frame.simulationTimeSeconds,
        _config.dayLengthSeconds
    ) / _config.dayLengthSeconds;
    _fixedAccumulator += _frame.simulationDeltaSeconds;
    _frame.fixedSteps = static_cast<std::uint32_t>(std::floor(
        _fixedAccumulator / _frame.fixedStepSeconds
    ));
    _fixedAccumulator -= static_cast<double>(_frame.fixedSteps) *
                         _frame.fixedStepSeconds;
}

void TimeSystem::setPaused(bool paused) {
    _paused = paused;
}

void TimeSystem::setTimeScale(double timeScale) {
    if (!std::isfinite(timeScale) || timeScale < 0.0) {
        throw std::runtime_error("Time scale must be finite and non-negative");
    }
    _timeScale = timeScale;
}

bool TimeSystem::paused() const {
    return _paused;
}

double TimeSystem::timeScale() const {
    return _timeScale;
}

const TimeFrame& TimeSystem::frame() const {
    return _frame;
}
