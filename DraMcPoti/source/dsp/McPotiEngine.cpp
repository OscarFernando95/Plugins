#include "McPotiEngine.h"

#include <cmath>

namespace mcpoti
{
namespace
{
constexpr float gamma = 0.5f; // normalización inicial de lo añadido (por afinar)
constexpr float refVocalDb = -24.0f;
constexpr float refMasterDb = -18.0f;
constexpr int seda = 4 + 2; // era Moderno (4) + voicing 3 (índice 2)

float smoothCoeff (double fs, double ms) { return 1.0f - std::exp (-1.0f / float (0.001 * ms * fs)); }

double lnCosh (double x) noexcept
{
    x = std::abs (x);
    return x + std::log1p (std::exp (-2.0 * x)) - 0.693147180559945309;
}

/** No linealidades de la sección 4 de la ficha (Cinta aún sin pre-énfasis; los DC se quitan con el pasa-altos posterior). */
double shape (int voice, double u) noexcept
{
    switch (voice)
    {
        case 0: return std::tanh (1.5 * u) + 0.25 * std::tanh (u) * std::tanh (u); // Clásico (u² acotado con tanh²)
        case 1: return std::tanh (u);                                           // Cinta (simplificada)
        case 2: return std::tanh (1.2 * (u + 0.2)) - std::tanh (1.2 * 0.2);     // Válvula
        default: return u / std::sqrt (1.0 + u * u);                            // Transistor
    }
}

/** Antiderivada de `shape`: todas tienen forma cerrada, así que las cuatro llevan ADAA. */
double antiderivative (int voice, double u) noexcept
{
    switch (voice)
    {
        case 0: return lnCosh (1.5 * u) / 1.5 + 0.25 * (u - std::tanh (u)); // ∫tanh² = u − tanh u
        case 1: return lnCosh (u);
        case 2: return lnCosh (1.2 * (u + 0.2)) / 1.2 - std::tanh (1.2 * 0.2) * u;
        default: return std::sqrt (1.0 + u * u);
    }
}

/** ADAA de primer orden: (F(v) − F(v₋₁)) / (v − v₋₁), con la función en el punto medio si dv es casi nulo. */
double adaaProcess (int voice, double v, double& vPrev, double& FPrev, bool& primed) noexcept
{
    const double Fv = antiderivative (voice, v);
    if (! primed)
    {
        vPrev = v;
        FPrev = Fv;
        primed = true;
    }
    const double dv = v - vPrev;
    const double y = std::abs (dv) > 1.0e-5 ? (Fv - FPrev) / dv : shape (voice, 0.5 * (v + vPrev));
    vPrev = v;
    FPrev = Fv;
    return y;
}

float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }
} // namespace

void McPotiEngine::prepare (double sampleRate, int maxBlock)
{
    fs = sampleRate;
    maxBlockSize = std::max (maxBlock, 1);
    const float c20 = smoothCoeff (fs, 20.0);
    for (auto& d : driveSmooth) d.coeff = c20;
    for (auto& l : levelSmooth) l.coeff = c20;
    inSmooth.coeff = outSmooth.coeff = mixSmooth.coeff = c20;
    gate.coeff = smoothCoeff (fs, 5.0);
    freqCoeff = 1.0f - std::exp (-float (controlInterval) / float (0.030 * fs)); // 30 ms por paso de control
    envAttack = smoothCoeff (fs, 5.0);
    envRelease = smoothCoeff (fs, 120.0);

    // Todo se reserva aquí, para el máximo (8×).
    const auto size = static_cast<size_t> (maxBlockSize);
    hiBuffer.assign (size * maxFactor, 0.0f);
    for (auto& v : driveBuf) v.assign (size, 0.0f);
    for (auto& v : levelBuf) v.assign (size, 0.0f);
    for (auto& c : channels)
    {
        c.dryDelay.prepare (latencySamples());
        for (size_t b = 0; b < nb; ++b)
        {
            c.up[b].prepare (maxBlockSize);
            c.down[b].prepare (maxBlockSize);
            c.bandDelay[b].prepare (latencySamples());
            c.addDelay[b].prepare (latencySamples());
            c.bandSig[b].assign (size, 0.0f);
            c.nlIn[b].assign (size, 0.0f);
            c.nlOut[b].assign (size, 0.0f);
        }
    }
    factor = 0;
    reset();
}

void McPotiEngine::reset() noexcept
{
    for (auto& c : channels)
    {
        c.split.reset();
        c.dryDelay.reset();
        for (size_t b = 0; b < nb; ++b)
        {
            c.hpIn[b].reset();
            c.hpOut[b].reset();
            c.shelf[b].reset();
            c.up[b].reset();
            c.down[b].reset();
            c.adaa[b] = {};
            c.bandDelay[b].reset();
            c.addDelay[b].reset();
        }
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
    gate.snap (1.0f);
    primed = false;
    snapControl = true;
}

/** Lo añadido por el generador pasa por subir y bajar (latencia propia del factor); Seda no sobremuestrea, así que
    se retrasa la latencia total. En los dos casos lo añadido llega alineado con el seco. */
void McPotiEngine::applyAddDelays() noexcept
{
    const int delay = activeVoice == seda ? latencySamples() : latencySamples() - ferb::OsDesign::latencyFor (factor, false);
    for (auto& c : channels)
        for (auto& d : c.addDelay)
        {
            d.reset();
            d.setDelay (delay);
        }
}

/** Shelf de segundo orden de +15 dB en el borde inferior de cada banda (Cuerpo: 100 Hz); Drive mezcla el realce. */
void McPotiEngine::designShelves() noexcept
{
    const double corners[nb] = { 100.0, double (lastXo[0]), double (lastXo[1]), double (lastXo[2]) };
    for (size_t b = 0; b < nb; ++b)
    {
        const auto shelf = ferb::Biquad::highShelf (fs, std::min (corners[b], 0.45 * fs), 15.0);
        for (auto& c : channels)
            c.shelf[b].setCoefficients (shelf);
    }
    shelfXo = lastXo;
}

/** Cambia el factor de sobremuestreo (el motor ya está en silencio para lo añadido) y alinea los retardos:
    lo añadido pasa por subir y bajar (latencia propia del factor) y el resto de la latencia total se rellena. */
void McPotiEngine::configureFactor (int newFactor) noexcept
{
    factor = newFactor;
    for (auto& c : channels)
        for (size_t b = 0; b < nb; ++b)
        {
            c.up[b].setConfig (factor, false);
            c.down[b].setConfig (factor, false);
            c.adaa[b] = {};
            c.hpOut[b].reset();
        }
    applyAddDelays();
    lastHp.fill (0.0f); // fuerza a rediseñar los pasa-altos de salida a la nueva frecuencia de muestreo
    for (auto& c : channels)
    {
        c.dryDelay.setDelay (latencySamples());
        for (auto& d : c.bandDelay)
            d.setDelay (latencySamples());
    }
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
        if (activeVoice == seda)
            designShelves();
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
            const auto hpLow = ferb::Biquad::highPass (fs, limited);
            const auto hpHigh = ferb::Biquad::highPass (fs * std::max (factor, 1), limited);
            for (auto& c : channels)
            {
                c.hpIn[b].setCoefficients (hpLow);
                c.hpOut[b].setCoefficients (hpHigh);
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
    const int total = buffer.getNumSamples();
    if (buffer.getNumChannels() == 0 || total == 0)
        return;
    for (int start = 0; start < total; start += maxBlockSize)
        processChunk (buffer, start, std::min (maxBlockSize, total - start), s);
}

void McPotiEngine::processChunk (juce::AudioBuffer<float>& buffer, int start, int n, const Settings& s) noexcept
{
    const int numCh = std::min (buffer.getNumChannels(), maxChannels);
    const int targetFactor = 1 << juce::jlimit (0, 3, s.osIndex);
    const int voiceKey = juce::jlimit (0, 1, s.era) * 4 + juce::jlimit (0, 3, s.voice);

    if (! primed)
    {
        primed = true;
        inSmooth.snap (dbToGain (s.inputDb));
        outSmooth.snap (dbToGain (s.outputDb));
        mixSmooth.snap (s.mixPercent * 0.01f);
        for (size_t b = 0; b < nb; ++b)
        {
            driveSmooth[b].snap (s.bands[b].on ? s.bands[b].drive * 0.01f : 0.0f);
            levelSmooth[b].snap (s.bands[b].level * 0.01f);
        }
        activeVoice = voiceKey;
        configureFactor (targetFactor);
        updateControl (s);
        if (activeVoice == seda)
            designShelves();
    }

    // Cambiar el sobremuestreo o el carácter reinicia filtros y ADAA (cada carácter tiene su antiderivada):
    // lo añadido se funde a silencio, se cambia y vuelve (sin clics ni picos).
    float gateTarget = 1.0f;
    if (targetFactor != factor || voiceKey != activeVoice)
    {
        gateTarget = 0.0f;
        if (gate.value < 1.0e-3f)
        {
            if (targetFactor != factor)
            {
                configureFactor (targetFactor);
                updateControl (s);
            }
            if (voiceKey != activeVoice)
            {
                activeVoice = voiceKey;
                for (auto& c : channels)
                    for (size_t b = 0; b < nb; ++b)
                    {
                        c.adaa[b] = {};
                        c.hpOut[b].reset();
                        c.shelf[b].reset();
                    }
                applyAddDelays();
                if (activeVoice == seda)
                    designShelves();
            }
        }
    }

    bool anySolo = false;
    for (const auto& b : s.bands)
        anySolo = anySolo || b.solo;
    const bool shelfMode = activeVoice == seda;
    const int voice = activeVoice & 3;
    const bool implemented = activeVoice < 4 || shelfMode; // Dinámico, Adaptativo y Aire llegan después

    std::array<float*, maxChannels> data {};
    for (int ch = 0; ch < numCh; ++ch)
        data[static_cast<size_t> (ch)] = buffer.getWritePointer (ch) + start;

    // Pasada 1 (frecuencia base): cruce, envolventes, control y entrada del generador.
    for (int i = 0; i < n; ++i)
    {
        if ((i % controlInterval) == 0)
            updateControl (s);

        const float inGain = inSmooth.step (dbToGain (s.inputDb));
        std::array<float, nb> drive {};
        for (size_t b = 0; b < nb; ++b)
        {
            drive[b] = driveSmooth[b].step (s.bands[b].on ? s.bands[b].drive * 0.01f : 0.0f) * dynFactor[b];
            driveBuf[b][static_cast<size_t> (i)] = std::min (drive[b], 2.0f);
            levelBuf[b][static_cast<size_t> (i)] = levelSmooth[b].step (s.bands[b].level * 0.01f);
        }

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = channels[static_cast<size_t> (ch)];
            const float x = data[static_cast<size_t> (ch)][i] * inGain;
            data[static_cast<size_t> (ch)][i] = x; // la pasada 2 parte de la entrada ya con ganancia
            std::array<float, nb> bands {};
            c.split.process (x, bands);
            for (size_t b = 0; b < nb; ++b)
            {
                c.bandSig[b][static_cast<size_t> (i)] = bands[b];
                const float p = bands[b] * bands[b];
                c.env[b] += (p > c.env[b] ? envAttack : envRelease) * (p - c.env[b]);
                const float a = driveBuf[b][static_cast<size_t> (i)];
                if (shelfMode)
                    c.nlOut[b][static_cast<size_t> (i)] = c.shelf[b].process (bands[b]) - bands[b]; // solo el realce
                else
                    c.nlIn[b][static_cast<size_t> (i)] = c.hpIn[b].process (bands[b]) * (1.0f + 4.0f * a);
            }
        }
    }

    // Generador: subir, no linealidad con ADAA, pasa-altos y bajar (por banda y canal). Seda no lo usa.
    for (int ch = 0; ch < (shelfMode ? 0 : numCh); ++ch)
    {
        auto& c = channels[static_cast<size_t> (ch)];
        for (size_t b = 0; b < nb; ++b)
        {
            const int hiN = n * factor;
            c.up[b].process (c.nlIn[b].data(), hiBuffer.data(), n);
            auto& st = c.adaa[b];
            for (int j = 0; j < hiN; ++j)
            {
                const double y = adaaProcess (voice, double (hiBuffer[static_cast<size_t> (j)]), st.vPrev, st.FPrev, st.primed);
                hiBuffer[static_cast<size_t> (j)] = c.hpOut[b].process (static_cast<float> (y));
            }
            c.down[b].process (hiBuffer.data(), c.nlOut[b].data(), n);
        }
    }

    // Pasada 2 (frecuencia base): alinear retardos, aplicar nivel y mezclar.
    for (int i = 0; i < n; ++i)
    {
        const float outGain = outSmooth.step (dbToGain (s.outputDb));
        const float mix = mixSmooth.step (s.mixPercent * 0.01f);
        const float g = gate.step (gateTarget);
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = channels[static_cast<size_t> (ch)];
            const float dry = c.dryDelay.process (data[static_cast<size_t> (ch)][i]);
            float addedSum = 0.0f, soloSum = 0.0f;
            for (size_t b = 0; b < nb; ++b)
            {
                const float delayedBand = c.bandDelay[b].process (c.bandSig[b][static_cast<size_t> (i)]);
                float h = c.addDelay[b].process (c.nlOut[b][static_cast<size_t> (i)]);
                h = implemented ? h * driveBuf[b][static_cast<size_t> (i)] * (shelfMode ? 1.0f : gamma) * levelBuf[b][static_cast<size_t> (i)] * g : 0.0f;
                addedPower[b] += (h * h - addedPower[b]) * 0.0005f;
                addedSum += h;
                if (s.bands[b].solo)
                    soloSum += delayedBand + mix * h;
            }
            const float y = s.delta ? mix * addedSum : (anySolo ? soloSum : dry + mix * addedSum);
            data[static_cast<size_t> (ch)][i] = y * outGain;
        }
    }

    for (size_t b = 0; b < nb; ++b)
        added[b].store (std::sqrt (addedPower[b]), std::memory_order_relaxed);
}
} // namespace mcpoti
