// Ajuste de los pesos del clasificador con señales sintéticas. No es una prueba: se ejecuta a mano,
//   ./build/ChauS/ChauSTests "[.train]"
// e imprime los pesos para pegarlos en `Classifier::initial()`. Los datos de ajuste usan semillas y
// rangos distintos de los de las pruebas de detección.

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdio>

#include "Signals.h"
#include "dsp/SibilanceDetector.h"

using namespace chaus;

namespace
{
struct Sample
{
    std::array<float, Classifier::numFeatures> x;
    float label, weight;
    int tag;
};

constexpr const char* tagNames[] = { "sibilante", "vocales", "aire", "platillo", "hi-hat", "t" };

/** Rasgos de los cuadros cuyo final cae entre `from` y `to` (muestras). */
void collect (std::vector<Sample>& out, const std::vector<float>& audio, double fs, int from, int to, float label, float weight, int tag)
{
    SibilanceDetector d;
    d.prepare (fs);
    const float centre = std::sqrt (4000.0f * 12000.0f);
    long seen = 0;
    for (int i = 0; i < int (audio.size()); ++i)
    {
        d.tick (audio[size_t (i)]);
        if (d.frameCount() != seen)
        {
            seen = d.frameCount();
            if (i >= from && i <= to && d.features().levelDb > SibilanceDetector::silenceFloorDb)
                out.push_back ({ Classifier::normalise (d.features(), centre), label, weight, tag });
        }
    }
}

int ms (double fs, double milliseconds) { return int (milliseconds * 0.001 * fs); }
} // namespace

TEST_CASE ("Ajuste del clasificador", "[.train]")
{
    std::vector<Sample> data;
    sig::Rng rng (424242);
    int positives = 0;

    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (int rep = 0; rep < 60; ++rep)
        {
            const double f0 = rng.uni (90.0f, 250.0f), vowelDb = rng.uni (-32.0f, -16.0f);
            const auto vowelBed = [&] (double seconds) { return sig::vowels (fs, int (seconds * fs), f0, vowelDb, rng); };

            { // sibilante (positivo)
                const double c = rng.uni (5000.0f, 10000.0f);
                auto bed = vowelBed (0.45);
                sig::mixAt (bed, sig::sibilant (fs, c * rng.uni (0.55f, 0.9f), c * rng.uni (1.15f, 1.7f), 0.25, rng.uni (-34.0f, -12.0f), rng, 0.03, 0.05, rng.index (2) == 1), ms (fs, 100));
                const size_t before = data.size();
                collect (data, bed, fs, ms (fs, 100 + 75), ms (fs, 100 + 210), 1.0f, 1.0f, 0);
                positives += int (data.size() - before);
            }
            { // solo vocales
                collect (data, vowelBed (0.4), fs, ms (fs, 60), ms (fs, 400), 0.0f, 1.0f, 1);
            }
            { // respiración y ruido de sala (graves y medios)
                auto bed = vowelBed (0.4);
                ferb::Biquad lp = ferb::Biquad::lowPass (fs, rng.uni (2000.0f, 6000.0f));
                std::vector<float> air (bed.size());
                for (auto& v : air) v = lp.process (rng.gauss());
                sig::setRms (air, rng.uni (-40.0f, -22.0f));
                sig::mixAt (bed, air, 0);
                collect (data, bed, fs, ms (fs, 60), ms (fs, 400), 0.0f, 1.0f, 2);
            }
            { // platillo
                auto bed = vowelBed (1.0);
                sig::mixAt (bed, sig::cymbal (fs, 0.9, rng.uni (-30.0f, -10.0f), rng, rng.uni (0.4f, 1.5f)), ms (fs, 50));
                collect (data, bed, fs, ms (fs, 100), ms (fs, 900), 0.0f, 2.0f, 3);
            }
            { // hi-hat
                auto bed = vowelBed (0.5);
                sig::mixAt (bed, sig::hihat (fs, 0.25, rng.uni (-30.0f, -10.0f), rng, rng.uni (0.04f, 0.15f), rng.uni (5500.0f, 8000.0f)), ms (fs, 100));
                collect (data, bed, fs, ms (fs, 105), ms (fs, 330), 0.0f, 3.0f, 4);
            }
            { // «t»
                auto bed = vowelBed (0.5);
                const double len = rng.uni (0.010f, 0.025f);
                const auto burst = sig::tBurst (fs, rng.uni (-32.0f, -10.0f), rng, rng.uni (4000.0f, 5600.0f), len);
                sig::mixAt (bed, burst, ms (fs, 200));
                collect (data, bed, fs, ms (fs, 200), ms (fs, 200) + int (burst.size()) + ms (fs, 25), 0.0f, 3.0f, 5);
            }
        }

    // Pesos iguales para las dos clases.
    double wPos = 0.0, wNeg = 0.0;
    for (const auto& s : data)
        (s.label > 0.5f ? wPos : wNeg) += s.weight;
    const double scalePos = 0.5 * (wPos + wNeg) / wPos, scaleNeg = 0.5 * (wPos + wNeg) / wNeg;

    constexpr int n = Classifier::numFeatures;
    std::array<double, n + 1> w {}, m {}, v {}; // pesos, bias al final, y momentos de Adam
    for (int it = 1; it <= 6000; ++it)
    {
        std::array<double, n + 1> grad {};
        for (const auto& s : data)
        {
            double z = w[n];
            for (int i = 0; i < n; ++i) z += w[size_t (i)] * s.x[size_t (i)];
            const double p = 1.0 / (1.0 + std::exp (-z));
            const double g = (p - s.label) * s.weight * (s.label > 0.5f ? scalePos : scaleNeg) / double (data.size());
            for (int i = 0; i < n; ++i) grad[size_t (i)] += g * s.x[size_t (i)];
            grad[n] += g;
        }
        for (int i = 0; i < n; ++i) grad[size_t (i)] += 1.0e-2 * w[size_t (i)];
        for (int i = 0; i <= n; ++i)
        {
            m[size_t (i)] = 0.9 * m[size_t (i)] + 0.1 * grad[size_t (i)];
            v[size_t (i)] = 0.999 * v[size_t (i)] + 0.001 * grad[size_t (i)] * grad[size_t (i)];
            w[size_t (i)] -= 0.03 * (m[size_t (i)] / (1 - std::pow (0.9, it))) / (std::sqrt (v[size_t (i)] / (1 - std::pow (0.999, it))) + 1e-8);
        }
    }

    int tp = 0, fn = 0, tn = 0, fp = 0;
    std::array<int, 6> tagTotal {}, tagPred {};
    std::array<std::array<double, n>, 6> tagMean {};
    for (const auto& s : data)
    {
        ++tagTotal[size_t (s.tag)];
        for (int i = 0; i < n; ++i) tagMean[size_t (s.tag)][size_t (i)] += s.x[size_t (i)];
        double z = w[n];
        for (int i = 0; i < n; ++i) z += w[size_t (i)] * s.x[size_t (i)];
        const bool pred = z > 0.0;
        tagPred[size_t (s.tag)] += pred ? 1 : 0;
        (s.label > 0.5f ? (pred ? tp : fn) : (pred ? fp : tn))++;
    }
    std::printf ("\nmuestras: %zu (positivas %d)\n", data.size(), positives);
    std::printf ("c.weights = { %.3ff, %.3ff, %.3ff, %.3ff, %.3ff, %.3ff };\nc.bias = %.3ff;\n", w[0], w[1], w[2], w[3], w[4], w[5], w[6]);
    for (size_t t = 0; t < 6; ++t)
    {
        std::printf ("  %-10s %6d cuadros, %5.1f %% sobre 0; medias:", tagNames[t], tagTotal[t], 100.0 * tagPred[t] / std::max (1, tagTotal[t]));
        for (int i = 0; i < n; ++i) std::printf (" %7.2f", tagMean[t][size_t (i)] / std::max (1, tagTotal[t]));
        std::printf ("\n");
    }
    std::printf ("ajuste: positivos %d de %d, negativos acertados %d de %d\n", tp, tp + fn, tn, tn + fp);
}
