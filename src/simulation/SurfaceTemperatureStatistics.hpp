#pragma once

class SurfaceTemperatureStatistics {
public:
    virtual ~SurfaceTemperatureStatistics() = default;
    [[nodiscard]] virtual float averageSurfaceTemperatureCelsius() const = 0;
};
