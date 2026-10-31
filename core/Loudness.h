#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <numbers>

#include "Biquad.h"

namespace ferb
{
/** Filtro K de ITU-R BS.1770: un estante de agudos (~+4 dB) y un paso alto RLB (~38 Hz). Los
    coeficientes se derivan para cualquier frecuencia de muestreo; a 48 kHz coinciden con la tabla de
    la recomendación (lo comprueba CoreTests). */
struct KWeighting
{
    static Biquad shelf (double fs) noexcept
    {
        constexpr double G = 3.999843853973347, Q = 0.7071752369554196, f0 = 1681.974450955533;
        const double K = std::tan (std::numbers::pi * f0 / fs);
        const double Vh = std::pow (10.0, G / 20.0);
        const double Vb = std::pow (Vh, 0.4996667741545416);
        const double a0 = 1.0 + K / Q + K * K;

        Biquad f;
        f.b0 = (Vh + Vb * K / Q + K * K) / a0;
        f.b1 = 2.0 * (K * K - Vh) / a0;
        f.b2 = (Vh - Vb * K / Q + K * K) / a0;
        f.a1 = 2.0 * (K * K - 1.0) / a0;
        f.a2 = (1.0 - K / Q + K * K) / a0;
        return f;
    }

    static Biquad highPass (double fs) noexcept
    {
        constexpr double Q = 0.5003270373238773, f0 = 38.13547087602444;
        const double K = std::tan (std::numbers::pi * f0 / fs);
        const double a0 = 1.0 + K / Q + K * K;

        Biquad f;
        f.b0 = 1.0;
        f.b1 = -2.0;
        f.b2 = 1.0;
        f.a1 = 2.0 * (K * K - 1.0) / a0;
        f.a2 = (1.0 - K / Q + K * K) / a0;
        return f;
    }
};

/** Medidor de sonoridad (ITU-R BS.1770-4 y EBU Tech 3341/3342), hasta 2 canales con peso 1.

    - M: ventana de 400 ms. S: ventana de 3 s. Las dos son deslizantes y sin puerta.
    - I: puerta absoluta de −70 LUFS y relativa 10 LU por debajo; bloques de 400 ms con 75 % de solape.
    - LRA: valores de corto plazo con puerta absoluta de −70 y relativa de −20 LU; P95 − P10.

    El hilo de audio solo filtra y acumula la energía de cada paso de 100 ms. Al cerrar un paso se
    actualizan los histogramas (0,1 LU de resolución, memoria constante) y se publican los valores en
    atómicos. No reserva memoria tras la construcción ni bloquea. */
class LoudnessMeter
{
public:
    /** Valor publicado cuando todavía no hay datos (ventana incompleta o todo bajo la puerta). */
    static constexpr float noValue = -200.0f;

    void prepare (double sampleRate, int numChannels) noexcept
    {
        fs = sampleRate;
        channels = std::clamp (numChannels, 1, 2);
        stepLength = std::max (1, static_cast<int> (std::lround (sampleRate * 0.1)));
        for (auto& f : shelfFilter)
            f = KWeighting::shelf (sampleRate);
        for (auto& f : highPassFilter)
            f = KWeighting::highPass (sampleRate);
        resetState();
    }

    /** Reinicia la integrada y el LRA (y todo el estado). Hilo de audio o con el audio parado. */
    void reset() noexcept { resetState(); }

    /** Pide el reinicio desde otro hilo; se atiende al inicio del siguiente `process`. */
    void requestReset() noexcept { resetRequested.store (true, std::memory_order_release); }

    void process (const float* const* in, int numSamples) noexcept
    {
        if (resetRequested.exchange (false, std::memory_order_acq_rel))
            resetState();

        for (int i = 0; i < numSamples; ++i)
        {
            double sum = 0.0;
            for (size_t c = 0; c < static_cast<size_t> (channels); ++c)
            {
                const double y = highPassFilter[c].process (shelfFilter[c].process (in[c][i]));
                sum += y * y;
            }
            stepSum += sum;

            if (++stepCount >= stepLength)
            {
                closeStep (stepSum / stepCount);
                stepSum = 0.0;
                stepCount = 0;
            }
        }
    }

    float momentary() const noexcept { return m.load (std::memory_order_relaxed); }
    float shortTerm() const noexcept { return s.load (std::memory_order_relaxed); }
    float integrated() const noexcept { return integ.load (std::memory_order_relaxed); }
    /** Rango de sonoridad en LU; 0 mientras no haya datos suficientes. */
    float range() const noexcept { return lraValue.load (std::memory_order_relaxed); }
    /** Segundos de programa medidos desde el último reinicio (la interfaz marca el LRA como inestable bajo 60 s). */
    float elapsedSeconds() const noexcept { return elapsed.load (std::memory_order_relaxed); }

    static float toLoudness (double meanSquare) noexcept
    {
        return meanSquare > 0.0 ? static_cast<float> (-0.691 + 10.0 * std::log10 (meanSquare)) : noValue;
    }

private:
    static constexpr int stepsPerBlock = 4;   // 400 ms
    static constexpr int stepsPerShort = 30;  // 3 s
    static constexpr double lowest = -70.0;
    static constexpr int numBins = 1000;      // de −70 a +30 LUFS, 0,1 LU por casilla

    struct Histogram
    {
        std::array<double, numBins> energy {}; // suma de energía de los bloques de cada casilla
        std::array<unsigned, numBins> count {};

        void clear() noexcept { energy.fill (0.0); count.fill (0u); }

        static int bin (double loudness) noexcept
        {
            return std::clamp (static_cast<int> (std::floor ((loudness - lowest) * 10.0)), 0, numBins - 1);
        }

        void add (double loudness, double meanSquare) noexcept
        {
            const int b = bin (loudness);
            energy[static_cast<size_t> (b)] += meanSquare;
            ++count[static_cast<size_t> (b)];
        }

        /** Energía media y cantidad de bloques desde la casilla `from`. */
        void tail (int from, double& meanEnergy, unsigned& n) const noexcept
        {
            double e = 0.0;
            n = 0;
            for (int b = from; b < numBins; ++b)
            {
                e += energy[static_cast<size_t> (b)];
                n += count[static_cast<size_t> (b)];
            }
            meanEnergy = n > 0 ? e / n : 0.0;
        }
    };

    void resetState() noexcept
    {
        for (auto& f : shelfFilter)
            f.reset();
        for (auto& f : highPassFilter)
            f.reset();
        stepSum = 0.0;
        stepCount = 0;
        history.fill (0.0);
        stepsSeen = 0;
        blockHistogram.clear();
        shortHistogram.clear();
        m.store (noValue);
        s.store (noValue);
        integ.store (noValue);
        lraValue.store (0.0f);
        elapsed.store (0.0f);
    }

    void closeStep (double stepMeanSquare) noexcept
    {
        history[static_cast<size_t> (stepsSeen % stepsPerShort)] = stepMeanSquare;
        ++stepsSeen;
        elapsed.store (static_cast<float> (static_cast<double> (stepsSeen) * 0.1), std::memory_order_relaxed);

        if (stepsSeen >= stepsPerBlock)
        {
            const double z = windowMean (stepsPerBlock);
            m.store (toLoudness (z), std::memory_order_relaxed);
            const double l = -0.691 + 10.0 * std::log10 (std::max (z, 1e-30));
            if (l >= lowest)
                blockHistogram.add (l, z);
            integ.store (computeIntegrated(), std::memory_order_relaxed);
        }

        if (stepsSeen >= stepsPerShort)
        {
            const double z = windowMean (stepsPerShort);
            s.store (toLoudness (z), std::memory_order_relaxed);
            const double l = -0.691 + 10.0 * std::log10 (std::max (z, 1e-30));
            if (l >= lowest)
                shortHistogram.add (l, z);
            lraValue.store (computeRange(), std::memory_order_relaxed);
        }
    }

    double windowMean (int steps) const noexcept
    {
        double sum = 0.0;
        for (int k = 1; k <= steps; ++k)
            sum += history[static_cast<size_t> ((stepsSeen - k) % stepsPerShort)];
        return sum / steps;
    }

    float computeIntegrated() const noexcept
    {
        double meanAbs;
        unsigned nAbs;
        blockHistogram.tail (0, meanAbs, nAbs);
        if (nAbs == 0)
            return noValue;

        const double relGate = -0.691 + 10.0 * std::log10 (meanAbs) - 10.0;
        double mean;
        unsigned n;
        blockHistogram.tail (Histogram::bin (std::max (relGate, lowest)), mean, n);
        return n > 0 ? toLoudness (mean) : noValue;
    }

    float computeRange() const noexcept
    {
        double meanAbs;
        unsigned nAbs;
        shortHistogram.tail (0, meanAbs, nAbs);
        if (nAbs == 0)
            return 0.0f;

        const double relGate = -0.691 + 10.0 * std::log10 (meanAbs) - 20.0;
        const int from = Histogram::bin (std::max (relGate, lowest));

        unsigned total = 0;
        for (int b = from; b < numBins; ++b)
            total += shortHistogram.count[static_cast<size_t> (b)];
        if (total < 2)
            return 0.0f;

        const auto percentile = [&] (double p)
        {
            const double target = p * (total - 1);
            unsigned acc = 0;
            for (int b = from; b < numBins; ++b)
            {
                acc += shortHistogram.count[static_cast<size_t> (b)];
                if (acc > target)
                    return lowest + (b + 0.5) / 10.0;
            }
            return lowest + (numBins - 0.5) / 10.0;
        };
        return static_cast<float> (std::max (0.0, percentile (0.95) - percentile (0.10)));
    }

    double fs = 48000.0;
    int channels = 2, stepLength = 4800;
    std::array<Biquad, 2> shelfFilter, highPassFilter;
    double stepSum = 0.0;
    int stepCount = 0;

    std::array<double, stepsPerShort> history {};
    long stepsSeen = 0;
    Histogram blockHistogram, shortHistogram;

    std::atomic<float> m { noValue }, s { noValue }, integ { noValue }, lraValue { 0.0f }, elapsed { 0.0f };
    std::atomic<bool> resetRequested { false };
};
} // namespace ferb
