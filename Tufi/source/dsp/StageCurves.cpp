#include "StageCurves.h"

#include <algorithm>
#include <cmath>
#include <functional>

namespace tufi
{
namespace
{
double softplus (double a) { return a > 30.0 ? a : std::log1p (std::exp (a)); }

// Modelo de Koren, triodo (12AX7): corriente de placa en amperios.
double triodeIp (double Eg, double Ep)
{
    constexpr double mu = 100.0, kp = 600.0, kvb = 300.0, kg1 = 1060.0, kx = 1.4;
    const double E1 = (Ep / kp) * softplus (kp * (1.0 / mu + Eg / std::sqrt (kvb + Ep * Ep)));
    return E1 > 0.0 ? 2.0 * std::pow (E1, kx) / kg1 : 0.0;
}

// Modelo de Koren, pentodo con rejilla pantalla a tensión fija (tipo EL34).
double pentodeIp (double Eg, double Ep)
{
    constexpr double mu = 11.0, kp = 60.0, kvb = 24.0, kg1 = 650.0, kx = 1.35, Eg2 = 250.0;
    const double E1 = (Eg2 / kp) * softplus (kp * (1.0 / mu + Eg / Eg2));
    return E1 > 0.0 ? 2.0 * std::pow (E1, kx) / kg1 * std::atan (Ep / kvb) : 0.0;
}

/** Tensión de placa con recta de carga: Ep = B − Ip·RL, con Ip = ip(Eg, Ep). Bisección. */
double plateVoltage (const std::function<double (double, double)>& ip, double Eg, double B, double RL)
{
    double lo = 0.0, hi = B / RL;
    for (int i = 0; i < 60; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        (mid - ip (Eg, B - mid * RL) < 0.0 ? lo : hi) = mid;
    }
    return B - 0.5 * (lo + hi) * RL;
}

/** Curva y(x) = (Ep0 − Ep(Eg0 + G·x)) / pendiente, con la polarización Eg0 elegida para que la placa
    repose en `rest`·B. */
Table tubeCurve (const std::function<double (double, double)>& ip, double B, double RL, double rest, double G)
{
    double lo = -80.0, hi = 0.0; // más negativa la rejilla, más tensión de placa
    for (int i = 0; i < 60; ++i)
    {
        const double mid = 0.5 * (lo + hi);
        // Placa por encima del reposo: la rejilla es demasiado negativa, hay que subirla.
        (plateVoltage (ip, mid, B, RL) > rest * B ? lo : hi) = mid;
    }
    const double Eg0 = 0.5 * (lo + hi);
    const double Ep0 = plateVoltage (ip, Eg0, B, RL);

    const double d = 1.0e-4;
    const double slope = (plateVoltage (ip, Eg0 - G * d, B, RL) - plateVoltage (ip, Eg0 + G * d, B, RL)) / (2.0 * d);

    Table t (tableSize);
    for (int i = 0; i < tableSize; ++i)
    {
        const double x = -tableHalfRange + i * tableStep;
        t[static_cast<size_t> (i)] = (Ep0 - plateVoltage (ip, Eg0 + G * x, B, RL)) / slope;
    }
    return t;
}

/** Par antiparalelo de diodos de Shockley con resistencia en serie: Vin = Vd + A·sinh(Vd/vt). Simétrica. */
Table diodeCurve()
{
    constexpr double vt = 0.05, A = 1.0e-4, Vs = 0.5;
    const double slope = 1.0 / (1.0 + A / vt); // pendiente en cero de Vd(Vin)

    Table t (tableSize);
    for (int i = 0; i < tableSize; ++i)
    {
        const double x = -tableHalfRange + i * tableStep;
        const double vin = x * Vs;
        double lo = -std::abs (vin), hi = std::abs (vin);
        for (int k = 0; k < 80; ++k)
        {
            const double mid = 0.5 * (lo + hi);
            (mid + A * std::sinh (mid / vt) - vin < 0.0 ? lo : hi) = mid;
        }
        t[static_cast<size_t> (i)] = 0.5 * (lo + hi) / Vs / slope;
    }
    return t;
}

Table makeStage (int stage)
{
    switch (stage)
    {
        case 0:  return tubeCurve (triodeIp, 300.0, 100.0e3, 0.55, 2.5);
        case 1:  return tubeCurve (pentodeIp, 400.0, 2500.0, 0.5, 8.0);
        default: return diodeCurve();
    }
}
} // namespace

const Table& stageBase (int stage)
{
    static const Table tables[3] = { makeStage (0), makeStage (1), makeStage (2) };
    return tables[std::clamp (stage, 0, 2)];
}

double lookup (const Table& table, double x) noexcept
{
    const double pos = (x + tableHalfRange) / tableStep;
    const int i = std::clamp (static_cast<int> (std::floor (pos)), 0, tableSize - 2);
    const double u = pos - i;
    return table[static_cast<size_t> (i)] + u * (table[static_cast<size_t> (i) + 1] - table[static_cast<size_t> (i)]);
}
} // namespace tufi
