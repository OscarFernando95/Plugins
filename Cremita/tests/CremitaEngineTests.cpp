#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <random>

#include "AliasingMeter.h"
#include "TestUtils.h"
#include "dsp/CremitaCurve.h"
#include "dsp/CremitaEngine.h"

using namespace testutil;
using Catch::Approx;

namespace
{
constexpr int block = 256;
const int latency = cremita::CremitaEngine::getLatencySamples(); // 47, constante en todos los modos

struct Stereo
{
    std::vector<float> l, r;
};

/** Pasa una señal estéreo por el motor. Devuelve la salida SIN compensar la latencia. */
Stereo runStereo (cremita::CremitaEngine& engine, const Stereo& in, const cremita::Settings& s, int chunk = block)
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

/** Mono, con la latencia compensada: el resultado se alinea con la entrada. */
std::vector<float> run (cremita::CremitaEngine& engine, std::vector<float> in, const cremita::Settings& s)
{
    const size_t n = in.size();
    in.resize (n + size_t (latency), 0.0f);

    juce::AudioBuffer<float> buf (1, block);
    std::vector<float> out (in.size());
    for (size_t pos = 0; pos < in.size(); pos += block)
    {
        const int len = int (std::min<size_t> (block, in.size() - pos));
        buf.setSize (1, len, false, false, true);
        std::copy_n (in.begin() + long (pos), len, buf.getWritePointer (0));
        engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), len, out.begin() + long (pos));
    }
    return { out.begin() + latency, out.begin() + latency + long (n) };
}

cremita::Settings base()
{
    cremita::Settings s;
    s.effectPercent = 100.0f;
    return s;
}

std::vector<float> runFresh (const std::vector<float>& in, const cremita::Settings& s, double sr = 48000.0)
{
    cremita::CremitaEngine engine;
    engine.prepare (sr, block);
    return run (engine, in, s);
}
} // namespace

// ------------------------------------------------------------------ curva

TEST_CASE ("Curva: monótona, f(0) = 0 y f(±1) = ±1 para todo Curva, Sesgo y Clip")
{
    for (int c = -50; c <= 50; c += 5)
        for (double bias : { 0.0, 25.0, 50.0, 75.0, 100.0 })
            for (bool clip : { true, false })
            {
                const auto p = cremita::CurveParams::make (c, bias, clip);
                INFO ("Curva " << c << ", Sesgo " << bias << ", clip " << clip);
                CHECK (cremita::curveF (p, 0.0) == Approx (0.0).margin (1e-12));
                CHECK (cremita::curveF (p, 1.0) == Approx (1.0).margin (1e-12));
                CHECK (cremita::curveF (p, -1.0) == Approx (-1.0).margin (1e-12));

                double prev = cremita::curveF (p, -3.0);
                bool monotone = true;
                for (double x = -3.0 + 1e-3; x <= 3.0; x += 1e-3)
                {
                    const double y = cremita::curveF (p, x);
                    monotone = monotone && y >= prev - 1e-12;
                    prev = y;
                }
                CHECK (monotone);
                CHECK (cremita::curveF (p, 3.0) <= 2.0 + 1e-12);
                CHECK (cremita::curveF (p, -3.0) >= -2.0 - 1e-12);
                if (clip)
                    CHECK (cremita::curveF (p, 3.0) == Approx (1.0).margin (1e-12));
            }
}

TEST_CASE ("Curva: la pendiente en cero da +1,0 / +3,5 / −2,5 dB con Curva 0 / +50 / −50")
{
    const auto slopeDb = [] (double c)
    {
        const auto p = cremita::CurveParams::make (c, 0.0, true);
        const double h = 1e-6;
        return toDb ((cremita::curveF (p, h) - cremita::curveF (p, -h)) / (2 * h));
    };
    CHECK (slopeDb (0.0) == Approx (1.0).margin (0.05));
    CHECK (slopeDb (50.0) == Approx (3.5).margin (0.05));
    CHECK (slopeDb (-50.0) == Approx (-2.5).margin (0.05));
}

TEST_CASE ("Curva: el tercer armónico coincide con la teoría y el segundo solo aparece con Sesgo")
{
    const double fs = 48000.0, A = 0.5;
    const int n = 4800; // 100 periodos de 1 kHz
    const auto x = sine (n, 1000.0, fs, A);
    const auto eval = [&] (const cremita::CurveParams& p)
    {
        std::vector<float> y (x.size());
        for (size_t i = 0; i < x.size(); ++i)
            y[i] = float (cremita::curveF (p, x[i]));
        return y;
    };

    // Sin Sesgo: f = x + k·x − k·x³, así que H3 = k·A³/4.
    const auto p = cremita::CurveParams::make (50.0, 0.0, true);
    const auto y = eval (p);
    const double h3 = measureAmplitude (y.data(), n, 3000.0, fs);
    CHECK (toDb (h3) == Approx (toDb (p.k * A * A * A / 4.0)).margin (0.5));
    CHECK (toDb (measureAmplitude (y.data(), n, 2000.0, fs)) < -110.0);

    const auto yb = eval (cremita::CurveParams::make (0.0, 100.0, true));
    CHECK (toDb (measureAmplitude (yb.data(), n, 2000.0, fs)) > -60.0);
}

TEST_CASE ("ADAA: la derivada de la antiderivada es la curva, y x casi igual no da NaN")
{
    for (int c : { -50, 0, 50 })
        for (double bias : { 0.0, 100.0 })
            for (bool clip : { true, false })
            {
                const auto p = cremita::CurveParams::make (c, bias, clip);
                for (double x = -3.0; x <= 3.0; x += 0.0137)
                {
                    const double h = 1e-5;
                    const double d = (cremita::curveAntiderivative (p, x + h) - cremita::curveAntiderivative (p, x - h)) / (2 * h);
                    INFO ("Curva " << c << ", Sesgo " << bias << ", clip " << clip << ", x " << x);
                    REQUIRE (d == Approx (cremita::curveF (p, x)).margin (2e-5));
                }

                cremita::CurveAdaa adaa;
                for (int i = 0; i < 100; ++i)
                {
                    const double y = adaa.process (p, 0.4); // x constante: dx = 0
                    REQUIRE (std::isfinite (y));
                    REQUIRE (y == Approx (cremita::curveF (p, 0.4)).margin (1e-9));
                }
            }
}

// ------------------------------------------------------------------ motor

TEST_CASE ("Efecto 0 %: la salida es la entrada retrasada, en todos los sobremuestreos")
{
    const auto noiseIn = noise (24000, 0.8f);
    for (int os = 0; os < 4; ++os)
        for (bool ms : { false, true })
        {
            auto s = base();
            s.effectPercent = 0.0f;
            s.oversample = os;
            s.curve = 50.0f;
            s.biasPercent = 100.0f;
            s.midSide = ms;

            cremita::CremitaEngine engine;
            engine.prepare (48000.0, block);
            const Stereo in { noiseIn, noise (24000, 0.8f, 99) };
            const auto out = runStereo (engine, in, s);

            double worst = -400.0;
            for (size_t i = 4000; i + size_t (latency) < in.l.size(); ++i)
                worst = std::max (worst, std::max (toDb (std::abs (out.l[i + size_t (latency)] - in.l[i])),
                                                   toDb (std::abs (out.r[i + size_t (latency)] - in.r[i]))));
            INFO ("sobremuestreo " << (1 << os) << "×, M/S " << ms);
            CHECK (worst < -130.0);
        }
}

TEST_CASE ("Latencia: un impulso sale en la muestra declarada con cualquier sobremuestreo y bandas")
{
    for (int os = 0; os < 4; ++os)
        for (bool split : { false, true })
        {
            auto s = base();
            s.oversample = os;
            s.split = split;
            std::vector<float> in (4096, 0.0f);
            in[1000] = 0.01f;
            cremita::CremitaEngine engine;
            engine.prepare (48000.0, block);
            // Se descarta el primer bloque: el motor arranca con un fundido de estructura.
            const auto out = run (engine, in, s);

            size_t peak = 0;
            for (size_t i = 0; i < out.size(); ++i)
                if (std::abs (out[i]) > std::abs (out[peak]))
                    peak = i;
            INFO ("sobremuestreo " << (1 << os) << "×, bandas " << split << ", pico en " << peak);
            // Los cruces son pasos todo y mueven el pico un par de muestras; sin ellos es exacto.
            if (split)
                CHECK (std::abs (int (peak) - 1000) <= 3);
            else
                CHECK (peak == 1000);
        }
}

TEST_CASE ("Ganancia con señal baja: Curva 0 sube 1,0 dB, también con tres bandas")
{
    for (bool split : { false, true })
        for (double freq : { 100.0, 1000.0, 5000.0, 10000.0 })
        {
            auto s = base();
            s.split = split;
            const auto in = sine (48000 * 2, freq, 48000.0, 0.01);
            const auto out = runFresh (in, s);
            const double gain = toDb (measureAmplitude (out.data() + 48000, 48000, freq, 48000.0)
                                      / measureAmplitude (in.data() + 48000, 48000, freq, 48000.0));
            INFO ("bandas " << split << ", " << freq << " Hz");
            CHECK (gain == Approx (toDb (1.125)).margin (0.1));
        }
}

TEST_CASE ("Band Split: sin proceso, la suma de las tres bandas es plana (±0,1 dB)")
{
    for (double freq : { 60.0, 200.0, 800.0, 3000.0, 9000.0 })
    {
        auto s = base();
        s.split = true;
        s.effectPercent = 0.0f;
        const auto in = sine (48000 * 2, freq, 48000.0, 0.5);
        const auto out = runFresh (in, s);
        const double gain = toDb (measureAmplitude (out.data() + 48000, 48000, freq, 48000.0)
                                  / measureAmplitude (in.data() + 48000, 48000, freq, 48000.0));
        INFO (freq << " Hz");
        CHECK (gain == Approx (0.0).margin (0.1));
    }
}

TEST_CASE ("Clip 0 dB: con Input ≤ 0 dB ninguna muestra supera 0 dBFS; apagado deja pasar hasta ~+6 dB")
{
    for (int os = 0; os < 4; ++os)
        for (float curve : { -50.0f, 0.0f, 50.0f })
        {
            auto s = base();
            s.oversample = os;
            s.curve = curve;
            s.biasPercent = 60.0f;
            s.clip = true;
            const auto out = runFresh (noise (24000, 1.0f), s);
            float peak = 0.0f;
            for (float v : out)
                peak = std::max (peak, std::abs (v));
            INFO ("sobremuestreo " << (1 << os) << "×, Curva " << curve);
            CHECK (peak <= 1.0f);
        }

    auto s = base();
    s.clip = false;
    s.inputDb = 12.0f;
    s.outputDb = 0.0f;
    const auto out = runFresh (noise (24000, 1.0f), s);
    float peak = 0.0f;
    for (float v : out)
        peak = std::max (peak, std::abs (v));
    CHECK (peak > 1.0f);
    CHECK (peak < 3.0f); // ±2 de la cola más el rebasamiento del filtro de submuestreo en ruido muy recortado (medido 2,73)
}

TEST_CASE ("Aliasing: seno de 8 kHz a −6 dBFS, Curva +50, 2× con ADAA, por debajo de −70 dBc")
{
    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (double freq : { 5000.0, 8000.0, 9500.0 })
        {
            auto s = base();
            s.curve = 50.0f;
            s.oversample = 1;
            const int n = 65536 + 8192;
            const auto out = runFresh (sine (n, freq, sr, fromDb (-6.0)), s, sr);
            const double alias = aliasingDb (out, 8192, freq, sr);
            INFO (sr << " Hz, " << freq << " Hz: " << alias << " dBc");
            CHECK (alias < -70.0);
        }
}

TEST_CASE ("Aliasing: el sobremuestreo y el ADAA ayudan frente a 1× (informe)")
{
    for (int os = 0; os < 4; ++os)
    {
        auto s = base();
        s.curve = 50.0f;
        s.oversample = os;
        const auto out = runFresh (sine (65536 + 8192, 9500.0, 48000.0, fromDb (-6.0)), s);
        WARN ("9,5 kHz (el 3.er armónico se pliega a 19,5 kHz), Curva +50, " << (1 << os) << "×: " << aliasingDb (out, 8192, 9500.0, 48000.0) << " dBc");
    }
}

TEST_CASE ("Sin clics: mover cualquier control con audio sonando no salta más que la señal estable")
{
    using Mod = std::function<void (cremita::Settings&)>;
    struct Case { const char* name; Mod from, to; };
    const std::vector<Case> cases = {
        { "Input", [] (auto& s) { s.inputDb = 0; }, [] (auto& s) { s.inputDb = 6; } },
        { "Efecto", [] (auto& s) { s.effectPercent = 100; }, [] (auto& s) { s.effectPercent = 20; } },
        { "Curva", [] (auto& s) { s.curve = 0; }, [] (auto& s) { s.curve = 50; } },
        { "Sesgo", [] (auto& s) { s.biasPercent = 0; }, [] (auto& s) { s.biasPercent = 100; } },
        { "Output", [] (auto& s) { s.outputDb = 0; }, [] (auto& s) { s.outputDb = -6; } },
        { "Cruce 1", [] (auto& s) { s.split = true; s.crossover1 = 200; }, [] (auto& s) { s.split = true; s.crossover1 = 600; } },
        { "Cruce 2", [] (auto& s) { s.split = true; s.crossover2 = 3000; }, [] (auto& s) { s.split = true; s.crossover2 = 1500; } },
        { "Ajuste LF", [] (auto& s) { s.split = true; s.trimDb[0] = 0; }, [] (auto& s) { s.split = true; s.trimDb[0] = 6; } },
        { "Sobremuestreo", [] (auto& s) { s.oversample = 1; }, [] (auto& s) { s.oversample = 3; } },
        { "Band Split", [] (auto& s) { s.split = false; }, [] (auto& s) { s.split = true; } },
        { "Mid/Side", [] (auto& s) { s.midSide = false; }, [] (auto& s) { s.midSide = true; } },
        { "Clip", [] (auto& s) { s.clip = true; }, [] (auto& s) { s.clip = false; } },
        { "In/Out", [] (auto& s) { s.inOn = true; }, [] (auto& s) { s.inOn = false; } },
        { "Bypass", [] (auto& s) { s.bypass = false; }, [] (auto& s) { s.bypass = true; } },
        { "Igualar", [] (auto& s) { s.match = false; }, [] (auto& s) { s.match = true; } },
    };

    for (const auto& c : cases)
    {
        auto a = base(), b = base();
        a.curve = b.curve = 20.0f;
        c.from (a);
        c.to (b);
        // Para los controles que dependen de Band Split, las dos configuraciones lo usan igual.
        const auto tone = sine (48000 * 3, 440.0, 48000.0, 0.45);
        const Stereo in { tone, tone };

        const auto steady = [&] (const cremita::Settings& s)
        {
            cremita::CremitaEngine engine;
            engine.prepare (48000.0, block);
            const auto o = runStereo (engine, in, s);
            return stepMax (o.l, 60000, o.l.size());
        };

        cremita::CremitaEngine engine;
        engine.prepare (48000.0, block);
        Stereo out { std::vector<float> (in.l.size()), std::vector<float> (in.l.size()) };
        juce::AudioBuffer<float> buf (2, block);
        for (size_t pos = 0; pos + block <= in.l.size(); pos += block)
        {
            std::copy_n (in.l.begin() + long (pos), block, buf.getWritePointer (0));
            std::copy_n (in.r.begin() + long (pos), block, buf.getWritePointer (1));
            engine.process (buf, pos < 72000 ? a : b);
            std::copy_n (buf.getReadPointer (0), block, out.l.begin() + long (pos));
        }

        const double reference = std::max (steady (a), steady (b));
        const double jump = stepMax (out.l, 24000, out.l.size() - out.l.size() % block);
        INFO (c.name << ": salto " << jump << ", estable " << reference);
        CHECK (jump <= 1.3 * reference);
    }
}

TEST_CASE ("Igualar sonoridad: la salida queda a ±0,2 LU de la entrada")
{
    const double fs = 48000.0;
    const int n = int (fs * 30.0);
    // Ruido con envolvente lenta, para tener una sonoridad variada pero estable en promedio.
    std::mt19937 rng (5);
    std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
    std::vector<float> in (static_cast<size_t> (n));
    for (int i = 0; i < n; ++i)
        in[size_t (i)] = 0.3f * dist (rng) * (0.7f + 0.3f * float (std::sin (2.0 * 3.14159265 * 0.5 * i / fs)));

    for (float curve : { 50.0f, -50.0f })
    {
        auto s = base();
        s.inputDb = 6.0f;
        s.curve = curve;
        s.biasPercent = 40.0f;
        s.match = true;

        cremita::CremitaEngine engine;
        engine.prepare (fs, block);
        const auto out = run (engine, in, s);

        ferb::LoudnessMeter mi, mo;
        mi.prepare (fs, 2);
        mo.prepare (fs, 2);
        const size_t from = size_t (fs * 18.0);
        for (size_t pos = from; pos + 480 <= in.size(); pos += 480)
        {
            const float* a[2] = { in.data() + pos, in.data() + pos };
            const float* b[2] = { out.data() + pos, out.data() + pos };
            mi.process (a, 480);
            mo.process (b, 480);
        }
        INFO ("Curva " << curve << ": entrada " << mi.integrated() << " LUFS, salida " << mo.integrated() << " LUFS, ganancia aplicada "
                         << engine.getMatchGainDb() << " dB");
        CHECK (mo.integrated() == Approx (mi.integrated()).margin (0.2));
        CHECK (std::abs (engine.getMatchGainDb()) > 0.5f); // el efecto sí cambió la sonoridad
    }
}

TEST_CASE ("Medidores: el de efecto marca 0 con Efecto 0 y el de sobrecarga avisa sobre 0 dBFS")
{
    cremita::CremitaEngine engine;
    engine.prepare (48000.0, block);
    auto s = base();
    s.effectPercent = 0.0f;
    run (engine, sine (48000, 1000.0, 48000.0, 0.5), s);
    CHECK (engine.getEffectLevel() < 1e-6f);
    CHECK (engine.getInputLevel() == Approx (0.5f / std::sqrt (2.0f)).epsilon (0.03)); // la cola de ceros de `run` baja el VU ~1,5 %

    s.effectPercent = 100.0f;
    s.curve = 50.0f;
    run (engine, sine (48000, 1000.0, 48000.0, 0.5), s);
    CHECK (engine.getEffectLevel() > 0.01f);

    engine.getOverloadMeter().consume();
    s.inputDb = 12.0f;
    run (engine, sine (4800, 1000.0, 48000.0, 0.9), s);
    CHECK (engine.getOverloadMeter().consume() >= 1.0f);
}

TEST_CASE ("Robustez: silencio, continua, NaN, infinitos y subnormales; tasas y bloques extremos")
{
    juce::Random rng (3);
    for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int chunk : { 1, 7, 64, 255, 256, 257, 4096 })
        {
            cremita::CremitaEngine engine;
            engine.prepare (sr, chunk);
            juce::AudioBuffer<float> buf (2, chunk);

            for (int round = 0; round < 40; ++round)
            {
                auto s = base();
                s.inputDb = rng.nextFloat() * 24.0f - 12.0f;
                s.effectPercent = rng.nextFloat() * 100.0f;
                s.curve = rng.nextFloat() * 100.0f - 50.0f;
                s.biasPercent = rng.nextFloat() * 100.0f;
                s.split = rng.nextBool();
                s.crossover1 = 80.0f + rng.nextFloat() * 420.0f;
                s.crossover2 = 1500.0f + rng.nextFloat() * 6500.0f;
                s.midSide = rng.nextBool();
                s.oversample = rng.nextInt (4);
                s.clip = rng.nextBool();
                s.match = rng.nextBool();
                s.trimDb = { rng.nextFloat() * 12 - 6, rng.nextFloat() * 12 - 6, rng.nextFloat() * 12 - 6 };

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

// Informe de CPU (no es una prueba): ./build/Cremita/CremitaTests "[.report]"
TEST_CASE ("Informe: CPU del motor, estéreo a 48 kHz, bloque de 256", "[.report]")
{
    const double fs = 48000.0;
    const int seconds = 20;
    const Stereo in { noise (int (fs) * seconds, 0.4f, 1), noise (int (fs) * seconds, 0.4f, 2) };

    struct Config { const char* name; int os; bool split; };
    for (const auto& c : { Config { "banda completa 1×", 0, false }, Config { "banda completa 2×", 1, false },
                           Config { "banda completa 4×", 2, false }, Config { "3 bandas 2×", 1, true },
                           Config { "3 bandas 4×", 2, true }, Config { "3 bandas 8×", 3, true } })
    {
        auto s = base();
        s.oversample = c.os;
        s.split = c.split;
        s.curve = 30.0f;
        s.biasPercent = 30.0f;
        cremita::CremitaEngine engine;
        engine.prepare (fs, block);
        const auto t0 = std::chrono::steady_clock::now();
        runStereo (engine, in, s);
        const double elapsed = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        WARN (c.name << ": " << 100.0 * elapsed / seconds << " % de un núcleo");
    }
}

TEST_CASE ("Informe: aliasing con saturación fuerte (Input +12 dB)", "[.report]")
{
    for (bool clip : { true, false })
        for (int os = 0; os < 3; ++os)
        {
            auto s = base();
            s.curve = 50.0f;
            s.inputDb = 12.0f;
            s.clip = clip;
            s.oversample = os;
            const auto out = runFresh (sine (65536 + 8192, 3100.0, 48000.0, fromDb (-6.0)), s);
            WARN ("3,1 kHz, Input +12 dB, clip " << clip << ", " << (1 << os) << "×: " << aliasingDb (out, 8192, 3100.0, 48000.0) << " dBc");
        }
}
