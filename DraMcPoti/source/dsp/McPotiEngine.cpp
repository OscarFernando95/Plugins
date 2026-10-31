#include "McPotiEngine.h"

#include <cmath>

namespace mcpoti
{
namespace
{
constexpr float gamma = 0.5f; // normalización inicial de lo añadido (por afinar)
constexpr float refVocalDb = -24.0f;
constexpr float refMasterDb = -18.0f;
// Claves era·4 + voicing de la era Moderno.
constexpr int dinamico = 4, adaptativo = 5, seda = 6, aire = 7;
constexpr bool linearVoice (int key) noexcept { return key == dinamico || key == seda || key == aire; } // sin generador no lineal
constexpr float adaptTargetDb = -9.0f; // razón Presencia/Medios a la que tiende Adaptativo (valor inicial)

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
    tonalCoeff = smoothCoeff (fs, 2000.0);
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
    constexpr double apFreq[4] = { 600.0, 1800.0, 5000.0, 11000.0 };
    for (auto& c : channels)
        for (size_t k = 0; k < 4; ++k)
            c.phaseAp[k].setFrequency (fs, std::min (apFreq[k], 0.45 * fs));
    sideSplit.setFrequency (fs, 150.0);
    guardFilter[0] = ferb::Biquad::highPass (fs, 5000.0);
    guardFilter[1] = ferb::Biquad::lowPass (fs, std::min (10000.0, 0.45 * fs));
    guardFilter[2] = ferb::Biquad::highPass (fs, 300.0);
    guardFilter[3] = ferb::Biquad::lowPass (fs, 3000.0);
    guardEnvCoeff = smoothCoeff (fs, 2.0);
    guardAttack = smoothCoeff (fs, 1.0);   // ataque de 1 ms; el lookahead lo da el retardo de lo añadido (≈ 1 ms)
    guardRelease = smoothCoeff (fs, 40.0);
    powerCoeff = smoothCoeff (fs, 3000.0);
    guardBuf.assign (size, 1.0f);
    vuIn.prepare (fs);
    vuOut.prepare (fs);
    for (auto& c : channels)
        for (size_t k = 0; k < 2; ++k)
        {
            c.kDry[k] = k == 0 ? ferb::KWeighting::shelf (fs) : ferb::KWeighting::highPass (fs);
            c.kOut[k] = c.kDry[k];
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
            c.adaaOdd[b] = {};
            c.bandDelay[b].reset();
            c.addDelay[b].reset();
        }
        for (auto& ap : c.phaseAp)
            ap.reset();
        c.soften.reset();
        for (auto& f : c.kDry) f.reset();
        for (auto& f : c.kOut) f.reset();
        c.env.fill (0.0f);
        c.gcTarget.fill (1.0f);
        c.gcSmooth.fill (1.0f);
    }
    sideSplit.reset();
    for (auto& f : guardFilter) f.reset();
    guardHi = guardMid = guardDb = 0.0f;
    sibilant = false;
    powerDry = powerOut = 0.0f;
    matchLin = 1.0f;
    matchDb.store (0.0f);
    lastSoften = 0.0f;
    tonal.fill (0.0f);
    adaptGain = 1.0f;
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

/** Lo añadido por el generador pasa por subir y bajar (latencia propia del factor); Los caracteres lineales (Dinámico, Seda y Aire) no sobremuestrean, así que
    se retrasa la latencia total. En los dos casos lo añadido llega alineado con el seco. */
void McPotiEngine::applyAddDelays() noexcept
{
    const int delay = linearVoice (activeVoice) ? latencySamples() : latencySamples() - ferb::OsDesign::latencyFor (factor, false);
    for (auto& c : channels)
        for (auto& d : c.addDelay)
        {
            d.reset();
            d.setDelay (delay);
        }
}

/** Seda: shelf de +15 dB en el borde inferior de cada banda (Cuerpo: 100 Hz). Aire: shelf de +10 dB en la esquina de aire
    (acotada a 0,45·fs: por encima de Nyquist no hay señal que realzar a esta frecuencia de muestreo). Drive mezcla el realce. */
void McPotiEngine::designShelves() noexcept
{
    const bool air = activeVoice == aire;
    const double edges[nb] = { 100.0, double (lastXo[0]), double (lastXo[1]), double (lastXo[2]) };
    for (size_t b = 0; b < nb; ++b)
    {
        const double corner = air ? std::max (double (smoothAir), edges[b]) : edges[b];
        const auto shelf = ferb::Biquad::highShelf (fs, std::min (corner, 0.45 * fs), air ? 10.0 : 15.0);
        for (auto& c : channels)
            c.shelf[b].setCoefficients (shelf);
    }
    shelfXo = lastXo;
    shelfAir = smoothAir;
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
            c.adaaOdd[b] = {};
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
        smoothAir = s.airCornerHz;
        smoothSoften = s.softenHz;
    }
    else
    {
        for (size_t i = 0; i < 3; ++i)
            smoothXo[i] += freqCoeff * (target[i] - smoothXo[i]);
        smoothTune += freqCoeff * (s.tuneHz - smoothTune);
        smoothAir += freqCoeff * (s.airCornerHz - smoothAir);
        smoothSoften += freqCoeff * (s.softenHz - smoothSoften);
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
        if (activeVoice == seda || activeVoice == aire)
            designShelves();
    }
    else if (activeVoice == aire && std::abs (smoothAir - shelfAir) > 0.002f * smoothAir)
        designShelves();

    // Suavizar: paso bajo de lo añadido; a 20 kHz o más queda apagado.
    if (std::abs (smoothSoften - lastSoften) > 0.002f * smoothSoften)
    {
        lastSoften = smoothSoften;
        const auto lp = ferb::Biquad::lowPass (fs, std::min (double (smoothSoften), 0.45 * fs));
        for (auto& c : channels)
            c.soften.setCoefficients (lp);
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

    // Adaptativo: la razón Presencia/Medios (promedio de 2 s) mueve el drive hasta ±6 dB hacia el objetivo tonal.
    if (activeVoice == adaptativo && tonal[0] > 1.0e-9f)
    {
        const float ratioDb = 10.0f * std::log10 ((tonal[1] + 1.0e-12f) / tonal[0]);
        adaptGain = std::pow (10.0f, juce::jlimit (-6.0f, 6.0f, adaptTargetDb - ratioDb) / 20.0f);
    }
    else
        adaptGain = 1.0f;

    // Dinámica por banda: a = drive/100 · 2^(−dyn·e/50), e = clamp((L − Lref)/12, −1, 1), con el nivel enlazado.
    // En Dinámico la Dinámica fija la relación del compander, no el drive.
    const float ref = s.master ? refMasterDb : refVocalDb;
    const float link = juce::jlimit (0.0f, 1.0f, s.linkPercent * 0.01f);
    for (size_t b = 0; b < numBands; ++b)
    {
        float mean = 0.0f;
        for (auto& c : channels)
            mean += c.env[b];
        mean *= 1.0f / float (maxChannels);
        const float power = link * mean + (1.0f - link) * channels[0].env[b];
        const float levelDb = 10.0f * std::log10 (power + 1.0e-12f);
        const float e = juce::jlimit (-1.0f, 1.0f, (levelDb - ref) / 12.0f);
        dynFactor[b] = activeVoice == dinamico ? 1.0f : std::exp2 (-s.bands[b].dyn * e / 50.0f) * adaptGain;

        // Compander de Dinámico (−20 dB, 2:1 a 8:1) y aplanado del shelf de Aire (−24 dB, 4:1), con el nivel propio de cada canal.
        const float thresholdDb = activeVoice == dinamico ? -20.0f : -24.0f;
        const float ratio = activeVoice == dinamico ? 2.0f + 6.0f * (s.bands[b].dyn + 50.0f) * 0.01f : 4.0f;
        for (auto& c : channels)
        {
            const float over = 10.0f * std::log10 (c.env[b] + 1.0e-12f) - thresholdDb;
            c.gcTarget[b] = over > 0.0f ? std::pow (10.0f, -over * (1.0f - 1.0f / ratio) / 20.0f) : 1.0f;
        }
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
        if (activeVoice == seda || activeVoice == aire)
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
                        c.adaaOdd[b] = {};
                        c.hpOut[b].reset();
                        c.shelf[b].reset();
                        c.gcSmooth[b] = 1.0f;
                    }
                applyAddDelays();
                if (activeVoice == seda || activeVoice == aire)
                    designShelves();
            }
        }
    }

    bool anySolo = false;
    for (const auto& b : s.bands)
        anySolo = anySolo || b.solo;
    const bool shelfMode = linearVoice (activeVoice); // Dinámico, Seda y Aire: sin generador no lineal
    const int voice = activeVoice == adaptativo ? 2 : activeVoice & 3; // Adaptativo = Válvula ligera
    const bool implemented = true;

    std::array<float*, maxChannels> data {};
    for (int ch = 0; ch < numCh; ++ch)
        data[static_cast<size_t> (ch)] = buffer.getWritePointer (ch) + start;

    std::array<const float*, maxChannels> vuPtr {};
    for (int ch = 0; ch < numCh; ++ch)
        vuPtr[static_cast<size_t> (ch)] = data[static_cast<size_t> (ch)];
    vuIn.process (vuPtr.data(), numCh, n); // la entrada tal como llega, antes de procesar

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

        float detector = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = channels[static_cast<size_t> (ch)];
            const float x = data[static_cast<size_t> (ch)][i] * inGain;
            detector += x / float (numCh);
            data[static_cast<size_t> (ch)][i] = x; // la pasada 2 parte de la entrada ya con ganancia
            std::array<float, nb> bands {};
            c.split.process (x, bands);
            for (size_t b = 0; b < nb; ++b)
            {
                c.bandSig[b][static_cast<size_t> (i)] = bands[b];
                const float p = bands[b] * bands[b];
                c.env[b] += (p > c.env[b] ? envAttack : envRelease) * (p - c.env[b]);
                if (b == 1 || b == 2)
                    tonal[b - 1] += tonalCoeff * (p - tonal[b - 1]);
                const float a = driveBuf[b][static_cast<size_t> (i)];
                if (shelfMode)
                {
                    float& gc = c.gcSmooth[b];
                    gc += 0.05f * (c.gcTarget[b] - gc);
                    float added = 0.0f;
                    if (activeVoice == dinamico)
                        added = bands[b] * gc; // copia comprimida, sumada al original
                    else if (activeVoice == aire)
                        added = (c.shelf[b].process (bands[b]) - bands[b]) * gc; // realce que se aplana al comprimir
                    else
                        added = c.shelf[b].process (bands[b]) - bands[b]; // Seda: solo el realce
                    c.nlOut[b][static_cast<size_t> (i)] = added;
                }
                else
                    c.nlIn[b][static_cast<size_t> (i)] = c.hpIn[b].process (bands[b]) * (1.0f + 4.0f * a);
            }
        }

        // Guarda de sibilancia: energía de 5–10 kHz frente a 300 Hz–3 kHz, con histéresis (por afinar con voz real).
        const float hiBand = guardFilter[1].process (guardFilter[0].process (detector));
        const float midBand = guardFilter[3].process (guardFilter[2].process (detector));
        guardHi += guardEnvCoeff * (hiBand * hiBand - guardHi);
        guardMid += guardEnvCoeff * (midBand * midBand - guardMid);
        const float ratio = guardHi / (guardMid + 1.0e-9f);
        sibilant = sibilant ? ratio > 0.25f : (ratio > 0.5f && guardHi > 1.0e-6f);
        const float guardTarget = sibilant ? -12.0f * juce::jlimit (0.0f, 1.0f, s.guardPercent * 0.01f) : 0.0f;
        guardDb += (guardTarget < guardDb ? guardAttack : guardRelease) * (guardTarget - guardDb);
        guardBuf[static_cast<size_t> (i)] = s.master ? 1.0f : std::pow (10.0f, guardDb / 20.0f);
    }

    const float color = juce::jlimit (-1.0f, 1.0f, s.colorPercent * 0.01f);
    const double bias = color < 0.0f ? -0.35 * double (color) : 0.0; // sesgo que suma pares
    const float oddMix = color > 0.0f ? color : 0.0f;

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
                // Color: hacia pares (−) se desplaza el punto de trabajo (asimetría); hacia impares (+) se mezcla con la parte impar f(u) − f(−u).
                const double v = double (hiBuffer[static_cast<size_t> (j)]);
                double y = adaaProcess (voice, v + bias, st.vPrev, st.FPrev, st.primed);
                if (oddMix > 0.0f)
                {
                    auto& so = c.adaaOdd[b];
                    const double neg = adaaProcess (voice, -v, so.vPrev, so.FPrev, so.primed);
                    y += double (oddMix) * (0.5 * (y - neg) - y);
                }
                hiBuffer[static_cast<size_t> (j)] = c.hpOut[b].process (static_cast<float> (y));
            }
            c.down[b].process (hiBuffer.data(), c.nlOut[b].data(), n);
        }
    }

    // Pasada 2 (frecuencia base): alinear retardos, nivel de banda, Suavizar, Fase, estéreo (Master) y mezcla.
    const float phasePos = juce::jlimit (0.0f, 1.0f, s.phasePercent * 0.01f) * 4.0f;
    const int phaseBase = std::min (int (phasePos), 3);
    const float phaseFrac = phasePos - float (phaseBase);
    const bool softenOn = s.softenHz < 19999.0f;
    const bool stereoOn = s.master && numCh == 2;
    const float width = s.widthPercent * 0.01f;

    for (int i = 0; i < n; ++i)
    {
        const float outGain = outSmooth.step (dbToGain (s.outputDb));
        const float mix = mixSmooth.step (s.mixPercent * 0.01f);
        const float g = gate.step (gateTarget);
        std::array<float, maxChannels> dry {}, addedSum {}, soloSum {};
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto& c = channels[static_cast<size_t> (ch)];
            const auto k = static_cast<size_t> (ch);
            dry[k] = c.dryDelay.process (data[k][i]);
            for (size_t b = 0; b < nb; ++b)
            {
                const float delayedBand = c.bandDelay[b].process (c.bandSig[b][static_cast<size_t> (i)]);
                float h = c.addDelay[b].process (c.nlOut[b][static_cast<size_t> (i)]);
                h = implemented ? h * driveBuf[b][static_cast<size_t> (i)] * (shelfMode ? 1.0f : gamma) * levelBuf[b][static_cast<size_t> (i)] * g : 0.0f;
                if (b >= 2)
                    h *= guardBuf[static_cast<size_t> (i)];
                addedPower[b] += (h * h - addedPower[b]) * 0.0005f;
                addedSum[k] += h;
                if (s.bands[b].solo)
                    soloSum[k] += delayedBand + mix * h;
            }

            float x = addedSum[k];
            if (softenOn)
                x = c.soften.process (x);
            // Fase: de cero a cuatro secciones pasa-todo, con transición continua entre la k-ésima y la siguiente.
            std::array<float, 5> stage { x, 0, 0, 0, 0 };
            for (size_t j = 0; j < 4; ++j)
            {
                float lo, hi;
                c.phaseAp[j].process (stage[j], lo, hi);
                stage[j + 1] = lo + hi;
            }
            addedSum[k] = stage[static_cast<size_t> (phaseBase)] + phaseFrac * (stage[static_cast<size_t> (phaseBase) + 1] - stage[static_cast<size_t> (phaseBase)]);
        }

        if (stereoOn)
        {
            // Ancho escala el lateral de lo añadido; Mono seguro quita del lateral lo que queda bajo 150 Hz.
            const float mid = 0.5f * (addedSum[0] + addedSum[1]);
            float side = 0.5f * (addedSum[0] - addedSum[1]);
            float sideLow, sideHigh;
            sideSplit.process (side, sideLow, sideHigh);
            side = (s.monoSafe ? sideHigh : side) * width;
            addedSum[0] = mid + side;
            addedSum[1] = mid - side;
        }

        // Igualar: la sonoridad K de 3 s de la salida se lleva a la de la entrada (±12 dB); apagado en Delta y Solo.
        const bool matching = s.match && ! s.delta && ! anySolo;
        std::array<float, maxChannels> out {};
        float kDry = 0.0f, kOut = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
        {
            const auto k = static_cast<size_t> (ch);
            auto& c = channels[k];
            out[k] = s.delta ? mix * addedSum[k] : (anySolo ? soloSum[k] : dry[k] + mix * addedSum[k]);
            const float kd = c.kDry[1].process (c.kDry[0].process (dry[k]));
            const float ko = c.kOut[1].process (c.kOut[0].process (out[k]));
            kDry += kd * kd;
            kOut += ko * ko;
        }
        powerDry += powerCoeff * (kDry - powerDry);
        powerOut += powerCoeff * (kOut - powerOut);
        if ((i % controlInterval) == 0)
        {
            const float wantedDb = matching && powerDry > 1.0e-9f && powerOut > 1.0e-9f
                                       ? juce::jlimit (-12.0f, 12.0f, 10.0f * std::log10 (powerDry / powerOut))
                                       : 0.0f;
            matchDb.store (wantedDb, std::memory_order_relaxed);
        }
        matchLin += 0.002f * (dbToGain (matchDb.load (std::memory_order_relaxed)) - matchLin);
        for (int ch = 0; ch < numCh; ++ch)
            data[static_cast<size_t> (ch)][i] = out[static_cast<size_t> (ch)] * matchLin * outGain;
    }

    for (int ch = 0; ch < numCh; ++ch)
        vuPtr[static_cast<size_t> (ch)] = data[static_cast<size_t> (ch)];
    vuOut.process (vuPtr.data(), numCh, n);

    for (size_t b = 0; b < nb; ++b)
        added[b].store (std::sqrt (addedPower[b]), std::memory_order_relaxed);
}
} // namespace mcpoti
