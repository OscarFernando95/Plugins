#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>
#include <random>
#include <vector>

namespace testutil
{
inline double toDb (double linear) { return 20.0 * std::log10 (std::max (linear, 1e-12)); }
inline double fromDb (double db) { return std::pow (10.0, db / 20.0); }

inline std::vector<float> sine (int n, double freq, double sampleRate, double amp = 1.0, double dc = 0.0)
{
    std::vector<float> out (static_cast<size_t> (n));
    for (int i = 0; i < n; ++i)
        out[static_cast<size_t> (i)] = static_cast<float> (dc + amp * std::sin (2.0 * std::numbers::pi * freq * i / sampleRate));
    return out;
}

inline std::vector<float> noise (int n, float amp = 0.5f, unsigned seed = 1234)
{
    std::mt19937 rng (seed);
    std::uniform_real_distribution<float> dist (-amp, amp);
    std::vector<float> out (static_cast<size_t> (n));
    for (auto& s : out)
        s = dist (rng);
    return out;
}

/** Amplitud de una senoidal de frecuencia conocida: proyección con ventana de Hann. */
inline double measureAmplitude (const float* x, int n, double freq, double sampleRate)
{
    std::complex<double> acc = 0.0;
    double wSum = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double w = 0.5 - 0.5 * std::cos (2.0 * std::numbers::pi * i / (n - 1));
        acc += w * x[i] * std::polar (1.0, -2.0 * std::numbers::pi * freq * i / sampleRate);
        wSum += w;
    }
    return 2.0 * std::abs (acc) / wSum;
}

/** Diferencia máxima entre dos señales, en dBFS. */
inline double residualDb (const float* a, const float* b, int n)
{
    double peak = 0.0;
    for (int i = 0; i < n; ++i)
        peak = std::max (peak, std::abs (static_cast<double> (a[i]) - b[i]));
    return toDb (peak);
}
/** Salto máximo entre muestras consecutivas desde `from`. Solo mira bloques completos: las pruebas que
    procesan por bloques dejan sin rellenar la cola del vector. */
inline double stepMax (const std::vector<float>& x, size_t from, size_t to, size_t block = 256)
{
    to = std::min (to, x.size() - x.size() % block);
    double m = 0.0;
    for (size_t i = from + 1; i < to; ++i)
        m = std::max (m, double (std::abs (x[i] - x[i - 1])));
    return m;
}
} // namespace testutil
