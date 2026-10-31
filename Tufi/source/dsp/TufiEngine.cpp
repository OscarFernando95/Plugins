#include "TufiEngine.h"

namespace tufi
{
namespace
{
constexpr double gainSmoothSeconds = 0.020;
constexpr double bypassFadeSeconds = 0.010;
constexpr double switchFadeSeconds = 0.005;
constexpr double shapeSmoothAlpha = 0.35; // por trozo de 256 muestras: ~15 ms
constexpr int switchHoldSamples = TufiEngine::chunkSize;
constexpr int neutralKey = 0;

float dbToGain (float db) noexcept { return juce::Decibels::decibelsToGain (db); }
size_t idx (int i) noexcept { return static_cast<size_t> (i); }
int factorForKey (int key) noexcept { return 1 << juce::jmax (0, key - 1); }
} // namespace

TufiEngine::TufiEngine() : builder (std::make_unique<ShapeTableBuilder>()) {}

TufiEngine::~TufiEngine() = default;

void TufiEngine::prepare (double newSampleRate, int)
{
    sampleRate = newSampleRate;

    drive.reset (sampleRate, gainSmoothSeconds);
    trim.reset (sampleRate, gainSmoothSeconds);
    bypassMix.reset (sampleRate, bypassFadeSeconds);

    // Toda la memoria se reserva aquí, para el máximo (8×): cambiar de modo no asigna.
    constexpr int osMax = chunkSize * ferb::Oversampler::maxFactor;
    for (auto* group : { &x, &neutral, &wet, &rawOut })
        for (auto& v : *group)
            v.assign (idx (chunkSize), 0.0f);
    os.assign (idx (osMax), 0.0f);
    driveRamp.assign (idx (chunkSize), 1.0f);
    trimRamp.assign (idx (chunkSize), 1.0f);
    bypassRamp.assign (idx (chunkSize), 0.0f);

    for (auto& u : ups)
        u.prepare (ferb::Oversampler::maxFactor, chunkSize);
    for (auto& d : downs)
        d.prepare (ferb::Oversampler::maxFactor, chunkSize);
    for (auto& d : dcBlockers)
        d.prepare (sampleRate);

    for (auto* group : { &rawDelay, &neutralDelay, &wetDelay })
        for (auto& d : *group)
            d.prepare (getLatencySamples());
    for (auto& d : rawDelay) d.setDelay (getLatencySamples());
    for (auto& d : neutralDelay) d.setDelay (getLatencySamples());

    lut.setSize();
    target.assign (tableSize, 0.0);
    for (int s = 0; s < 3; ++s)
        stageBase (s); // calcula las curvas base ahora, no en el hilo de audio

    modeSwitch.prepare (sampleRate, switchFadeSeconds, switchHoldSamples);

    if (! synchronousTables && ! builder->isRunning())
        builder->start();

    reset();
}

void TufiEngine::reset()
{
    drive.setCurrentAndTargetValue (1.0f);
    trim.setCurrentAndTargetValue (1.0f);
    bypassMix.setCurrentAndTargetValue (0.0f);

    for (auto& d : rawDelay) d.reset();
    for (auto& d : neutralDelay) d.reset();

    applyStructure (neutralKey);
    modeSwitch.reset (neutralKey);
    meter.reset();
    asleep = false;
    needsInit = true;
}

/** Fija el factor de sobremuestreo y reinicia solo el camino de la curva (el de la señal original,
    y el de ganancia simple, no se tocan: así no se abre un hueco en ellos). No asigna memoria. */
void TufiEngine::applyStructure (int key)
{
    factor = factorForKey (key);
    const int pad = getLatencySamples() - ferb::Oversampler::latencyFor (factor);

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        ups[idx (ch)].setFactor (factor);
        downs[idx (ch)].setFactor (factor);
        adaas[idx (ch)].reset();
        dcBlockers[idx (ch)].reset();
        wetDelay[idx (ch)].reset();
        wetDelay[idx (ch)].setDelay (pad);
    }
}

void TufiEngine::publishTable (const ShapeResult& r) noexcept
{
    target = r.y; // misma longitud: no asigna
    slope.store (r.slope, std::memory_order_relaxed);
    ceilingDb.store (r.ceilingDb, std::memory_order_relaxed);
    asymmetry.store (r.asymmetry, std::memory_order_relaxed);
}

/** Pide la curva que corresponde a los controles y acerca la curva de trabajo a la tabla lista. */
void TufiEngine::updateShape (const Settings& s) noexcept
{
    const auto params = ShapeParams::fromControls (s.stage, s.lowAmount, s.peakAmount, s.peakHardness,
                                                   s.evenTop, s.evenBottom);

    if (needsInit)
    {
        // Primer bloque: la tabla se calcula aquí mismo, sin pasar por la cola (que solo tiene un consumidor).
        ShapeTableBuilder::build (params, initial);
        publishTable (initial);
        lut.y = target;
        lut.rebuildIntegral();
        ++lutVersion;
        converged = true;
        lastRequested = params;
        return;
    }

    if (! (params == lastRequested))
    {
        builder->request (params);
        lastRequested = params;
    }

    if (synchronousTables)
        builder->pump(); // sin hilo secundario: se atiende en este mismo hilo

    if (builder->takeLatest())
    {
        publishTable (builder->latest());
        converged = false;
    }

    if (! converged)
    {
        double worst = 0.0;
        for (size_t i = 0; i < lut.y.size(); ++i)
        {
            const double d = target[i] - lut.y[i];
            lut.y[i] += shapeSmoothAlpha * d;
            worst = std::max (worst, std::abs (d));
        }

        if (worst < 1.0e-7)
        {
            lut.y = target;
            converged = true;
        }
        lut.rebuildIntegral();
        ++lutVersion;
    }
}

void TufiEngine::process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept
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

void TufiEngine::processChunk (juce::AudioBuffer<float>& chunk, const Settings& s) noexcept
{
    const int n = chunk.getNumSamples();
    const int channels = chunk.getNumChannels();

    drive.setTargetValue (dbToGain (s.driveDb));
    trim.setTargetValue (dbToGain (s.trimDb));
    bypassMix.setTargetValue (s.bypass ? 1.0f : 0.0f);

    // 1. Señal original retrasada lo mismo que el camino procesado (para el bypass).
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto* in = chunk.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
            rawOut[idx (ch)][idx (i)] = rawDelay[idx (ch)].process (in[i]);
    }

    // Bypass total: sale la señal original retrasada y el DSP duerme.
    if (! bypassMix.isSmoothing() && bypassMix.getTargetValue() >= 1.0f)
    {
        const float g = drive.getTargetValue() * trim.getTargetValue();
        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = chunk.getWritePointer (ch);
            for (int i = 0; i < n; ++i)
            {
                neutralDelay[idx (ch)].process (data[i] * g); // se mantiene alimentado para despertar sin hueco
                data[i] = rawOut[idx (ch)][idx (i)];
            }
        }
        drive.skip (n);
        trim.skip (n);
        asleep = true;
        meter.publishMax (chunk.getMagnitude (0, n));
        return;
    }

    const auto params = ShapeParams::fromControls (s.stage, s.lowAmount, s.peakAmount, s.peakHardness,
                                                   s.evenTop, s.evenBottom);
    const int wantedKey = params.isNeutral() ? neutralKey : 1 + juce::jlimit (0, 3, s.oversample);

    if (asleep || needsInit)
    {
        // Primer bloque o despertar del bypass: la cadena arranca limpia, ya en la estructura pedida.
        applyStructure (wantedKey);
        modeSwitch.reset (wantedKey);
        asleep = false;
    }

    updateShape (s);
    needsInit = false;

    const bool blend = bypassMix.isSmoothing() || bypassMix.getTargetValue() > 0.0f;
    for (int i = 0; i < n; ++i)
    {
        driveRamp[idx (i)] = drive.getNextValue();
        trimRamp[idx (i)] = trim.getNextValue();
        bypassRamp[idx (i)] = blend ? bypassMix.getNextValue() : 0.0f;
    }

    // 2. Cambio de sobremuestreo o paso por cero de la forma: fundido hacia la ruta simple, cambio, y vuelta.
    const auto step = modeSwitch.update (wantedKey, n);
    if (step.switched)
        applyStructure (step.mode);
    const bool fullPath = step.mode != neutralKey;

    // 3. Ganancia de entrada y ruta de ganancia simple (siempre retrasada la latencia completa).
    for (int ch = 0; ch < channels; ++ch)
    {
        const auto* in = chunk.getReadPointer (ch);
        for (int i = 0; i < n; ++i)
        {
            const float v = in[i] * driveRamp[idx (i)];
            x[idx (ch)][idx (i)] = v;
            neutral[idx (ch)][idx (i)] = neutralDelay[idx (ch)].process (v * trimRamp[idx (i)]);
        }
    }

    // 4. Ruta de la curva: subir, curva con ADAA, bajar, filtro de DC, retraso restante y salida.
    if (fullPath)
    {
        for (int ch = 0; ch < channels; ++ch)
        {
            const float* in = x[idx (ch)].data();
            auto& up = ups[idx (ch)];
            auto& adaa = adaas[idx (ch)];

            up.process (in, os.data(), n);
            for (int j = 0; j < n * factor; ++j)
                os[idx (j)] = adaa.process (lut, static_cast<double> (os[idx (j)]), lutVersion);
            downs[idx (ch)].process (os.data(), wet[idx (ch)].data(), n);

            for (int i = 0; i < n; ++i)
            {
                const float y = dcBlockers[idx (ch)].process (wet[idx (ch)][idx (i)]) * trimRamp[idx (i)];
                wet[idx (ch)][idx (i)] = wetDelay[idx (ch)].process (y);
            }
        }
    }

    // 5. Mezcla de rutas, fundido con el bypass y medidor.
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* data = chunk.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
        {
            float y = neutral[idx (ch)][idx (i)];
            if (fullPath)
            {
                const float t = static_cast<float> (i + 1) / static_cast<float> (n);
                const float g = step.gainStart + (step.gainEnd - step.gainStart) * t;
                y += g * (wet[idx (ch)][idx (i)] - y);
            }
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
} // namespace tufi
