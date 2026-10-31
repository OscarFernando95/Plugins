#pragma once

// Señales sintéticas para probar ChauS: voz (vocales), sibilantes y los sonidos que no deben detectarse
// (platillo, hi-hat, «t», respiración). Son modelos sencillos, no grabaciones: sirven para comprobar el
// comportamiento del detector, no para afirmar tasas de acierto sobre voces reales.

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>
#include <vector>

#include <Biquad.h>

namespace sig
{
struct Rng
{
    std::mt19937 g;
    explicit Rng (unsigned seed) : g (seed) {}
    float uni (float a, float b) { return std::uniform_real_distribution<float> (a, b) (g); }
    float gauss() { return std::normal_distribution<float> (0.0f, 1.0f) (g); }
    int index (int n) { return std::uniform_int_distribution<int> (0, n - 1) (g); }
};

inline double dbToLin (double db) { return std::pow (10.0, db / 20.0); }

/** Paso banda RBJ con ganancia 0 dB en el centro. */
inline ferb::Biquad bandPass (double fs, double f, double q)
{
    const double w0 = 2.0 * std::numbers::pi * f / fs, alpha = std::sin (w0) / (2.0 * q), a0 = 1.0 + alpha;
    ferb::Biquad b;
    b.b0 = alpha / a0; b.b1 = 0.0; b.b2 = -alpha / a0;
    b.a1 = -2.0 * std::cos (w0) / a0; b.a2 = (1.0 - alpha) / a0;
    return b;
}

inline double rms (const std::vector<float>& x)
{
    double s = 0.0;
    for (float v : x)
        s += double (v) * v;
    return std::sqrt (s / std::max<size_t> (1, x.size()));
}

/** Lleva la señal a un valor eficaz dado (dBFS). */
inline void setRms (std::vector<float>& x, double levelDb)
{
    const double g = dbToLin (levelDb) / std::max (rms (x), 1e-12);
    for (auto& v : x)
        v = float (v * g);
}

/** Envolvente: subida y bajada en coseno elevado (en segundos) con meseta. */
inline void applyEnvelope (std::vector<float>& x, double fs, double attack, double release)
{
    const int n = int (x.size()), a = std::min (n / 2, int (attack * fs)), r = std::min (n / 2, int (release * fs));
    for (int i = 0; i < n; ++i)
    {
        double g = 1.0;
        if (i < a) g = 0.5 - 0.5 * std::cos (std::numbers::pi * i / a);
        if (i >= n - r) g = std::min (g, 0.5 - 0.5 * std::cos (std::numbers::pi * (n - 1 - i) / r));
        x[size_t (i)] = float (x[size_t (i)] * g);
    }
}

/** Voz sonora: diente de sierra con tres formantes y algo de aire; el eje de los fonemas sin sibilante. */
inline std::vector<float> vowels (double fs, int n, double f0, double levelDb, Rng& rng, double breathDb = -45.0)
{
    std::vector<float> x (static_cast<size_t> (n));
    ferb::Biquad f1 = bandPass (fs, 560.0, 5.0), f2 = bandPass (fs, 1600.0, 8.0), f3 = bandPass (fs, 2700.0, 10.0);
    ferb::Biquad lp = ferb::Biquad::lowPass (fs, 7000.0);
    double phase = 0.0;
    for (int i = 0; i < n; ++i)
    {
        const double f = f0 * (1.0 + 0.01 * std::sin (2.0 * std::numbers::pi * 5.0 * i / fs)); // vibrato leve
        phase += f / fs;
        phase -= std::floor (phase);
        const float saw = float (2.0 * phase - 1.0);
        x[size_t (i)] = lp.process (f1.process (saw) + 0.6f * f2.process (saw) + 0.3f * f3.process (saw));
    }
    setRms (x, levelDb);
    ferb::Biquad air = ferb::Biquad::lowPass (fs, 6000.0);
    const double g = dbToLin (breathDb);
    for (auto& v : x)
        v += float (g * air.process (rng.gauss()) * 0.5);
    return x;
}

/** Sibilante: ruido filtrado entre `lo` y `hi` (pendientes de 12 dB por octava) con envolvente. `levelDb` = nivel de la meseta. */
inline std::vector<float> sibilant (double fs, double lo, double hi, double durSeconds, double levelDb, Rng& rng, double attack = 0.03, double release = 0.05, bool gentleTail = false)
{
    std::vector<float> x (static_cast<size_t> (durSeconds * fs));
    ferb::Biquad h1 = ferb::Biquad::highPass (fs, lo), h2 = ferb::Biquad::highPass (fs, lo);
    ferb::Biquad l1 = ferb::Biquad::lowPass (fs, std::min (hi, fs * 0.45)), l2 = ferb::Biquad::lowPass (fs, std::min (hi, fs * 0.45));
    // Con `gentleTail` la caída por encima de `hi` es de 6 dB por octava (una s real llega más arriba que un filtro de cuarto orden).
    ferb::Biquad tail = ferb::Biquad::lowPass (fs, std::min (hi * 2.0, fs * 0.45));
    for (auto& v : x)
    {
        const float shaped = h2.process (h1.process (rng.gauss()));
        v = gentleTail ? l1.process (shaped) + 0.35f * tail.process (shaped) : l2.process (l1.process (shaped));
    }
    setRms (x, levelDb);
    applyEnvelope (x, fs, attack, release);
    return x;
}

/** Platillo: parciales inarmónicos entre 300 Hz y 14 kHz con caída lenta, más ruido brillante. */
inline std::vector<float> cymbal (double fs, double durSeconds, double levelDb, Rng& rng, double decaySeconds = 0.8)
{
    const int n = int (durSeconds * fs);
    std::vector<float> x (static_cast<size_t> (n), 0.0f);
    const int partials = 60;
    for (int p = 0; p < partials; ++p)
    {
        const double f = 300.0 * std::pow (14000.0 / 300.0, double (rng.uni (0.0f, 1.0f)));
        const double ph = rng.uni (0.0f, 6.28f), amp = rng.uni (0.3f, 1.0f);
        for (int i = 0; i < n; ++i)
            x[size_t (i)] += float (amp * std::sin (2.0 * std::numbers::pi * f * i / fs + ph));
    }
    ferb::Biquad hp = ferb::Biquad::highPass (fs, 3000.0);
    std::vector<float> noise (static_cast<size_t> (n));
    for (auto& v : noise) v = hp.process (rng.gauss());
    setRms (noise, -20.0);
    setRms (x, -20.0);
    for (int i = 0; i < n; ++i)
        x[size_t (i)] = (x[size_t (i)] + 0.6f * noise[size_t (i)]) * float (std::exp (-i / (decaySeconds * fs)));
    setRms (x, levelDb);
    applyEnvelope (x, fs, 0.002, 0.05);
    return x;
}

/** Hi-hat estilo 808: seis cuadradas inarmónicas filtradas por un paso alto de 7 kHz, con ruido. */
inline std::vector<float> hihat (double fs, double durSeconds, double levelDb, Rng& rng, double decaySeconds = 0.09, double cutHz = 7000.0, double noiseAmount = 1.5)
{
    const int n = int (durSeconds * fs);
    std::vector<float> x (static_cast<size_t> (n), 0.0f);
    const double base = rng.uni (330.0f, 480.0f);
    const double ratios[] = { 2.0, 3.0, 4.16, 5.43, 6.79, 8.21 };
    for (double r : ratios) // cuadradas de banda limitada (suma de armónicos impares): sin aliasing
    {
        const double f = base * r;
        for (int k = 1; k * f < 0.45 * fs; k += 2)
            for (int i = 0; i < n; ++i)
                x[size_t (i)] += float (1.27324 / k * std::sin (2.0 * std::numbers::pi * k * f * i / fs));
    }
    ferb::Biquad h1 = ferb::Biquad::highPass (fs, cutHz), h2 = ferb::Biquad::highPass (fs, cutHz);
    ferb::Biquad n1 = ferb::Biquad::highPass (fs, cutHz * 0.85);
    for (int i = 0; i < n; ++i)
        x[size_t (i)] = h2.process (h1.process (x[size_t (i)])) + float (noiseAmount) * n1.process (rng.gauss());
    for (int i = 0; i < n; ++i)
        x[size_t (i)] *= float (std::exp (-i / (decaySeconds * fs)));
    setRms (x, levelDb);
    applyEnvelope (x, fs, 0.0005, 0.01);
    return x;
}

/** «t»: ráfaga corta (10 a 25 ms) de ruido en torno a 4 a 5,5 kHz y soplo breve de menor nivel detrás. */
inline std::vector<float> tBurst (double fs, double levelDb, Rng& rng, double centreHz = 4800.0, double burstSeconds = 0.015)
{
    const int burst = int (burstSeconds * fs), breath = int (0.04 * fs);
    ferb::Biquad b1 = bandPass (fs, centreHz, 1.2), b2 = bandPass (fs, centreHz, 1.2);
    std::vector<float> x (size_t (burst + breath));
    for (int i = 0; i < burst; ++i)
        x[size_t (i)] = b2.process (b1.process (rng.gauss())) * float (std::exp (-i / (0.004 * fs)));
    ferb::Biquad s1 = bandPass (fs, 3500.0, 0.6);
    for (int i = 0; i < breath; ++i)
        x[size_t (burst + i)] = s1.process (rng.gauss()) * 0.05f * float (0.5 + 0.5 * std::cos (std::numbers::pi * i / breath));
    std::vector<float> head (x.begin(), x.begin() + burst);
    const double g = dbToLin (levelDb) / std::max (rms (head), 1e-12);
    for (auto& v : x)
        v = float (v * g);
    return x;
}

/** Suma `part` a `bed` desde `start`. */
inline void mixAt (std::vector<float>& bed, const std::vector<float>& part, int start)
{
    for (size_t i = 0; i < part.size() && size_t (start) + i < bed.size(); ++i)
        bed[size_t (start) + i] += part[i];
}
} // namespace sig
