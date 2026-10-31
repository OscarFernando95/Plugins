#include "OversamplerN.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <mutex>
#include <numbers>

#include "Fft.h"

namespace ferb
{
namespace
{
// Mitad del orden de cada etapa (longitud 2M+1). La primera trabaja con una transición estrecha
// (de 20 a 28 kHz con 48 kHz de base); las siguientes ya reciben una señal limitada en banda.
constexpr int halfOrder[OsDesign::maxStages] = { 39, 11, 7, 5, 5 };
constexpr double kaiserBeta = 10.06; // ≈ 100 dB

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

std::vector<double> linearHalfband (int M)
{
    const int L = 2 * M + 1;
    std::vector<double> h (static_cast<size_t> (L));
    const double i0 = bessel0 (kaiserBeta);
    double sum = 0.0;
    for (int n = 0; n < L; ++n)
    {
        const int e = n - M;
        const double r = static_cast<double> (e) / (M + 1);
        const double w = bessel0 (kaiserBeta * std::sqrt (1.0 - r * r)) / i0;
        const double sinc = e == 0 ? 0.5 : std::sin (std::numbers::pi * e / 2.0) / (std::numbers::pi * e);
        h[static_cast<size_t> (n)] = sinc * w;
        sum += h[static_cast<size_t> (n)];
    }
    for (auto& v : h)
        v /= sum;
    return h;
}

/** Pasa un FIR de fase lineal a fase mínima con el mismo módulo (cepstro real). */
std::vector<double> toMinimumPhase (const std::vector<double>& h, size_t length)
{
    constexpr size_t N = 16384;
    std::vector<std::complex<double>> a (N, 0.0);
    for (size_t i = 0; i < h.size(); ++i)
        a[i] = h[i];
    fft (a, false);

    for (auto& v : a)
        v = std::log (std::max (std::abs (v), 1.0e-9));
    fft (a, true); // cepstro real

    std::vector<std::complex<double>> folded (N, 0.0);
    folded[0] = a[0].real();
    for (size_t n = 1; n < N / 2; ++n)
        folded[n] = 2.0 * a[n].real();
    folded[N / 2] = a[N / 2].real();

    fft (folded, false);
    for (auto& v : folded)
        v = std::exp (v);
    fft (folded, true);

    std::vector<double> out (length);
    double sum = 0.0;
    for (size_t i = 0; i < length; ++i)
    {
        out[i] = folded[i].real();
        sum += out[i];
    }
    for (auto& v : out)
        v /= sum;
    return out;
}

double groupDelayAtDc (const std::vector<double>& h)
{
    double num = 0.0, den = 0.0;
    for (size_t n = 0; n < h.size(); ++n)
    {
        num += static_cast<double> (n) * h[n];
        den += h[n];
    }
    return num / den;
}

std::array<OsDesign::Stage, OsDesign::maxStages> build (bool minimumPhase)
{
    std::array<OsDesign::Stage, OsDesign::maxStages> out;
    for (size_t j = 0; j < out.size(); ++j)
    {
        auto h = linearHalfband (halfOrder[j]);
        if (minimumPhase)
            h = toMinimumPhase (h, h.size());
        out[j].delay = groupDelayAtDc (h);
        out[j].taps.assign (h.begin(), h.end());
    }
    return out;
}
} // namespace

const std::array<OsDesign::Stage, OsDesign::maxStages>& OsDesign::stages (bool minimumPhase)
{
    static const auto linear = build (false);
    static const auto minimum = build (true);
    return minimumPhase ? minimum : linear;
}

int OsDesign::stagesFor (int factor) noexcept
{
    return factor >= 32 ? 5 : factor >= 16 ? 4 : factor >= 8 ? 3 : factor >= 4 ? 2 : factor >= 2 ? 1 : 0;
}

// Un filtro de la etapa j, que sube hacia la frecuencia F/2^(stages−1−j), retrasa `delay` muestras
// de su frecuencia de salida; la suma de subir y bajar, en muestras de la frecuencia más alta F, es
// Σ_j 2·delay_j · F / 2^(j+1) = Σ_j delay_j · F / 2^j.
int OsDesign::unitsFor (int factor, bool minimumPhase)
{
    const auto& st = stages (minimumPhase);
    double units = 0.0;
    for (int j = 0; j < stagesFor (factor); ++j)
        units += st[static_cast<size_t> (j)].delay * factor / static_cast<double> (1 << j);
    return static_cast<int> (std::lround (units));
}

int OsDesign::padFor (int factor, bool minimumPhase)
{
    if (factor <= 1)
        return 0;
    return (factor - unitsFor (factor, minimumPhase) % factor) % factor;
}

int OsDesign::latencyFor (int factor, bool minimumPhase)
{
    if (factor <= 1)
        return 0;
    return (unitsFor (factor, minimumPhase) + padFor (factor, minimumPhase)) / factor;
}

// ------------------------------------------------------------------ UpN

void UpN::prepare (int maxBlockSize)
{
    for (size_t j = 0; j < history.size(); ++j)
    {
        const size_t L = OsDesign::stages (false)[j].taps.size();
        history[j].assign (L, 0.0f);
        ext[j].assign (L + static_cast<size_t> (maxBlockSize) * (size_t { 1 } << j), 0.0f);
    }
    mid[0].assign (static_cast<size_t> (maxBlockSize) * 16, 0.0f);
    mid[1].assign (static_cast<size_t> (maxBlockSize) * 16, 0.0f);
}

void UpN::reset()
{
    for (auto& h : history)
        std::fill (h.begin(), h.end(), 0.0f);
}

void UpN::setConfig (int newFactor, bool minimumPhase) noexcept
{
    factor = newFactor;
    minPhase = minimumPhase;
    reset();
}

void UpN::runStage (int j, const float* in, float* out, int n) noexcept
{
    const auto& taps = OsDesign::stages (minPhase)[static_cast<size_t> (j)].taps;
    const int L = static_cast<int> (taps.size());
    auto& hist = history[static_cast<size_t> (j)];
    float* x = ext[static_cast<size_t> (j)].data();

    std::copy (hist.begin(), hist.end(), x);
    std::copy (in, in + n, x + L);

    // Polifásico: las salidas pares usan los taps pares, y las impares, los impares. Ganancia 2.
    for (int k = 0; k < n; ++k)
    {
        const float* v = x + L + k;
        float even = 0.0f, odd = 0.0f;
        for (int p = 0; 2 * p < L; ++p)
            even += taps[static_cast<size_t> (2 * p)] * v[-p];
        for (int p = 0; 2 * p + 1 < L; ++p)
            odd += taps[static_cast<size_t> (2 * p + 1)] * v[-p];
        out[2 * k] = 2.0f * even;
        out[2 * k + 1] = 2.0f * odd;
    }

    std::copy (x + n, x + n + L, hist.begin());
}

void UpN::process (const float* in, float* out, int n) noexcept
{
    const int st = OsDesign::stagesFor (factor);
    if (st == 0)
    {
        std::copy (in, in + n, out);
        return;
    }

    const float* src = in;
    int len = n;
    for (int j = 0; j < st; ++j)
    {
        float* dst = j == st - 1 ? out : mid[j & 1].data();
        runStage (j, src, dst, len);
        src = dst;
        len *= 2;
    }
}

// ------------------------------------------------------------------ DownN

void DownN::prepare (int maxBlockSize)
{
    for (size_t j = 0; j < history.size(); ++j)
    {
        const size_t L = OsDesign::stages (false)[j].taps.size();
        history[j].assign (L, 0.0f);
        ext[j].assign (L + static_cast<size_t> (maxBlockSize) * (size_t { 2 } << j), 0.0f);
    }
    mid[0].assign (static_cast<size_t> (maxBlockSize) * 16, 0.0f);
    mid[1].assign (static_cast<size_t> (maxBlockSize) * 16, 0.0f);
    padded.assign (static_cast<size_t> (maxBlockSize) * OsDesign::maxFactor, 0.0f);
    padHistory.assign (OsDesign::maxFactor, 0.0f);
}

void DownN::reset()
{
    for (auto& h : history)
        std::fill (h.begin(), h.end(), 0.0f);
    std::fill (padHistory.begin(), padHistory.end(), 0.0f);
}

void DownN::setConfig (int newFactor, bool minimumPhase) noexcept
{
    factor = newFactor;
    minPhase = minimumPhase;
    reset();
}

void DownN::runStage (int j, const float* in, float* out, int n) noexcept
{
    const auto& taps = OsDesign::stages (minPhase)[static_cast<size_t> (j)].taps;
    const int L = static_cast<int> (taps.size());
    auto& hist = history[static_cast<size_t> (j)];
    float* x = ext[static_cast<size_t> (j)].data();

    std::copy (hist.begin(), hist.end(), x);
    std::copy (in, in + 2 * n, x + L);

    for (int k = 0; k < n; ++k)
    {
        const float* v = x + L + 2 * k;
        float acc = 0.0f;
        for (int i = 0; i < L; ++i)
            acc += taps[static_cast<size_t> (i)] * v[-i];
        out[k] = acc;
    }

    std::copy (x + 2 * n, x + 2 * n + L, hist.begin());
}

void DownN::process (const float* in, float* out, int n) noexcept
{
    const int st = OsDesign::stagesFor (factor);
    if (st == 0)
    {
        std::copy (in, in + n, out);
        return;
    }

    const int pad = OsDesign::padFor (factor, minPhase);
    const int total = n * factor;
    const float* src = in;
    if (pad > 0)
    {
        std::copy (padHistory.begin(), padHistory.begin() + pad, padded.begin());
        std::copy (in, in + total - pad, padded.begin() + pad);
        std::copy (in + total - pad, in + total, padHistory.begin());
        src = padded.data();
    }

    int len = total / 2;
    for (int j = st - 1; j >= 0; --j)
    {
        float* dst = j == 0 ? out : mid[j & 1].data();
        runStage (j, src, dst, len);
        src = dst;
        len /= 2;
    }
}
} // namespace ferb
