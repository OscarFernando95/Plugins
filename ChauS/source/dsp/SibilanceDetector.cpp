#include "SibilanceDetector.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include <Fft.h>

namespace chaus
{
namespace
{
constexpr double tiny = 1.0e-20;
float db10 (double x) noexcept { return static_cast<float> (10.0 * std::log10 (std::max (x, 1.0e-12))); }
} // namespace

// ───────────────────────────────── Clasificador ─────────────────────────────────

Classifier Classifier::initial()
{
    Classifier c;
    // Valores iniciales: ajustados con `ChauSTests "[.train]"` (señales sintéticas, regularización L2 de 0,01) y con el sesgo
    // bajado 0,3 respecto del ajuste (barrido en `[.report]`: pierde menos falsos positivos con detección aún de 99 %).
    // Pendiente: reajustar con grabaciones de voz (M6).
    c.weights = { 0.255f, 1.733f, 2.050f, -1.356f, 0.290f, -1.012f };
    c.bias = -1.365f;
    return c;
}

std::array<float, Classifier::numFeatures> Classifier::normalise (const Features& f, float bandCentreHz) noexcept
{
    const float centreOct = std::log2 (std::max (f.centroidHz, 100.0f) / std::max (bandCentreHz, 100.0f));
    return { (f.levelDb + 40.0f) / 20.0f,
             f.ratioDb / 20.0f,
             f.flatness,
             -centreOct * centreOct,
             f.slopeDbOct / 10.0f,
             f.airDb / 20.0f };
}

float Classifier::logit (const Features& f, float bandCentreHz) const noexcept
{
    const auto x = normalise (f, bandCentreHz);
    float z = bias;
    for (size_t i = 0; i < x.size(); ++i)
        z += weights[i] * x[i];
    return z;
}

float Classifier::probability (const Features& f, float bandCentreHz) const noexcept
{
    return 1.0f / (1.0f + std::exp (-std::clamp (logit (f, bandCentreHz), -30.0f, 30.0f)));
}

// ───────────────────────────────── Rasgos ─────────────────────────────────

Features extractFeatures (const double* power, int N, double fs, float hpHz, float lpHz) noexcept
{
    const double df = fs / N;
    const int half = N / 2;
    const auto firstBin = [&] (double hz) { return std::clamp (static_cast<int> (std::ceil (hz / df)), 1, half); };
    const auto lastBin = [&] (double hz) { return std::clamp (static_cast<int> (std::floor (hz / df)), 1, half); };

    // Hann: la suma de P en un intervalo, escalada así, da la media cuadrática de la señal en él (senoidal de amplitud A → A²/2).
    const double scale = 2.0 / (static_cast<double> (N) * N * 0.375);

    // Devuelve la suma y el número de bins del intervalo.
    const auto bandSum = [&] (double lo, double hi, int* bins = nullptr)
    {
        double sum = 0.0;
        const int a = firstBin (lo), b = std::max (a, lastBin (hi));
        for (int k = a; k <= b; ++k)
            sum += power[k];
        if (bins != nullptr)
            *bins = b - a + 1;
        return sum * scale;
    };

    Features out;
    const int k0 = firstBin (hpHz), k1 = std::max (k0, lastBin (std::min<double> (lpHz, fs * 0.5)));

    double sumP = 0.0, sumFP = 0.0, sumLogP = 0.0;
    for (int k = k0; k <= k1; ++k)
    {
        sumP += power[k];
        sumFP += k * df * power[k];
    }
    const int bins = k1 - k0 + 1;
    const double meanP = sumP / bins;
    const double floorP = meanP * 1.0e-4 + tiny;
    for (int k = k0; k <= k1; ++k)
        sumLogP += std::log (power[k] + floorP);

    out.levelDb = db10 (sumP * scale);
    out.ratioDb = std::clamp (db10 (sumP * scale) - db10 (bandSum (300.0, 3000.0)), -40.0f, 40.0f);
    out.flatness = static_cast<float> (std::clamp (std::exp (sumLogP / bins) / (meanP + floorP), 0.0, 1.0));
    out.centroidHz = sumP > tiny ? static_cast<float> (sumFP / sumP) : static_cast<float> (0.5 * (hpHz + lpHz));

    // Aire: densidad espectral (potencia media por bin) de 12 a 18 kHz frente a 5 a 9 kHz, independiente de HP y LP.
    {
        int airBins = 1, refBins = 1;
        const double top = std::min (18000.0, fs * 0.48);
        const double air = top > 12500.0 ? bandSum (12000.0, top, &airBins) : 0.0;
        const double ref = bandSum (5000.0, 9000.0, &refBins);
        out.airDb = std::clamp (db10 (air / airBins + 1.0e-14) - db10 (ref / refBins + 1.0e-14), -40.0f, 20.0f);
    }

    // Dispersión en octavas alrededor del centroide.
    if (sumP > tiny)
    {
        const double c = std::max (static_cast<double> (out.centroidHz), df);
        double var = 0.0;
        for (int k = k0; k <= k1; ++k)
        {
            const double o = std::log2 (std::max (k * df, df) / c);
            var += power[k] * o * o;
        }
        out.spreadOct = static_cast<float> (std::sqrt (var / sumP));
    }

    // Pendiente de 2 a 4 kHz: regresión de la densidad espectral (potencia media por bin, en dB) de cuatro
    // subbandas de igual ancho en octavas frente a la frecuencia en octavas.
    constexpr double edges[5] = { 2000.0, 2378.4, 2828.4, 3363.6, 4000.0 };
    double sx = 0.0, sy = 0.0, sxx = 0.0, sxy = 0.0;
    for (int i = 0; i < 4; ++i)
    {
        const double x = std::log2 (std::sqrt (edges[i] * edges[i + 1]) / 2000.0);
        int bins = 1;
        const double sum = bandSum (edges[i], edges[i + 1], &bins);
        const double y = db10 (sum / bins + 1.0e-14);
        sx += x; sy += y; sxx += x * x; sxy += x * y;
    }
    const double denom = 4.0 * sxx - sx * sx;
    out.slopeDbOct = static_cast<float> (std::clamp ((4.0 * sxy - sx * sy) / denom, -60.0, 60.0));
    return out;
}

// ───────────────────────────────── Detector ─────────────────────────────────

void SibilanceDetector::prepare (double sampleRate)
{
    fs = sampleRate;
    const int factor = sampleRate >= 150000.0 ? 4 : (sampleRate >= 75000.0 ? 2 : 1);
    frameLength = 512 * factor;
    hop = 128 * factor;

    ring.assign (static_cast<size_t> (frameLength), 0.0f);
    spectrum.assign (static_cast<size_t> (frameLength), {});
    power.assign (static_cast<size_t> (frameLength / 2 + 1), 0.0);
    window.resize (static_cast<size_t> (frameLength));
    for (int n = 0; n < frameLength; ++n) // Hann periódica
        window[static_cast<size_t> (n)] = 0.5 - 0.5 * std::cos (2.0 * std::numbers::pi * n / frameLength);

    envelopeCoeff = 1.0 - std::exp (-1.0 / (0.001 * fs));
    scoreCoeff = static_cast<float> (1.0 - std::exp (-static_cast<double> (hop) / (0.001 * scoreSmoothingMs * fs)));
    setBand (hpHz, lpHz);
    reset();
}

void SibilanceDetector::setBand (float hp, float lp) noexcept
{
    hpHz = hp;
    lpHz = std::max (lp, hp * 1.1f);
    const double top = std::min<double> (lpHz, fs * 0.45);
    highPass.setCoefficients (ferb::Biquad::highPass (fs, hpHz));
    lowPass.setCoefficients (ferb::Biquad::lowPass (fs, top));
}

void SibilanceDetector::reset() noexcept
{
    std::fill (ring.begin(), ring.end(), 0.0f);
    highPass.reset();
    lowPass.reset();
    envelope = 0.0;
    writePos = sinceHop = 0;
    frames = 0;
    lastFeatures = {};
    lastProbability = score = 0.0f;
    active = false;
}

SibilanceDetector::Output SibilanceDetector::tick (float x) noexcept
{
    if (! std::isfinite (x))
        x = 0.0f;

    const float band = lowPass.process (highPass.process (x));
    envelope += envelopeCoeff * (static_cast<double> (band) * band - envelope);

    if (classifierOn)
    {
        ring[static_cast<size_t> (writePos)] = x;
        writePos = (writePos + 1) % frameLength;
        if (++sinceHop >= hop)
        {
            sinceHop = 0;
            analyse();
        }

        if (score >= onThreshold)
            active = true;
        else if (score <= offThreshold)
            active = false;
    }
    else
    {
        score = 1.0f;
        active = true;
    }
    return { db10 (envelope), score, active, band };
}

void SibilanceDetector::analyse() noexcept
{
    // Del más antiguo al más reciente, con la ventana.
    for (int n = 0; n < frameLength; ++n)
        spectrum[static_cast<size_t> (n)] = ring[static_cast<size_t> ((writePos + n) % frameLength)] * window[static_cast<size_t> (n)];
    ferb::fft (spectrum, false);
    for (int k = 0; k <= frameLength / 2; ++k)
        power[static_cast<size_t> (k)] = std::norm (spectrum[static_cast<size_t> (k)]);

    lastFeatures = extractFeatures (power.data(), frameLength, fs, hpHz, lpHz);
    ++frames;

    const float centre = std::sqrt (hpHz * std::min<float> (lpHz, static_cast<float> (fs * 0.45)));
    lastProbability = lastFeatures.levelDb < silenceFloorDb ? 0.0f : classifier.probability (lastFeatures, centre);
    score += scoreCoeff * (lastProbability - score);
}

// ───────────────────────────────── Umbral adaptativo ─────────────────────────────────

void AdaptiveReference::prepare (double sampleRate)
{
    fs = sampleRate;
    windowSamples = static_cast<long> (windowSeconds * sampleRate);
    minEventSamples = static_cast<long> (0.001 * minEventMs * sampleRate);
    smoothingCoeff = static_cast<float> (1.0 - std::exp (-1.0 / (smoothingSeconds * sampleRate)));
    reset();
}

void AdaptiveReference::reset (float initialDb) noexcept
{
    now = eventStart = 0;
    inEvent = false;
    eventPeak = -200.0f;
    head = count = 0;
    target = smoothed = initialDb;
}

void AdaptiveReference::tick (bool active, float levelDb) noexcept
{
    ++now;
    if (active)
    {
        if (! inEvent)
        {
            inEvent = true;
            eventStart = now;
            eventPeak = -200.0f;
        }
        eventPeak = std::max (eventPeak, levelDb);
    }
    else if (inEvent)
    {
        inEvent = false;
        closeEvent();
    }

    // Los eventos de más de 5 s salen de la ventana.
    bool changed = false;
    while (count > 0 && now - events[static_cast<size_t> ((head + capacity - count) % capacity)].end > windowSamples)
    {
        --count;
        changed = true;
    }
    if (changed)
        recompute();

    smoothed += smoothingCoeff * (target - smoothed);
}

void AdaptiveReference::closeEvent() noexcept
{
    if (now - eventStart < minEventSamples)
        return;
    events[static_cast<size_t> (head)] = { now, eventPeak };
    head = (head + 1) % capacity;
    count = std::min (count + 1, capacity);
    recompute();
}

void AdaptiveReference::recompute() noexcept
{
    if (count == 0)
        return; // sin eventos recientes se conserva el último valor
    std::array<float, capacity> values {};
    for (int i = 0; i < count; ++i)
        values[static_cast<size_t> (i)] = events[static_cast<size_t> ((head + capacity - 1 - i) % capacity)].peakDb;
    const int rank = static_cast<int> (std::floor (0.9 * (count - 1) + 0.5));
    std::nth_element (values.begin(), values.begin() + rank, values.begin() + count);
    target = values[static_cast<size_t> (rank)];
}
} // namespace chaus
