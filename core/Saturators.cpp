#include "Saturators.h"

#include <algorithm>
#include <cmath>

namespace ferb::saturators
{
namespace
{
constexpr double tubeBias = 0.25; // b
constexpr double geKPositive = 3.0;
constexpr double geKNegative = 2.0;

double sech2 (double x) noexcept
{
    const double t = std::tanh (x);
    return 1.0 - t * t;
}

// Antiderivada de asinh(k·v)/k, con constante 0 en v = 0 sumada para que sea continua.
double geHalf (double v, double k) noexcept
{
    return (v * std::asinh (k * v) - std::sqrt (1.0 + k * k * v * v) / k) / k;
}
} // namespace

double logCosh (double v) noexcept
{
    const double a = std::abs (v);
    return a + std::log1p (std::exp (-2.0 * a)) - 0.6931471805599453;
}

double f (SatType type, double v) noexcept
{
    switch (type)
    {
        case SatType::Tube:
            return (std::tanh (v + tubeBias) - std::tanh (tubeBias)) / sech2 (tubeBias);
        case SatType::Tape:
            return std::tanh (v);
        case SatType::Solid:
            return v / std::sqrt (1.0 + v * v);
        case SatType::GermaniumDiode:
        {
            const double k = v >= 0.0 ? geKPositive : geKNegative;
            return std::asinh (k * v) / k;
        }
        case SatType::Digital:
            return std::clamp (v, -1.0, 1.0);
    }
    return v;
}

double F (SatType type, double v) noexcept
{
    switch (type)
    {
        case SatType::Tube:
            return (logCosh (v + tubeBias) - std::tanh (tubeBias) * v) / sech2 (tubeBias);
        case SatType::Tape:
            return logCosh (v);
        case SatType::Solid:
            return std::sqrt (1.0 + v * v);
        case SatType::GermaniumDiode:
        {
            // La mitad negativa se desplaza para igualar el valor en 0 de la positiva.
            constexpr double shift = -1.0 / (geKPositive * geKPositive) + 1.0 / (geKNegative * geKNegative);
            return v >= 0.0 ? geHalf (v, geKPositive) : geHalf (v, geKNegative) + shift;
        }
        case SatType::Digital:
            return std::abs (v) <= 1.0 ? 0.5 * v * v : std::abs (v) - 0.5;
    }
    return 0.5 * v * v;
}
} // namespace ferb::saturators
