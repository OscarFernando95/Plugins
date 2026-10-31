#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "Loudness.h"
#include "TestUtils.h"

using namespace testutil;

namespace
{
/** Alimenta el medidor con una senoidal estéreo de `seconds` segundos a `dbfs` (pico), en bloques de 480. */
void feed (ferb::LoudnessMeter& meter, double seconds, double dbfs, double fs = 48000.0, double freq = 1000.0)
{
    const int total = static_cast<int> (seconds * fs);
    const double amp = fromDb (dbfs);
    double phase = 0.0;
    const double inc = 2.0 * std::numbers::pi * freq / fs;
    std::vector<float> l (480), r (480);
    for (int pos = 0; pos < total; pos += 480)
    {
        const int n = std::min (480, total - pos);
        for (int i = 0; i < n; ++i, phase += inc)
            l[size_t (i)] = r[size_t (i)] = static_cast<float> (amp * std::sin (phase));
        const float* chans[2] = { l.data(), r.data() };
        meter.process (chans, n);
    }
}
} // namespace

TEST_CASE ("Filtro K: a 48 kHz coincide con la tabla de ITU-R BS.1770")
{
    const auto shelf = ferb::KWeighting::shelf (48000.0);
    CHECK (shelf.b0 == Catch::Approx (1.53512485958697).epsilon (1e-9));
    CHECK (shelf.b1 == Catch::Approx (-2.69169618940638).epsilon (1e-9));
    CHECK (shelf.b2 == Catch::Approx (1.19839281085285).epsilon (1e-9));
    CHECK (shelf.a1 == Catch::Approx (-1.69065929318241).epsilon (1e-9));
    CHECK (shelf.a2 == Catch::Approx (0.73248077421585).epsilon (1e-9));

    const auto hp = ferb::KWeighting::highPass (48000.0);
    CHECK (hp.a1 == Catch::Approx (-1.99004745483398).epsilon (1e-9));
    CHECK (hp.a2 == Catch::Approx (0.99007225036621).epsilon (1e-9));
}

TEST_CASE ("Sonoridad: una senoidal estéreo de 1 kHz marca su nivel en LUFS")
{
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (double level : { -18.0, -23.0, -33.0 })
        {
            ferb::LoudnessMeter meter;
            meter.prepare (fs, 2);
            feed (meter, 20.0, level, fs);

            INFO ("fs " << fs << ", nivel " << level);
            // La ficha de Nel pide −18,0 LUFS para −18 dBFS; EBU Tech 3341 admite ±0,1 LU.
            CHECK (meter.momentary() == Catch::Approx (level).margin (0.1));
            CHECK (meter.shortTerm() == Catch::Approx (level).margin (0.1));
            CHECK (meter.integrated() == Catch::Approx (level).margin (0.1));
            CHECK (meter.range() < 0.3f);
        }
}

TEST_CASE ("Sonoridad: sin datos hasta tener una ventana completa")
{
    ferb::LoudnessMeter meter;
    meter.prepare (48000.0, 2);
    feed (meter, 0.3, -23.0);
    CHECK (meter.momentary() == ferb::LoudnessMeter::noValue);
    CHECK (meter.integrated() == ferb::LoudnessMeter::noValue);
    feed (meter, 0.2, -23.0);
    CHECK (meter.momentary() != ferb::LoudnessMeter::noValue);
    CHECK (meter.shortTerm() == ferb::LoudnessMeter::noValue);
}

TEST_CASE ("Sonoridad: la puerta relativa ignora los pasajes silenciosos (EBU Tech 3341, caso con puertas)")
{
    ferb::LoudnessMeter meter;
    meter.prepare (48000.0, 2);
    feed (meter, 10.0, -36.0);
    feed (meter, 60.0, -23.0);
    feed (meter, 10.0, -36.0);
    CHECK (meter.integrated() == Catch::Approx (-23.0).margin (0.1));

    ferb::LoudnessMeter quiet;
    quiet.prepare (48000.0, 2);
    feed (quiet, 20.0, -90.0); // bajo la puerta absoluta
    CHECK (quiet.integrated() == ferb::LoudnessMeter::noValue);
}

TEST_CASE ("Sonoridad: el rango de dos niveles separados 10 LU da unos 10 LU")
{
    ferb::LoudnessMeter meter;
    meter.prepare (48000.0, 2);
    feed (meter, 30.0, -20.0);
    feed (meter, 30.0, -30.0);
    CHECK (meter.range() == Catch::Approx (10.0).margin (1.0));
}

TEST_CASE ("Sonoridad: mono pesa un canal y el reinicio borra la integrada")
{
    ferb::LoudnessMeter mono;
    mono.prepare (48000.0, 1);
    const auto x = sine (48000 * 5, 1000.0, 48000.0, fromDb (-18.0));
    for (size_t pos = 0; pos + 480 <= x.size(); pos += 480)
    {
        const float* chans[1] = { x.data() + pos };
        mono.process (chans, 480);
    }
    CHECK (mono.integrated() == Catch::Approx (-21.0).margin (0.1)); // un solo canal: −3 dB frente al estéreo

    mono.requestReset();
    const float* chans[1] = { x.data() };
    mono.process (chans, 480);
    CHECK (mono.integrated() == ferb::LoudnessMeter::noValue);
    CHECK (mono.elapsedSeconds() == 0.0f);
}

TEST_CASE ("Sonoridad: no produce NaN con silencio, DC ni valores extremos")
{
    ferb::LoudnessMeter meter;
    meter.prepare (48000.0, 2);
    std::vector<float> zeros (4800, 0.0f), dc (4800, 1.0f);
    const float* z[2] = { zeros.data(), zeros.data() };
    const float* d[2] = { dc.data(), dc.data() };
    for (int i = 0; i < 20; ++i) { meter.process (z, 4800); meter.process (d, 4800); }
    CHECK (std::isfinite (meter.momentary()));
    CHECK (std::isfinite (meter.shortTerm()));
    CHECK (std::isfinite (meter.integrated()));
    CHECK (std::isfinite (meter.range()));
}

#include "LoudnessHistory.h"

TEST_CASE ("Historial de sonoridad: tres niveles que cubren 10 min, 2 h y 10 h con memoria fija")
{
    ferb::LoudnessHistory h;
    CHECK (ferb::LoudnessHistory::tiers[0].capacity * ferb::LoudnessHistory::tiers[0].seconds == 600);
    CHECK (ferb::LoudnessHistory::tiers[1].capacity * ferb::LoudnessHistory::tiers[1].seconds == 7200);
    CHECK (ferb::LoudnessHistory::tiers[2].capacity * ferb::LoudnessHistory::tiers[2].seconds == 36000);

    // 11 h de programa: −20 LUFS constante con un pico de −3 dBTP cada hora.
    for (int s = 0; s < 11 * 3600; ++s)
        h.push (-20.0f, -20.0f, s % 3600 == 0 ? -3.0f : -30.0f);

    const auto fine = h.snapshot (0), mid = h.snapshot (1), coarse = h.snapshot (2);
    CHECK (fine.size() == 600);
    CHECK (mid.size() == 720);
    CHECK (coarse.size() == 600);
    for (const auto& p : coarse)
    {
        CHECK (p.momentary == Catch::Approx (-20.0f).margin (0.01));
        CHECK (p.shortTerm == Catch::Approx (-20.0f).margin (0.01));
    }

    // El máximo de true peak se conserva al pasar a un nivel más grueso: hay un −3 por hora en el de 10 h.
    int peaks = 0;
    for (const auto& p : coarse)
        peaks += p.truePeak > -10.0f ? 1 : 0;
    CHECK (peaks == 10);

    h.clear();
    CHECK (h.snapshot (0).empty());
}

TEST_CASE ("Historial de sonoridad: promedia en energía y trata «sin dato» como energía cero")
{
    ferb::LoudnessHistory h;
    // 10 s: cinco a −20 LUFS y cinco sin dato → un punto de 10 s a −20 − 3,01 LUFS.
    for (int s = 0; s < 10; ++s)
        h.push (s < 5 ? -20.0f : ferb::LoudnessHistory::noValue, -20.0f, -6.0f);
    const auto mid = h.snapshot (1);
    REQUIRE (mid.size() == 1);
    CHECK (mid[0].momentary == Catch::Approx (-23.01f).margin (0.02));
    CHECK (mid[0].shortTerm == Catch::Approx (-20.0f).margin (0.02));
    CHECK (mid[0].truePeak == Catch::Approx (-6.0f).margin (0.001));
}
