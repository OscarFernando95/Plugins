#include "PotyEngine.h"

namespace poty
{
namespace
{
constexpr double smoothSeconds = 0.020;
constexpr double bypassFadeSeconds = 0.010;
constexpr double switchFadeSeconds = 0.005;
constexpr int switchHoldSamples = PotyEngine::chunkSize; // más que la latencia de la cadena
constexpr float intensityScale[3] = { 0.6f, 1.0f, 1.8f };
constexpr int defaultKey = 1 * 3 + 0; // 2×, estéreo (se corrige en el primer bloque)

float dbToGain (float db) noexcept { return juce::Decibels::decibelsToGain (db); }
size_t idx (int i) noexcept { return static_cast<size_t> (i); }

int factorFor (int quality) noexcept { return 1 << juce::jlimit (0, 3, quality); }
int makeKey (const Settings& s) noexcept { return juce::jlimit (0, 3, s.quality) * 3 + juce::jlimit (0, 2, s.mode); }
} // namespace

void PotyEngine::prepare (double newSampleRate, int)
{
    sampleRate = newSampleRate;

    inGain.reset (sampleRate, smoothSeconds);
    outGain.reset (sampleRate, smoothSeconds);
    mix.reset (sampleRate, smoothSeconds);
    bypassMix.reset (sampleRate, bypassFadeSeconds);
    for (auto& d : drive)
        d.reset (sampleRate, smoothSeconds);

    for (auto& f : filters)
        f.prepare (sampleRate);

    // Toda la memoria se reserva aquí, para el máximo (4×): cambiar de modo no asigna.
    constexpr int osMax = chunkSize * ferb::Oversampler::maxFactor;
    for (auto* group : { &work, &delta, &sum, &wet, &dryOut, &rawOut })
        for (auto& v : *group)
            v.assign (idx (chunkSize), 0.0f);
    for (auto& v : osSum)
        v.assign (idx (osMax), 0.0f);
    up.assign (idx (osMax), 0.0f);
    dBase.assign (idx (chunkSize), 1.0f);
    dOs.assign (idx (osMax), 1.0f);
    inRamp.assign (idx (chunkSize), 1.0f);
    outRamp.assign (idx (chunkSize), 1.0f);
    mixRamp.assign (idx (chunkSize), 1.0f);
    bypassRamp.assign (idx (chunkSize), 0.0f);

    for (auto& band : ups)
        for (auto& u : band)
            u.prepare (ferb::Oversampler::maxFactor, chunkSize);
    for (auto& band : sats)
        for (auto& s : band)
            s.prepare (osMax);
    for (auto& d : downs)
        d.prepare (ferb::Oversampler::maxFactor, chunkSize);

    for (auto* group : { &rawDelay, &dryDelay, &wetDelay })
        for (auto& d : *group)
            d.prepare (getLatencySamples());
    for (auto& d : rawDelay) d.setDelay (getLatencySamples());
    for (auto& d : dryDelay) d.setDelay (getLatencySamples());

    modeSwitch.prepare (sampleRate, switchFadeSeconds, switchHoldSamples);

    reset();
}

void PotyEngine::reset()
{
    inGain.setCurrentAndTargetValue (1.0f);
    outGain.setCurrentAndTargetValue (1.0f);
    mix.setCurrentAndTargetValue (1.0f);
    bypassMix.setCurrentAndTargetValue (0.0f);

    for (auto& d : rawDelay) d.reset();
    for (auto& d : dryDelay) d.reset();

    applyStructure (defaultKey); // se corrige en el primer bloque con los ajustes reales
    modeSwitch.reset (defaultKey);
    meter.reset();
    asleep = false;
    needsInit = true;
}

/** Fija factor, modo y relleno de latencia. No asigna memoria; reinicia solo el camino saturado
    (el camino de la señal original no se toca, así no se abre un hueco en él). */
void PotyEngine::applyStructure (int key)
{
    factor = factorFor (key / 3);
    workMode = key % 3;
    latencyPad = getLatencySamples() - ferb::Oversampler::latencyFor (factor);

    for (auto& band : ups)
        for (auto& u : band)
            u.setFactor (factor);
    for (auto& band : sats)
        for (auto& s : band)
            s.setSampleRate (sampleRate * factor);
    for (auto& d : downs)
        d.setFactor (factor);

    resetDsp();
}

void PotyEngine::resetDsp() noexcept
{
    for (auto& f : filters)
        f.resetState();
    for (auto& band : ups)
        for (auto& u : band)
            u.reset();
    for (auto& band : sats)
        for (auto& s : band)
            s.reset();
    for (auto& d : downs)
        d.reset();
    for (auto& d : wetDelay)
    {
        d.reset();
        d.setDelay (latencyPad);
    }
    bandActive.fill (false);
}

void PotyEngine::process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept
{
    const int total = buffer.getNumSamples();
    const int channels = juce::jmin (buffer.getNumChannels(), maxChannels);

    // El anfitrión puede entregar bloques de cualquier tamaño: se parte en trozos fijos.
    for (int start = 0; start < total; start += chunkSize)
    {
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), channels, start,
                                        juce::jmin (chunkSize, total - start));
        processChunk (chunk, settings);
    }
}

void PotyEngine::processChunk (juce::AudioBuffer<float>& chunk, const Settings& s) noexcept
{
    const int n = chunk.getNumSamples();
    const int channels = chunk.getNumChannels();

    inGain.setTargetValue (dbToGain (s.inGainDb));
    outGain.setTargetValue (dbToGain (s.outGainDb));
    mix.setTargetValue (s.mixPercent / 100.0f);
    bypassMix.setTargetValue (s.bypass ? 1.0f : 0.0f);

    // 1. Señal original retrasada lo mismo que el camino procesado (para el bypass).
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto* in = chunk.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            rawOut[idx (ch)][idx (i)] = rawDelay[idx (ch)].process (in[i]);
    }

    // Bypass total: sale la señal original retrasada y el DSP duerme. El retardo del camino
    // procesado sigue alimentado para que, al despertar, no contenga datos viejos.
    if (! bypassMix.isSmoothing() && bypassMix.getTargetValue() >= 1.0f)
    {
        const float g = inGain.getTargetValue();
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = chunk.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
            {
                dryDelay[idx (ch)].process (data[i] * g);
                data[i] = rawOut[idx (ch)][idx (i)];
            }
        }

        inGain.skip (n);
        outGain.skip (n);
        mix.skip (n);
        asleep = true;
        meter.publishMax (chunk.getMagnitude (0, n));
        return;
    }

    if (asleep || needsInit)
    {
        // Primer bloque o despertar del bypass: la cadena arranca limpia, ya en la estructura pedida.
        applyStructure (makeKey (s));
        modeSwitch.reset (makeKey (s));
        asleep = false;
        needsInit = false;
    }

    const bool blend = bypassMix.isSmoothing() || bypassMix.getTargetValue() > 0.0f;
    for (int i = 0; i < n; ++i)
    {
        inRamp[idx (i)] = inGain.getNextValue();
        outRamp[idx (i)] = outGain.getNextValue();
        mixRamp[idx (i)] = mix.getNextValue();
        bypassRamp[idx (i)] = blend ? bypassMix.getNextValue() : 0.0f;
    }

    // 2. Cambio de calidad o de modo: fundido a silencio, cambio de estructura y fundido de vuelta.
    const auto step = modeSwitch.update (makeKey (s), n);
    if (step.switched)
        applyStructure (step.mode);

    // 3. Ganancia de entrada y señal original retrasada (Lmax, siempre).
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = chunk.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            data[i] *= inRamp[idx (i)];
            dryOut[idx (ch)][idx (i)] = dryDelay[idx (ch)].process (data[i]);
        }
    }

    // 4. Canales de trabajo: L/R, o solo mid, o solo side (mid = (L+R)/2, side = (L−R)/2).
    const bool midSide = channels == 2 && workMode != 0;
    const int nWork = midSide ? 1 : channels;
    if (midSide)
    {
        const auto* l = chunk.getReadPointer (0);
        const auto* r = chunk.getReadPointer (1);
        const float sign = workMode == 1 ? 1.0f : -1.0f;
        for (int i = 0; i < n; ++i)
            work[0][idx (i)] = 0.5f * (l[i] + sign * r[i]);
    }
    else
    {
        for (int ch = 0; ch < nWork; ++ch)
            std::copy_n (chunk.getReadPointer (ch), n, work[idx (ch)].begin());
    }

    // 5. Bandas: Δ = realce(x) − x a frecuencia base y saturación sobremuestreada.
    for (int w = 0; w < nWork; ++w)
    {
        std::fill_n (sum[idx (w)].begin(), n, 0.0f);
        std::fill_n (osSum[idx (w)].begin(), n * factor, 0.0f);
    }

    const float scale = intensityScale[juce::jlimit (0, 2, s.intensity)];

    for (int b = 0; b < numBands; ++b)
    {
        auto& filter = filters[idx (b)];
        const auto& bs = s.bands[idx (b)];
        const float targetDrive = 1.0f + 19.0f * bs.drive / 100.0f * scale;
        filter.setTarget (bs.on, bs.freq, bs.gainDb, bs.q);

        if (filter.isIdle())
        {
            // Banda saltada: no arrastra estado viejo cuando vuelva.
            if (bandActive[idx (b)])
            {
                filter.resetState();
                for (int w = 0; w < maxChannels; ++w)
                {
                    ups[idx (b)][idx (w)].reset();
                    sats[idx (b)][idx (w)].reset();
                }
                bandActive[idx (b)] = false;
            }
            drive[idx (b)].setCurrentAndTargetValue (targetDrive);
            continue;
        }
        bandActive[idx (b)] = true;

        drive[idx (b)].setTargetValue (targetDrive);
        for (int i = 0; i < n; ++i)
            dBase[idx (i)] = drive[idx (b)].getNextValue();
        for (int j = 0; j < n * factor; ++j)
            dOs[idx (j)] = dBase[idx (j / factor)];

        // Los coeficientes se acercan al objetivo una vez por sub-bloque, iguales en todos los canales.
        for (int pos = 0; pos < n; pos += BandFilter::subBlock)
        {
            filter.advance();
            const int len = juce::jmin (BandFilter::subBlock, n - pos);
            for (int w = 0; w < nWork; ++w)
                for (int i = pos; i < pos + len; ++i)
                {
                    const float x = work[idx (w)][idx (i)];
                    delta[idx (w)][idx (i)] = filter.process (w, x) - x;
                }
        }

        const auto type = static_cast<ferb::SatType> (juce::jlimit (0, ferb::numSatTypes - 1, bs.type));

        for (int w = 0; w < nWork; ++w)
        {
            auto& sat = sats[idx (b)][idx (w)];
            auto* d = delta[idx (w)].data();

            if (factor == 1)
            {
                if (! s.linear)
                {
                    sat.setType (type);
                    sat.process (d, dBase.data(), n);
                }
                for (int i = 0; i < n; ++i)
                    sum[idx (w)][idx (i)] += d[i];
            }
            else
            {
                ups[idx (b)][idx (w)].process (d, up.data(), n);
                if (! s.linear)
                {
                    sat.setType (type);
                    sat.process (up.data(), dOs.data(), n * factor);
                }
                for (int j = 0; j < n * factor; ++j)
                    osSum[idx (w)][idx (j)] += up[idx (j)];
            }
        }
    }

    // 6. Una sola bajada de frecuencia, relleno de latencia y fundido del cambio de modo.
    for (int w = 0; w < nWork; ++w)
    {
        if (factor > 1)
            downs[idx (w)].process (osSum[idx (w)].data(), sum[idx (w)].data(), n);

        for (int i = 0; i < n; ++i)
        {
            const float t = static_cast<float> (i + 1) / static_cast<float> (n);
            const float g = step.gainStart + (step.gainEnd - step.gainStart) * t;
            wet[idx (w)][idx (i)] = wetDelay[idx (w)].process (sum[idx (w)][idx (i)]) * g;
        }
    }

    // 7. Salida: (original + mezcla · aporte) · ganancia, y fundido con el bypass.
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = chunk.getWritePointer (ch);
        const float sign = (midSide && workMode == 2 && ch == 1) ? -1.0f : 1.0f;
        const auto& w = wet[idx (midSide ? 0 : ch)];

        for (int i = 0; i < n; ++i)
        {
            float y = (dryOut[idx (ch)][idx (i)] + mixRamp[idx (i)] * sign * w[idx (i)]) * outRamp[idx (i)];
            if (blend)
            {
                const float m = bypassRamp[idx (i)];
                y = rawOut[idx (ch)][idx (i)] * m + y * (1.0f - m);
            }
            data[i] = y;
        }
    }

    meter.publishMax (chunk.getMagnitude (0, n));
}
} // namespace poty
