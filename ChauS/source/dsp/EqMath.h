#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

#include <Biquad.h>

namespace chaus::eq
{
/** Campana (RBJ) con ganancia en dB, normalizada por a0. */
inline ferb::Biquad peaking (double fs, double freq, double q, double gainDb) noexcept
{
    const double A = std::pow (10.0, gainDb / 40.0), w0 = 2.0 * std::numbers::pi * freq / fs;
    const double alpha = std::sin (w0) / (2.0 * q), cw = std::cos (w0);
    const double a0 = 1.0 + alpha / A;
    ferb::Biquad f;
    f.b0 = (1.0 + alpha * A) / a0;
    f.b1 = -2.0 * cw / a0;
    f.b2 = (1.0 - alpha * A) / a0;
    f.a1 = -2.0 * cw / a0;
    f.a2 = (1.0 - alpha / A) / a0;
    return f;
}

/** Respuesta en magnitud (dB) de un biquad a `freq`. */
inline double magnitudeDb (const ferb::Biquad& f, double freq, double fs) noexcept
{
    const double w = 2.0 * std::numbers::pi * freq / fs;
    const std::complex<double> z1 = std::polar (1.0, -w), z2 = std::polar (1.0, -2.0 * w);
    const auto num = f.b0 + f.b1 * z1 + f.b2 * z2, den = 1.0 + f.a1 * z1 + f.a2 * z2;
    return 20.0 * std::log10 (std::max (std::abs (num) / std::max (std::abs (den), 1.0e-12), 1.0e-12));
}
} // namespace chaus::eq
