#include "ChausEngine.h"

#include <algorithm>
#include <cmath>

#include "EqMath.h"

namespace chaus
{
namespace
{
constexpr float kneeDb = 3.0f;
constexpr double attackSeconds = 0.0005, shapeSeconds = 0.030, trackFastSeconds = 0.020, trackSlowSeconds = 0.200, gateSeconds = 0.002;
constexpr double bypassSeconds = 0.020, listenSeconds = 0.010, outputSeconds = 0.020;
constexpr double adoptSeconds = 0.150, lookFadeSeconds = 0.050, structureFadeSeconds = 0.005;
constexpr int coefficientInterval = 16;

double coeff (double seconds, double fs) { return 1.0 - std::exp (-1.0 / (seconds * fs)); }
float sane (float x) noexcept { return std::isfinite (x) ? x : 0.0f; }
float dbToLin (float db) noexcept { return std::exp (db * 0.11512925f); }

/** Reducción con rodilla suave: 0 por debajo de −rodilla/2, el exceso por encima de +rodilla/2 y una parábola entre ambos. */
float kneeReduction (float excess) noexcept
{
    if (excess <= -0.5f * kneeDb)
        return 0.0f;
    if (excess >= 0.5f * kneeDb)
        return excess;
    const float t = excess + 0.5f * kneeDb;
    return t * t / (2.0f * kneeDb);
}

} // namespace

// ───────────────────────────────── Retardo de lookahead ─────────────────────────────────

void ChausEngine::LookaheadDelay::prepare (int maxSamples, int fadeSamples)
{
    int size = 1;
    while (size < maxSamples + 2)
        size <<= 1;
    buffer.assign (static_cast<size_t> (size), 0.0f);
    mask = size - 1;
    fadeLen = std::max (1, fadeSamples);
    reset();
}

void ChausEngine::LookaheadDelay::reset() noexcept
{
    std::fill (buffer.begin(), buffer.end(), 0.0f);
    write = 0;
    delay = previous = 0;
    fadePos = fadeLen;
}

void ChausEngine::LookaheadDelay::setDelay (int samples) noexcept
{
    samples = std::clamp (samples, 0, mask);
    if (samples == delay)
        return;
    previous = delay;
    delay = samples;
    fadePos = 0;
}

void ChausEngine::LookaheadDelay::jumpTo (int samples) noexcept
{
    delay = previous = std::clamp (samples, 0, mask);
    fadePos = fadeLen;
}

float ChausEngine::LookaheadDelay::process (float x) noexcept
{
    buffer[static_cast<size_t> (write)] = x;
    float y;
    if (fadePos < fadeLen)
    {
        const float t = static_cast<float> (fadePos++) / static_cast<float> (fadeLen);
        y = read (previous) * (1.0f - t) + read (delay) * t;
    }
    else
        y = read (delay);
    write = (write + 1) & mask;
    return y;
}

// ───────────────────────────────── Motor ─────────────────────────────────

int ChausEngine::latencyFor (const Settings& s, double sampleRate) noexcept
{
    return s.lookaheadMs > 0.0f ? static_cast<int> (std::lround (std::min (s.lookaheadMs, maxLookaheadMs) * 0.001 * sampleRate)) : 0;
}

void ChausEngine::prepare (double sampleRate, int)
{
    fs = sampleRate;
    const int maxDelay = static_cast<int> (std::ceil (maxLookaheadMs * 0.001 * fs)) + 4, fade = static_cast<int> (lookFadeSeconds * fs);
    for (auto& lane : lanes)
    {
        lane.detector.prepare (fs);
        lane.reference.prepare (fs);
        lane.delay.prepare (maxDelay, fade);
        lane.bandDelay.prepare (maxDelay, fade);
        lane.gateDelay.prepare (maxDelay, fade);
    }
    for (auto& d : passDelay)
        d.prepare (maxDelay, fade);

    attackCoeff = static_cast<float> (coeff (attackSeconds, fs));
    shapeCoeff = static_cast<float> (coeff (shapeSeconds, fs));
    trackFastCoeff = static_cast<float> (coeff (trackFastSeconds, fs));
    trackSlowCoeff = static_cast<float> (coeff (trackSlowSeconds, fs));
    gateCoeff = static_cast<float> (coeff (gateSeconds, fs));
    bypassCoeff = coeff (bypassSeconds, fs);
    listenCoeff = coeff (listenSeconds, fs);
    outputCoeff = coeff (outputSeconds, fs);
    pointInterval = std::max (1, static_cast<int> (std::lround (Telemetry::secondsPerPoint * fs)));
    modeSwitch.prepare (fs, structureFadeSeconds, 256);
    reset();
}

void ChausEngine::resetLane (Lane& lane) noexcept
{
    lane.detector.reset();
    lane.bell.reset();
    lane.shelf.reset();
    lane.bellNow = lane.bellStep = lane.shelfNow = lane.shelfStep = {};
    lane.ringHead = lane.ringCount = lane.sinceMedian = 0;
    lane.median = 0.0f;
    lane.lastFrame = 0;
    lane.rawDb = lane.smoothDb = lane.linkedDb = lane.gate = 0.0f;
    lane.eventSamples = lane.lastEventSamples = lane.activeAge = 0;
    lane.shapeWeight = 0.0f;
    lane.logCentre = std::log (std::sqrt (4000.0f * 12000.0f));
    lane.q = 3.0f;
}

void ChausEngine::reset()
{
    for (auto& lane : lanes)
    {
        resetLane (lane);
        lane.reference.reset();
        lane.delay.reset();
        lane.bandDelay.reset();
        lane.gateDelay.reset();
    }
    for (auto& d : passDelay)
        d.reset();
    needsInit = true;
    requestedDelay = adoptedDelay = -1;
    stableSamples = 0;
    bypassMix = listenMix = 0.0;
    outputGain = 1.0;
    sinceCoefficients = 0;
    lastHp = lastLp = -1.0f;
    sinceTelemetry = 0;
    accLevel = -120.0f;
    accThreshold = accReduction = 0.0f;
    accCentre = 7000.0f;
    accDetected = false;
    telemetry.written.store (0, std::memory_order_relaxed);
}

float ChausEngine::rawReduction (Lane& lane, const Settings& s, const SibilanceDetector::Output& o) noexcept
{
    const float ref = lane.reference.referenceDb();
    float threshold = (s.voiceMode && s.adaptive) ? ref + (s.thresholdDb + 30.0f) : s.thresholdDb;

    // La sibilante es «dura» si su nivel supera a la referencia: cada clase desplaza el umbral con su propia sensibilidad.
    const float hard = s.voiceMode ? std::clamp ((o.levelDb - ref + 6.0f) / 12.0f, 0.0f, 1.0f) : 0.5f;
    threshold -= (1.0f - hard) * s.sensSoftDb + hard * s.sensHardDb;
    lane.lastThresholdDb = threshold;

    float reduction = std::min (kneeReduction (o.levelDb - threshold), s.rangeDb);
    if (s.voiceMode)
    {
        // Entrada progresiva: la edad del evento (tiempo seguido «activo») escala la reducción con un cuadrado, para que una
        // plosiva o un chasquido de 10 a 20 ms apenas reciba reducción y una «s» de 100 ms casi toda.
        lane.activeAge = o.active ? lane.activeAge + 1 : 0;
        if (! o.active)
            reduction = 0.0f;
        else if (s.eventRampMs > 0.0f)
        {
            const float ramp = std::min (1.0f, static_cast<float> (lane.activeAge) / static_cast<float> (s.eventRampMs * 0.001 * fs));
            reduction *= ramp * ramp;
        }
    }
    return std::max (reduction, 0.0f);
}

void ChausEngine::track (Lane& lane, const Settings& s) noexcept
{
    const auto& o = lane.tick;
    const float centre = std::sqrt (s.hpHz * s.lpHz);

    // Cada cuadro nuevo con sibilante aporta su centroide a la mediana de los últimos 2 s (unos 750 cuadros).
    if (lane.detector.frameCount() != lane.lastFrame)
    {
        lane.lastFrame = lane.detector.frameCount();
        if (o.active)
        {
            lane.centroids[static_cast<size_t> (lane.ringHead)] = lane.detector.features().centroidHz;
            lane.ringHead = (lane.ringHead + 1) % static_cast<int> (lane.centroids.size());
            lane.ringCount = std::min (lane.ringCount + 1, 750);
        }
        if (lane.ringCount > 0 && ++lane.sinceMedian >= 24)
        {
            lane.sinceMedian = 0;
            std::array<float, 750> values {};
            const int size = static_cast<int> (lane.centroids.size());
            for (int i = 0; i < lane.ringCount; ++i)
                values[static_cast<size_t> (i)] = lane.centroids[static_cast<size_t> ((lane.ringHead - 1 - i + 2 * size) % size)];
            const auto mid = values.begin() + lane.ringCount / 2;
            std::nth_element (values.begin(), mid, values.begin() + lane.ringCount);
            lane.median = *mid;
        }
    }

    float target = centre;
    if (s.track)
        target = o.active ? lane.detector.features().centroidHz : (lane.ringCount > 0 ? lane.median : centre);
    target = std::clamp (target, s.hpHz, s.lpHz);
    lane.logCentre += (s.track && ! o.active ? trackSlowCoeff : trackFastCoeff) * (std::log (target) - lane.logCentre);

    float qTarget = s.q;
    if (s.qAuto)
        qTarget = o.active ? std::clamp (0.5f / std::max (lane.detector.features().spreadOct, 0.05f), 2.0f, 6.0f) : lane.q;
    lane.q += trackFastCoeff * (qTarget - lane.q);
}

void ChausEngine::updateCoefficients (Lane& lane, const Settings& s, float reductionDb) noexcept
{
    const double fc = std::clamp (static_cast<double> (std::exp (lane.logCentre)), 200.0, 0.45 * fs);
    const double q = std::clamp (static_cast<double> (lane.q), 0.5, 10.0);

    const auto target = [] (const ferb::Biquad& f) { return Coefficients { f.b0, f.b1, f.b2, f.a1, f.a2 }; };
    const auto bell = target (eq::peaking (fs, fc, q, -reductionDb));
    const auto shelf = target (ferb::Biquad::highShelf (fs, fc, -reductionDb));

    const auto aim = [] (Coefficients& now, Coefficients& step, const Coefficients& t)
    {
        const double k = 1.0 / coefficientInterval;
        step = { (t.b0 - now.b0) * k, (t.b1 - now.b1) * k, (t.b2 - now.b2) * k, (t.a1 - now.a1) * k, (t.a2 - now.a2) * k };
    };
    aim (lane.bellNow, lane.bellStep, bell);
    aim (lane.shelfNow, lane.shelfStep, shelf);

    // Forma: la campana por defecto; el estante si la s cae por encima de 9 kHz y el ancho es grande (Q pequeña).
    const float wanted = s.shape == Shape::Shelf ? 1.0f : (s.shape == Shape::Bell ? 0.0f : (fc > 9000.0 && q < 2.5 ? 1.0f : 0.0f));
    lane.shapeWeight += (wanted - lane.shapeWeight) * (1.0f - std::pow (1.0f - shapeCoeff, static_cast<float> (coefficientInterval)));
}

void ChausEngine::configureLane (Lane& lane, const Settings& s) noexcept
{
    if (std::abs (s.hpHz - lastHp) > 1.0e-3f || std::abs (s.lpHz - lastLp) > 1.0e-3f)
        lane.detector.setBand (s.hpHz, s.lpHz);
    lane.detector.setClassifierEnabled (s.voiceMode);
}

void ChausEngine::process (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* sidechain, const Settings& request, Probe* probe) noexcept
{
    const int channels = std::min (buffer.getNumChannels(), static_cast<int> (maxChannels));
    const int total = buffer.getNumSamples();
    if (channels == 0 || total == 0)
        return;

    Settings s = request;
    s.thresholdDb = std::clamp (s.thresholdDb, -60.0f, 0.0f);
    s.rangeDb = std::clamp (s.rangeDb, 0.0f, 24.0f);
    s.hpHz = std::clamp (s.hpHz, 2000.0f, 12000.0f);
    s.lpHz = std::clamp (s.lpHz, 5000.0f, 20000.0f);
    s.lpHz = std::max (s.lpHz, s.hpHz * 1.2f);
    s.q = std::clamp (s.q, 1.0f, 8.0f);
    s.sensSoftDb = std::clamp (s.sensSoftDb, -6.0f, 6.0f);
    s.sensHardDb = std::clamp (s.sensHardDb, -6.0f, 6.0f);
    s.releaseMs = std::clamp (s.releaseMs, 10.0f, 100.0f);
    s.outputDb = std::clamp (s.outputDb, -12.0f, 12.0f);
    const float link = std::clamp (s.linkPercent, 0.0f, 100.0f) * 0.01f;
    if (channels == 1)
        s.stereo = Stereo::LeftRight;

    const bool hasSidechain = s.externalSidechain && sidechain != nullptr && sidechain->getNumChannels() > 0 && sidechain->getNumSamples() >= total;
    float* data[2] = { buffer.getWritePointer (0), channels > 1 ? buffer.getWritePointer (1) : nullptr };
    const float* sc[2] = { nullptr, nullptr };
    if (hasSidechain)
    {
        sc[0] = sidechain->getReadPointer (0);
        sc[1] = sidechain->getNumChannels() > 1 ? sidechain->getReadPointer (1) : sc[0];
    }

    for (int pos = 0; pos < total; pos += chunkSize)
    {
        const int n = std::min (chunkSize, total - pos);

        // Estructura (Mid/Side y banda ancha): cambia con la salida en silencio.
        const int key = static_cast<int> (s.stereo) * 2 + (s.wideBand ? 1 : 0);
        if (needsInit)
        {
            modeSwitch.reset (key);
            for (auto& lane : lanes)
                resetLane (lane);
            needsInit = false;
        }
        const auto step = modeSwitch.update (key, n);
        if (step.switched)
            for (auto& lane : lanes)
                resetLane (lane);
        const auto activeStereo = static_cast<Stereo> (step.mode / 2);
        const bool wide = (step.mode % 2) == 1;

        // Lookahead: se adopta cuando el valor lleva 150 ms estable; el cambio de toma es un fundido de 50 ms.
        const int wantedDelay = latencyFor (s, fs);
        if (adoptedDelay < 0)
        {
            requestedDelay = adoptedDelay = wantedDelay;
            for (auto& lane : lanes) { lane.delay.jumpTo (wantedDelay); lane.bandDelay.jumpTo (wantedDelay); lane.gateDelay.jumpTo (wantedDelay); }
            for (auto& d : passDelay) d.jumpTo (wantedDelay);
        }
        if (wantedDelay != requestedDelay)
        {
            requestedDelay = wantedDelay;
            stableSamples = 0;
        }
        else if (adoptedDelay != requestedDelay && (stableSamples += n) >= static_cast<int> (adoptSeconds * fs))
        {
            adoptedDelay = requestedDelay;
            for (auto& lane : lanes) { lane.delay.setDelay (adoptedDelay); lane.bandDelay.setDelay (adoptedDelay); lane.gateDelay.setDelay (adoptedDelay); }
            for (auto& d : passDelay) d.setDelay (adoptedDelay);
        }

        const int processed = (activeStereo == Stereo::LeftRight) ? channels : 1;
        for (int p = 0; p < processed; ++p)
            configureLane (lanes[static_cast<size_t> (p)], s);
        lastHp = s.hpHz;
        lastLp = s.lpHz;

        const double bypassTarget = s.bypass ? 1.0 : 0.0;
        const double listenTarget = (s.listenTrigger || s.listenSidechain) ? 1.0 : 0.0;
        const double outputTarget = std::pow (10.0, static_cast<double> (s.outputDb) / 20.0);

        for (int i = 0; i < n; ++i)
        {
            const int index = pos + i;
            const float x0 = sane (data[0][index]), x1 = data[1] != nullptr ? sane (data[1][index]) : 0.0f;
            float sc0 = x0, sc1 = x1;
            if (hasSidechain)
            {
                sc0 = sane (sc[0][index]);
                sc1 = sane (sc[1][index]);
            }

            // Componentes: dos lanes (L/R) o una (M o S) más el componente que pasa sin procesar.
            float audio[2] = { x0, x1 }, detectorIn[2] = { sc0, sc1 }, passthrough = 0.0f;
            if (activeStereo != Stereo::LeftRight)
            {
                const float m = 0.5f * (x0 + x1), d = 0.5f * (x0 - x1), sm = 0.5f * (sc0 + sc1), sd = 0.5f * (sc0 - sc1);
                const bool mid = activeStereo == Stereo::Mid;
                audio[0] = mid ? m : d;
                passthrough = mid ? d : m;
                detectorIn[0] = mid ? sm : sd;
            }

            // Detección y reducción pedida por cada lane.
            float wanted[2] = { 0.0f, 0.0f };
            for (int p = 0; p < processed; ++p)
            {
                auto& lane = lanes[static_cast<size_t> (p)];
                lane.tick = lane.detector.tick (detectorIn[p]);
                if (s.voiceMode)
                    lane.reference.tick (lane.tick.active, lane.tick.levelDb);
                wanted[p] = rawReduction (lane, s, lane.tick);
            }
            // Enlace: la reducción de cada canal es mezcla de la propia y la mayor de las dos.
            const float biggest = std::max (wanted[0], processed > 1 ? wanted[1] : 0.0f);
            for (int p = 0; p < processed; ++p)
                lanes[static_cast<size_t> (p)].linkedDb = processed > 1 ? (1.0f - link) * wanted[p] + link * biggest : wanted[p];

            float wet[2] = { 0.0f, 0.0f }, dry[2] = { 0.0f, 0.0f }, listen[2] = { 0.0f, 0.0f };
            for (int p = 0; p < processed; ++p)
            {
                auto& lane = lanes[static_cast<size_t> (p)];

                // Balística: ataque de 0,5 ms; liberación de 10 a 60 ms según la duración de la s (o la fijada).
                if (lane.linkedDb > 0.05f)
                    ++lane.eventSamples;
                else if (lane.eventSamples > 0)
                {
                    lane.lastEventSamples = lane.eventSamples;
                    lane.eventSamples = 0;
                }
                const double eventSeconds = static_cast<double> (std::max (lane.eventSamples, lane.lastEventSamples)) / fs;
                const double releaseSeconds = s.releaseAuto ? std::clamp (0.25 * eventSeconds, 0.010, 0.060) : static_cast<double> (s.releaseMs) * 0.001;
                const float c = lane.linkedDb > lane.smoothDb ? attackCoeff : static_cast<float> (coeff (releaseSeconds, fs));
                lane.smoothDb += c * (lane.linkedDb - lane.smoothDb);
                if (lane.smoothDb < 1.0e-5f && lane.linkedDb <= 0.0f)
                    lane.smoothDb = 0.0f;

                track (lane, s);

                const float delayed = lane.delay.process (audio[p]);
                dry[p] = delayed;
                if (wide)
                    wet[p] = delayed * dbToLin (-lane.smoothDb);
                else
                {
                    if (sinceCoefficients == 0)
                        updateCoefficients (lane, s, lane.smoothDb);
                    const auto advance = [] (ferb::Biquad& f, Coefficients& now, const Coefficients& st)
                    {
                        now.b0 += st.b0; now.b1 += st.b1; now.b2 += st.b2; now.a1 += st.a1; now.a2 += st.a2;
                        f.b0 = now.b0; f.b1 = now.b1; f.b2 = now.b2; f.a1 = now.a1; f.a2 = now.a2;
                    };
                    advance (lane.bell, lane.bellNow, lane.bellStep);
                    advance (lane.shelf, lane.shelfNow, lane.shelfStep);
                    const float yb = lane.bell.process (delayed), ys = lane.shelf.process (delayed);
                    wet[p] = yb + lane.shapeWeight * (ys - yb);
                }

                // Escucha: lo que oye el detector (con o sin la puerta de la sibilante), con el mismo retardo que el audio.
                lane.gate += gateCoeff * ((lane.tick.active ? 1.0f : 0.0f) - lane.gate);
                const float heard = lane.bandDelay.process (lane.tick.band), gate = lane.gateDelay.process (lane.gate);
                listen[p] = s.listenTrigger && ! s.listenSidechain ? heard * gate : heard;

                if (probe != nullptr)
                {
                    const auto put = [&] (std::array<float*, 2>& a, float v) { if (a[static_cast<size_t> (p)] != nullptr) a[static_cast<size_t> (p)][index] = v; };
                    put (probe->reductionDb, lane.smoothDb);
                    put (probe->centreHz, std::exp (lane.logCentre));
                    put (probe->levelDb, lane.tick.levelDb);
                    put (probe->thresholdDb, lane.lastThresholdDb);
                    put (probe->score, lane.tick.score);
                }
            }
            sinceCoefficients = (sinceCoefficients + 1) % coefficientInterval;

            // Pantalla: un punto cada 8 ms con el máximo del intervalo.
            for (int p = 0; p < processed; ++p)
            {
                const auto& lane = lanes[static_cast<size_t> (p)];
                if (lane.tick.levelDb > accLevel)
                {
                    accLevel = lane.tick.levelDb;
                    accThreshold = lane.lastThresholdDb;
                }
                if (lane.smoothDb >= accReduction)
                {
                    accReduction = lane.smoothDb;
                    accCentre = std::exp (lane.logCentre);
                }
                accDetected |= s.voiceMode ? lane.tick.active : lane.smoothDb > 0.1f;
            }
            if (++sinceTelemetry >= pointInterval)
            {
                sinceTelemetry = 0;
                const uint64_t count = telemetry.written.load (std::memory_order_relaxed);
                telemetry.points[static_cast<size_t> (count) & (Telemetry::capacity - 1)] = { accLevel, accThreshold, accReduction, accCentre, accDetected };
                telemetry.written.store (count + 1, std::memory_order_release);
                const auto& lane0 = lanes[0];
                telemetry.centreHz.store (accCentre, std::memory_order_relaxed);
                telemetry.q.store (lane0.q, std::memory_order_relaxed);
                telemetry.reductionDb.store (accReduction, std::memory_order_relaxed);
                telemetry.shapeWeight.store (lane0.shapeWeight, std::memory_order_relaxed);
                telemetry.score.store (lane0.tick.score, std::memory_order_relaxed);
                accLevel = -120.0f;
                accReduction = 0.0f;
                accDetected = false;
            }

            // Recomposición.
            float outWet[2], outDry[2], outListen[2];
            if (activeStereo == Stereo::LeftRight)
            {
                for (int p = 0; p < 2; ++p)
                {
                    outWet[p] = wet[p < processed ? p : 0];
                    outDry[p] = dry[p < processed ? p : 0];
                    outListen[p] = listen[p < processed ? p : 0];
                }
            }
            else
            {
                const float other = passDelay[0].process (passthrough);
                const float sign = 1.0f;
                (void) sign;
                const bool mid = activeStereo == Stereo::Mid;
                outWet[0] = mid ? wet[0] + other : other + wet[0];
                outWet[1] = mid ? wet[0] - other : other - wet[0];
                outDry[0] = mid ? dry[0] + other : other + dry[0];
                outDry[1] = mid ? dry[0] - other : other - dry[0];
                outListen[0] = outListen[1] = listen[0];
            }

            bypassMix += bypassCoeff * (bypassTarget - bypassMix);
            listenMix += listenCoeff * (listenTarget - listenMix);
            outputGain += outputCoeff * (outputTarget - outputGain);
            const float fade = step.gainStart + (step.gainEnd - step.gainStart) * static_cast<float> (i + 1) / static_cast<float> (n);
            for (int ch = 0; ch < channels; ++ch)
            {
                const double processedSample = (outWet[ch] + listenMix * (outListen[ch] - outWet[ch])) * outputGain;
                data[ch][index] = static_cast<float> ((processedSample + bypassMix * (outDry[ch] - processedSample)) * fade);
            }
        }
    }
}
} // namespace chaus
