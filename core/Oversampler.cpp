#include "Oversampler.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace ferb
{
namespace
{
// Mitad del orden de cada etapa (longitud 2M+1, M impar).
//   A: base → 2×   (transición de 20 kHz a 28 kHz con 48 kHz de base)
//   B: 2× → 4×     (la señal ya está limitada: transición muy ancha)
//   C: 4× → 8×
constexpr int halfOrder[Oversampler::maxStages] = { 39, 11, 7 };
constexpr double kaiserBeta = 10.06; // ≈ 100 dB

int stagesFor (int factor) noexcept { return factor >= 8 ? 3 : (factor >= 4 ? 2 : (factor >= 2 ? 1 : 0)); }

double bessel0 (double x)
{
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 60; ++k)
    {
        term *= (x / (2.0 * k)) * (x / (2.0 * k));
        sum += term;
    }
    return sum;
}

/** Taps h[2p], p = 0..M, de una media banda de longitud 2M+1 (M impar). Son los de posición impar
    respecto del centro; el centro vale 0,5 y los demás de posición par son cero. */
std::vector<double> designHalfband (int M)
{
    std::vector<double> h (static_cast<size_t> (M + 1));
    const double i0 = bessel0 (kaiserBeta);
    double sum = 0.0;

    for (int p = 0; p <= M; ++p)
    {
        const int e = 2 * p - M; // desplazamiento respecto al centro, siempre impar
        const double r = static_cast<double> (e) / (M + 1);
        const double w = bessel0 (kaiserBeta * std::sqrt (1.0 - r * r)) / i0;
        const double sinc = ((((e - 1) / 2) % 2 == 0) ? 1.0 : -1.0) / (std::numbers::pi * e);
        h[static_cast<size_t> (p)] = sinc * w;
        sum += h[static_cast<size_t> (p)];
    }

    // La suma de todos los taps vale 1: el centro aporta 0,5 y el resto, 0,5.
    for (auto& v : h)
        v *= 0.5 / sum;

    return h;
}
} // namespace

// Latencia total en unidades de la frecuencia más alta F: Σ_j M_j · F / 2^j.
int Oversampler::padFor (int factor) noexcept
{
    int units = 0;
    for (int j = 0; j < stagesFor (factor); ++j)
        units += halfOrder[j] * factor / (1 << j);
    return (factor - units % factor) % factor;
}

int Oversampler::latencyFor (int factor) noexcept
{
    if (factor <= 1)
        return 0;
    int units = padFor (factor);
    for (int j = 0; j < stagesFor (factor); ++j)
        units += halfOrder[j] * factor / (1 << j);
    return units / factor;
}

// ---------------------------------------------------------------- UpSampler

void UpSampler::prepare (int newMaxFactor, int maxBlockSize)
{
    maxFactor = newMaxFactor;
    factor = newMaxFactor;

    for (int j = 0; j < stagesFor (maxFactor); ++j)
    {
        auto& s = stages[static_cast<size_t> (j)];
        const int M = halfOrder[j];
        const auto h = designHalfband (M);
        s.M = M;
        s.taps.resize (h.size());
        for (size_t i = 0; i < h.size(); ++i)
            s.taps[i] = static_cast<float> (2.0 * h[i]); // ganancia 2 por el relleno de ceros
        s.history.assign (static_cast<size_t> (M), 0.0f);
        s.ext.assign (static_cast<size_t> (M + maxBlockSize * (1 << j)), 0.0f);
    }

    midA.assign (static_cast<size_t> (maxBlockSize) * 2, 0.0f);
    midB.assign (static_cast<size_t> (maxBlockSize) * 4, 0.0f);
}

void UpSampler::setFactor (int newFactor) noexcept
{
    factor = std::min (newFactor, maxFactor);
    reset();
}

void UpSampler::reset()
{
    for (auto& s : stages)
        std::fill (s.history.begin(), s.history.end(), 0.0f);
}

void UpSampler::runStage (Stage& s, const float* in, float* out, int n) noexcept
{
    const int M = s.M;
    float* ext = s.ext.data();

    std::copy (s.history.begin(), s.history.end(), ext);
    std::copy (in, in + n, ext + M);

    const float* taps = s.taps.data();
    const int delayTap = (M - 1) / 2;

    for (int k = 0; k < n; ++k)
    {
        const float* x = ext + M + k;
        float acc = 0.0f;
        for (int p = 0; p <= M; ++p)
            acc += taps[p] * x[-p];

        out[2 * k] = acc;
        out[2 * k + 1] = x[-delayTap];
    }

    std::copy (ext + n, ext + n + M, s.history.begin());
}

void UpSampler::process (const float* in, float* out, int n) noexcept
{
    switch (stagesFor (factor))
    {
        case 0: std::copy (in, in + n, out); break;
        case 1: runStage (stages[0], in, out, n); break;
        case 2:
            runStage (stages[0], in, midA.data(), n);
            runStage (stages[1], midA.data(), out, 2 * n);
            break;
        default:
            runStage (stages[0], in, midA.data(), n);
            runStage (stages[1], midA.data(), midB.data(), 2 * n);
            runStage (stages[2], midB.data(), out, 4 * n);
            break;
    }
}

// -------------------------------------------------------------- DownSampler

void DownSampler::prepare (int newMaxFactor, int maxBlockSize)
{
    maxFactor = newMaxFactor;
    factor = newMaxFactor;

    for (int j = 0; j < stagesFor (maxFactor); ++j)
    {
        auto& s = stages[static_cast<size_t> (j)];
        const int M = halfOrder[j];
        const auto h = designHalfband (M);
        s.M = M;
        s.taps.resize (h.size());
        for (size_t i = 0; i < h.size(); ++i)
            s.taps[i] = static_cast<float> (h[i]);
        s.history.assign (static_cast<size_t> (2 * M), 0.0f);
        s.ext.assign (static_cast<size_t> (2 * M + 2 * maxBlockSize * (1 << j)), 0.0f);
    }

    midA.assign (static_cast<size_t> (maxBlockSize) * 2, 0.0f);
    midB.assign (static_cast<size_t> (maxBlockSize) * 4, 0.0f);
    padded.assign (static_cast<size_t> (maxBlockSize) * 8, 0.0f);
}

void DownSampler::setFactor (int newFactor) noexcept
{
    factor = std::min (newFactor, maxFactor);
    reset();
}

void DownSampler::reset()
{
    for (auto& s : stages)
        std::fill (s.history.begin(), s.history.end(), 0.0f);
    padHistory.fill (0.0f);
}

void DownSampler::runStage (Stage& s, const float* in, float* out, int n) noexcept
{
    const int M = s.M;
    const int H = 2 * M;
    float* ext = s.ext.data();

    std::copy (s.history.begin(), s.history.end(), ext);
    std::copy (in, in + 2 * n, ext + H);

    const float* taps = s.taps.data();
    for (int k = 0; k < n; ++k)
    {
        const float* v = ext + H + 2 * k;
        float acc = s.center * v[-M];
        for (int p = 0; p <= M; ++p)
            acc += taps[p] * v[-2 * p];
        out[k] = acc;
    }

    std::copy (ext + 2 * n, ext + 2 * n + H, s.history.begin());
}

void DownSampler::process (const float* in, float* out, int n) noexcept
{
    const int st = stagesFor (factor);
    if (st == 0)
    {
        std::copy (in, in + n, out);
        return;
    }

    // Retardo a la frecuencia más alta: deja la latencia total en un número entero de muestras base.
    const int pad = Oversampler::padFor (factor);
    const int total = n * factor;
    const float* src = in;
    if (pad > 0)
    {
        std::copy (padHistory.begin(), padHistory.begin() + pad, padded.begin());
        std::copy (in, in + total - pad, padded.begin() + pad);
        std::copy (in + total - pad, in + total, padHistory.begin());
        src = padded.data();
    }

    switch (st)
    {
        case 1: runStage (stages[0], src, out, n); break;
        case 2:
            runStage (stages[1], src, midA.data(), 2 * n);
            runStage (stages[0], midA.data(), out, n);
            break;
        default:
            runStage (stages[2], src, midB.data(), 4 * n);
            runStage (stages[1], midB.data(), midA.data(), 2 * n);
            runStage (stages[0], midA.data(), out, n);
            break;
    }
}
} // namespace ferb
