#pragma once

#include <cmath>

#include "Saturators.h"

namespace ferb
{
/** ADAA de primer orden: y = (F(v) − F(v₋₁)) / (v − v₋₁). Un canal por instancia.

    Con v casi igual a v₋₁ la división es inestable y se usa f en el punto medio (el error
    es de segundo orden en dv). El umbral es mayor que el de la ficha original (1e-5) porque
    aquí todo va en doble precisión y el punto medio es exacto a ese nivel. */
class Adaa
{
public:
    void reset() noexcept { vPrev = 0.0; FPrev = 0.0; primed = false; }

    double process (SatType type, double v) noexcept
    {
        const double Fv = saturators::F (type, v);

        if (! primed)
        {
            vPrev = v;
            FPrev = Fv;
            primed = true;
        }

        const double dv = v - vPrev;
        const double y = std::abs (dv) > threshold ? (Fv - FPrev) / dv
                                                   : saturators::f (type, 0.5 * (v + vPrev));
        vPrev = v;
        FPrev = Fv;
        return y;
    }

private:
    static constexpr double threshold = 1.0e-4;
    double vPrev = 0.0, FPrev = 0.0;
    bool primed = false;
};
} // namespace ferb
