#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <random>

#include <Biquad.h>

#include "AliasingMeter.h"
#include "TestUtils.h"
#include "dsp/NelEngine.h"

using namespace testutil;
using Catch::Approx;

namespace
{
constexpr int block = 256;

struct Stereo
{
    std::vector<float> l, r;
};

Stereo runStereo (nel::NelEngine& engine, const Stereo& in, const nel::Settings& s, int chunk = block)
{
    Stereo out { std::vector<float> (in.l.size()), std::vector<float> (in.l.size()) };
    juce::AudioBuffer<float> buf (2, chunk);
    for (size_t pos = 0; pos < in.l.size(); pos += size_t (chunk))
    {
        const int n = int (std::min<size_t> (size_t (chunk), in.l.size() - pos));
        buf.setSize (2, n, false, false, true);
        std::copy_n (in.l.begin() + long (pos), n, buf.getWritePointer (0));
        std::copy_n (in.r.begin() + long (pos), n, buf.getWritePointer (1));
        engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), n, out.l.begin() + long (pos));
        std::copy_n (buf.getReadPointer (1), n, out.r.begin() + long (pos));
    }
    return out;
}

Stereo runFresh (const Stereo& in, const nel::Settings& s, double sr = 48000.0)
{
    nel::NelEngine engine;
    engine.prepare (sr, block);
    return runStereo (engine, in, s);
}

Stereo dual (const std::vector<float>& x) { return { x, x }; }

/** True peak con un sobremuestreo propio de `factor`× (independiente del medidor del motor). El estándar
    ITU-R BS.1770 usa 4×; 8× es una comprobación más estricta. */
double truePeak (const std::vector<float>& x, size_t from, int factor = 4)
{
    ferb::UpN up;
    up.prepare (block);
    up.setConfig (factor, false);
    std::vector<float> os (size_t (block) * size_t (factor));
    double peak = 0.0;
    for (size_t pos = 0; pos + block <= x.size(); pos += block)
    {
        up.process (x.data() + pos, os.data(), block);
        if (pos >= from)
            for (float v : os)
                peak = std::max (peak, double (std::abs (v)));
    }
    return peak;
}
} // namespace

// ------------------------------------------------------------------ etapa A

TEST_CASE ("Etapa A: la ganancia suavizada nunca supera la requerida de la muestra a la que se aplica")
{
    std::mt19937 rng (4);
    std::uniform_real_distribution<float> dist (0.0f, 1.0f);
    for (int window : { 1, 2, 3, 4, 5, 8, 17, 64, 241, 1000 })
    {
        nel::PeakStage stage;
        stage.prepare (window + 2);
        stage.configure (window);

        std::vector<float> required;
        std::vector<float> smooth;
        for (int n = 0; n < 6000; ++n)
        {
            // Rachas tranquilas con picos aislados y pasajes largos de reducción.
            float g = 1.0f;
            if (dist (rng) < 0.02f) g = 0.1f + 0.9f * dist (rng);
            if ((n / 700) % 3 == 1) g = std::min (g, 0.5f + 0.1f * dist (rng));
            required.push_back (g);
            smooth.push_back (stage.process (g));
        }

        for (size_t n = size_t (window - 1); n < required.size(); ++n)
        {
            INFO ("ventana " << window << ", n " << n);
            REQUIRE (smooth[n] <= required[n - size_t (window - 1)] + 1e-6f);
            REQUIRE (smooth[n] <= 1.0f + 1e-6f);
        }
    }

    nel::PeakStage idle;
    idle.prepare (100);
    idle.configure (50);
    for (int n = 0; n < 500; ++n)
        REQUIRE (idle.process (1.0f) == 1.0f);
}

TEST_CASE ("Codo suave: continuo, monótono y cero bajo el codo")
{
    for (float knee : { 0.0f, 1.0f, 6.0f, 12.0f })
    {
        float prev = 0.0f;
        for (float x = -30.0f; x <= 30.0f; x += 0.01f)
        {
            const float y = nel::softKnee (x, knee);
            REQUIRE (y >= prev - 1e-4f);
            REQUIRE (y >= 0.0f);
            prev = y;
        }
        CHECK (nel::softKnee (-knee, knee) == Approx (0.0f).margin (1e-5));
        CHECK (nel::softKnee (knee, knee) == Approx (knee).margin (1e-4));
    }
}

// ------------------------------------------------------------------ techo

TEST_CASE ("Techo: ningún over con los ocho modos, 4× y +24 dB de ganancia")
{
    std::mt19937 rng (11);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
    const int n = 48000 * 4;

    std::vector<float> noiseSig (static_cast<size_t> (n)), pulses (static_cast<size_t> (n), 0.0f);
    // Ruido con ancho de banda de programa (paso bajo de 4.º orden a 16 kHz): el blanco hasta Nyquist
    // cae en la banda de transición del sobremuestreo y no es material musical.
    {
        const auto lp = ferb::Biquad::lowPass (48000.0, 16000.0);
        ferb::Biquad a = lp, b = lp;
        for (auto& v : noiseSig) v = 2.0f * b.process (a.process (0.4f * dist (rng)));
    }
    for (size_t i = 0; i < pulses.size(); i += 5000) pulses[i] = 0.9f;

    struct Signal { const char* name; std::vector<float> x; };
    const std::vector<Signal> signals = {
        { "ruido", noiseSig }, { "senoidal 1 kHz", sine (n, 1000.0, 48000.0, 0.5) },
        { "senoidal 15 kHz", sine (n, 15000.0, 48000.0, 0.5) }, { "impulsos", pulses },
    };

    double worst4 = -400.0, worst8 = -400.0;
    for (int m = 0; m < nel::numModes; ++m)
        for (const auto& sig : signals)
        {
            nel::Settings s;
            s.mode = m;
            s.gainDb = 24.0f;
            s.ceilingDb = -1.0f;
            s.lookaheadMs = nel::modes[size_t (m)].lookaheadMs;
            s.attackMs = nel::modes[size_t (m)].attackMs;
            s.releaseMs = nel::modes[size_t (m)].releaseMs;
            s.kneeDb = nel::modes[size_t (m)].kneeDb;
            s.oversample = 2;

            nel::NelEngine engine;
            engine.prepare (48000.0, block);
            const auto out = runStereo (engine, dual (sig.x), s);
            const double ceilLin = fromDb (-1.0);
            const double peak = truePeak (out.l, 48000), strict = truePeak (out.l, 48000, 8);
            INFO ("modo " << nel::modes[size_t (m)].name << ", " << sig.name << ": true peak 4× " << toDb (peak) << " dBTP, 8× " << toDb (strict) << " dBTP, margen " << engine.getSafetyMarginDb());
            worst4 = std::max (worst4, toDb (peak));
            worst8 = std::max (worst8, toDb (strict));
            CHECK (peak <= ceilLin * 1.001); // 0,01 dB
            CHECK (strict <= ceilLin * 1.03); // con 8× el estándar de 4× puede subestimar hasta ~0,25 dB
        }
    WARN ("Techo −1 dBTP, +24 dB, 8 modos y 4 señales: peor true peak 4× " << worst4 << " dBTP, con 8× " << worst8 << " dBTP");
}

TEST_CASE ("Techo: también con 1× (pico de muestra) y con 32×")
{
    const auto x = noise (48000 * 3, 0.5f, 5);
    for (int os : { 0, 1, 5 })
    {
        nel::Settings s;
        s.gainDb = 18.0f;
        s.oversample = os;
        s.truePeak = os > 0;
        const auto out = runFresh (dual (x), s);
        float peak = 0.0f;
        for (size_t i = 48000; i < out.l.size(); ++i)
            peak = std::max (peak, std::abs (out.l[i]));
        INFO ("sobremuestreo " << (1 << os) << "×");
        CHECK (peak <= float (fromDb (-1.0)) * 1.0001f);
    }
}

// ------------------------------------------------------------------ transparencia y latencia

TEST_CASE ("Transparencia: con ganancia 0 y señal bajo el techo la salida es la entrada retrasada")
{
    const auto x = sine (48000 * 2, 1000.0, 48000.0, 0.25);
    for (int os : { 0, 2 })
    {
        nel::Settings s;
        s.oversample = os;
        const int latency = nel::NelEngine::latencyFor (s, 48000.0);
        const auto out = runFresh (dual (x), s);
        double worst = -400.0;
        for (size_t i = 24000; i + size_t (latency) < x.size(); ++i)
            worst = std::max (worst, toDb (std::abs (out.l[i + size_t (latency)] - x[i])));
        INFO ("sobremuestreo " << (1 << os) << "×: " << worst << " dBFS");
        // Con 1× es exacto; con sobremuestreo el rizado de los filtros limita a ~−100 dB.
        CHECK (worst < (os == 0 ? -130.0 : -90.0));
    }
}

TEST_CASE ("Latencia: un impulso sale en la muestra informada, con todos los factores, fases y lookaheads")
{
    for (bool minPhase : { false, true })
        for (int os : { 0, 1, 2, 3, 4, 5 })
            for (float look : { 0.0f, 1.0f, 3.5f })
            {
                nel::Settings s;
                s.oversample = os;
                s.minimumPhase = minPhase;
                s.lookaheadMs = look;
                const int latency = nel::NelEngine::latencyFor (s, 48000.0);
                std::vector<float> x (24000, 0.0f);
                x[10000] = 0.1f;
                const auto out = runFresh (dual (x), s);

                // En render offline sube un escalón de sobremuestreo, pero la latencia declarada no cambia.
                auto offline = s;
                offline.nonRealtime = true;
                CHECK (nel::NelEngine::latencyFor (offline, 48000.0) == latency);
                const auto outOffline = runFresh (dual (x), offline);
                size_t peakOffline = 0;
                for (size_t i = 0; i < outOffline.l.size(); ++i)
                    if (std::abs (outOffline.l[i]) > std::abs (outOffline.l[peakOffline]))
                        peakOffline = i;
                CHECK (std::abs (int (peakOffline) - 10000 - latency) <= (minPhase ? 1 : 0));

                size_t peak = 0;
                for (size_t i = 0; i < out.l.size(); ++i)
                    if (std::abs (out.l[i]) > std::abs (out.l[peak]))
                        peak = i;
                INFO ("factor " << (1 << os) << ", mínima " << minPhase << ", lookahead " << look << ", latencia " << latency << ", pico en " << int (peak) - 10000);
                CHECK (std::abs (int (peak) - 10000 - latency) <= (minPhase ? 1 : 0));
            }
}

TEST_CASE ("Latencia: a 96 y 192 kHz el sobremuestreo efectivo baja (8× y 4×)")
{
    nel::Settings s;
    s.oversample = 5;
    CHECK (nel::NelEngine::effectiveExponent (s, 48000.0) == 5);
    CHECK (nel::NelEngine::effectiveExponent (s, 96000.0) == 3);
    CHECK (nel::NelEngine::effectiveExponent (s, 192000.0) == 2);
    s.oversample = 2;
    s.nonRealtime = true;
    CHECK (nel::NelEngine::effectiveExponent (s, 48000.0) == 3); // en render sube un escalón
    s.offlineBoost = false;
    CHECK (nel::NelEngine::effectiveExponent (s, 48000.0) == 2);
    s.offlineBoost = true;
    s.oversample = 0;
    CHECK (nel::NelEngine::effectiveExponent (s, 48000.0) == 0); // 1× = latencia mínima, no se sube
    s.lookaheadMs = 0.0f;
    CHECK (nel::NelEngine::latencyFor (s, 48000.0) == 0);
}

// ------------------------------------------------------------------ funciones

TEST_CASE ("Ganancia unitaria, Audición y Bypass")
{
    const auto x = sine (48000 * 2, 440.0, 48000.0, 0.2);
    nel::Settings s;
    s.gainDb = 6.0f;
    s.oversample = 0;
    s.lookaheadMs = 1.0f;
    const int latency = nel::NelEngine::latencyFor (s, 48000.0);

    // Ganancia unitaria: sin reducción, la salida iguala a la entrada.
    s.unityGain = true;
    auto out = runFresh (dual (x), s);
    CHECK (measureAmplitude (out.l.data() + 48000, 24000, 440.0, 48000.0) == Approx (0.2).epsilon (0.001));

    // Audición: sin reducción solo queda ruido de redondeo.
    s.unityGain = false;
    s.audition = true;
    s.gainDb = 0.0f;
    out = runFresh (dual (x), s);
    float peak = 0.0f;
    for (size_t i = 24000; i < out.l.size(); ++i) peak = std::max (peak, std::abs (out.l[i]));
    CHECK (toDb (peak) < -120.0);

    // Audición con limitación: lo que se oye es lo que el limitador quita (≠ 0).
    s.gainDb = 12.0f;
    s.ceilingDb = -6.0f;
    out = runFresh (dual (x), s);
    peak = 0.0f;
    for (size_t i = 24000; i < out.l.size(); ++i) peak = std::max (peak, std::abs (out.l[i]));
    CHECK (peak > 0.1f);

    // Bypass: la entrada retrasada, sin tocar.
    s = {};
    s.gainDb = 24.0f;
    s.bypass = true;
    s.oversample = 2;
    out = runFresh (dual (x), s);
    double worst = -400.0;
    const size_t bypassLatency = size_t (nel::NelEngine::latencyFor (s, 48000.0));
    for (size_t i = 24000; i + bypassLatency < x.size(); ++i)
        worst = std::max (worst, toDb (std::abs (out.l[i + bypassLatency] - x[i])));
    INFO ("bypass: " << worst << " dBFS");
    CHECK (worst < -130.0);
}

TEST_CASE ("Dither: la salida cae en la rejilla de la profundidad elegida; el perfil fuerte desplaza ruido a agudos")
{
    const auto x = sine (48000 * 2, 440.0, 48000.0, 0.3);
    for (int shape : { 0, 3 })
    {
        nel::Settings s;
        s.oversample = 0;
        s.ditherBits = 16;
        s.ditherShape = shape;
        const auto out = runFresh (dual (x), s);
        const double lsb = std::ldexp (1.0, 1 - 16);
        double offGrid = 0.0;
        const size_t lat = size_t (nel::NelEngine::latencyFor (s, 48000.0));
        for (size_t i = 24000; i < out.l.size(); ++i)
        {
            const double q = out.l[i] / lsb;
            offGrid = std::max (offGrid, std::abs (q - std::round (q)) * lsb);
        }
        INFO ("perfil " << shape);
        CHECK (offGrid < 2e-7); // la rejilla de 16 bits (el float la representa casi exacta)

        // Ruido de baja frecuencia (por debajo de 3 kHz, sin la senoidal) frente a alta (por encima de 15 kHz).
        std::vector<double> diff (24000);
        for (size_t i = 0; i < diff.size(); ++i)
            diff[i] = double (out.l[24000 + i + lat]) - double (x[24000 + i]);
        double low = 0.0, high = 0.0;
        for (double f = 1000.0; f < 3000.0; f += 50.0)
        {
            double re = 0.0, im = 0.0;
            for (size_t i = 0; i < diff.size(); ++i) { re += diff[i] * std::cos (2 * 3.14159265 * f * double (i) / 48000.0); im += diff[i] * std::sin (2 * 3.14159265 * f * double (i) / 48000.0); }
            low += re * re + im * im;
        }
        for (double f = 15000.0; f < 20000.0; f += 100.0)
        {
            double re = 0.0, im = 0.0;
            for (size_t i = 0; i < diff.size(); ++i) { re += diff[i] * std::cos (2 * 3.14159265 * f * double (i) / 48000.0); im += diff[i] * std::sin (2 * 3.14159265 * f * double (i) / 48000.0); }
            high += re * re + im * im;
        }
        if (shape == 3)
            CHECK (high > 10.0 * low);
    }
}

// ------------------------------------------------------------------ sin clics

TEST_CASE ("Sin clics: mover controles y cambiar de estructura con audio sonando")
{
    using Mod = std::function<void (nel::Settings&)>;
    struct Case { const char* name; Mod from, to; };
    const std::vector<Case> cases = {
        { "Ganancia", [] (auto& s) { s.gainDb = 6; }, [] (auto& s) { s.gainDb = 14; } },
        { "Techo", [] (auto& s) { s.ceilingDb = -3; }, [] (auto& s) { s.ceilingDb = -8; } },
        { "Carácter", [] (auto& s) { s.characterPercent = 0; }, [] (auto& s) { s.characterPercent = 80; } },
        { "Ataque", [] (auto& s) { s.attackMs = 3; }, [] (auto& s) { s.attackMs = 30; } },
        { "Release", [] (auto& s) { s.releaseMs = 300; }, [] (auto& s) { s.releaseMs = 1500; } },
        { "Knee", [] (auto& s) { s.kneeDb = 0; }, [] (auto& s) { s.kneeDb = 10; } },
        { "Enlaces", [] (auto& s) { s.linkTransient = 100; s.linkRelease = 100; }, [] (auto& s) { s.linkTransient = 0; s.linkRelease = 0; } },
        { "Modo", [] (auto& s) { s.mode = 0; }, [] (auto& s) { s.mode = 5; } },
        { "Sobremuestreo", [] (auto& s) { s.oversample = 1; }, [] (auto& s) { s.oversample = 4; } },
        { "Fase", [] (auto& s) { s.minimumPhase = false; }, [] (auto& s) { s.minimumPhase = true; } },
        { "Lookahead", [] (auto& s) { s.lookaheadMs = 1; }, [] (auto& s) { s.lookaheadMs = 4; } },
        { "Filtro DC", [] (auto& s) { s.dcFilter = false; }, [] (auto& s) { s.dcFilter = true; } },
        { "Ganancia unitaria", [] (auto& s) { s.unityGain = false; }, [] (auto& s) { s.unityGain = true; } },
        { "Bypass", [] (auto& s) { s.bypass = false; }, [] (auto& s) { s.bypass = true; } },
    };

    const auto tone = sine (48000 * 4, 440.0, 48000.0, 0.3);
    for (const auto& c : cases)
    {
        nel::Settings a, b;
        a.gainDb = b.gainDb = 10.0f;
        a.ceilingDb = b.ceilingDb = -3.0f;
        c.from (a);
        c.to (b);

        const auto steady = [&] (const nel::Settings& s)
        {
            const auto o = runFresh (dual (tone), s);
            return stepMax (o.l, 100000, o.l.size());
        };

        nel::NelEngine engine;
        engine.prepare (48000.0, block);
        std::vector<float> out (tone.size());
        juce::AudioBuffer<float> buf (2, block);
        for (size_t pos = 0; pos + block <= tone.size(); pos += block)
        {
            std::copy_n (tone.begin() + long (pos), block, buf.getWritePointer (0));
            std::copy_n (tone.begin() + long (pos), block, buf.getWritePointer (1));
            engine.process (buf, pos < 96000 ? a : b);
            std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
        }
        const double reference = std::max (steady (a), steady (b));
        const double jump = stepMax (out, 48000, out.size() - out.size() % block);
        INFO (c.name << ": salto " << jump << ", estable " << reference);
        CHECK (jump <= 1.3 * reference);
    }
}

// ------------------------------------------------------------------ robustez

TEST_CASE ("Robustez: silencio, continua, NaN, infinitos y subnormales; tasas y bloques extremos")
{
    juce::Random rng (3);
    for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int chunk : { 1, 7, 64, 255, 256, 257, 4096 })
        {
            nel::NelEngine engine;
            engine.prepare (sr, chunk);
            juce::AudioBuffer<float> buf (2, chunk);

            for (int round = 0; round < 30; ++round)
            {
                nel::Settings s;
                s.gainDb = rng.nextFloat() * 24.0f;
                s.ceilingDb = -rng.nextFloat() * 30.0f;
                s.mode = rng.nextInt (8);
                s.characterPercent = rng.nextFloat() * 100.0f;
                s.lookaheadMs = rng.nextFloat() * 5.0f;
                s.attackMs = 0.1f + rng.nextFloat() * 499.9f;
                s.releaseMs = 10.0f + rng.nextFloat() * 1990.0f;
                s.releaseAuto = rng.nextBool();
                s.kneeDb = rng.nextFloat() * 12.0f;
                s.linkTransient = rng.nextFloat() * 100.0f;
                s.linkRelease = rng.nextFloat() * 100.0f;
                s.truePeak = rng.nextBool();
                s.oversample = rng.nextInt (6);
                s.minimumPhase = rng.nextBool();
                s.ditherBits = rng.nextBool() ? 0 : 16 + 2 * rng.nextInt (4);
                s.ditherShape = rng.nextInt (4);
                s.dcFilter = rng.nextBool();
                s.unityGain = rng.nextBool();
                s.audition = rng.nextInt (5) == 0;
                s.bypass = rng.nextInt (6) == 0;
                s.nonRealtime = rng.nextInt (4) == 0;

                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < chunk; ++i)
                    {
                        float v;
                        switch (round % 6)
                        {
                            case 0: v = 0.0f; break;
                            case 1: v = 1.0f; break;
                            case 2: v = (i % 5 == 0) ? std::numeric_limits<float>::quiet_NaN() : 0.3f; break;
                            case 3: v = (i % 7 == 0) ? std::numeric_limits<float>::infinity() : -0.3f; break;
                            case 4: v = 1e-40f; break;
                            default: v = rng.nextFloat() * 2.0f - 1.0f; break;
                        }
                        buf.setSample (ch, i, v);
                    }
                engine.process (buf, s);
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < chunk; ++i)
                    {
                        const float v = buf.getSample (ch, i);
                        INFO ("sr " << sr << ", bloque " << chunk << ", ronda " << round);
                        REQUIRE (std::isfinite (v));
                        REQUIRE (std::abs (v) < 40.0f);
                    }
            }
        }
}

// ------------------------------------------------------------------ informes

TEST_CASE ("Informe: aliasing y CPU", "[.report]")
{
    // Senoidal empujada 6 dB sobre el techo (ficha: de 10 a 20 kHz), en el modo por defecto y en el más agresivo.
    for (int mode : { 0, 5 })
        for (double f : { 10000.0, 14000.0 })
            for (int os : { 0, 1, 2, 3 })
            {
                nel::Settings s;
                s.gainDb = 11.0f;
                s.ceilingDb = -1.0f;
                s.oversample = os;
                s.mode = mode;
                s.lookaheadMs = nel::modes[size_t (mode)].lookaheadMs;
                s.attackMs = nel::modes[size_t (mode)].attackMs;
                const auto out = runFresh (dual (sine (65536 + 16384, f, 48000.0, 0.5)), s);
                WARN (nel::modes[size_t (mode)].name << ", " << f << " Hz, " << (1 << os) << "×: " << aliasingDb (out.l, 16384, f, 48000.0) << " dBc");
            }

    const double fs = 48000.0;
    const int seconds = 20;
    const Stereo in { noise (int (fs) * seconds, 0.4f, 1), noise (int (fs) * seconds, 0.4f, 2) };
    for (int os : { 0, 1, 2, 3, 4, 5 })
        for (bool minPhase : { false, true })
        {
            nel::Settings s;
            s.gainDb = 12.0f;
            s.oversample = os;
            s.minimumPhase = minPhase;
            nel::NelEngine engine;
            engine.prepare (fs, block);
            const auto t0 = std::chrono::steady_clock::now();
            runStereo (engine, in, s);
            const double elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            WARN ((1 << os) << "×, " << (minPhase ? "mínima" : "lineal") << ": " << 100.0 * elapsed / seconds << " % de un núcleo");
        }
}

TEST_CASE ("Aliasing: senoidal 6 dB sobre el techo, de 10 a 18 kHz: −80 dBc con 4× hasta 16 kHz (Agresivo −72), y −80 con 8× en todos los modos")
{
    double worst4 = -400.0, worst4Aggressive = -400.0, worst8 = -400.0;
    for (int mode = 0; mode < nel::numModes; ++mode)
        for (double f : { 10000.0, 12000.0, 14000.0, 16000.0, 18000.0 })
            for (int os : { 2, 3 })
            {
                nel::Settings s;
                s.gainDb = 11.0f;
                s.ceilingDb = -1.0f;
                s.oversample = os;
                s.mode = mode;
                s.lookaheadMs = nel::modes[size_t (mode)].lookaheadMs;
                s.attackMs = nel::modes[size_t (mode)].attackMs;
                s.releaseMs = nel::modes[size_t (mode)].releaseMs;
                s.kneeDb = nel::modes[size_t (mode)].kneeDb;
                const auto out = runFresh (dual (sine (65536 + 16384, f, 48000.0, 0.5)), s);
                const double alias = aliasingDb (out.l, 16384, f, 48000.0);
                INFO (nel::modes[size_t (mode)].name << ", " << f << " Hz, " << (1 << os) << "×: " << alias << " dBc");
                if (os == 3)
                {
                    worst8 = std::max (worst8, alias);
                    CHECK (alias < -80.0);
                }
                else if (mode == 5)
                {
                    worst4Aggressive = std::max (worst4Aggressive, alias);
                    CHECK (alias < (f > 17000.0 ? -68.0 : -72.0));
                }
                else
                {
                    worst4 = std::max (worst4, alias);
                    CHECK (alias < (f > 17000.0 ? -77.0 : -80.0)); // a 18 kHz, Moderno llega a −78,4 dBc
                }
            }
    WARN ("Peor aliasing: 4× sin Agresivo " << worst4 << " dBc, 4× Agresivo " << worst4Aggressive << " dBc, 8× todos " << worst8 << " dBc");
}

TEST_CASE ("Lookahead: la reducción empieza `lookahead` ms antes del pico y al llegar el pico ya está al techo")
{
    const double fs = 48000.0;
    for (int os : { 0, 2 })
        for (float look : { 0.5f, 1.0f, 2.0f, 5.0f })
        {
            // Continua de 0,1 que salta a 1,0: con el techo en −6 dB la ganancia necesaria es 0,5.
            const int stepAt = 24000;
            std::vector<float> x (48000, 0.1f);
            for (size_t i = size_t (stepAt); i < x.size(); ++i)
                x[i] = 1.0f;

            nel::Settings s;
            s.mode = 7; // Seguro: sin recorte previo
            s.ceilingDb = -6.0f;
            s.oversample = os;
            s.lookaheadMs = look;
            s.truePeak = false;
            const int latency = nel::NelEngine::latencyFor (s, fs);
            const auto out = runFresh (dual (x), s);

            // Salida alineada con la entrada: y[n] = out[n + latencia].
            const auto y = [&] (int n) { return double (out.l[size_t (n + latency)]); };
            int dipStart = -1;
            for (int n = stepAt - int (0.0075 * fs); n < stepAt; ++n)
                if (y (n) < 0.1 * 0.995)
                {
                    dipStart = n;
                    break;
                }

            const double measuredMs = dipStart < 0 ? 0.0 : double (stepAt - dipStart) / fs * 1000.0;
            INFO ("sobremuestreo " << (1 << os) << "×, lookahead " << look << " ms: la reducción empieza " << measuredMs << " ms antes");
            REQUIRE (dipStart >= 0);
            // El arranque de la rampa es cuadrático: un umbral del 0,5 % la detecta algo tarde (~7 % de la ventana).
            CHECK (measuredMs == Approx (look).margin (0.15 + 0.08 * look));
            // Al llegar el pico, la ganancia ya llegó al techo y ninguna muestra lo supera.
            CHECK (y (stepAt) <= 0.5 * 1.001);
            double peak = 0.0;
            for (int n = stepAt; n < stepAt + 4000; ++n)
                peak = std::max (peak, std::abs (y (n)));
            CHECK (peak <= 0.5 * 1.002);
        }
}

TEST_CASE ("True peak del medidor: senoidal a fs/4 con fase de 45° (pico de muestra 3 dB bajo el real), como los casos de EBU Tech 3341")
{
    // Con fase de 45° las muestras valen ±0,707·A y el pico real es A: un medidor de pico de muestra
    // leería 3 dB de menos; el de true peak debe leer A (tolerancia de la norma: +0,2 / −0,4 dB).
    for (double levelDb : { -12.0, -6.0, -3.0, -1.5 })
    {
        const int n = 48000 * 2;
        std::vector<float> x (static_cast<size_t> (n));
        for (int i = 0; i < n; ++i)
            x[size_t (i)] = float (fromDb (levelDb) * std::sin (2.0 * 3.14159265358979 * 0.25 * i + 3.14159265358979 / 4.0));

        nel::Settings s;
        s.oversample = 0;
        s.lookaheadMs = 0.0f;
        s.ceilingDb = 0.0f; // sin limitación: el medidor lee la señal tal cual
        nel::NelEngine engine;
        engine.prepare (48000.0, block);
        runStereo (engine, dual (x), s);

        const double tp = toDb (engine.getTruePeak (0).consume());
        INFO ("nivel real " << levelDb << " dBTP, lectura " << tp);
        CHECK (tp >= levelDb - 0.4);
        CHECK (tp <= levelDb + 0.2);

        // El pico de muestra de la misma señal queda 3 dB por debajo.
        float samplePeak = 0.0f;
        for (float v : x)
            samplePeak = std::max (samplePeak, std::abs (v));
        CHECK (toDb (samplePeak) == Approx (levelDb - 3.01).margin (0.05));
    }
}
