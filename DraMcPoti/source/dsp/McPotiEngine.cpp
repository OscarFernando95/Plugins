#include "McPotiEngine.h"

#include <cmath>

namespace mcpoti
{
namespace
{
constexpr float gamma = 0.5f; // normalización inicial de lo añadido (por afinar)
constexpr float refVocalDb = -24.0f;
constexpr float refMasterDb = -18.0f;

float smoothCoeff (double fs, double ms) { return 1.0f - std::exp (-1.0f / float (0.001 * ms * fs)); }

/** No linealidades de la sección 4 de la ficha (Cinta aún sin pre-énfasis; los DC se quitan con el pasa-altos posterior). */
float shape (int voice, float u) noexcept
{
    switch (voice)
    {
        case 0: return std::tanh (1.5f * u) + 0.25f * u * u;                       // Clásico
        case 1: return std::tanh (u);                                              // Cinta (simplificada)
        case 2: return std::tanh (1.2f * (u + 0.2f)) - std::tanh (1.2f * 0.2f);    // Válvula
        default: return u / std::sqrt (1.0f + u * u);                              // Transistor
    }
}

float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
} // namespace

void McPotiEngine::prepare (double sampleRate, int)
{
    fs = sampleRate;
    const float c20 = smoothCoeff (fs, 20.0);
    for (auto& d : driveSmooth) d.coeff = c20;
    for (auto& l : levelSmooth) l.coeff = c20;
    inSmooth.coeff = outSmooth.coeff = mixSmooth.coeff = c20;
    freqCoeff = 1.0f - std::exp (-float (controlInterval) / float (0.030 * fs)); // 30 ms por paso de control
    envAttack = smoothCoeff (fs, 5.0);
    envRelease = smoothCoeff (fs, 120.0);
    reset();
}

void McPotiEngine::reset() noexcept
{
    for (auto& c : channels)
    {
        c.split.reset();
        for (auto& b : c.hpIn) b.reset();
        for (auto& b : c.hpOut) b.reset();
        c.env.fill (0.0f);
    }
    dynFactor.fill (1.0f);
    lastHp.fill (0.0f);
    lastXo.fill (0.0f);
    addedPower.fill (0.0f);
    for (auto& a : added) a.store (0.0f);
    for (auto& d : driveSmooth) d.snap (0.0f);
    for (auto& l : levelSmooth) l.snap (0.0f);
    inSmooth.snap (1.0f);
    outSmooth.snap (1.0f);
    mixSmooth.snap (1.0f);
    primed = false;
    snapControl = true;
}

void McPotiEngine::updateControl (const Settings& s) noexcept
{
    // Cruces y Sintonización se suavizan (30 ms) para que moverlos no haga clic. Se imponen xo1 < xo2 < xo3 con razón mínima 1,5.
    const float x2t = std::max (s.xo2, s.xo1 * 1.5f);
    const std::array<float, 3> target { s.xo1, x2t, std::max (s.xo3, x2t * 1.5f) };
    if (snapControl)
    {
        snapControl = false;
        smoothXo = target;
        smoothTune = s.tuneHz;
    }
    else
    {
        for (size_t i = 0; i < 3; ++i)
            smoothXo[i] += freqCoeff * (target[i] - smoothXo[i]);
        smoothTune += freqCoeff * (s.tuneHz - smoothTune);
    }
    const float x1 = smoothXo[0], x2 = smoothXo[1], x3 = smoothXo[2];
    bool moved = false;
    for (size_t i = 0; i < 3; ++i)
        moved = moved || std::abs (smoothXo[i] - lastXo[i]) > 0.001f * smoothXo[i];
    if (moved)
    {
        lastXo = smoothXo;
        for (auto& c : channels)
            c.split.setFrequencies (fs, x1, x2, x3);
    }

    // Pasa-altos del generador de cada banda: max(Sintonización, borde inferior); Cuerpo usa 40 Hz.
    const float edges[numBands] = { 40.0f, x1, x2, x3 };
    for (size_t b = 0; b < numBands; ++b)
    {
        const float f = b == 0 ? 40.0f : std::max (smoothTune, edges[b]);
        const float limited = std::min (f, 0.45f * float (fs));
        if (std::abs (limited - lastHp[b]) > 0.001f * limited)
        {
            lastHp[b] = limited;
            const auto hp = ferb::Biquad::highPass (fs, limited);
            for (auto& c : channels)
            {
                c.hpIn[b].setCoefficients (hp);
                c.hpOut[b].setCoefficients (hp);
            }
        }
    }

    // Dinámica por banda: a = drive/100 · 2^(−dyn·e/50), e = clamp((L − Lref)/12, −1, 1), con el nivel enlazado.
    const float ref = s.master ? refMasterDb : refVocalDb;
    const float link = juce::jlimit (0.0f, 1.0f, s.linkPercent * 0.01f);
    for (size_t b = 0; b < numBands; ++b)
    {
        float own = 0.0f, mean = 0.0f;
        for (auto& c : channels)
            mean += c.env[b];
        mean *= 1.0f / float (maxChannels);
        own = channels[0].env[b];
        const float power = link * mean + (1.0f - link) * own;
        const float levelDb = 10.0f * std::log10 (power + 1.0e-12f);
        const float e = juce::jlimit (-1.0f, 1.0f, (levelDb - ref) / 12.0f);
        dynFactor[b] = std::exp2 (-s.bands[b].dyn * e / 50.0f);
    }
}

void McPotiEngine::process (juce::AudioBuffer<float>& buffer, const Settings& s) noexcept
{
    const int numCh = std::min (buffer.getNumChannels(), maxChannels);
    const int n = buffer.getNumSamples();
    if (numCh == 0 || n == 0)
        return;

    if (! primed)
    {
        primed = true;
        inSmooth.snap (dbToGain (s.inputDb));
        outSmooth.snap (dbToGain (s.outputDb));
        mixSmooth.snap (s.mixPercent * 0.01f);
        for (size_t b = 0; b < numBands; ++b)
        {
            driveSmooth[b].snap (s.bands[b].on ? s.bands[b].drive * 0.01f : 0.0f);
            levelSmooth[b].snap (s.bands[b].level * 0.01f);
        }
        updateControl (s);
    }

    bool anySolo = false;
    for (const auto& b : s.bands)
        anySolo = anySolo || b.solo;
    const int voice = juce::jlimit (0, 3, s.voice);
    const bool implemented = s.era == 0; // los caracteres Modernos llegan en la fase 3

    std::array<float*, maxChannels> data {};
    for (int ch = 0; ch < numCh; ++ch)
        data[static_cast<size_t> (ch)] = buffer.getWritePointer (ch);

    for (int i = 0; i < n; ++i)
    {
        if ((i % controlInterval) == 0)
            updateControl (s);

        const float inGain = inSmooth.step (dbToGain (s.inputDb));
        const float outGain = outSmooth.step (dbToGain (s.outputDb));
        const float mix = mixSmooth.step (s.mixPercent * 0.01f);

        std::array<std::array<float, numBands>, maxChannels> bandSig {};
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = channels[static_cast<size_t> (ch)];
            const float x = data[static_cast<size_t> (ch)][i] * inGain;
            std::array<float, numBands> bands {};
            c.split.process (x, bands);
            for (size_t b = 0; b < numBands; ++b)
            {
                bandSig[static_cast<size_t> (ch)][b] = bands[b];
                const float p = bands[b] * bands[b];
                c.env[b] += (p > c.env[b] ? envAttack : envRelease) * (p - c.env[b]);
            }
        }

        std::array<float, numBands> drive {}, level {};
        for (size_t b = 0; b < numBands; ++b)
        {
            drive[b] = driveSmooth[b].step (s.bands[b].on ? s.bands[b].drive * 0.01f : 0.0f) * dynFactor[b];
            level[b] = levelSmooth[b].step (s.bands[b].level * 0.01f);
        }

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = channels[static_cast<size_t> (ch)];
            const float dry = data[static_cast<size_t> (ch)][i] * inGain;
            float addedSum = 0.0f, soloSum = 0.0f;
            for (size_t b = 0; b < numBands; ++b)
            {
                float h = 0.0f;
                if (implemented && drive[b] > 0.0f)
                {
                    const float a = std::min (drive[b], 2.0f);
                    const float u = c.hpIn[b].process (bandSig[static_cast<size_t> (ch)][b]) * (1.0f + 4.0f * a);
                    h = c.hpOut[b].process (shape (voice, u)) * a * gamma;
                }
                h *= level[b];
                addedPower[b] += (h * h - addedPower[b]) * 0.0005f;
                addedSum += h;
                if (s.bands[b].solo)
                    soloSum += bandSig[static_cast<size_t> (ch)][b] + mix * h;
            }
            float y = s.delta ? mix * addedSum : (anySolo ? soloSum : dry + mix * addedSum);
            data[static_cast<size_t> (ch)][i] = y * outGain;
        }
    }

    for (size_t b = 0; b < numBands; ++b)
        added[b].store (std::sqrt (addedPower[b]), std::memory_order_relaxed);
}
} // namespace mcpoti
