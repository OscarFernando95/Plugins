#pragma once

#include <algorithm>
#include <cmath>

namespace cremita
{
/** Curva de transferencia estática de Cremita, con x normalizado a ±1 después de Input:

        f(x) = x + k·x·(1 − x²) + b·x²·(1 − |x|)          para |x| ≤ 1

    - k sigue al control Curva (c, de −50 a +50): k = 0,125 + 0,0075·c. La pendiente en 0 es 1 + k.
    - b sigue al control Sesgo y añade armónicos pares: b = Sesgo · min(0,3; 1 − 2k). El límite
      mantiene la curva monótona (f'(1) = 1 − 2k − b ≥ 0). Con Curva = +50, k = 0,5 y b se anula.
    - f(0) = 0 y f(±1) = ±1.

    Más allá de ±1 (solo llega con Input > 0 dB) la curva se prolonga con una cola suave que sigue
    la pendiente de f en ese punto y sube hasta ±2:  f(x) = ±(1 + tanh(m·(|x| − 1))). Con Clip 0 dB
    encendido m = 0 y la curva se queda en ±1. La ficha no da esta prolongación; es una decisión de
    diseño (continua, monótona, con pendiente continua mientras m ≥ mínimo).

    Valores iniciales: se afinan de oído. */
struct CurveParams
{
    double k = 0.125;
    double b = 0.0;
    double mPos = 0.0; // pendiente de la cola en x > 1 (0 = recorte)
    double mNeg = 0.0; // idem en x < −1

    static constexpr double minTailSlope = 0.25;

    static double kFromCurve (double c) noexcept { return 0.125 + 0.0075 * c; }
    static double biasLimit (double k) noexcept { return std::max (0.0, std::min (0.3, 1.0 - 2.0 * k)); }

    /** `curve` en −50…+50, `biasPercent` en 0…100. */
    static CurveParams make (double curve, double biasPercent, bool clip) noexcept
    {
        CurveParams p;
        p.k = kFromCurve (curve);
        p.b = biasPercent / 100.0 * biasLimit (p.k);
        if (! clip)
        {
            p.mPos = std::max (1.0 - 2.0 * p.k - p.b, minTailSlope);
            p.mNeg = std::max (1.0 - 2.0 * p.k + p.b, minTailSlope);
        }
        return p;
    }
};

namespace detail
{
/** ln cosh(a) sin desbordar. */
inline double logCosh (double a) noexcept
{
    a = std::abs (a);
    return a + std::log1p (std::exp (-2.0 * a)) - 0.6931471805599453;
}
} // namespace detail

inline double curveF (const CurveParams& p, double x) noexcept
{
    const double ax = std::abs (x);
    if (ax <= 1.0)
        return x + p.k * x * (1.0 - x * x) + p.b * x * x * (1.0 - ax);

    const double u = ax - 1.0;
    const double m = x > 0.0 ? p.mPos : p.mNeg;
    const double tail = m > 0.0 ? std::tanh (m * u) : 0.0;
    return x > 0.0 ? 1.0 + tail : -(1.0 + tail);
}

/** Antiderivada de f con F(0) = 0 (continua en ±1, donde empalma con la cola). */
inline double curveAntiderivative (const CurveParams& p, double x) noexcept
{
    const double ax = std::abs (x);
    const auto base = [&] (double v) // ∫₀ᵛ f, para |v| ≤ 1
    {
        const double v2 = v * v, v3 = v2 * v;
        return v2 / 2.0 + p.k * (v2 / 2.0 - v2 * v2 / 4.0) + p.b * (v3 / 3.0 - v3 * std::abs (v) / 4.0);
    };

    if (ax <= 1.0)
        return base (x);

    const double u = ax - 1.0;
    const double m = x > 0.0 ? p.mPos : p.mNeg;
    return base (x > 0.0 ? 1.0 : -1.0) + u + (m > 0.0 ? detail::logCosh (m * u) / m : 0.0);
}

/** ADAA de primer orden de la curva: promedia f entre la muestra anterior y la actual,
    (F(x) − F(x₋₁)) / (x − x₋₁), lo que suprime el aliasing. Un canal por instancia.

    El promedio es un pasa bajos (cos(π·f/fs) a la frecuencia sobremuestreada: −0,5 dB a 10 kHz con 2×
    incluso con señales débiles) y retrasa media muestra. Se repone con (f(x) − f(x₋₁))/2, que vale
    s·dx/2 con la pendiente local s: en la zona lineal la respuesta queda plana y sin retardo, y donde
    la curva satura vale casi 0. Cuesta unos 3–9 dB de supresión de aliasing solo con saturación
    extrema (medido: Input +12 dB, 2×, −66 frente a −75 dBc), y a cambio la respuesta con señales
    normales es plana. Los parámetros pueden cambiar en cada muestra: F(x₋₁) se recalcula con los
    parámetros actuales, así la diferencia es siempre coherente. Con x casi igual a x₋₁ se usa f en el
    punto medio. */
class CurveAdaa
{
public:
    void reset() noexcept { xPrev = 0.0; primed = false; }

    double process (const CurveParams& p, double x) noexcept
    {
        if (! primed)
        {
            xPrev = x;
            primed = true;
        }

        const double dx = x - xPrev;
        const double fx = curveF (p, x), fPrev = curveF (p, xPrev);
        // (f(x) − f(x₋₁))/2 = pendiente local · dx/2: repone el medio paso que el promedio retrasa.
        const double y = std::abs (dx) > threshold
                             ? (curveAntiderivative (p, x) - curveAntiderivative (p, xPrev)) / dx + 0.5 * (fx - fPrev)
                             : curveF (p, 0.5 * (x + xPrev)) + 0.5 * (fx - fPrev);
        xPrev = x;
        return y;
    }

private:
    static constexpr double threshold = 1.0e-5;
    double xPrev = 0.0;
    bool primed = false;
};
} // namespace cremita
