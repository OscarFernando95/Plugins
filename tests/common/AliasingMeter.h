#pragma once

#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

#include "TestUtils.h"

namespace testutil
{
/** Pico del espectro audible (hasta 20 kHz) respecto de la fundamental, fuera de los armónicos.
    Por encima de 20 kHz no se mide: el filtro de media banda tiene ahí su zona de transición.
    Usa 65 536 muestras de `y` desde `skip`, con ventana Blackman-Harris. */
inline double aliasingDb (const std::vector<float>& y, int skip, double f, double sr)
{
    constexpr int N = 65536;
    juce::dsp::FFT fft (16);
    std::vector<float> data (2 * N, 0.0f);
    for (int i = 0; i < N; ++i)
    {
        const double t = 2.0 * std::numbers::pi * i / (N - 1);
        const double w = 0.35875 - 0.48829 * std::cos (t) + 0.14128 * std::cos (2 * t) - 0.01168 * std::cos (3 * t);
        data[size_t (i)] = float (w * y[size_t (skip + i)]);
    }
    fft.performFrequencyOnlyForwardTransform (data.data());

    const double binHz = sr / N;
    std::vector<char> masked (N / 2, 0);
    auto mask = [&] (double hz)
    {
        const int c = int (std::lround (hz / binHz));
        for (int k = std::max (0, c - 8); k <= std::min (N / 2 - 1, c + 8); ++k)
            masked[size_t (k)] = 1;
    };
    mask (0.0);
    for (double h = f; h < sr / 2; h += f)
        mask (h);

    const int fundamentalBin = int (std::lround (f / binHz));
    float fundamental = 0.0f;
    for (int k = fundamentalBin - 4; k <= fundamentalBin + 4; ++k)
        fundamental = std::max (fundamental, data[size_t (k)]);

    float worst = 0.0f;
    const int lastBin = int (20000.0 / binHz);
    for (int k = 0; k < lastBin; ++k)
        if (! masked[size_t (k)])
            worst = std::max (worst, data[size_t (k)]);

    return toDb (worst) - toDb (fundamental);
}
} // namespace testutil
