#pragma once

#include <cmath>

#include "StageCurves.h"

namespace tufi
{
/** Controles de forma convertidos a coeficientes. */
struct ShapeParams
{
    int stage = 0;
    float lowSlope = 0.0f;   // k: pendiente extra en cero, lowAmount / 10
    float peakMix = 0.0f;    // peakAmount / 10
    float ceiling = 1.0f;    // umbral T según la dureza, antes de la asimetría
    float ceilingTop = 1.0f; // umbral de la mitad positiva
    float ceilingBottom = 1.0f;

    /** Con lowAmount y peakAmount en 0 la curva es una recta: el motor lo trata como ganancia simple. */
    bool isNeutral() const noexcept { return same (lowSlope, 0.0f) && same (peakMix, 0.0f); }

    /** Igualdad exacta a propósito: detecta que un control no cambió. */
    bool operator== (const ShapeParams& o) const noexcept
    {
        return stage == o.stage && same (lowSlope, o.lowSlope) && same (peakMix, o.peakMix) && same (ceiling, o.ceiling)
               && same (ceilingTop, o.ceilingTop) && same (ceilingBottom, o.ceilingBottom);
    }

    static bool same (float a, float b) noexcept { return ! (a < b) && ! (a > b); }

    static ShapeParams fromControls (int stage, float lowAmount, float peakAmount, float peakHardness,
                                     float evenTop, float evenBottom) noexcept
    {
        // Techo de −3 dBFS (dureza 0) a −10 dBFS (dureza 10).
        const float T = std::pow (10.0f, (-3.0f - 0.7f * peakHardness) / 20.0f);

        ShapeParams p;
        p.stage = stage;
        p.lowSlope = lowAmount / 10.0f;
        p.peakMix = peakAmount / 10.0f;
        p.ceiling = T;
        p.ceilingTop = T * (1.0f - 0.4f * evenTop / 10.0f);
        p.ceilingBottom = T * (1.0f - 0.4f * evenBottom / 10.0f);
        return p;
    }
};

inline double lowLevel (double x, double k) noexcept { return x + k * x * std::exp (-x * x / 0.02); }

inline double peakSoft (double x, double T) noexcept
{
    const double a = std::abs (x);
    if (a <= T)
        return x;
    return std::copysign (T + (1.0 - T) * std::tanh ((a - T) / (1.0 - T)), x);
}

/** Curva de transferencia completa. x ya lleva la ganancia de entrada.

    Diferencia con el fragmento de la ficha: el mezclado de picos parte de la señal con el nivel bajo
    aplicado (y0), no de la curva de la etapa. Así, con Picos · Cantidad en 0 la curva es solo el término
    de nivel bajo y, con ambos en 0, una recta exacta (Tufi transparente), como dice la ficha. */
inline double shape (double x, const ShapeParams& p, const Table& base) noexcept
{
    const double y0 = lowLevel (x, p.lowSlope);
    const double b = lookup (base, y0);
    const double soft = peakSoft (b, b >= 0.0 ? p.ceilingTop : p.ceilingBottom);
    return y0 + p.peakMix * (soft - y0);
}
} // namespace tufi
