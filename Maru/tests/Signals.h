#pragma once

// Señales sintéticas para probar Maru: ruido blanco, voz sintética (armónicos con modulación de sílabas), pulsaciones.

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
#include <vector>

namespace sig
{
inline double dbToLin (double db) { return std::pow (10.0, db / 20.0); }
inline double toDb (double lin) { return 20.0 * std::log10 (std::max (lin, 1.0e-12)); }

inline std::vector<float> whiteNoise (int n, double levelDb, unsigned seed = 1)
{
    std::mt19937 rng (seed);
    std::normal_distribution<float> dist (0.0f, float (dbToLin (levelDb)));
    std::vector<float> x (static_cast<size_t> (n));
    for (auto& v : x)
        v = dist (rng);
    return x;
}

inline double rms (const std::vector<float>& x, int from, int to)
{
    to = std::min<int> (to, int (x.size()));
    double s = 0.0;
    for (int i = from; i < to; ++i)
        s += double (x[size_t (i)]) * x[size_t (i)];
    return std::sqrt (s / std::max (1, to - from));
}

/** Voz sintética: 25 armónicos de una fundamental que ondula, con envolvente espectral de formantes y sílabas de ≈ 4 Hz con pausas. */
inline std::vector<float> voice (int n, double fs, double levelDb, double f0 = 130.0)
{
    std::vector<float> x (static_cast<size_t> (n), 0.0f);
    double phase[25] = {};
    for (int i = 0; i < n; ++i)
    {
        const double t = double (i) / fs;
        const double f = f0 * (1.0 + 0.06 * std::sin (2.0 * std::numbers::pi * 0.7 * t));
        const double syllable = std::max (0.0, std::sin (2.0 * std::numbers::pi * 3.5 * t));
        const double word = std::sin (2.0 * std::numbers::pi * 0.45 * t) > -0.6 ? 1.0 : 0.0; // pausas entre palabras
        double s = 0.0;
        for (int h = 1; h <= 25; ++h)
        {
            phase[h - 1] += 2.0 * std::numbers::pi * f * h / fs;
            const double hz = f * h, formant = std::exp (-std::pow ((hz - 600.0) / 500.0, 2.0)) + 0.6 * std::exp (-std::pow ((hz - 1800.0) / 700.0, 2.0)) + 0.1;
            s += formant / h * std::sin (phase[h - 1]);
        }
        x[size_t (i)] = float (s * syllable * word);
    }
    const double r = rms (x, 0, n);
    const double g = dbToLin (levelDb) / std::max (r, 1.0e-12);
    for (auto& v : x)
        v = float (v * g);
    return x;
}

inline std::vector<float> add (std::vector<float> a, const std::vector<float>& b)
{
    for (size_t i = 0; i < a.size() && i < b.size(); ++i)
        a[i] += b[i];
    return a;
}
} // namespace sig
