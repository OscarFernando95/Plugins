#include "NelEngine.h"

namespace nel
{
namespace
{
constexpr double smoothSeconds = 0.020;
constexpr double bypassFadeSeconds = 0.010;
constexpr double switchFadeSeconds = 0.050;
constexpr double adoptSeconds = 0.150;
constexpr int switchHoldSamples = 1024; // más que la latencia de la cadena
constexpr float marginMin = 0.1f, marginMax = 0.5f, marginUp = 0.05f, marginDownPerSecond = 0.01f;

/** Recorte suave con rodilla: idéntico a x hasta `knee · techo` (no toca lo que queda por debajo) y, por
    encima, una curva tipo tanh con pendiente continua que se acerca al techo sin pasarlo. */
float softClip (float x, float ceilingLin) noexcept
{
    constexpr float knee = 0.6f;
    const float u = std::abs (x) / ceilingLin;
    if (u <= knee)
        return x;
    const float y = knee + (1.0f - knee) * std::tanh ((u - knee) / (1.0f - knee));
    return std::copysign (y * ceilingLin, x);
}

size_t idx (int i) noexcept { return static_cast<size_t> (i); }
float dbToGain (float db) noexcept { return std::pow (10.0f, db / 20.0f); }
float gainToDb (float g) noexcept { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
} // namespace

int NelEngine::effectiveExponent (const Settings& s, double fs) noexcept
{
    int e = std::clamp (s.oversample, 0, 5);
    if (s.nonRealtime && s.offlineBoost)
        e = std::min (e + 1, 5);
    if (fs >= 176000.0)
        e = std::min (e, 2); // a 176,4 kHz o más, hasta 4×
    else if (fs >= 88000.0)
        e = std::min (e, 3); // a 96 kHz o más, hasta 8×
    return e;
}

int NelEngine::lookaheadSamples (const Settings& s, double fs) noexcept
{
    return static_cast<int> (std::lround (static_cast<double> (std::clamp (s.lookaheadMs, 0.0f, 5.0f)) * fs / 1000.0));
}

int NelEngine::latencyFor (const Settings& s, double fs)
{
    return ferb::OsDesign::latencyFor (1 << effectiveExponent (s, fs), s.minimumPhase) + lookaheadSamples (s, fs);
}

void NelEngine::prepare (double newSampleRate, int)
{
    sampleRate = newSampleRate;
    maxLookaheadBase = static_cast<int> (std::ceil (0.005 * sampleRate)) + 1;

    Settings widest;
    widest.oversample = 5;
    widest.nonRealtime = true;
    const int maxFactor = 1 << effectiveExponent (widest, sampleRate);
    const int maxWindow = maxLookaheadBase * maxFactor;
    const int maxLatency = ferb::OsDesign::latencyFor (maxFactor, false) + maxLookaheadBase + 16;

    for (auto* s : { &gain, &ceiling, &character, &unity })
        s->reset (sampleRate, smoothSeconds);
    for (auto* s : { &bypassMix, &auditionMix })
        s->reset (sampleRate, bypassFadeSeconds);

    for (size_t c = 0; c < maxChannels; ++c)
    {
        for (auto* v : { &base[c], &raw[c], &gainedDry[c], &outBase[c], &bodyGain[c] })
            v->assign (idx (chunkSize), 0.0f);
        os[c].assign (idx (chunkSize * maxFactor), 0.0f);
        tpScratch[c].assign (idx (chunkSize * 4), 0.0f);

        ups[c].prepare (chunkSize);
        downs[c].prepare (chunkSize);
        truePeakUps[c].prepare (chunkSize);
        truePeakUps[c].setConfig (4, false);
        dcBlockers[c].prepare (sampleRate, 5.0);
        peakStages[c].prepare (maxWindow + 2);
        audioDelay[c].prepare (maxWindow + 2);
        rawDelay[c].prepare (maxLatency);
        gainedDelay[c].prepare (maxLatency);
    }
    for (auto* r : { &gainRamp, &ceilRamp, &charRamp, &bypassRamp, &unityRamp, &auditionRamp })
        r->assign (idx (chunkSize), 1.0f);

    modeSwitch.prepare (sampleRate, switchFadeSeconds, switchHoldSamples);
    loudness.prepare (sampleRate, 2);
    reset();
}

void NelEngine::reset()
{
    gain.setCurrentAndTargetValue (1.0f);
    ceiling.setCurrentAndTargetValue (dbToGain (-1.0f));
    character.setCurrentAndTargetValue (0.0f);
    bypassMix.setCurrentAndTargetValue (0.0f);
    unity.setCurrentAndTargetValue (1.0f);
    auditionMix.setCurrentAndTargetValue (0.0f);
    for (size_t c = 0; c < maxChannels; ++c)
    {
        rawDelay[c].reset();
        gainedDelay[c].reset();
        dcBlockers[c].reset();
        dithers[c].reset (static_cast<std::uint32_t> (c + 1));
        rmsState[c] = 0.0f;
        rms[c].store (0.0f);
        inPeak[c].reset();
        outPeak[c].reset();
        truePeakMeter[c].reset();
        envFast[c] = envSlow[c] = 0.0f;
    }
    reductionMeter.reset();
    reductionNow.store (0.0f);
    marginDb.store (marginMin);
    overs.store (0);
    loudness.reset();
    requestedKey = adoptedKey = -1;
    stableSamples = 0;
    asleep = false;
    needsInit = true;
}

void NelEngine::applyStructure (int key) noexcept
{
    exponent = key & 7;
    minPhase = (key & 8) != 0;
    lookaheadBase = key >> 4;
    factor = 1 << exponent;
    const int n = lookaheadBase * factor;
    windowOs = n + 1;
    latency = ferb::OsDesign::latencyFor (factor, minPhase) + lookaheadBase;

    for (size_t c = 0; c < maxChannels; ++c)
    {
        ups[c].setConfig (factor, minPhase);
        downs[c].setConfig (factor, minPhase);
        peakStages[c].configure (windowOs);
        audioDelay[c].setDelay (n);
        audioDelay[c].reset();
        followers[c].reset();
        bodyGainPrev[c] = 1.0f;
        bodySmooth[c] = 0.0f;
        detectorHighPass[c] = ferb::Biquad::highPass (sampleRate * factor, 80.0);
        gainedDelay[c].setDelay (latency);
        gainedDelay[c].reset();
        rawDelay[c].setDelay (latency);
    }
    effectiveFactor.store (factor, std::memory_order_relaxed);
    activeLatency.store (latency, std::memory_order_relaxed);
}

void NelEngine::process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept
{
    const int total = buffer.getNumSamples();
    const int channels = juce::jmin (buffer.getNumChannels(), maxChannels);

    for (int start = 0; start < total; start += chunkSize)
    {
        juce::AudioBuffer<float> chunk (buffer.getArrayOfWritePointers(), channels, start, juce::jmin (chunkSize, total - start));
        processChunk (chunk, settings);
    }
}

void NelEngine::processChunk (juce::AudioBuffer<float>& chunk, const Settings& s) noexcept
{
    const int n = chunk.getNumSamples();
    const int channels = chunk.getNumChannels();
    const auto& mode = modes[idx (std::clamp (s.mode, 0, numModes - 1))];

    gain.setTargetValue (dbToGain (s.gainDb));
    ceiling.setTargetValue (dbToGain (std::clamp (s.ceilingDb, -30.0f, 0.0f)));
    character.setTargetValue (std::clamp (s.characterPercent, 0.0f, 100.0f) / 100.0f);
    bypassMix.setTargetValue (s.bypass ? 1.0f : 0.0f);
    unity.setTargetValue (s.unityGain ? 1.0f / dbToGain (s.gainDb) : 1.0f);
    auditionMix.setTargetValue (s.audition ? 1.0f : 0.0f);

    // 1. Entrada original retrasada (para el bypass) y medidor de entrada. Si el DSP duerme, el retardo
    //    sigue la latencia que se declara con los ajustes actuales.
    if (asleep || needsInit)
        for (int ch = 0; ch < channels; ++ch)
            rawDelay[idx (ch)].setDelay (latencyFor (s, sampleRate));
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* in = chunk.getWritePointer (ch);
        float peak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (in[i]))
                in[i] = 0.0f;
            peak = std::max (peak, std::abs (in[i]));
            raw[idx (ch)][idx (i)] = rawDelay[idx (ch)].process (in[i]);
        }
        inPeak[idx (ch)].publishMax (peak);
    }

    if (s.loudnessFromInput)
    {
        const float* p[2] = { chunk.getReadPointer (0), chunk.getReadPointer (channels > 1 ? 1 : 0) };
        loudness.process (p, n);
    }

    // Bypass total: sale la entrada retrasada y el DSP duerme.
    if (! bypassMix.isSmoothing() && bypassMix.getTargetValue() >= 1.0f)
    {
        for (int ch = 0; ch < channels; ++ch)
            std::copy_n (raw[idx (ch)].begin(), n, chunk.getWritePointer (ch));
        gain.skip (n);
        ceiling.skip (n);
        character.skip (n);
        unity.skip (n);
        auditionMix.skip (n);
        asleep = true;
        reductionNow.store (0.0f, std::memory_order_relaxed);
        if (! s.loudnessFromInput)
        {
            const float* p[2] = { chunk.getReadPointer (0), chunk.getReadPointer (channels > 1 ? 1 : 0) };
            loudness.process (p, n);
        }
        return;
    }

    // 2. Estructura: se adopta cuando el valor pedido lleva 150 ms estable; el cambio se hace en silencio.
    const int exp = effectiveExponent (s, sampleRate);
    const int key = makeKey (exp, s.minimumPhase, std::min (lookaheadSamples (s, sampleRate), maxLookaheadBase));
    if (needsInit || asleep)
    {
        applyStructure (key);
        modeSwitch.reset (key);
        adoptedKey = requestedKey = key;
        stableSamples = 0;
        needsInit = false;
        asleep = false;
    }
    if (key != requestedKey)
    {
        requestedKey = key;
        stableSamples = 0;
    }
    else
    {
        stableSamples += n;
        if (stableSamples >= static_cast<int> (adoptSeconds * sampleRate))
            adoptedKey = requestedKey;
    }
    const auto step = modeSwitch.update (adoptedKey, n);
    if (step.switched)
        applyStructure (step.mode);

    const bool blend = bypassMix.isSmoothing() || bypassMix.getTargetValue() > 0.0f;
    for (int i = 0; i < n; ++i)
    {
        gainRamp[idx (i)] = gain.getNextValue();
        ceilRamp[idx (i)] = ceiling.getNextValue();
        charRamp[idx (i)] = character.getNextValue();
        bypassRamp[idx (i)] = blend ? bypassMix.getNextValue() : 0.0f;
        unityRamp[idx (i)] = unity.getNextValue();
        auditionRamp[idx (i)] = auditionMix.getNextValue();
    }

    // 3. Ganancia, filtro de DC y copia retrasada de la señal con ganancia (para Audición).
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto* in = chunk.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            float v = in[i] * gainRamp[idx (i)];
            if (s.dcFilter)
                v = dcBlockers[idx (ch)].process (v);
            base[idx (ch)][idx (i)] = v;
            gainedDry[idx (ch)][idx (i)] = gainedDelay[idx (ch)].process (v);
        }
    }

    // 4. Sobremuestreo.
    const int total = n * factor;
    for (int ch = 0; ch < channels; ++ch)
        ups[idx (ch)].process (base[idx (ch)].data(), os[idx (ch)].data(), n);

    // 5. En la frecuencia alta: realce de transitorios, Carácter y recorte suave previo.
    const float boostMax = dbToGain (mode.transientBoostDb) - 1.0f;
    const float fastCoef = static_cast<float> (std::exp (-1.0 / (0.001 * sampleRate * factor)));
    const float slowCoef = static_cast<float> (std::exp (-1.0 / (0.030 * sampleRate * factor)));
    const float marginNow = marginDb.load (std::memory_order_relaxed) + mode.peakMarginDb;
    const float marginLin = dbToGain (-marginNow);

    for (int k = 0; k < total; ++k)
    {
        const int b = k / factor;
        const float c = std::clamp (charRamp[idx (b)] + mode.fixedCharacter, 0.0f, 1.0f);
        const float ceilNow = ceilRamp[idx (b)] * marginLin;
        const float kk = 1.0f + 2.0f * c;

        for (int ch = 0; ch < channels; ++ch)
        {
            float x = os[idx (ch)][idx (k)];
            if (boostMax > 0.0f)
            {
                const float a = std::abs (x);
                envFast[idx (ch)] = a + fastCoef * (envFast[idx (ch)] - a);
                envSlow[idx (ch)] = a + slowCoef * (envSlow[idx (ch)] - a);
                x *= 1.0f + boostMax * std::clamp (envFast[idx (ch)] / (envSlow[idx (ch)] + 1.0e-9f) - 1.0f, 0.0f, 1.0f);
            }
            if (c > 0.0f)
                x += c * (std::tanh (kk * x) / kk - x);
            if (mode.preClip > 0.0f)
                x += mode.preClip * (softClip (x, ceilNow) - x);
            os[idx (ch)][idx (k)] = x;
        }
    }

    // 6. Etapa B (cuerpo): a la frecuencia base, con el pico de cada grupo de muestras.
    const float attackCoef = timeToCoef (s.attackMs / 1000.0f, sampleRate);
    const float releaseSlow = std::max (s.releaseMs, 1.0f) / 1000.0f;
    const float slowCoef2 = timeToCoef (releaseSlow, sampleRate);
    const float fastCoef2 = timeToCoef (releaseSlow / 12.0f, sampleRate);
    const float smoothCoef = timeToCoef (mode.extraSmoothing / 1000.0f, sampleRate);
    const float linkR = std::clamp (s.linkRelease, 0.0f, 100.0f) / 100.0f;
    float bodyRedMax = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        float p[maxChannels] = { 0.0f, 0.0f };
        for (int ch = 0; ch < channels; ++ch)
        {
            const float* x = os[idx (ch)].data() + idx (i * factor);
            float peak = 0.0f;
            if (mode.detectorHighPass)
                for (int j = 0; j < factor; ++j)
                    peak = std::max (peak, std::abs (detectorHighPass[idx (ch)].process (x[j])));
            else
                for (int j = 0; j < factor; ++j)
                    peak = std::max (peak, std::abs (x[j]));
            p[ch] = peak;
        }
        const float pmax = channels > 1 ? std::max (p[0], p[1]) : p[0];
        const float thresholdDb = gainToDb (ceilRamp[idx (i)] * marginLin) - mode.bodyMarginDb;

        for (int ch = 0; ch < channels; ++ch)
        {
            const float linked = linkR * pmax + (1.0f - linkR) * p[ch];
            const float target = softKnee (gainToDb (linked) - thresholdDb, s.kneeDb);
            float red = followers[idx (ch)].process (target, attackCoef, slowCoef2, fastCoef2, s.releaseAuto, slowCoef2);
            if (mode.extraSmoothing > 0.0f)
            {
                bodySmooth[idx (ch)] = smoothCoef * bodySmooth[idx (ch)] + (1.0f - smoothCoef) * red;
                red = bodySmooth[idx (ch)];
            }
            bodyRedMax = std::max (bodyRedMax, red);
            bodyGain[idx (ch)][idx (i)] = dbToGain (-red);
        }
    }
    for (int ch = 0; ch < channels; ++ch)
    {
        float prev = bodyGainPrev[idx (ch)];
        auto* x = os[idx (ch)].data();
        for (int i = 0; i < n; ++i)
        {
            const float cur = bodyGain[idx (ch)][idx (i)];
            for (int j = 0; j < factor; ++j)
                x[i * factor + j] *= prev + (cur - prev) * static_cast<float> (j + 1) / static_cast<float> (factor);
            prev = cur;
        }
        bodyGainPrev[idx (ch)] = prev;
    }

    // 7. Etapa A (picos con lookahead) y seguro de pico.
    const float linkT = std::clamp (s.linkTransient, 0.0f, 100.0f) / 100.0f;
    float minPeakGain = 1.0f;
    for (int k = 0; k < total; ++k)
    {
        const float ceilNow = ceilRamp[idx (k / factor)] * marginLin;
        const float a0 = std::abs (os[0][idx (k)]);
        const float a1 = channels > 1 ? std::abs (os[1][idx (k)]) : a0;
        const float mx = std::max (a0, a1);

        for (int ch = 0; ch < channels; ++ch)
        {
            const float own = ch == 0 ? a0 : a1;
            const float pk = linkT * mx + (1.0f - linkT) * own;
            const float required = pk > ceilNow ? ceilNow / pk : 1.0f;
            const float smooth = peakStages[idx (ch)].process (required);
            minPeakGain = std::min (minPeakGain, smooth);
            const float delayed = audioDelay[idx (ch)].process (os[idx (ch)][idx (k)]);
            os[idx (ch)][idx (k)] = std::clamp (delayed * smooth, -ceilNow, ceilNow);
        }
    }
    const float reduction = bodyRedMax + (minPeakGain < 1.0f ? -gainToDb (minPeakGain) : 0.0f);
    reductionMeter.publishMax (reduction);
    reductionNow.store (reduction, std::memory_order_relaxed);

    // 8. Submuestreo.
    for (int ch = 0; ch < channels; ++ch)
        downs[idx (ch)].process (os[idx (ch)].data(), outBase[idx (ch)].data(), n);

    // 9. True peak de la salida (4×): mide, cuenta «overs» y ajusta el margen del techo interno.
    {
        float tp[maxChannels] = { 0.0f, 0.0f }, samplePeak = 0.0f;
        for (int ch = 0; ch < channels; ++ch)
        {
            truePeakUps[idx (ch)].process (outBase[idx (ch)].data(), tpScratch[idx (ch)].data(), n);
            for (int i = 0; i < n; ++i)
                samplePeak = std::max (samplePeak, std::abs (outBase[idx (ch)][idx (i)]));
            for (int j = 0; j < n * 4; ++j)
                tp[ch] = std::max (tp[ch], std::abs (tpScratch[idx (ch)][idx (j)]));
            truePeakMeter[idx (ch)].publishMax (tp[ch]);
        }
        const float measured = s.truePeak ? std::max (tp[0], tp[1]) : samplePeak;
        const float limit = ceilRamp[idx (n - 1)] * 1.0001f;
        float m = marginDb.load (std::memory_order_relaxed);
        if (measured > limit && step.gainEnd >= 0.999f)
        {
            overs.fetch_add (1, std::memory_order_relaxed);
            m = std::min (marginMax, m + marginUp);
        }
        else
        {
            m = std::max (marginMin, m - marginDownPerSecond * static_cast<float> (n / sampleRate));
        }
        marginDb.store (m, std::memory_order_relaxed);
    }

    // 10. Audición o ganancia unitaria, dither, fundido de estructura, bypass y medidores de salida.
    const float rmsCoef = static_cast<float> (1.0 - std::exp (-1.0 / (0.300 * sampleRate)));
    for (int ch = 0; ch < channels; ++ch)
        dithers[idx (ch)].configure (s.ditherBits, s.ditherShape);

    float outPeakNow[maxChannels] = { 0.0f, 0.0f };
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = chunk.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            const float t = static_cast<float> (i + 1) / static_cast<float> (n);
            const float fade = step.gainStart + (step.gainEnd - step.gainStart) * t;

            const float processed = outBase[idx (ch)][idx (i)];
            const float am = auditionRamp[idx (i)];
            float y = processed * unityRamp[idx (i)];
            if (am > 0.0f)
                y = y * (1.0f - am) + (gainedDry[idx (ch)][idx (i)] - processed) * am;
            y = dithers[idx (ch)].process (y) * fade;

            rmsState[idx (ch)] += (y * y - rmsState[idx (ch)]) * rmsCoef;
            if (blend)
            {
                const float m = bypassRamp[idx (i)];
                y = raw[idx (ch)][idx (i)] * m + y * (1.0f - m);
            }
            outPeakNow[ch] = std::max (outPeakNow[ch], std::abs (y));
            data[i] = std::isfinite (y) ? y : 0.0f;
        }
        outPeak[idx (ch)].publishMax (outPeakNow[ch]);
        rms[idx (ch)].store (std::sqrt (std::max (rmsState[idx (ch)], 0.0f)), std::memory_order_relaxed);
    }

    if (! s.loudnessFromInput)
    {
        const float* p[2] = { chunk.getReadPointer (0), chunk.getReadPointer (channels > 1 ? 1 : 0) };
        loudness.process (p, n);
    }
}
} // namespace nel
