#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "TestUtils.h"
#include "dsp/McPotiEngine.h"

using namespace testutil;
using namespace mcpoti;

namespace
{
constexpr double fs = 48000.0;

Settings settings (float drive, int voice = 0)
{
    Settings s;
    for (auto& b : s.bands)
        b.drive = drive;
    s.voice = voice;
    s.tuneHz = 1500.0f;
    return s;
}

std::vector<float> run (const std::vector<float>& in, const Settings& s, int block = 256)
{
    McPotiEngine engine;
    engine.prepare (fs, block);
    std::vector<float> out (in.size());
    juce::AudioBuffer<float> buf (2, block);
    for (size_t pos = 0; pos < in.size(); pos += size_t (block))
    {
        const int n = int (std::min (size_t (block), in.size() - pos));
        buf.setSize (2, n, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            std::copy (in.begin() + long (pos), in.begin() + long (pos) + n, buf.getWritePointer (ch));
        engine.process (buf, s);
        std::copy (buf.getReadPointer (0), buf.getReadPointer (0) + n, out.begin() + long (pos));
    }
    return out;
}
} // namespace

TEST_CASE ("McPoti: con Drive 0 la salida es la entrada (el cruce suma plano)")
{
    const auto in = noise (int (fs), 0.3f);
    const auto out = run (in, settings (0.0f));
    CHECK (residualDb (in.data(), out.data(), int (in.size())) < -90.0);
}

TEST_CASE ("McPoti: con Mezcla 0 % la salida es la entrada aunque haya Drive")
{
    const auto in = noise (int (fs), 0.3f);
    auto s = settings (100.0f);
    s.mixPercent = 0.0f;
    const auto out = run (in, s);
    CHECK (residualDb (in.data(), out.data(), int (in.size())) < -90.0);
}

TEST_CASE ("McPoti: Delta es la señal añadida y seco + Delta = salida completa")
{
    const auto in = noise (int (fs), 0.3f);
    auto s = settings (70.0f);
    const auto full = run (in, s);
    s.delta = true;
    const auto delta = run (in, s);
    std::vector<float> sum (in.size());
    for (size_t i = 0; i < in.size(); ++i)
        sum[i] = in[i] + delta[i];
    CHECK (residualDb (full.data(), sum.data(), int (in.size())) < -90.0);
    double energy = 0.0;
    for (float v : delta)
        energy += double (v) * v;
    CHECK (energy > 1.0e-3);
}

TEST_CASE ("McPoti: armónicos por carácter (seno de 3 kHz a −18 dBFS, Drive 100)")
{
    const int n = int (fs);
    const auto in = sine (n, 3000.0, fs, fromDb (-18.0));
    auto harmonics = [&] (int voice)
    {
        auto s = settings (100.0f, voice);
        s.delta = true;
        const auto out = run (in, s);
        return std::pair { measureAmplitude (out.data() + n / 2, n / 2, 6000.0, fs), measureAmplitude (out.data() + n / 2, n / 2, 9000.0, fs) };
    };
    const auto [valve2, valve3] = harmonics (2);
    const auto [transistor2, transistor3] = harmonics (3);
    const auto [classic2, classic3] = harmonics (0);
    INFO ("válvula 2.º " << toDb (valve2) << " dB, 3.º " << toDb (valve3) << " dB");
    CHECK (valve2 > valve3);            // pares dominantes
    CHECK (transistor3 > transistor2 * 4.0); // impares dominantes
    CHECK (classic2 > 1.0e-4);          // Clásico crea 2.º y 3.º
    CHECK (classic3 > 1.0e-4);
}

TEST_CASE ("McPoti: la dinámica positiva da más armónicos en pasajes suaves y la negativa en picos")
{
    const int n = int (fs);
    auto added = [&] (float levelDb, float dyn)
    {
        auto s = settings (60.0f, 3);
        s.delta = true;
        for (auto& b : s.bands)
            b.dyn = dyn;
        const auto in = sine (n, 3000.0, fs, fromDb (levelDb));
        const auto out = run (in, s);
        return measureAmplitude (out.data() + n / 2, n / 2, 9000.0, fs) / fromDb (levelDb); // relativo al nivel de entrada
    };
    CHECK (added (-40.0f, 50.0f) > added (-40.0f, 0.0f));  // suave y Dinámica +: más
    CHECK (added (-6.0f, -50.0f) > added (-6.0f, 0.0f));   // fuerte y Dinámica −: más
}

TEST_CASE ("McPoti: Solo deja oír únicamente las bandas elegidas")
{
    const int n = int (fs);
    const auto in = sine (n, 100.0, fs, 0.3);
    auto s = settings (0.0f);
    s.bands[3].solo = true; // Aire: no hay nada a 100 Hz
    const auto out = run (in, s);
    CHECK (measureAmplitude (out.data() + n / 2, n / 2, 100.0, fs) < 0.01);
    s.bands[0].solo = true; // Cuerpo: sí
    const auto both = run (in, s);
    CHECK (measureAmplitude (both.data() + n / 2, n / 2, 100.0, fs) > 0.2);
}

TEST_CASE ("McPoti: sin NaN ni clics al mover Drive, cruces y Dinámica durante el audio")
{
    const auto in = sine (int (fs * 2.0), 440.0, fs, 0.3);
    McPotiEngine engine;
    engine.prepare (fs, 128);
    juce::AudioBuffer<float> buf (2, 128);
    std::vector<float> out;
    unsigned seed = 7;
    auto rnd = [&] { seed = seed * 1664525u + 1013904223u; return float (seed >> 8) / float (1 << 24); };
    Settings s = settings (50.0f, 2);
    const bool moveCrossovers = true;
    for (size_t pos = 0; pos + 128 <= in.size(); pos += 128)
    {
        if ((pos / 128) % 20 == 0)
        {
            for (auto& b : s.bands)
            {
                b.drive = 100.0f * rnd();
                b.dyn = -50.0f + 100.0f * rnd();
            }
            if (moveCrossovers) s.xo1 = 80.0f + 320.0f * rnd();
            if (moveCrossovers) s.xo2 = 600.0f + 3400.0f * rnd();
            if (moveCrossovers) s.xo3 = 3000.0f + 9000.0f * rnd();
        }
        for (int ch = 0; ch < 2; ++ch)
            std::copy (in.begin() + long (pos), in.begin() + long (pos) + 128, buf.getWritePointer (ch));
        engine.process (buf, s);
        for (int i = 0; i < 128; ++i)
            out.push_back (buf.getSample (0, i));
    }
    for (float v : out)
        REQUIRE (std::isfinite (v));
    const double steady = stepMax (in, 0, in.size());
    CHECK (stepMax (out, 4800, out.size()) < steady * 4.0 + 0.1);
}
