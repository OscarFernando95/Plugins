#pragma once

#include <cmath>
#include <numbers>

namespace ferb
{
/** Biquad en forma transpuesta directa II, doble precisión, un canal. Coeficientes normalizados por a0. */
struct Biquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;

    void reset() noexcept { z1 = z2 = 0.0; }

    float process (float x) noexcept
    {
        const double in = x;
        const double y = b0 * in + z1;
        z1 = b1 * in - a1 * y + z2;
        z2 = b2 * in - a2 * y;
        return static_cast<float> (y);
    }

    /** Estante de agudos (RBJ). `gainDb` positivo realza. */
    static Biquad highShelf (double sampleRate, double freq, double gainDb, double slope = 0.7) noexcept
    {
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * std::numbers::pi * freq / sampleRate;
        const double cw = std::cos (w0), sw = std::sin (w0);
        const double alpha = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0);
        const double sq = 2.0 * std::sqrt (A) * alpha;

        const double a0 = (A + 1.0) - (A - 1.0) * cw + sq;
        Biquad f;
        f.b0 = A * ((A + 1.0) + (A - 1.0) * cw + sq) / a0;
        f.b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cw) / a0;
        f.b2 = A * ((A + 1.0) + (A - 1.0) * cw - sq) / a0;
        f.a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cw) / a0;
        f.a2 = ((A + 1.0) - (A - 1.0) * cw - sq) / a0;
        return f;
    }

    /** El filtro inverso (ceros y polos intercambiados). Estable si el original es de fase mínima. */
    Biquad inverse() const noexcept
    {
        Biquad f;
        f.b0 = 1.0 / b0;
        f.b1 = a1 / b0;
        f.b2 = a2 / b0;
        f.a1 = b1 / b0;
        f.a2 = b2 / b0;
        return f;
    }
};
} // namespace ferb
