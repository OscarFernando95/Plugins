#include "CremitaEngine.h"

namespace cremita
{
namespace
{
constexpr double smoothSeconds = 0.020;
constexpr double bypassFadeSeconds = 0.010;
constexpr double switchFadeSeconds = 0.005;
constexpr double matchSeconds = 1.0;
constexpr float matchLimitDb = 12.0f;
constexpr float silenceFloorLufs = -70.0f;
constexpr int switchHoldSamples = CremitaEngine::chunkSize; // más que la latencia de la cadena

float dbToGain (float db) noexcept { return juce::Decibels::decibelsToGain (db); }
size_t idx (int i) noexcept { return static_cast<size_t> (i); }

/** Clave de estructura: sobremuestreo (2 bits), bandas, M/S y clip. */
int makeKey (const Settings& s, int channels) noexcept
{
    const int os = juce::jlimit (0, 3, s.oversample);
    const int ms = (s.midSide && channels == 2) ? 1 : 0;
    return os | (s.split ? 4 : 0) | (ms ? 8 : 0) | (s.clip ? 16 : 0);
}
} // namespace

void CremitaEngine::prepare (double newSampleRate, int)
{
    sampleRate = newSampleRate;

    for (auto* s : { &inGain, &outGain, &effect, &curveC, &biasP, &xo1, &xo2 })
        s->reset (sampleRate, smoothSeconds);
    for (auto& t : trim)
        t.reset (sampleRate, smoothSeconds);
    bypassMix.reset (sampleRate, bypassFadeSeconds);

    // Toda la memoria se reserva aquí, para el máximo (8×): cambiar de modo no asigna.
    constexpr int osMax = chunkSize * ferb::Oversampler::maxFactor;
    for (auto* group : { &work, &dryAP, &acc, &rawOut, &outBuf })
        for (auto& v : *group)
            v.assign (idx (chunkSize), 0.0f);
    for (auto& v : bandIn)
        v.assign (idx (chunkSize), 0.0f);
    for (auto& v : trimRamp)
        v.assign (idx (chunkSize), 1.0f);
    drySig.assign (idx (chunkSize), 0.0f);
    wetBase.assign (idx (chunkSize), 0.0f);
    effectRect.assign (idx (chunkSize), 0.0f);
    osIn.assign (idx (osMax), 0.0f);
    osOut.assign (idx (osMax), 0.0f);
    params.assign (idx (chunkSize), CurveParams {});
    for (auto* r : { &inRamp, &outRamp, &effectRamp, &bypassRamp, &matchRamp })
        r->assign (idx (chunkSize), 1.0f);

    for (auto& ch : ups)
        for (auto& u : ch)
            u.prepare (ferb::Oversampler::maxFactor, chunkSize);
    for (auto& ch : downs)
        for (auto& d : ch)
            d.prepare (ferb::Oversampler::maxFactor, chunkSize);

    for (auto& d : dcBlockers)
        d.prepare (sampleRate, 5.0);

    const int latency = getLatencySamples();
    for (auto& d : rawDelay)
    {
        d.prepare (latency);
        d.setDelay (latency);
    }
    for (auto& ch : dryDelay)
        for (auto& d : ch)
        {
            d.prepare (latency);
            d.setDelay (latency);
        }
    for (auto& ch : wetDelay)
        for (auto& d : ch)
            d.prepare (latency);

    modeSwitch.prepare (sampleRate, switchFadeSeconds, switchHoldSamples);

    for (auto* v : { &inputVu, &outputVu, &effectVu })
        v->prepare (sampleRate);
    loudnessIn.prepare (sampleRate, 2);
    loudnessOut.prepare (sampleRate, 2);

    reset();
}

void CremitaEngine::reset()
{
    for (auto* s : { &inGain, &outGain })
        s->setCurrentAndTargetValue (1.0f);
    effect.setCurrentAndTargetValue (1.0f);
    curveC.setCurrentAndTargetValue (0.0f);
    biasP.setCurrentAndTargetValue (0.0f);
    xo1.setCurrentAndTargetValue (200.0f);
    xo2.setCurrentAndTargetValue (3000.0f);
    for (auto& t : trim)
        t.setCurrentAndTargetValue (1.0f);
    bypassMix.setCurrentAndTargetValue (0.0f);

    for (auto& d : rawDelay)
        d.reset();
    for (auto& ch : dryDelay)
        for (auto& d : ch)
            d.reset();

    applyStructure (1 | 16); // 2×, completa, estéreo, clip: se corrige en el primer bloque
    modeSwitch.reset (1 | 16);

    for (auto* v : { &inputVu, &outputVu, &effectVu })
        v->reset();
    overloadMeter.reset();
    workMeter.reset();
    loudnessIn.reset();
    loudnessOut.reset();
    matchState = 0.0f;
    matchGainDb.store (0.0f);
    asleep = false;
    needsInit = true;
    lastXo1 = lastXo2 = 0.0f;
}

/** Fija factor, bandas, M/S y relleno de latencia. No asigna memoria. */
void CremitaEngine::applyStructure (int key)
{
    factor = 1 << (key & 3);
    splitOn = (key & 4) != 0;
    midSideOn = (key & 8) != 0;
    clipOn = (key & 16) != 0;
    latencyPad = getLatencySamples() - ferb::Oversampler::latencyFor (factor);

    for (auto& ch : ups)
        for (auto& u : ch)
            u.setFactor (factor);
    for (auto& ch : downs)
        for (auto& d : ch)
            d.setFactor (factor);

    resetDsp();
}

void CremitaEngine::resetDsp() noexcept
{
    for (auto& ch : ups)
        for (auto& u : ch)
            u.reset();
    for (auto& ch : downs)
        for (auto& d : ch)
            d.reset();
    for (auto& ch : adaas)
        for (auto& a : ch)
            a.reset();
    for (auto& s : splits)
        s.reset();
    for (auto& d : dcBlockers)
        d.reset();
    for (auto& ch : dryDelay)
        for (auto& d : ch)
            d.reset();
    for (auto& ch : wetDelay)
        for (auto& d : ch)
        {
            d.reset();
            d.setDelay (latencyPad);
        }
    lastXo1 = lastXo2 = 0.0f; // fuerza recalcular los cruces
}

void CremitaEngine::updateCrossovers (int n) noexcept
{
    // Los cruces se mueven con la frecuencia suavizada, recalculando coeficientes cada pocas muestras.
    const float f1 = xo1.skip (n), f2 = xo2.skip (n);
    if (std::abs (f1 - lastXo1) > 0.01f || std::abs (f2 - lastXo2) > 0.01f)
    {
        for (auto& s : splits)
            s.setFrequencies (sampleRate, f1, f2);
        lastXo1 = f1;
        lastXo2 = f2;
    }
}

void CremitaEngine::process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept
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

void CremitaEngine::processChunk (juce::AudioBuffer<float>& chunk, const Settings& s) noexcept
{
    const int n = chunk.getNumSamples();
    const int channels = chunk.getNumChannels();

    inGain.setTargetValue (dbToGain (s.inputDb));
    outGain.setTargetValue (dbToGain (s.outputDb));
    effect.setTargetValue (juce::jlimit (0.0f, 100.0f, s.effectPercent) / 100.0f);
    curveC.setTargetValue (s.curve);
    biasP.setTargetValue (s.biasPercent);
    xo1.setTargetValue (s.crossover1);
    xo2.setTargetValue (juce::jmax (s.crossover2, s.crossover1 * 1.2f));
    for (size_t b = 0; b < trim.size(); ++b)
        trim[b].setTargetValue (dbToGain (s.trimDb[b]));
    bypassMix.setTargetValue ((s.bypass || ! s.inOn) ? 1.0f : 0.0f);

    // 1. Entrada original retrasada lo mismo que la cadena procesada (para el bypass y el botón In).
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* in = chunk.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (in[i])) // un NaN o infinito de entrada no debe quedarse en los filtros
                in[i] = 0.0f;
            rawOut[idx (ch)][idx (i)] = rawDelay[idx (ch)].process (in[i]);
        }
    }

    // Bypass total: sale la entrada retrasada y el DSP duerme.
    if (! bypassMix.isSmoothing() && bypassMix.getTargetValue() >= 1.0f)
    {
        for (int ch = 0; ch < channels; ++ch)
            std::copy_n (rawOut[idx (ch)].begin(), n, chunk.getWritePointer (ch));

        for (auto* sm : { &inGain, &outGain, &effect, &curveC, &biasP })
            sm->skip (n);
        for (auto& t : trim)
            t.skip (n);
        xo1.skip (n);
        xo2.skip (n);
        asleep = true;
        workMeter.publishMax (chunk.getMagnitude (0, n));
        return;
    }

    if (asleep || needsInit)
    {
        // Primer bloque o despertar del bypass: la cadena arranca limpia, ya en la estructura pedida.
        applyStructure (makeKey (s, channels));
        modeSwitch.reset (makeKey (s, channels));
        loudnessIn.reset();
        loudnessOut.reset();
        asleep = false;
        needsInit = false;
    }

    // Sonoridad de la entrada original (antes de Input), para «Igualar sonoridad».
    {
        const float* ptrs[2] = { chunk.getReadPointer (0), chunk.getReadPointer (channels > 1 ? 1 : 0) };
        loudnessIn.process (ptrs, n);
    }

    const bool blend = bypassMix.isSmoothing() || bypassMix.getTargetValue() > 0.0f;
    for (int i = 0; i < n; ++i)
    {
        inRamp[idx (i)] = inGain.getNextValue();
        outRamp[idx (i)] = outGain.getNextValue();
        effectRamp[idx (i)] = effect.getNextValue();
        bypassRamp[idx (i)] = blend ? bypassMix.getNextValue() : 0.0f;
        params[idx (i)] = CurveParams::make (curveC.getNextValue(), biasP.getNextValue(), clipOn);
        for (size_t b = 0; b < trim.size(); ++b)
            trimRamp[b][idx (i)] = trim[b].getNextValue();
    }

    // 2. Cambio de estructura: fundido a silencio, cambio y fundido de vuelta.
    const auto step = modeSwitch.update (makeKey (s, channels), n);
    if (step.switched)
        applyStructure (step.mode);

    // 3. Igualar sonoridad: la ganancia se acerca a (entrada − salida) en 1 s y se congela en silencio.
    {
        float target = 0.0f;
        if (s.match)
        {
            const float lin = loudnessIn.shortTerm(), lout = loudnessOut.shortTerm();
            if (lin > silenceFloorLufs && lout > silenceFloorLufs)
                target = juce::jlimit (-matchLimitDb, matchLimitDb, lin - lout);
            else
                target = matchState;
        }
        const float before = matchState;
        matchState += (target - matchState) * (1.0f - std::exp (-static_cast<float> (n / (sampleRate * matchSeconds))));
        matchGainDb.store (matchState, std::memory_order_relaxed);
        const float g0 = dbToGain (before), g1 = dbToGain (matchState);
        for (int i = 0; i < n; ++i)
            matchRamp[idx (i)] = g0 + (g1 - g0) * static_cast<float> (i + 1) / static_cast<float> (n);
    }

    // 4. Ganancia de entrada, medidor de trabajo y canales de trabajo (L/R o mid/side).
    float peak = 0.0f;
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = chunk.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            data[i] *= inRamp[idx (i)];
            peak = juce::jmax (peak, std::abs (data[i]));
        }
    }
    workMeter.publishMax (peak);
    {
        const float* ptrs[2] = { chunk.getReadPointer (0), chunk.getReadPointer (channels > 1 ? 1 : 0) };
        inputVu.process (ptrs, channels, n);
    }

    const int nWork = channels;
    if (midSideOn)
    {
        const auto* l = chunk.getReadPointer (0);
        const auto* r = chunk.getReadPointer (1);
        for (int i = 0; i < n; ++i)
        {
            work[0][idx (i)] = 0.5f * (l[i] + r[i]);
            work[1][idx (i)] = 0.5f * (l[i] - r[i]);
        }
    }
    else
    {
        for (int ch = 0; ch < nWork; ++ch)
            std::copy_n (chunk.getReadPointer (ch), n, work[idx (ch)].begin());
    }

    // 5. Bandas: seco retrasado, curva sobremuestreada y aporte ponderado de cada banda.
    if (splitOn)
        updateCrossovers (n);

    const int nb = splitOn ? numBands : 1;
    for (int w = 0; w < nWork; ++w)
    {
        std::fill_n (dryAP[idx (w)].begin(), n, 0.0f);
        std::fill_n (acc[idx (w)].begin(), n, 0.0f);

        if (splitOn)
        {
            std::array<float, 3> bands {};
            for (int i = 0; i < n; ++i)
            {
                splits[idx (w)].process (work[idx (w)][idx (i)], bands);
                for (int b = 0; b < numBands; ++b)
                    bandIn[idx (b)][idx (i)] = bands[idx (b)];
            }
        }
        else
        {
            std::copy_n (work[idx (w)].begin(), n, bandIn[0].begin());
        }

        for (int b = 0; b < nb; ++b)
        {
            const float* in = bandIn[idx (b)].data();

            for (int i = 0; i < n; ++i)
            {
                drySig[idx (i)] = dryDelay[idx (w)][idx (b)].process (in[i]);
                dryAP[idx (w)][idx (i)] += drySig[idx (i)];
            }

            const float* os = in;
            if (factor > 1)
            {
                ups[idx (w)][idx (b)].process (in, osIn.data(), n);
                os = osIn.data();
            }

            auto& adaa = adaas[idx (w)][idx (b)];
            const int total = n * factor;
            for (int j = 0; j < total; ++j)
                osOut[idx (j)] = static_cast<float> (adaa.process (params[idx (j / factor)], static_cast<double> (os[j])));

            if (factor > 1)
                downs[idx (w)][idx (b)].process (osOut.data(), wetBase.data(), n);
            else
                std::copy_n (osOut.begin(), n, wetBase.begin());

            for (int i = 0; i < n; ++i)
            {
                const float wet = wetDelay[idx (w)][idx (b)].process (wetBase[idx (i)]);
                acc[idx (w)][idx (i)] += (splitOn ? trimRamp[idx (b)][idx (i)] : 1.0f) * (wet - drySig[idx (i)]);
            }
        }

        // La curva con sesgo crea componente continua: se quita solo del aporte, no de la señal seca.
        for (int i = 0; i < n; ++i)
            acc[idx (w)][idx (i)] = dcBlockers[idx (w)].process (acc[idx (w)][idx (i)]);
    }

    // 6. Mezcla con Efecto, fundido de estructura y paso a L/R.
    for (int i = 0; i < n; ++i)
    {
        const float t = static_cast<float> (i + 1) / static_cast<float> (n);
        const float g = step.gainStart + (step.gainEnd - step.gainStart) * t;
        const float e = effectRamp[idx (i)] * g;

        if (midSideOn)
        {
            const float wm = e * acc[0][idx (i)], ws = e * acc[1][idx (i)];
            const float dm = g * dryAP[0][idx (i)], ds = g * dryAP[1][idx (i)];
            outBuf[0][idx (i)] = dm + ds + wm + ws;
            outBuf[1][idx (i)] = dm - ds + wm - ws;
            effectRect[idx (i)] = 0.5f * (std::abs (wm + ws) + std::abs (wm - ws));
        }
        else
        {
            float rect = 0.0f;
            for (int ch = 0; ch < channels; ++ch)
            {
                const float wv = e * acc[idx (ch)][idx (i)];
                outBuf[idx (ch)][idx (i)] = g * dryAP[idx (ch)][idx (i)] + wv;
                rect += std::abs (wv);
            }
            effectRect[idx (i)] = rect / static_cast<float> (channels);
        }
    }
    effectVu.processRectified (effectRect.data(), n);

    {
        const float* ptrs[2] = { outBuf[0].data(), outBuf[channels > 1 ? 1 : 0].data() };
        loudnessOut.process (ptrs, n);
    }

    // 7. Igualar sonoridad, Output, recorte final a 0 dBFS y mezcla con la entrada retrasada.
    float over = 0.0f;
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = chunk.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            float y = outBuf[idx (ch)][idx (i)] * matchRamp[idx (i)] * outRamp[idx (i)];
            over = juce::jmax (over, std::abs (y));
            if (clipOn)
                y = juce::jlimit (-1.0f, 1.0f, y);
            if (blend)
            {
                const float m = bypassRamp[idx (i)];
                y = rawOut[idx (ch)][idx (i)] * m + y * (1.0f - m);
            }
            data[i] = y;
        }
    }
    overloadMeter.publishMax (over);

    const float* ptrs[2] = { chunk.getReadPointer (0), chunk.getReadPointer (channels > 1 ? 1 : 0) };
    outputVu.process (ptrs, channels, n);
}
} // namespace cremita
