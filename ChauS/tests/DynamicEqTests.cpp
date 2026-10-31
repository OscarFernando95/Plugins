#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>

#include "Signals.h"
#include "TestUtils.h"
#include "dsp/ChausEngine.h"

using Catch::Approx;
using namespace chaus;
using sig::makePhrase;
using sig::ms;

namespace
{
struct Render
{
    std::vector<float> l, r;
    std::array<std::vector<float>, 2> reduction, centre, level, threshold, score;
};

/** Pasa audio (mono si `right` está vacío) por el motor en trozos de `chunk`. Devuelve la salida sin compensar la latencia. */
Render render (const std::vector<float>& left, const std::vector<float>& right, const Settings& s, double fs, int chunk = 256,
               const std::vector<float>* scLeft = nullptr, const std::vector<float>* scRight = nullptr)
{
    const int channels = right.empty() ? 1 : 2;
    const size_t total = left.size();
    ChausEngine engine;
    engine.prepare (fs, chunk);

    Render out;
    out.l.assign (total, 0.0f);
    if (channels == 2)
        out.r.assign (total, 0.0f);
    for (int c = 0; c < 2; ++c)
        for (auto* v : { &out.reduction[size_t (c)], &out.centre[size_t (c)], &out.level[size_t (c)], &out.threshold[size_t (c)], &out.score[size_t (c)] })
            v->assign (total, 0.0f);

    juce::AudioBuffer<float> buf (channels, chunk), side (2, chunk);
    for (size_t pos = 0; pos < total; pos += size_t (chunk))
    {
        const int n = int (std::min<size_t> (size_t (chunk), total - pos));
        buf.setSize (channels, n, false, false, true);
        std::copy_n (left.begin() + long (pos), n, buf.getWritePointer (0));
        if (channels == 2)
            std::copy_n (right.begin() + long (pos), n, buf.getWritePointer (1));

        Probe probe;
        for (size_t c = 0; c < 2; ++c)
        {
            probe.reductionDb[c] = out.reduction[c].data() + pos;
            probe.centreHz[c] = out.centre[c].data() + pos;
            probe.levelDb[c] = out.level[c].data() + pos;
            probe.thresholdDb[c] = out.threshold[c].data() + pos;
            probe.score[c] = out.score[c].data() + pos;
        }

        const juce::AudioBuffer<float>* scPtr = nullptr;
        if (scLeft != nullptr)
        {
            side.setSize (2, n, false, false, true);
            std::copy_n (scLeft->begin() + long (pos), n, side.getWritePointer (0));
            std::copy_n ((scRight != nullptr ? scRight : scLeft)->begin() + long (pos), n, side.getWritePointer (1));
            scPtr = &side;
        }
        engine.process (buf, scPtr, s, &probe);
        std::copy_n (buf.getReadPointer (0), n, out.l.begin() + long (pos));
        if (channels == 2)
            std::copy_n (buf.getReadPointer (1), n, out.r.begin() + long (pos));
    }
    return out;
}

Render renderMono (const std::vector<float>& x, const Settings& s, double fs, int chunk = 256) { return render (x, {}, s, fs, chunk); }

/** La entrada retrasada `delay` muestras (lo que sale con Rango 0). */
std::vector<float> delayed (const std::vector<float>& x, int delay)
{
    std::vector<float> y (x.size(), 0.0f);
    for (size_t i = size_t (delay); i < x.size(); ++i)
        y[i] = x[i - size_t (delay)];
    return y;
}

Settings base()
{
    Settings s;
    s.lookaheadMs = 10.0f;
    return s;
}

/** Valor eficaz en dB de la señal filtrada con un paso banda de `lo` a `hi` (4.º orden), entre `from` y `to`. */
double bandLevelDb (const std::vector<float>& x, double fs, double lo, double hi, int from, int to)
{
    ferb::Biquad h1 = ferb::Biquad::highPass (fs, lo), h2 = ferb::Biquad::highPass (fs, lo);
    ferb::Biquad l1 = ferb::Biquad::lowPass (fs, hi), l2 = ferb::Biquad::lowPass (fs, hi);
    double sum = 0.0;
    int count = 0;
    for (int i = 0; i < int (x.size()) && i < to; ++i)
    {
        const double y = l2.process (l1.process (h2.process (h1.process (x[size_t (i)]))));
        if (i >= from)
        {
            sum += y * y;
            ++count;
        }
    }
    return 10.0 * std::log10 (std::max (sum / std::max (1, count), 1e-20));
}

double lowLevelDb (const std::vector<float>& x, double fs, double hz, int from, int to)
{
    ferb::Biquad a = ferb::Biquad::lowPass (fs, hz), b = ferb::Biquad::lowPass (fs, hz);
    double sum = 0.0;
    int count = 0;
    for (int i = 0; i < int (x.size()) && i < to; ++i)
    {
        const double y = b.process (a.process (x[size_t (i)]));
        if (i >= from)
        {
            sum += y * y;
            ++count;
        }
    }
    return 10.0 * std::log10 (std::max (sum / std::max (1, count), 1e-20));
}

/** Reducción de la meseta de una s: la mediana sobre el 40 % central del evento, en el tiempo del audio (la reducción calculada en t se aplica a t − L). */
float plateau (const std::vector<float>& reduction, const sig::Phrase::Event& e, int latency)
{
    const int span = e.end - e.start, from = e.start + int (0.3 * span) + latency, to = e.start + int (0.7 * span) + latency;
    std::vector<float> v (reduction.begin() + from, reduction.begin() + std::min<int> (to, int (reduction.size())));
    std::nth_element (v.begin(), v.begin() + long (v.size() / 2), v.end());
    return v[v.size() / 2];
}

/** Lo que faltó por quitar en cada instante del audio: la reducción que pedía el nivel (sin puerta ni balística) menos la aplicada. */
struct Shortfall { double worstDb = 0.0, msAbove3Db = 0.0; };
Shortfall shortfall (const Render& r, const sig::Phrase::Event& e, int latency, float range, double fs)
{
    Shortfall out;
    for (int tau = e.start - ms (fs, 5); tau < e.end; ++tau)
    {
        const float excess = r.level[0][size_t (tau)] - r.threshold[0][size_t (tau)];
        const float needed = std::min (excess >= 1.5f ? excess : (excess <= -1.5f ? 0.0f : (excess + 1.5f) * (excess + 1.5f) / 6.0f), range);
        const float lack = needed - r.reduction[0][size_t (tau + latency)];
        out.worstDb = std::max (out.worstDb, double (lack));
        out.msAbove3Db += lack > 3.0f ? 1000.0 / fs : 0.0;
    }
    return out;
}

float maxOf (const std::vector<float>& v, int from, int to)
{
    float m = -1.0e9f;
    for (int i = from; i < to && i < int (v.size()); ++i)
        m = std::max (m, v[size_t (i)]);
    return m;
}
} // namespace

// ───────────────────────────── Latencia y transparencia ─────────────────────────────

TEST_CASE ("Latencia: es el lookahead en muestras enteras y un impulso sale con ese retardo")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (float look : { 0.0f, 3.0f, 10.0f, 15.0f })
        {
            auto s = base();
            s.lookaheadMs = look;
            s.rangeDb = 0.0f;
            const int declared = ChausEngine::latencyFor (s, fs);
            REQUIRE (declared == int (look * 0.001 * fs + 0.5));

            std::vector<float> x (size_t (fs * 0.5), 0.0f);
            x[2000] = 0.5f;
            const auto y = renderMono (x, s, fs);
            size_t peak = 0;
            for (size_t i = 0; i < y.l.size(); ++i)
                if (std::abs (y.l[i]) > std::abs (y.l[peak]))
                    peak = i;
            INFO ("fs " << fs << ", lookahead " << look);
            REQUIRE (int (peak) == 2000 + declared);
        }
}

TEST_CASE ("Rango 0: la salida es idéntica a la entrada retrasada, con banda dividida o ancha y en M/S")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 6, 11);
    for (bool wide : { false, true })
        for (auto stereo : { Stereo::LeftRight, Stereo::Mid, Stereo::Side })
            for (auto shape : { Shape::Bell, Shape::Shelf, Shape::Auto })
            {
                auto s = base();
                s.rangeDb = 0.0f;
                s.wideBand = wide;
                s.stereo = stereo;
                s.shape = shape;
                const int L = ChausEngine::latencyFor (s, fs);
                std::vector<float> r (phrase.audio.size());
                for (size_t i = 0; i < r.size(); ++i)
                    r[i] = phrase.audio[(i * 7 + 13) % r.size()] * 0.5f; // otro canal
                const auto y = render (phrase.audio, r, s, fs);
                const auto dl = delayed (phrase.audio, L), dr = delayed (r, L);
                // La fundido inicial de la estructura (5 ms) puede tocar los primeros milisegundos si la estructura difiere del inicio.
                const int from = L + 4000;
                const double err = std::max (testutil::residualDb (y.l.data() + from, dl.data() + from, int (y.l.size()) - from),
                                             testutil::residualDb (y.r.data() + from, dr.data() + from, int (y.r.size()) - from));
                INFO ("banda ancha " << wide << ", estéreo " << int (stereo) << ", forma " << int (shape));
                REQUIRE (err < -110.0);
            }
}

// ───────────────────────────── Rango y banda dividida ─────────────────────────────

TEST_CASE ("Rango: la reducción nunca lo supera y llega a él con eses fuertes")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 10, 21, -12.0); // eses muy por encima del umbral
    for (float range : { 3.0f, 8.0f, 24.0f })
        for (bool wide : { false, true })
        {
            auto s = base();
            s.rangeDb = range;
            s.wideBand = wide;
            s.thresholdDb = -60.0f;
            const auto y = renderMono (phrase.audio, s, fs);
            const float peak = maxOf (y.reduction[0], 0, int (y.l.size()));
            INFO ("rango " << range << ", ancha " << wide);
            REQUIRE (peak <= range + 1.0e-3f);
            REQUIRE (peak >= range - 0.5f);
        }
}

TEST_CASE ("Banda dividida: fuera de la banda de la s el nivel cambia menos de 0,5 dB")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        const auto phrase = makePhrase (fs, 10, 31, -14.0);
        auto s = base();
        s.rangeDb = 24.0f;
        s.thresholdDb = -60.0f;
        const auto y = renderMono (phrase.audio, s, fs);
        const int L = ChausEngine::latencyFor (s, fs), from = ms (fs, 300), to = int (phrase.audio.size()) - L;
        const auto ref = delayed (phrase.audio, L);

        // Reducción efectiva en la banda de la s.
        const double inBand = bandLevelDb (ref, fs, 5500.0, 9500.0, from, to), outBand = bandLevelDb (y.l, fs, 5500.0, 9500.0, from, to);
        // Debajo de 2 kHz (voz).
        const double inLow = lowLevelDb (ref, fs, 2000.0, from, to), outLow = lowLevelDb (y.l, fs, 2000.0, from, to);
        INFO ("fs " << fs << ": banda de la s " << inBand << " → " << outBand << " dB; por debajo de 2 kHz " << inLow << " → " << outLow << " dB");
        REQUIRE (inBand - outBand > 3.0);
        REQUIRE (std::abs (outLow - inLow) < 0.5);
    }
}

TEST_CASE ("Banda ancha: baja todo el audio por igual")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 10, 33, -14.0);
    auto s = base();
    s.rangeDb = 12.0f;
    s.thresholdDb = -60.0f;
    s.wideBand = true;
    const auto y = renderMono (phrase.audio, s, fs);
    const int L = ChausEngine::latencyFor (s, fs), from = ms (fs, 300), to = int (phrase.audio.size()) - L;
    const auto ref = delayed (phrase.audio, L);
    REQUIRE (lowLevelDb (ref, fs, 2000.0, from, to) - lowLevelDb (y.l, fs, 2000.0, from, to) > 0.2);
}

// ───────────────────────────── Lookahead ─────────────────────────────

TEST_CASE ("Lookahead de 10 ms: la reducción llega al 90 % de la meseta antes del pico de la s")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        const auto phrase = makePhrase (fs, 12, 41, -18.0);
        auto s = base();
        s.rangeDb = 12.0f;
        s.thresholdDb = -40.0f;
        const auto y = renderMono (phrase.audio, s, fs);
        const int L = ChausEngine::latencyFor (s, fs);
        int checked = 0, onTime = 0;
        for (const auto& e : phrase.events)
        {
            const float top = plateau (y.reduction[0], e, L);
            if (top < 3.0f)
                continue; // una s que no pasa del umbral
            ++checked;
            int t90 = -1;
            for (int i = e.start - ms (fs, 5); i < e.end + ms (fs, 30); ++i)
                if (y.reduction[0][size_t (i)] >= 0.9f * top)
                {
                    t90 = i;
                    break;
                }
            // La reducción calculada en el instante t se aplica al audio de t − L.
            onTime += (t90 >= 0 && t90 - L <= (e.start + e.end) / 2) ? 1 : 0;
        }
        INFO ("fs " << fs << ": " << onTime << " de " << checked);
        REQUIRE (checked >= 6);
        REQUIRE (onTime == checked);
    }
}

TEST_CASE ("Lookahead: con 10 ms lo que falta por quitar en cada s baja respecto de no usarlo y queda bajo 4 dB")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 12, 41, -18.0);
    double worst[2] = {};
    int index = 0;
    for (float look : { 0.0f, 10.0f })
    {
        auto s = base();
        s.lookaheadMs = look;
        s.rangeDb = 12.0f;
        s.thresholdDb = -40.0f;
        const auto y = renderMono (phrase.audio, s, fs);
        const int L = ChausEngine::latencyFor (s, fs);
        int n = 0;
        for (const auto& e : phrase.events)
            if (plateau (y.reduction[0], e, L) >= 3.0f)
            {
                worst[index] += shortfall (y, e, L, s.rangeDb, fs).worstDb;
                ++n;
            }
        worst[index++] /= n;
    }
    INFO ("faltante medio sin lookahead " << worst[0] << " dB, con 10 ms " << worst[1] << " dB");
    REQUIRE (worst[1] < worst[0] - 1.0);
    REQUIRE (worst[1] < 4.0);
}

TEST_CASE ("Lookahead: informe del instante en que la reducción llega al 90 % respecto del inicio de la s", "[.report]")
{
    for (float look : { 0.0f, 5.0f, 10.0f, 15.0f })
    {
        const double fs = 48000.0;
        const auto phrase = makePhrase (fs, 12, 41, -18.0);
        auto s = base();
        s.lookaheadMs = look;
        s.rangeDb = 12.0f;
        s.thresholdDb = -40.0f;
        const auto y = renderMono (phrase.audio, s, fs);
        const int L = ChausEngine::latencyFor (s, fs);
        double sum = 0.0, worst = -1e9, lack = 0.0, lackMs = 0.0;
        int n = 0;
        for (const auto& e : phrase.events)
        {
            const float top = plateau (y.reduction[0], e, L);
            if (top < 3.0f)
                continue;
            const auto sf = shortfall (y, e, L, s.rangeDb, fs);
            lack += sf.worstDb;
            lackMs += sf.msAbove3Db;
            for (int i = e.start - ms (fs, 5); i < e.end + ms (fs, 30); ++i)
                if (y.reduction[0][size_t (i)] >= 0.9f * top)
                {
                    const double delay = double (i - L - e.start) / fs * 1000.0;
                    sum += delay;
                    worst = std::max (worst, delay);
                    ++n;
                    break;
                }
        }
        std::printf ("lookahead %4.1f ms: el 90 %% de la meseta llega %+.1f ms tras el inicio de la s (media; peor %+.1f ms); faltante máximo %.1f dB y %.1f ms con más de 3 dB de faltante, por s (%d eses)\n",
                     look, n > 0 ? sum / n : 0.0, worst, n > 0 ? lack / n : 0.0, n > 0 ? lackMs / n : 0.0, n);
    }
}

// ───────────────────────────── Umbral adaptativo ─────────────────────────────

TEST_CASE ("Umbral adaptativo: la misma frase 6 dB más baja da reducciones parecidas (diferencia menor de 1,5 dB)")
{
    for (double fs : { 44100.0, 48000.0 })
    {
        double mean[2] = {};
        int index = 0;
        for (double gainDb : { -6.0, -12.0 })
        {
            auto phrase = makePhrase (fs, 16, 51, -20.0);
            const float g = float (testutil::fromDb (gainDb));
            for (auto& v : phrase.audio)
                v *= g;
            auto s = base();
            s.rangeDb = 12.0f;
            s.thresholdDb = -38.0f; // 8 dB bajo la referencia: reduce de verdad
            const auto y = renderMono (phrase.audio, s, fs);
            double sum = 0.0;
            int n = 0;
            for (size_t e = 6; e < phrase.events.size(); ++e) // cuando la referencia ya se asentó
            {
                sum += maxOf (y.reduction[0], phrase.events[e].start - ms (fs, 5), phrase.events[e].end + ms (fs, 30));
                ++n;
            }
            mean[index++] = sum / n;
        }
        INFO ("fs " << fs << ": reducción media " << mean[0] << " y " << mean[1] << " dB");
        REQUIRE (std::abs (mean[0] - mean[1]) < 1.5);
        REQUIRE (mean[0] > 0.5); // y hay reducción de verdad
    }
}

TEST_CASE ("Umbral absoluto: sin adaptativo, una frase más baja se reduce menos")
{
    const double fs = 48000.0;
    double mean[2] = {};
    int index = 0;
    for (double gainDb : { 0.0, -12.0 })
    {
        auto phrase = makePhrase (fs, 12, 52, -22.0);
        for (auto& v : phrase.audio)
            v *= float (testutil::fromDb (gainDb));
        auto s = base();
        s.adaptive = false;
        s.thresholdDb = -34.0f;
        s.rangeDb = 24.0f;
        const auto y = renderMono (phrase.audio, s, fs);
        double sum = 0.0;
        for (const auto& e : phrase.events)
            sum += maxOf (y.reduction[0], e.start - ms (fs, 5), e.end + ms (fs, 30));
        mean[index++] = sum / double (phrase.events.size());
    }
    INFO (mean[0] << " y " << mean[1]);
    REQUIRE (mean[0] - mean[1] > 6.0);
}

// ───────────────────────────── Seguimiento ─────────────────────────────

namespace
{
/** Ruido que pasa por un paso banda de ancho fijo (en octavas) cuyo centro sube de `from` a `to` Hz en `seconds`. */
std::vector<float> sweptSibilant (double fs, double from, double to, double seconds, double levelDb, sig::Rng& rng, std::vector<double>& centreAt)
{
    const int n = int (seconds * fs);
    std::vector<float> x (static_cast<size_t> (n));
    centreAt.resize (static_cast<size_t> (n));
    ferb::Biquad a, b;
    for (int i = 0; i < n; ++i)
    {
        const double t = double (i) / n, centre = from * std::pow (to / from, t);
        centreAt[size_t (i)] = centre;
        if (i % 32 == 0)
        {
            a.setCoefficients (sig::bandPass (fs, centre, 2.2));
            b.setCoefficients (sig::bandPass (fs, centre, 2.2));
        }
        x[size_t (i)] = b.process (a.process (rng.gauss()));
    }
    sig::setRms (x, levelDb);
    sig::applyEnvelope (x, fs, 0.05, 0.05);
    return x;
}
} // namespace

TEST_CASE ("Seguimiento: la campana sigue una s que sube de 6 a 9 kHz con error medio menor de 300 Hz")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        sig::Rng rng (61);
        std::vector<double> centreAt;
        auto x = sig::vowels (fs, int (2.0 * fs), 150.0, -28.0, rng);
        const auto sweep = sweptSibilant (fs, 6000.0, 9000.0, 1.5, -20.0, rng, centreAt);
        const int start = ms (fs, 200);
        sig::mixAt (x, sweep, start);

        auto s = base();
        s.thresholdDb = -60.0f;
        s.rangeDb = 12.0f;
        const auto y = renderMono (x, s, fs);

        double err = 0.0;
        int n = 0;
        for (int i = start + ms (fs, 150); i < start + int (sweep.size()) - ms (fs, 150); ++i)
            if (y.score[0][size_t (i)] > 0.6f)
            {
                err += std::abs (double (y.centre[0][size_t (i)]) - centreAt[size_t (i - start)]);
                ++n;
            }
        INFO ("fs " << fs << ": " << n << " muestras activas");
        REQUIRE (n > int (0.5 * 1.2 * fs));
        REQUIRE (err / n < 300.0);
    }
}

TEST_CASE ("Seguimiento apagado: la campana se queda en el centro geométrico de HP y LP")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 6, 71, -16.0, 7500.0, 10000.0);
    auto s = base();
    s.track = false;
    s.thresholdDb = -60.0f;
    const auto y = renderMono (phrase.audio, s, fs);
    const float expected = std::sqrt (s.hpHz * s.lpHz);
    REQUIRE (y.centre[0].back() == Approx (expected).margin (60.0f));
    for (size_t i = size_t (ms (fs, 400)); i < y.centre[0].size(); i += 977)
        REQUIRE (y.centre[0][i] == Approx (expected).margin (60.0f));
}

// ───────────────────────────── Estéreo y Mid/Side ─────────────────────────────

TEST_CASE ("Estéreo: con enlace del 100 % la reducción es la misma en L y R; sin enlace, cada canal la suya")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 10, 81, -16.0);
    std::vector<float> r (phrase.audio.size());
    const float g = float (testutil::fromDb (-9.0));
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = phrase.audio[i] * g;

    auto s = base();
    s.thresholdDb = -60.0f;
    s.rangeDb = 16.0f;
    s.linkPercent = 100.0f;
    auto linked = render (phrase.audio, r, s, fs);
    REQUIRE (maxOf (linked.reduction[0], 0, int (r.size())) > 3.0f);
    for (size_t i = 0; i < r.size(); ++i)
        REQUIRE (linked.reduction[0][i] == Approx (linked.reduction[1][i]).margin (1.0e-4));

    s.linkPercent = 0.0f;
    auto free = render (phrase.audio, r, s, fs);
    double diff = 0.0;
    for (size_t i = 0; i < r.size(); ++i)
        diff = std::max (diff, double (std::abs (free.reduction[0][i] - free.reduction[1][i])));
    REQUIRE (diff > 1.0);
}

TEST_CASE ("Mid/Side: en modo Side solo cambia el canal side, y en modo Mid solo el mid")
{
    const double fs = 48000.0;
    sig::Rng rng (91);
    const auto phrase = makePhrase (fs, 10, 91, -14.0);
    // Mid con voz, side con las eses: L = v + s, R = v − s.
    std::vector<float> l (phrase.audio.size()), r (phrase.audio.size());
    const auto voice = sig::vowels (fs, int (phrase.audio.size()), 150.0, -26.0, rng);
    for (size_t i = 0; i < l.size(); ++i)
    {
        l[i] = voice[i] + phrase.audio[i];
        r[i] = voice[i] - phrase.audio[i];
    }

    for (auto stereo : { Stereo::Side, Stereo::Mid })
    {
        auto s = base();
        s.stereo = stereo;
        s.thresholdDb = -60.0f;
        s.rangeDb = 18.0f;
        const auto y = render (l, r, s, fs);
        const int L = ChausEngine::latencyFor (s, fs), from = L + ms (fs, 100);
        const auto dl = delayed (l, L), dr = delayed (r, L);

        std::vector<float> midIn, midOut, sideIn, sideOut;
        for (size_t i = size_t (from); i < l.size(); ++i)
        {
            midIn.push_back (0.5f * (dl[i] + dr[i]));
            midOut.push_back (0.5f * (y.l[i] + y.r[i]));
            sideIn.push_back (0.5f * (dl[i] - dr[i]));
            sideOut.push_back (0.5f * (y.l[i] - y.r[i]));
        }
        const auto& untouchedIn = stereo == Stereo::Side ? midIn : sideIn;
        const auto& untouchedOut = stereo == Stereo::Side ? midOut : sideOut;
        const auto& changedIn = stereo == Stereo::Side ? sideIn : midIn;
        const auto& changedOut = stereo == Stereo::Side ? sideOut : midOut;

        INFO ("modo " << (stereo == Stereo::Side ? "Side" : "Mid"));
        REQUIRE (testutil::residualDb (untouchedOut.data(), untouchedIn.data(), int (untouchedIn.size())) < -110.0);
        if (stereo == Stereo::Side)
            REQUIRE (bandLevelDb (changedIn, fs, 5500.0, 9500.0, 0, int (changedIn.size())) - bandLevelDb (changedOut, fs, 5500.0, 9500.0, 0, int (changedOut.size())) > 1.0);
        else
            REQUIRE (testutil::residualDb (changedOut.data(), changedIn.data(), int (changedIn.size())) < -110.0); // el mid no tiene eses: no hay reducción
    }
}

// ───────────────────────────── Modo General, sidechain y escucha ─────────────────────────────

TEST_CASE ("Modo General: sin clasificador reduce cualquier energía de la banda sobre el umbral")
{
    const double fs = 48000.0;
    sig::Rng rng (101);
    auto x = sig::vowels (fs, int (2.0 * fs), 150.0, -26.0, rng);
    sig::mixAt (x, sig::cymbal (fs, 1.0, -16.0, rng, 1.0), ms (fs, 300)); // un platillo: en Voz sola apenas actuaría
    auto s = base();
    s.voiceMode = false;
    s.adaptive = false;
    s.thresholdDb = -32.0f;
    s.rangeDb = 12.0f;
    const auto general = renderMono (x, s, fs);
    REQUIRE (maxOf (general.reduction[0], ms (fs, 300), ms (fs, 1300)) > 4.0f);

    // En voz sola, las vocales solas no activan nada; en General, una frase tranquila bajo el umbral tampoco.
    const auto quiet = sig::vowels (fs, int (fs), 150.0, -40.0, rng);
    REQUIRE (maxOf (renderMono (quiet, s, fs).reduction[0], 0, int (quiet.size())) < 0.1f);
}

TEST_CASE ("Sidechain externo: la reducción la manda el sidechain, no el audio principal")
{
    const double fs = 48000.0;
    sig::Rng rng (111);
    const auto phrase = makePhrase (fs, 8, 111, -14.0);
    auto quietMain = sig::vowels (fs, int (phrase.audio.size()), 150.0, -26.0, rng);

    auto s = base();
    s.thresholdDb = -60.0f;
    s.rangeDb = 12.0f;
    s.externalSidechain = true;
    const auto withSc = render (quietMain, {}, s, fs, 256, &phrase.audio);
    REQUIRE (maxOf (withSc.reduction[0], 0, int (quietMain.size())) > 6.0f);

    s.externalSidechain = false; // el audio principal no tiene eses
    REQUIRE (maxOf (renderMono (quietMain, s, fs).reduction[0], 0, int (quietMain.size())) < 0.1f);
}

TEST_CASE ("Escuchar sidechain: sale la banda filtrada; escuchar disparo: solo cuando hay sibilante")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 8, 121, -16.0);
    const int from = ms (fs, 300), to = int (phrase.audio.size()) - ms (fs, 100);

    auto s = base();
    s.listenSidechain = true;
    auto y = renderMono (phrase.audio, s, fs);
    const int L = ChausEngine::latencyFor (s, fs);
    REQUIRE (lowLevelDb (y.l, fs, 1500.0, from, to) < lowLevelDb (phrase.audio, fs, 1500.0, from, to) - 30.0); // las vocales se van
    REQUIRE (bandLevelDb (y.l, fs, 5500.0, 9500.0, from, to) > bandLevelDb (phrase.audio, fs, 5500.0, 9500.0, from - L, to - L) - 3.0);

    s.listenSidechain = false;
    s.listenTrigger = true;
    y = renderMono (phrase.audio, s, fs);
    // Entre eses no sale nada; durante ellas, sí.
    double between = 0.0, during = 0.0;
    for (size_t e = 1; e + 1 < phrase.events.size(); ++e)
    {
        const int gapA = phrase.events[e].end + ms (fs, 60) + L, gapB = phrase.events[e + 1].start - ms (fs, 40) + L;
        for (int i = gapA; i < gapB; ++i)
            between = std::max (between, double (std::abs (y.l[size_t (i)])));
        const int mid = (phrase.events[e].start + phrase.events[e].end) / 2 + L;
        for (int i = mid - 200; i < mid + 200; ++i)
            during = std::max (during, double (std::abs (y.l[size_t (i)])));
    }
    INFO ("entre eses " << between << ", durante " << during);
    REQUIRE (during > 0.02);
    REQUIRE (between < during * 0.05);
}

// ───────────────────────────── Salida, bypass y clics ─────────────────────────────

TEST_CASE ("Salida: el control de ganancia suma sus dB y el bypass deja pasar la entrada retrasada")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 6, 131, -16.0);
    auto s = base();
    s.rangeDb = 0.0f;
    s.outputDb = 6.0f;
    auto y = renderMono (phrase.audio, s, fs);
    const int L = ChausEngine::latencyFor (s, fs), from = L + ms (fs, 400);
    const auto ref = delayed (phrase.audio, L);
    for (size_t i = size_t (from); i < y.l.size(); i += 331)
        REQUIRE (y.l[i] == Approx (ref[i] * testutil::fromDb (6.0)).margin (2.0e-5));

    s.outputDb = 6.0f;
    s.bypass = true;
    s.rangeDb = 24.0f;
    s.thresholdDb = -60.0f;
    y = renderMono (phrase.audio, s, fs);
    for (size_t i = size_t (from); i < y.l.size(); i += 331)
        REQUIRE (y.l[i] == Approx (ref[i]).margin (2.0e-5));
}

TEST_CASE ("Sin clics: mover cualquier control o cambiar la estructura no sube el salto entre muestras más de 1,3 veces")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 12, 141, -16.0);
    std::vector<float> r (phrase.audio.size());
    for (size_t i = 0; i < r.size(); ++i)
        r[i] = phrase.audio[(i + 211) % r.size()] * 0.8f;

    // Se mide con la señal de entrada de las pruebas: el salto máximo de la propia señal con Rango 0 es el de referencia.
    auto still = base();
    const auto stable = render (phrase.audio, r, still, fs);
    const double reference = std::max (testutil::stepMax (stable.l, 5000, stable.l.size()), testutil::stepMax (stable.r, 5000, stable.r.size()));

    struct Control { const char* name; std::function<void (Settings&, bool)> set; double scale = 1.0; };
    const std::vector<Control> controls {
        { "umbral", [] (Settings& s, bool a) { s.thresholdDb = a ? -58.0f : -10.0f; } },
        { "rango", [] (Settings& s, bool a) { s.rangeDb = a ? 24.0f : 0.0f; } },
        { "HP", [] (Settings& s, bool a) { s.hpHz = a ? 2000.0f : 12000.0f; } },
        { "LP", [] (Settings& s, bool a) { s.lpHz = a ? 5000.0f : 20000.0f; } },
        { "Q manual", [] (Settings& s, bool a) { s.qAuto = false; s.q = a ? 1.0f : 8.0f; } },
        { "S suaves", [] (Settings& s, bool a) { s.sensSoftDb = a ? -6.0f : 6.0f; } },
        { "S duras", [] (Settings& s, bool a) { s.sensHardDb = a ? -6.0f : 6.0f; } },
        { "enlace", [] (Settings& s, bool a) { s.linkPercent = a ? 0.0f : 100.0f; } },
        { "salida", [] (Settings& s, bool a) { s.outputDb = a ? -12.0f : 12.0f; }, 4.0 }, // la señal llega a ×4
        { "liberación", [] (Settings& s, bool a) { s.releaseAuto = false; s.releaseMs = a ? 10.0f : 100.0f; } },
        { "seguir", [] (Settings& s, bool a) { s.track = a; } },
        { "adaptativo", [] (Settings& s, bool a) { s.adaptive = a; } },
        { "modo", [] (Settings& s, bool a) { s.voiceMode = a; } },
        { "banda", [] (Settings& s, bool a) { s.wideBand = a; } },
        { "forma", [] (Settings& s, bool a) { s.shape = a ? Shape::Bell : Shape::Shelf; } },
        { "estéreo", [] (Settings& s, bool a) { s.stereo = a ? Stereo::LeftRight : Stereo::Mid; } },
        { "lookahead", [] (Settings& s, bool a) { s.lookaheadMs = a ? 2.0f : 15.0f; } },
        { "escuchar disparo", [] (Settings& s, bool a) { s.listenTrigger = a; } },
        { "escuchar sidechain", [] (Settings& s, bool a) { s.listenSidechain = a; } },
        { "bypass", [] (Settings& s, bool a) { s.bypass = a; } },
    };

    for (const auto& c : controls)
    {
        // Alterna el control cada 400 ms (el motor tarda 150 ms en adoptar un cambio de lookahead y de estructura).
        const size_t total = phrase.audio.size();
        ChausEngine engine;
        engine.prepare (fs, 256);
        std::vector<float> outL (total, 0.0f), outR (total, 0.0f);
        juce::AudioBuffer<float> buf (2, 256);
        auto s = base();
        s.thresholdDb = -40.0f;
        for (size_t pos = 0; pos + 256 <= total; pos += 256)
        {
            c.set (s, (pos / size_t (fs * 0.4)) % 2 == 0);
            std::copy_n (phrase.audio.begin() + long (pos), 256, buf.getWritePointer (0));
            std::copy_n (r.begin() + long (pos), 256, buf.getWritePointer (1));
            engine.process (buf, nullptr, s);
            std::copy_n (buf.getReadPointer (0), 256, outL.begin() + long (pos));
            std::copy_n (buf.getReadPointer (1), 256, outR.begin() + long (pos));
        }
        const double step = std::max (testutil::stepMax (outL, 5000, total), testutil::stepMax (outR, 5000, total));
        INFO (c.name << ": salto " << step << " frente a " << reference);
        REQUIRE (step <= 1.3 * c.scale * std::max (reference, 0.05));
    }
}

// ───────────────────────────── Robustez ─────────────────────────────

TEST_CASE ("Robustez: silencio, continua, NaN, infinitos y subnormales; todas las tasas y bloques")
{
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int chunk : { 1, 7, 64, 255, 256, 257, 1024, 4096 })
        {
            const int n = int (fs * 0.3);
            const auto check = [&] (const std::vector<float>& x)
            {
                auto s = base();
                s.thresholdDb = -60.0f;
                s.rangeDb = 24.0f;
                const auto y = render (x, x, s, fs, chunk);
                for (size_t i = 0; i < x.size(); ++i)
                {
                    INFO ("fs " << fs << ", bloque " << chunk << ", muestra " << i);
                    REQUIRE (std::isfinite (y.l[i]));
                    REQUIRE (std::isfinite (y.r[i]));
                    REQUIRE (std::abs (y.l[i]) < 8.0f);
                }
            };
            check (std::vector<float> (static_cast<size_t> (n), 0.0f));
            check (std::vector<float> (static_cast<size_t> (n), 0.5f));
            check (std::vector<float> (static_cast<size_t> (n), 1.0e-40f));
            std::vector<float> bad (static_cast<size_t> (n), 0.1f);
            for (size_t i = 0; i < bad.size(); i += 53)
                bad[i] = std::numeric_limits<float>::quiet_NaN();
            for (size_t i = 7; i < bad.size(); i += 211)
                bad[i] = std::numeric_limits<float>::infinity();
            check (bad);
        }
}

TEST_CASE ("El tamaño del bloque no cambia el resultado")
{
    const double fs = 48000.0;
    const auto phrase = makePhrase (fs, 8, 151, -16.0);
    auto s = base();
    s.thresholdDb = -50.0f;
    const auto a = renderMono (phrase.audio, s, fs, 256), b = renderMono (phrase.audio, s, fs, 1), c = renderMono (phrase.audio, s, fs, 1000);
    REQUIRE (testutil::residualDb (a.l.data(), b.l.data(), int (a.l.size())) < -100.0);
    REQUIRE (testutil::residualDb (a.l.data(), c.l.data(), int (a.l.size())) < -100.0);
}

// ───────────────────────────── Informe de M2 ─────────────────────────────

TEST_CASE ("Informe: cifras de M2 y CPU", "[.report]")
{
    const double fs = 48000.0;
    // CPU: dos canales, 48 kHz, voz con eses (todo el flujo, lookahead 10 ms).
    {
        const auto phrase = makePhrase (fs, 40, 5);
        std::vector<float> r (phrase.audio.size());
        for (size_t i = 0; i < r.size(); ++i)
            r[i] = phrase.audio[(i + 97) % r.size()];
        for (bool wide : { false, true })
            for (bool voice : { true, false })
            {
                ChausEngine engine;
                engine.prepare (fs, 512);
                auto s = base();
                s.wideBand = wide;
                s.voiceMode = voice;
                s.thresholdDb = -40.0f;
                juce::AudioBuffer<float> buf (2, 512);
                double seconds = 0.0;
                const auto start = std::chrono::steady_clock::now();
                size_t pos = 0;
                int blocks = 0;
                for (; pos + 512 <= phrase.audio.size(); pos += 512, ++blocks)
                {
                    std::copy_n (phrase.audio.begin() + long (pos), 512, buf.getWritePointer (0));
                    std::copy_n (r.begin() + long (pos), 512, buf.getWritePointer (1));
                    engine.process (buf, nullptr, s);
                }
                seconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();
                const double audioSeconds = double (pos) / fs;
                std::printf ("CPU estéreo 48 kHz, %s, %s: %.2f %% de un núcleo\n", wide ? "banda ancha" : "banda dividida", voice ? "voz sola" : "general", 100.0 * seconds / audioSeconds);
            }
    }
}

TEST_CASE ("Informe: banda dividida, seguimiento y umbral adaptativo", "[.report]")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        // Banda dividida: cambio fuera de banda con Rango 24.
        const auto phrase = makePhrase (fs, 10, 31, -14.0);
        auto s = base();
        s.rangeDb = 24.0f;
        s.thresholdDb = -60.0f;
        const auto y = renderMono (phrase.audio, s, fs);
        const int L = ChausEngine::latencyFor (s, fs), from = ms (fs, 300), to = int (phrase.audio.size()) - L;
        const auto ref = delayed (phrase.audio, L);
        std::printf ("%6.0f Hz | banda dividida, Rango 24: banda de la s %.1f dB; por debajo de 2 kHz %+.2f dB", fs,
                     bandLevelDb (ref, fs, 5500.0, 9500.0, from, to) - bandLevelDb (y.l, fs, 5500.0, 9500.0, from, to),
                     lowLevelDb (y.l, fs, 2000.0, from, to) - lowLevelDb (ref, fs, 2000.0, from, to));

        // Seguimiento.
        sig::Rng rng (61);
        std::vector<double> centreAt;
        auto x = sig::vowels (fs, int (2.0 * fs), 150.0, -28.0, rng);
        const auto sweep = sweptSibilant (fs, 6000.0, 9000.0, 1.5, -20.0, rng, centreAt);
        const int start = ms (fs, 200);
        sig::mixAt (x, sweep, start);
        auto t = base();
        t.thresholdDb = -60.0f;
        t.rangeDb = 12.0f;
        const auto z = renderMono (x, t, fs);
        double err = 0.0;
        int n = 0;
        for (int i = start + ms (fs, 150); i < start + int (sweep.size()) - ms (fs, 150); ++i)
            if (z.score[0][size_t (i)] > 0.6f)
            {
                err += std::abs (double (z.centre[0][size_t (i)]) - centreAt[size_t (i - start)]);
                ++n;
            }
        std::printf (" | seguimiento 6→9 kHz: error medio %.0f Hz\n", err / std::max (1, n));
    }
}

TEST_CASE ("Informe: umbral adaptativo frente a absoluto con la misma frase a −6 y −12 dB", "[.report]")
{
    for (double fs : { 44100.0, 48000.0 })
        for (bool adaptive : { true, false })
        {
            double mean[2] = {};
            int index = 0;
            for (double gainDb : { -6.0, -12.0 })
            {
                auto phrase = makePhrase (fs, 16, 51, -20.0);
                for (auto& v : phrase.audio)
                    v *= float (testutil::fromDb (gainDb));
                auto s = base();
                s.adaptive = adaptive;
                s.rangeDb = 12.0f;
                s.thresholdDb = adaptive ? -38.0f : -42.0f;
                const auto y = renderMono (phrase.audio, s, fs);
                double sum = 0.0;
                int n = 0;
                for (size_t e = 6; e < phrase.events.size(); ++e)
                {
                    sum += maxOf (y.reduction[0], phrase.events[e].start - ms (fs, 5), phrase.events[e].end + ms (fs, 30));
                    ++n;
                }
                mean[index++] = sum / n;
            }
            std::printf ("%6.0f Hz, %s: reducción media %.2f dB (−6 dB) y %.2f dB (−12 dB); diferencia %.2f dB\n", fs, adaptive ? "adaptativo" : "absoluto  ", mean[0], mean[1], mean[0] - mean[1]);
        }
}
