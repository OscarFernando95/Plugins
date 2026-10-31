#pragma once

#include <atomic>
#include <cmath>

namespace ferb
{
/** Balística de VU: promedio del valor rectificado con una constante de 65 ms, de modo que una
    señal que empieza de golpe llega al 99 % de su valor final en 300 ms. El resultado se calibra
    para que una senoidal marque su valor eficaz (RMS), como un VU analógico.

    El hilo de audio llama a `process`; la interfaz lee `level()` (lineal) con un atómico. */
class VuBallistics
{
public:
    static constexpr double timeConstant = 0.065;
    static constexpr double sineCalibration = 1.1107207345395915; // π / (2√2): promedio rectificado → RMS

    void prepare (double sampleRate) noexcept
    {
        coef = 1.0 - std::exp (-1.0 / (timeConstant * sampleRate));
        reset();
    }

    void reset() noexcept
    {
        average = 0.0;
        published.store (0.0f, std::memory_order_relaxed);
    }

    /** Promedia los canales muestra a muestra. */
    void process (const float* const* channels, int numChannels, int n) noexcept
    {
        const double inv = 1.0 / numChannels;
        for (int i = 0; i < n; ++i)
        {
            double r = 0.0;
            for (int c = 0; c < numChannels; ++c)
                r += std::abs (static_cast<double> (channels[c][i]));
            average += (r * inv - average) * coef;
        }
        published.store (static_cast<float> (average * sineCalibration), std::memory_order_relaxed);
    }

    /** Un valor rectificado por muestra (para el medidor de efecto). */
    void processRectified (const float* rectified, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
            average += (static_cast<double> (rectified[i]) - average) * coef;
        published.store (static_cast<float> (average * sineCalibration), std::memory_order_relaxed);
    }

    float level() const noexcept { return published.load (std::memory_order_relaxed); }

private:
    double coef = 0.001, average = 0.0;
    std::atomic<float> published { 0.0f };
};
} // namespace ferb
