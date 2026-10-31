#pragma once

#include <algorithm>
#include <cmath>

namespace nel
{
/** Reducción de ganancia (dB, ≥ 0) de la etapa de cuerpo: codo suave y relación infinita.
    `x` es el nivel del pico menos el umbral, en dB. */
inline float softKnee (float x, float knee) noexcept
{
    if (x <= 0.0f && 2.0f * x < -knee)
        return 0.0f;
    if (2.0f * std::abs (x) <= knee)
        return (x + knee * 0.5f) * (x + knee * 0.5f) / (2.0f * std::max (knee, 1e-6f));
    return x;
}

/** Seguidor de la reducción de la etapa de cuerpo (B) de un canal, a la frecuencia base: ataque
    fijo y release entre rápido y lento. Con el release automático, los picos aislados usan el release
    rápido (el lento dividido entre 12) y una reducción sostenida se acerca al lento. */
class BodyFollower
{
public:
    void reset() noexcept { reduction = 0.0f; sustained = 0.0f; }

    /** `target` en dB (≥ 0). Devuelve la reducción actual en dB. */
    float process (float target, float attackCoef, float releaseSlowCoef, float releaseFastCoef, bool automatic, float sustainedCoef) noexcept
    {
        float releaseCoef = releaseSlowCoef;
        if (automatic)
        {
            const float mix = std::clamp (sustained / 3.0f, 0.0f, 1.0f);
            releaseCoef = releaseFastCoef + (releaseSlowCoef - releaseFastCoef) * mix;
        }

        const float coef = target > reduction ? attackCoef : releaseCoef;
        reduction = coef * reduction + (1.0f - coef) * target;
        sustained = sustainedCoef * sustained + (1.0f - sustainedCoef) * reduction;
        return reduction;
    }

private:
    float reduction = 0.0f, sustained = 0.0f;
};

inline float timeToCoef (float seconds, double sampleRate) noexcept
{
    return seconds <= 0.0f ? 0.0f : static_cast<float> (std::exp (-1.0 / (static_cast<double> (seconds) * sampleRate)));
}
} // namespace nel
