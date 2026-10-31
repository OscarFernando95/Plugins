#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>

#include "Signals.h"
#include "TestUtils.h"
#include "dsp/MaruEngine.h"

using Catch::Approx;
using namespace maru;

namespace
{
struct Out
{
    std::vector<float> l, r;
};

Out run (const std::vector<float>& left, const std::vector<float>& right, const Settings& s, double fs, const std::vector<float>* scL = nullptr, const std::vector<float>* scR = nullptr, int chunk = 256)
{
    const int channels = right.empty() ? 1 : 2;
    MaruEngine engine;
    engine.prepare (fs, chunk);
    Out out;
    out.l.assign (left.size(), 0.0f);
    if (channels == 2)
        out.r.assign (left.size(), 0.0f);
    juce::AudioBuffer<float> buf (channels, chunk), side (2, chunk);
    for (size_t pos = 0; pos < left.size(); pos += size_t (chunk))
    {
        const int n = int (std::min<size_t> (size_t (chunk), left.size() - pos));
        buf.setSize (channels, n, false, false, true);
        std::copy_n (left.begin() + long (pos), n, buf.getWritePointer (0));
        if (channels == 2)
            std::copy_n (right.begin() + long (pos), n, buf.getWritePointer (1));
        const juce::AudioBuffer<float>* sc = nullptr;
        if (scL != nullptr)
        {
            side.setSize (2, n, false, false, true);
            std::copy_n (scL->begin() + long (pos), n, side.getWritePointer (0));
            std::copy_n ((scR != nullptr ? scR : scL)->begin() + long (pos), n, side.getWritePointer (1));
            sc = &side;
        }
        engine.process (buf, sc, s);
        std::copy_n (buf.getReadPointer (0), n, out.l.begin() + long (pos));
        if (channels == 2)
            std::copy_n (buf.getReadPointer (1), n, out.r.begin() + long (pos));
    }
    return out;
}

Settings gateSettings()
{
    Settings s;
    s.gate = true;
    s.amount = 0.5f;
    s.gateAuto = false;
    s.gateThreshDb = -40.0f;
    s.hystDb = 6.0f;
    s.holdMs = 40.0f;
    s.attackMs = 2.0f;
    s.recoveryMs = 100.0f;
    s.lookaheadMs = 0.0f;
    s.rangeDb = 40.0f;
    return s;
}

/** Senoidal de 1 kHz con un nivel eficaz (dBFS) por tramo: {nivel, segundos}. */
std::vector<float> levels (double fs, const std::vector<std::pair<double, double>>& plan)
{
    std::vector<float> x;
    double phase = 0.0;
    for (const auto& [db, seconds] : plan)
    {
        const double amp = sig::dbToLin (db) * std::sqrt (2.0);
        for (int i = 0; i < int (seconds * fs); ++i)
        {
            x.push_back (float (amp * std::sin (phase)));
            phase += 2.0 * std::numbers::pi * 1000.0 / fs;
        }
    }
    return x;
}

/** Ganancia (dB) de la salida frente a la entrada retrasada, en un tramo. */
double gainDb (const std::vector<float>& in, const std::vector<float>& out, int latency, int from, int to)
{
    std::vector<float> ref (in.size(), 0.0f);
    for (size_t i = size_t (latency); i < in.size(); ++i)
        ref[i] = in[i - size_t (latency)];
    return sig::toDb (sig::rms (out, from, to)) - sig::toDb (sig::rms (ref, from, to));
}
} // namespace

TEST_CASE ("Gate: abre sobre el umbral, cierra por debajo de la histéresis y entre ambos conserva su estado")
{
    const double fs = 48000.0;
    // Umbral manual −40; cierra por debajo de −46. Tramos de 0,6 s (hold 40 ms y release 100 ms ya pasaron al final de cada uno).
    const auto x = levels (fs, { { -60.0, 0.6 }, { -30.0, 0.6 }, { -43.0, 0.6 }, { -52.0, 0.6 }, { -43.0, 0.6 }, { -33.0, 0.6 } });
    const auto y = run (x, {}, gateSettings(), fs);
    const auto end = [&] (int seg) { return std::pair { int (fs * (0.6 * seg + 0.4)), int (fs * (0.6 * seg + 0.58)) }; };
    const auto g = [&] (int seg) { const auto [a, b] = end (seg); return gainDb (x, y.l, 0, a, b); };
    INFO ("ganancias: " << g (0) << ", " << g (1) << ", " << g (2) << ", " << g (3) << ", " << g (4) << ", " << g (5));
    REQUIRE (g (0) < -30.0);                    // por debajo del umbral: cerrado (Rango 40 dB)
    REQUIRE (g (1) == Approx (0.0).margin (0.2)); // −30: abierto
    REQUIRE (g (2) == Approx (0.0).margin (0.2)); // −43: entre −46 y −40, sigue abierto
    REQUIRE (g (3) < -30.0);                    // −52: cierra
    REQUIRE (g (4) < -30.0);                    // −43 desde cerrado: no reabre (no pasa de −40)
    REQUIRE (g (5) == Approx (0.0).margin (0.2)); // −33: abre
}

TEST_CASE ("Gate: el hold mantiene abierto el tiempo indicado y el release cierra con su constante")
{
    const double fs = 48000.0;
    for (float hold : { 40.0f, 200.0f })
    {
        auto s = gateSettings();
        s.holdMs = hold;
        s.recoveryMs = 100.0f;
        const auto x = levels (fs, { { -30.0, 1.0 }, { -70.0, 1.0 } }); // el nivel cae en t = 1 s
        const auto y = run (x, {}, s, fs);
        const int drop = int (fs * 1.0);
        const auto g = [&] (double afterMs) { return gainDb (x, y.l, 0, drop + int (fs * afterMs * 0.001) - 100, drop + int (fs * afterMs * 0.001) + 100); };
        INFO ("hold " << hold << ": a la mitad " << g (hold * 0.5) << " dB, a hold+150 ms " << g (hold + 150.0));
        REQUIRE (g (hold * 0.5) > -3.0);          // aún abierto dentro del hold (el detector tarda ~3 ms en bajar)
        REQUIRE (g (hold + 600.0) < -30.0);        // cerrado tras hold y release
    }
}

TEST_CASE ("Gate: ataque y release (10 % a 90 % de la apertura)")
{
    const double fs = 48000.0;
    for (float attack : { 2.0f, 10.0f })
    {
        auto s = gateSettings();
        s.attackMs = attack;
        s.rangeDb = 60.0f;
        const auto x = levels (fs, { { -70.0, 0.5 }, { -20.0, 0.5 } }); // abre en t = 0,5 s
        const auto y = run (x, {}, s, fs);
        // Envolvente de la salida frente a la entrada (ventanas de 0,25 ms): tiempo entre el 10 % y el 90 % de la ganancia lineal.
        const int step = int (fs * 0.00025);
        int t10 = -1, t90 = -1;
        const int start = int (fs * 0.5);
        for (int i = start; i < start + int (fs * 0.1); i += step)
        {
            const double ref = sig::rms (x, i, i + step), out = sig::rms (y.l, i, i + step);
            const double g = out / std::max (ref, 1e-9);
            if (t10 < 0 && g > 0.1 + 0.001) t10 = i;
            if (t90 < 0 && g > 0.9) t90 = i;
        }
        const double ms = double (t90 - t10) / fs * 1000.0;
        INFO ("ataque " << attack << " ms: 10–90 % en " << ms << " ms");
        REQUIRE (t10 > 0);
        REQUIRE (t90 > 0);
        REQUIRE (ms == Approx (attack * 0.6).margin (attack * 0.35)); // rampa de coseno: del 10 % al 90 % = 0,59 del tiempo total
    }
}

TEST_CASE ("Gate: umbral automático = piso de ruido + 6 + 18·Supresión")
{
    const double fs = 48000.0;
    // Ruido de fondo de −60 dBFS de banda ancha durante 4 s (el estimador aprende): en la banda del sidechain (80 Hz a 12 kHz) son −63 dB.
    // Después, un tono de −50 dBFS: 13 dB sobre el piso en banda.
    auto noise = sig::whiteNoise (int (fs * 7.0), -60.0, 4);
    const auto tone = levels (fs, { { -300.0, 4.0 }, { -50.0, 3.0 } });
    auto in = sig::add (noise, tone);
    for (float a : { 0.25f, 0.5f, 1.0f })
    {
        auto s = gateSettings();
        s.gateAuto = true;
        s.amount = a;
        s.rangeDb = 40.0f;
        const auto y = run (in, {}, s, fs);
        const double g = gainDb (in, y.l, 0, int (fs * 6.0), int (fs * 7.0));
        // Margen 6 + 18a: 10,5 (a = 0,25), 15 (a = 0,5) y 24 (a = 1) → un tono 13 dB sobre el piso abre con 0,25 y no con 0,5 ni con 1.
        INFO ("Supresión " << a << ": ganancia del tono " << g << " dB");
        if (a < 0.4f)
            REQUIRE (g > -3.0);
        else
            REQUIRE (g < -10.0);
    }
}

TEST_CASE ("Gate: con lookahead la apertura llega antes que el ataque del audio")
{
    const double fs = 48000.0;
    const auto x = levels (fs, { { -70.0, 0.5 }, { -20.0, 0.5 } });
    double first[2] = {};
    int index = 0;
    for (float look : { 0.0f, 5.0f })
    {
        auto s = gateSettings();
        s.attackMs = 2.0f;
        s.lookaheadMs = look;
        s.rangeDb = 60.0f;
        const auto y = run (x, {}, s, fs);
        const int L = MaruEngine::latencyFor (s, fs);
        REQUIRE (L == int (std::lround (look * 0.001 * fs)));
        // Ganancia en el primer milisegundo del tono, medida frente a la entrada retrasada.
        const int at = int (fs * 0.5) + L;
        first[index++] = gainDb (x, y.l, L, at, at + int (fs * 0.001));
    }
    INFO ("primer ms: sin lookahead " << first[0] << " dB, con 5 ms " << first[1] << " dB");
    REQUIRE (first[0] < -6.0);
    REQUIRE (first[1] > -1.0);
}

TEST_CASE ("Gate sin clics: el salto entre muestras no pasa de 1,3 veces el de la señal")
{
    const double fs = 48000.0;
    const auto x = levels (fs, { { -30.0, 0.3 }, { -70.0, 0.3 }, { -30.0, 0.3 }, { -70.0, 0.3 } });
    auto s = gateSettings();
    s.rangeDb = 60.0f;
    s.lookaheadMs = 3.0f;
    const auto y = run (x, {}, s, fs);
    const double reference = testutil::stepMax (std::vector<float> (x.begin(), x.begin() + int (fs * 0.25)), 0, int (fs * 0.25));
    REQUIRE (testutil::stepMax (y.l, 100, y.l.size()) <= 1.3 * reference);
}

TEST_CASE ("Gate: sidechain externo y filtros HP y LP del detector")
{
    const double fs = 48000.0;
    // Audio principal: tono de 1 kHz a −50 constante. Sidechain: silencio y luego ráfaga a −20.
    const auto main = levels (fs, { { -50.0, 2.0 } });
    const auto sc = levels (fs, { { -80.0, 1.0 }, { -20.0, 1.0 } });
    auto s = gateSettings();
    s.scExt = true;
    s.rangeDb = 40.0f;
    const auto y = run (main, {}, s, fs, &sc);
    REQUIRE (gainDb (main, y.l, 0, int (fs * 0.6), int (fs * 0.9)) < -30.0);          // sidechain en silencio: cerrado aunque el principal esté sobre el umbral
    REQUIRE (gainDb (main, y.l, 0, int (fs * 1.5), int (fs * 1.9)) == Approx (0.0).margin (0.3)); // sidechain fuerte: abierto

    // El HP del sidechain deja fuera un zumbido de 50 Hz fuerte: con HP de 400 Hz el gate no abre; con HP de 20 Hz, sí.
    std::vector<float> hum (size_t (fs * 2.0));
    for (size_t i = 0; i < hum.size(); ++i)
        hum[i] = float (sig::dbToLin (-20.0) * std::sqrt (2.0) * std::sin (2.0 * std::numbers::pi * 50.0 * double (i) / fs));
    auto h = gateSettings();
    h.rangeDb = 40.0f;
    h.scHpHz = 400.0f;
    REQUIRE (gainDb (hum, run (hum, {}, h, fs).l, 0, int (fs * 1.0), int (fs * 2.0)) < -20.0);
    h.scHpHz = 20.0f;
    REQUIRE (gainDb (hum, run (hum, {}, h, fs).l, 0, int (fs * 1.0), int (fs * 2.0)) > -0.5);
}

TEST_CASE ("Gate estéreo: enlace 100 % abre los dos canales a la vez; 0 % cada uno con el suyo")
{
    const double fs = 48000.0;
    const auto loud = levels (fs, { { -25.0, 1.0 } }), quiet = levels (fs, { { -70.0, 1.0 } });
    auto s = gateSettings();
    s.rangeDb = 40.0f;
    s.linkPercent = 100.0f;
    const auto linked = run (loud, quiet, s, fs); // L fuerte, R bajo el umbral
    REQUIRE (gainDb (loud, linked.l, 0, int (fs * 0.5), int (fs * 0.9)) == Approx (0.0).margin (0.5));
    REQUIRE (gainDb (quiet, linked.r, 0, int (fs * 0.5), int (fs * 0.9)) > -6.0); // R se abre con L (la mezcla mid supera el umbral)
    s.linkPercent = 0.0f;
    const auto free = run (loud, quiet, s, fs);
    REQUIRE (gainDb (loud, free.l, 0, int (fs * 0.5), int (fs * 0.9)) == Approx (0.0).margin (0.5));
    REQUIRE (gainDb (quiet, free.r, 0, int (fs * 0.5), int (fs * 0.9)) < -30.0);
}

TEST_CASE ("Gate con Supresión 0 %: la salida es la entrada retrasada")
{
    const double fs = 48000.0;
    const auto x = levels (fs, { { -30.0, 0.4 }, { -70.0, 0.4 }, { -30.0, 0.4 } });
    auto s = gateSettings();
    s.amount = 0.0f;
    s.lookaheadMs = 4.0f;
    const int L = MaruEngine::latencyFor (s, fs);
    const auto y = run (x, {}, s, fs);
    std::vector<float> ref (x.size(), 0.0f);
    for (size_t i = size_t (L); i < x.size(); ++i)
        ref[i] = x[i - size_t (L)];
    REQUIRE (testutil::residualDb (y.l.data() + 2 * L, ref.data() + 2 * L, int (x.size()) - 2 * L) < -110.0);
}

TEST_CASE ("Cambiar entre Supresión y Gate no produce clics y la latencia cambia a la del Gate")
{
    const double fs = 48000.0;
    const auto x = sig::add (sig::voice (int (fs * 6.0), fs, -24.0), sig::whiteNoise (int (fs * 6.0), -55.0, 5));
    MaruEngine engine;
    engine.prepare (fs, 256);
    juce::AudioBuffer<float> buf (1, 256);
    std::vector<float> out (x.size(), 0.0f);
    Settings s;
    for (size_t pos = 0; pos + 256 <= x.size(); pos += 256)
    {
        s.gate = (pos / size_t (fs * 1.0)) % 2 == 1; // alterna cada segundo
        s.lookaheadMs = 3.0f;
        std::copy_n (x.begin() + long (pos), 256, buf.getWritePointer (0));
        engine.process (buf, nullptr, s);
        std::copy_n (buf.getReadPointer (0), 256, out.begin() + long (pos));
    }
    const double reference = testutil::stepMax (x, 100, x.size());
    REQUIRE (testutil::stepMax (out, 100, out.size()) <= 1.3 * reference);
    Settings g;
    g.gate = true;
    g.lookaheadMs = 3.0f;
    REQUIRE (MaruEngine::latencyFor (g, fs) == 144);
    g.gate = false;
    REQUIRE (MaruEngine::latencyFor (g, fs) == 1024);
}
