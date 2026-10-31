#pragma once

#include <cmath>
#include <complex>
#include <numbers>
#include <vector>

namespace ferb
{
/** FFT de base 2 en doble precisión, para cálculos de diseño (no para el hilo de audio). `inverse`
    incluye la división entre N. */
inline void fft (std::vector<std::complex<double>>& a, bool inverse)
{
    const size_t n = a.size();
    for (size_t i = 1, j = 0; i < n; ++i)
    {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap (a[i], a[j]);
    }

    for (size_t len = 2; len <= n; len <<= 1)
    {
        const double angle = 2.0 * std::numbers::pi / static_cast<double> (len) * (inverse ? 1.0 : -1.0);
        const std::complex<double> step (std::cos (angle), std::sin (angle));
        for (size_t i = 0; i < n; i += len)
        {
            std::complex<double> w = 1.0;
            for (size_t k = 0; k < len / 2; ++k)
            {
                const auto u = a[i + k], v = a[i + k + len / 2] * w;
                a[i + k] = u + v;
                a[i + k + len / 2] = u - v;
                w *= step;
            }
        }
    }

    if (inverse)
        for (auto& v : a)
            v /= static_cast<double> (n);
}
} // namespace ferb
