#pragma once

#include <cmath>
#include <numbers>

namespace ferb
{
/** Paso alto de primer orden: y[n] = x[n] - x[n-1] + R·y[n-1]. Un canal por instancia. */
class DcBlocker
{
public:
    void prepare (double sampleRate, double cutoffHz = 10.0) noexcept
    {
        r = std::exp (-2.0 * std::numbers::pi * cutoffHz / sampleRate);
        reset();
    }

    void reset() noexcept { x1 = 0.0; y1 = 0.0; }

    float process (float x) noexcept
    {
        const double in = x;
        const double y = in - x1 + r * y1;
        x1 = in;
        y1 = y;
        return static_cast<float> (y);
    }

private:
    double r = 0.999;
    double x1 = 0.0, y1 = 0.0;
};
} // namespace ferb
