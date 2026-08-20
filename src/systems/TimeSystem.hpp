#pragma once

#include "core/ApplicationConfig.hpp"
#include "systems/System.hpp"
#include "time/Clock.hpp"

#include <chrono>

class TimeSystem final : public System, public Clock {
public:
    explicit TimeSystem(TimeConfig config);

    void init() override;
    void update();
    void setPaused(bool paused);
    void setTimeScale(double timeScale);

    [[nodiscard]] bool paused() const;
    [[nodiscard]] double timeScale() const;
    [[nodiscard]] const TimeFrame& frame() const override;

private:
    TimeConfig _config;
    std::chrono::steady_clock::time_point _lastFrame{};
    TimeFrame _frame;
    double _fixedAccumulator = 0.0;
    double _timeScale = 1.0;
    bool _paused = false;
};
