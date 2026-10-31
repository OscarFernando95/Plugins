#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace poty
{
/** Realce de una banda: biquad de campana (RBJ) con coeficientes interpolados.

    Los coeficientes se calculan al cambiar un parámetro y se acercan al objetivo una vez
    por sub-bloque de `subBlock` muestras, para evitar clics al arrastrar un nodo.
    Coeficientes y estados en doble precisión: a 20-90 Hz los polos quedan muy cerca de 1
    y en simple precisión el ruido de cuantización se nota. */
class BandFilter
{
public:
    static constexpr int subBlock = 32;
    static constexpr int maxChannels = 2;

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        current = target = identity;
        settled = true;
        hasTarget = false;
        resetState();
    }

    void resetState() noexcept
    {
        for (auto& s : state)
            s = {};
    }

    /** Fija el objetivo. Un realce de 0 dB (o banda apagada) es la identidad. */
    void setTarget (bool on, float freq, float gainDb, float q) noexcept
    {
        if (hasTarget && on == lastOn && same (freq, lastFreq) && same (gainDb, lastGainDb) && same (q, lastQ))
            return;

        lastOn = on;
        lastFreq = freq;
        lastGainDb = gainDb;
        lastQ = q;

        target = (on && gainDb > 0.0f) ? peaking (freq, gainDb, q) : identity;

        if (! hasTarget)
            current = target; // primera vez: sin rampa

        hasTarget = true;
        settled = (current == target);
    }

    /** Ni el objetivo ni el estado actual aportan nada: la banda se puede saltar. */
    bool isIdle() const noexcept { return settled && target == identity; }

    /** Avanza un paso hacia el objetivo; se llama una vez por sub-bloque. */
    void advance() noexcept
    {
        if (settled)
            return;

        constexpr double step = 0.1;
        bool done = true;
        for (size_t i = 0; i < current.size(); ++i)
        {
            const double d = target[i] - current[i];
            if (std::abs (d) < 1.0e-9)
                current[i] = target[i];
            else
            {
                current[i] += d * step;
                done = false;
            }
        }
        settled = done;
    }

    /** Respuesta del filtro (H·x) para una muestra. Forma transpuesta directa II. */
    float process (int ch, float x) noexcept
    {
        auto& s = state[static_cast<size_t> (ch)];
        const double in = x;
        const double y = current[0] * in + s.z1;
        s.z1 = current[1] * in - current[3] * y + s.z2;
        s.z2 = current[2] * in - current[4] * y;
        return static_cast<float> (y);
    }

private:
    /** Comparación exacta a propósito: detecta que un parámetro no cambió. */
    static bool same (float a, float b) noexcept { return ! (a < b) && ! (a > b); }

    // b0, b1, b2, a1, a2 (normalizados por a0)
    using Coeffs = std::array<double, 5>;
    static constexpr Coeffs identity { 1.0, 0.0, 0.0, 0.0, 0.0 };

    struct State { double z1 = 0.0, z2 = 0.0; };

    Coeffs peaking (float freq, float gainDb, float q) const noexcept
    {
        const double f = std::clamp (static_cast<double> (freq), 2.0, 0.45 * sampleRate);
        const double A = std::pow (10.0, gainDb / 40.0);
        const double w0 = 2.0 * std::numbers::pi * f / sampleRate;
        const double alpha = std::sin (w0) / (2.0 * static_cast<double> (q));
        const double c = std::cos (w0);

        const double a0 = 1.0 + alpha / A;
        return { (1.0 + alpha * A) / a0, -2.0 * c / a0, (1.0 - alpha * A) / a0,
                 -2.0 * c / a0, (1.0 - alpha / A) / a0 };
    }

    double sampleRate = 48000.0;
    Coeffs current = identity, target = identity;
    std::array<State, maxChannels> state;
    bool settled = true;
    bool hasTarget = false;
    bool lastOn = false;
    float lastFreq = 0.0f, lastGainDb = 0.0f, lastQ = 0.0f;
};
} // namespace poty
