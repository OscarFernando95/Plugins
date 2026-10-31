#pragma once

#include <cmath>
#include <complex>
#include <numbers>

namespace poty::ui
{
/** Respuesta en dB de una campana de realce (RBJ), la misma que usa el motor. */
inline double peakingDb (double freq, double gainDb, double q, double at, double sampleRate)
{
    if (gainDb <= 0.0)
        return 0.0;

    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = 2.0 * std::numbers::pi * std::min (freq, 0.45 * sampleRate) / sampleRate;
    const double alpha = std::sin (w0) / (2.0 * q);
    const double c = std::cos (w0);
    const double a0 = 1.0 + alpha / A;

    const double b0 = (1.0 + alpha * A) / a0, b1 = -2.0 * c / a0, b2 = (1.0 - alpha * A) / a0;
    const double a1 = -2.0 * c / a0, a2 = (1.0 - alpha / A) / a0;

    const auto z1 = std::polar (1.0, -2.0 * std::numbers::pi * at / sampleRate);
    const auto z2 = z1 * z1;
    const auto h = (b0 + b1 * z1 + b2 * z2) / (1.0 + a1 * z1 + a2 * z2);
    return 20.0 * std::log10 (std::max (std::abs (h), 1.0e-9));
}
} // namespace poty::ui
