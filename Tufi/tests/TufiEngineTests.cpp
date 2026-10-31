#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>

#include "AliasingMeter.h"
#include "TestUtils.h"
#include "dsp/TufiEngine.h"

using namespace testutil;
using Catch::Approx;

namespace
{
constexpr double sr = 48000.0;
constexpr int block = 256;
const int latency = tufi::TufiEngine::getLatencySamples(); // 47, constante en todos los modos

struct Fixture
{
    tufi::TufiEngine engine;
    Fixture() { engine.setSynchronousTables (true); engine.prepare (sr, block); }
};

/** Pasa una señal mono por el motor en bloques de 256 y devuelve la salida con la latencia compensada. */
std::vector<float> run (tufi::TufiEngine& engine, std::vector<float> in, const tufi::Settings& s, int chunk = block)
{
    const size_t n = in.size();
    in.resize (n + size_t (latency), 0.0f);
    std::vector<float> out (in.size());
    juce::AudioBuffer<float> buf (1, chunk);

    for (size_t pos = 0; pos < in.size(); pos += size_t (chunk))
    {
        const int len = int (std::min<size_t> (size_t (chunk), in.size() - pos));
        buf.setSize (1, len, false, false, true);
        std::copy_n (in.begin() + long (pos), len, buf.getWritePointer (0));
        engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), len, out.begin() + long (pos));
    }
    return { out.begin() + latency, out.begin() + latency + long (n) };
}

/** Ajustes de fábrica de la ficha. */
tufi::Settings factory()
{
    tufi::Settings s;
    s.stage = 0;
    s.lowAmount = 3.5f;
    s.peakAmount = 5.5f;
    s.peakHardness = 4.0f;
    s.evenTop = 3.0f;
    s.evenBottom = 4.5f;
    s.oversample = 1;
    return s;
}

/** Solo la curva de la etapa, sin nivel bajo ni asimetría de picos. */
tufi::Settings stageOnly (int stage)
{
    tufi::Settings s;
    s.stage = stage;
    s.peakAmount = 10.0f;
    s.peakHardness = 0.0f;
    s.oversample = 2;
    return s;
}

double harmonicDb (const std::vector<float>& y, int skip, int len, double f, int h)
{
    return toDb (measureAmplitude (y.data() + skip, len, h * f, sr)) - toDb (measureAmplitude (y.data() + skip, len, f, sr));
}
} // namespace

// ------------------------------------------------------------------ hito 3

TEST_CASE ("Transparencia: formas en 0 y 0 dB dejan la señal intacta")
{
    Fixture f;
    const auto in = noise (int (sr) * 2);
    const auto out = run (f.engine, in, {});
    REQUIRE (residualDb (in.data(), out.data(), int (in.size())) < -90.0);
}

TEST_CASE ("Entrada y Salida suman sus ganancias")
{
    Fixture f;
    tufi::Settings s;
    s.driveDb = 3.0f;
    s.trimDb = 3.0f;

    const auto in = sine (int (sr), 1000.0, sr, 0.1);
    const auto out = run (f.engine, in, s);

    const int skip = int (sr) / 2;
    const double gain = toDb (measureAmplitude (out.data() + skip, int (in.size()) - skip, 1000.0, sr))
                        - toDb (measureAmplitude (in.data() + skip, int (in.size()) - skip, 1000.0, sr));
    REQUIRE (gain == Approx (6.0).margin (0.01));
}

TEST_CASE ("Nivel bajo: la pendiente en cero es 1 + cantidad/10")
{
    for (int os : { 0, 1, 2, 3 })
    {
        Fixture f;
        tufi::Settings s;
        s.lowAmount = 5.0f;
        s.oversample = os;

        const auto in = sine (int (sr), 1000.0, sr, 0.001);
        const auto out = run (f.engine, in, s);

        const int skip = int (sr) / 2;
        const double ratio = measureAmplitude (out.data() + skip, int (in.size()) - skip, 1000.0, sr)
                             / measureAmplitude (in.data() + skip, int (in.size()) - skip, 1000.0, sr);
        INFO ("sobremuestreo " << (1 << os) << "x");
        REQUIRE (ratio == Approx (1.5).epsilon (0.01));
    }
}

TEST_CASE ("El filtro de DC elimina la continua de una curva asimétrica")
{
    Fixture f;
    auto s = factory();
    s.evenTop = 0.0f;
    s.evenBottom = 10.0f; // asimetría máxima

    const auto in = sine (int (sr) * 10, 1000.0, sr, 0.7);
    const auto out = run (f.engine, in, s);

    double mean = 0.0;
    const int tail = int (sr);
    for (size_t i = out.size() - size_t (tail); i < out.size(); ++i)
        mean += out[i];
    mean /= tail;
    REQUIRE (toDb (std::abs (mean)) < -80.0);
}

TEST_CASE ("La salida es finita y queda acotada con señal fuerte")
{
    Fixture f;
    tufi::Settings s;
    s.driveDb = 6.0f;
    s.lowAmount = 10.0f;
    s.peakAmount = 10.0f;
    s.peakHardness = 10.0f;
    s.evenTop = 10.0f;
    s.evenBottom = 10.0f;

    const auto out = run (f.engine, noise (int (sr), 1.0f), s);
    for (float v : out)
    {
        REQUIRE (std::isfinite (v));
        REQUIRE (std::abs (v) < 4.0f);
    }
}

TEST_CASE ("Bypass: tras el fundido la salida es la entrada, con la misma latencia")
{
    Fixture f;
    auto s = factory();
    s.bypass = true;

    const auto in = noise (int (sr));
    const auto out = run (f.engine, in, s);

    const int skip = int (sr) / 10;
    REQUIRE (residualDb (in.data() + skip, out.data() + skip, int (in.size()) - skip) < -140.0);
}

TEST_CASE ("El medidor publica el pico de salida")
{
    Fixture f;
    tufi::Settings s;
    s.driveDb = 6.0f;

    run (f.engine, sine (int (sr) / 10, 1000.0, sr, 0.25), s);
    REQUIRE (f.engine.getMeter().get() > 0.3f);
    REQUIRE (f.engine.getMeter().get() < 0.6f);
}

// ------------------------------------------------------------------ hito 5

TEST_CASE ("Curvas de etapa: normalizadas, monótonas y con la simetría esperada")
{
    for (int stage = 0; stage < 3; ++stage)
    {
        const auto& t = tufi::stageBase (stage);
        INFO ("etapa " << stage);

        REQUIRE (int (t.size()) == tufi::tableSize);
        REQUIRE (tufi::lookup (t, 0.0) == Approx (0.0).margin (1e-6));
        REQUIRE ((tufi::lookup (t, 0.001) - tufi::lookup (t, -0.001)) / 0.002 == Approx (1.0).epsilon (2e-3));

        for (size_t i = 1; i < t.size(); ++i)
        {
            REQUIRE (std::isfinite (t[i]));
            REQUIRE (t[i] >= t[i - 1]);
        }
    }

    // Triodo y pentodo: asimétricos (el pentodo, más cerca del límite). Diodo: simétrico.
    REQUIRE (std::abs (tufi::lookup (tufi::stageBase (0), 1.0) + tufi::lookup (tufi::stageBase (0), -1.0)) > 0.03);
    REQUIRE (std::abs (tufi::lookup (tufi::stageBase (1), 2.0) + tufi::lookup (tufi::stageBase (1), -2.0)) > 0.03);
    for (double x : { 0.3, 0.8, 1.5, 2.0 })
        REQUIRE (tufi::lookup (tufi::stageBase (2), x) + tufi::lookup (tufi::stageBase (2), -x) == Approx (0.0).margin (1e-9));
}

TEST_CASE ("Armónicos por etapa: triodo domina H2, pentodo más impares, diodo solo impares")
{
    const double f0 = 1000.0;
    std::array<std::array<double, 2>, 3> h; // [etapa][H2, H3] en dB

    for (int stage = 0; stage < 3; ++stage)
    {
        Fixture f;
        const int n = 65536 + 20000;
        const auto out = run (f.engine, sine (n, f0, sr, 0.5), stageOnly (stage));
        h[size_t (stage)] = { harmonicDb (out, 20000, 65536, f0, 2), harmonicDb (out, 20000, 65536, f0, 3) };
        INFO ("etapa " << stage << ": H2 = " << h[size_t (stage)][0] << " dB, H3 = " << h[size_t (stage)][1] << " dB");
    }

    REQUIRE (h[0][0] > h[0][1] + 6.0);  // triodo: H2 por encima de H3
    REQUIRE (h[1][1] > h[0][1]);        // pentodo: más H3 que el triodo
    REQUIRE (h[2][0] < -80.0);          // diodo simétrico: sin pares
    REQUIRE (h[2][1] > -60.0);          // …y con impares
}

TEST_CASE ("Pares: la asimetría de los techos añade armónicos pares al diodo")
{
    Fixture f;
    auto s = stageOnly (2);
    s.peakHardness = 8.0f;
    s.evenTop = 0.0f;
    s.evenBottom = 10.0f;

    const auto out = run (f.engine, sine (65536 + 20000, 1000.0, sr, 0.5), s);
    REQUIRE (harmonicDb (out, 20000, 65536, 1000.0, 2) > -50.0);
}

TEST_CASE ("Los contadores reflejan la curva")
{
    Fixture f;
    auto s = factory();
    run (f.engine, sine (4096, 1000.0, sr, 0.1), s);

    REQUIRE (f.engine.getSlope() == Approx (1.35).epsilon (0.01)); // 1 + 3,5/10
    REQUIRE (f.engine.getCeilingDb() == Approx (-3.0 - 0.7 * 4.0).margin (0.05)); // −5,8 dB: dureza 4,0
}

TEST_CASE ("Latencia constante: el pico del impulso cae en la latencia declarada en todos los modos")
{
    for (int os = 0; os < 4; ++os)
        for (bool neutralPath : { true, false })
        {
            Fixture f;
            tufi::Settings s;
            s.oversample = os;
            if (! neutralPath)
                s.peakAmount = 10.0f; // cruza la curva: con señal diminuta es una recta de pendiente 1

            const int len = 8192;
            std::vector<float> in (size_t (len), 0.0f);
            in[2000] = 1.0e-3f;

            // Sin compensar: se busca el pico en la salida cruda.
            juce::AudioBuffer<float> buf (1, block);
            std::vector<float> out (in.size());
            for (size_t pos = 0; pos < in.size(); pos += block)
            {
                std::copy_n (in.begin() + long (pos), block, buf.getWritePointer (0));
                f.engine.process (buf, s);
                std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
            }

            size_t peak = 0;
            for (size_t i = 0; i < out.size(); ++i)
                if (std::abs (out[i]) > std::abs (out[peak]))
                    peak = i;

            INFO ("sobremuestreo " << (1 << os) << "x, " << (neutralPath ? "ruta simple" : "curva"));
            REQUIRE (int (peak) == 2000 + latency);
        }
}

TEST_CASE ("Aliasing, 9 kHz a -6 dBFS, por sobremuestreo")
{
    // 4× y 8× cumplen los -80 dB de la ficha con mucho margen. 2× llega a ~-59 dB y sin
    // sobremuestreo a ~-46 dB: la ADAA sola no basta para armónicos de orden alto.
    const double limits[4] = { -40.0, -55.0, -80.0, -80.0 };
    const char* names[4] = { "sin sobremuestreo", "2×", "4×", "8×" };

    for (int os = 0; os < 4; ++os)
    {
        double worst = -200.0;
        for (int stage = 0; stage < 3; ++stage)
        {
            Fixture f;
            auto s = factory();
            s.stage = stage;
            s.oversample = os;
            const int skip = 20000;
            const auto out = run (f.engine, sine (65536 + skip, 9000.0, sr, 0.5), s);
            worst = std::max (worst, aliasingDb (out, skip, 9000.0, sr));
        }
        INFO (names[os] << ": peor aliasing " << worst << " dB");
        REQUIRE (worst < limits[os]);
    }
}

// ----------------------------------------------------- suavizado de perillas

TEST_CASE ("Mover las perillas de forma con audio sonando no produce clics")
{
    const auto tone = sine (int (sr) * 4, 440.0, sr, 0.4);

    // Referencia: el salto máximo estable en las combinaciones extremas.
    double reference = 0.0;
    for (float amount : { 0.0f, 10.0f })
        for (int stage = 0; stage < 3; ++stage)
        {
            Fixture f;
            auto s = factory();
            s.stage = stage;
            s.peakAmount = amount;
            s.lowAmount = amount;
            s.evenBottom = amount;
            const auto out = run (f.engine, sine (int (sr), 440.0, sr, 0.4), s);
            reference = std::max (reference, stepMax (out, 12000, out.size()));
        }

    Fixture f;
    auto s = factory();
    std::vector<float> out (tone.size());
    juce::AudioBuffer<float> buf (1, block);

    for (size_t pos = 0; pos + block <= tone.size(); pos += block)
    {
        const float t = float (pos) / float (tone.size());
        // Barridos continuos de todas las perillas de forma, más un cambio de etapa cada segundo.
        s.lowAmount = 10.0f * t;
        s.peakAmount = 10.0f * (1.0f - t);
        s.peakHardness = 10.0f * std::abs (std::sin (6.0f * t));
        s.evenTop = 10.0f * t;
        s.evenBottom = 10.0f * (1.0f - t);
        s.stage = int (pos / size_t (sr)) % 3;

        std::copy_n (tone.begin() + long (pos), block, buf.getWritePointer (0));
        f.engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
    }

    REQUIRE (stepMax (out, 12000, out.size()) < 1.4 * reference);
}

TEST_CASE ("Cambiar el sobremuestreo con audio sonando no produce clics")
{
    const auto tone = sine (int (sr) * 5, 440.0, sr, 0.4);

    double reference = 0.0;
    for (int os = 0; os < 4; ++os)
    {
        Fixture f;
        auto s = factory();
        s.oversample = os;
        const auto out = run (f.engine, sine (int (sr), 440.0, sr, 0.4), s);
        reference = std::max (reference, stepMax (out, 12000, out.size()));
    }

    Fixture f;
    auto s = factory();
    std::vector<float> out (tone.size());
    juce::AudioBuffer<float> buf (1, block);
    for (size_t pos = 0; pos + block <= tone.size(); pos += block)
    {
        s.oversample = int (pos / size_t (sr)) % 4;
        std::copy_n (tone.begin() + long (pos), block, buf.getWritePointer (0));
        f.engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
    }
    REQUIRE (stepMax (out, 12000, out.size()) < 1.3 * reference);
}

TEST_CASE ("Pasar por cero los controles de forma no produce clics")
{
    const auto tone = sine (int (sr) * 4, 440.0, sr, 0.4);

    double reference = 0.0;
    for (float amount : { 0.0f, 5.0f })
    {
        Fixture f;
        auto s = factory();
        s.lowAmount = amount;
        s.peakAmount = amount;
        const auto out = run (f.engine, sine (int (sr), 440.0, sr, 0.4), s);
        reference = std::max (reference, stepMax (out, 12000, out.size()));
    }

    Fixture f;
    auto s = factory();
    std::vector<float> out (tone.size());
    juce::AudioBuffer<float> buf (1, block);
    for (size_t pos = 0; pos + block <= tone.size(); pos += block)
    {
        const bool on = (pos / size_t (sr / 2)) % 2 == 0; // 0,5 s con forma, 0,5 s sin ella
        s.lowAmount = on ? 5.0f : 0.0f;
        s.peakAmount = on ? 5.0f : 0.0f;
        std::copy_n (tone.begin() + long (pos), block, buf.getWritePointer (0));
        f.engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
    }
    REQUIRE (stepMax (out, 12000, out.size()) < 1.3 * reference);
}

TEST_CASE ("Bypass activado y desactivado con audio sonando no produce clics")
{
    const auto tone = sine (int (sr) * 3, 440.0, sr, 0.4);
    Fixture f;
    auto s = factory();

    double reference = 0.0;
    {
        Fixture g;
        const auto out = run (g.engine, sine (int (sr), 440.0, sr, 0.4), s);
        reference = stepMax (out, 12000, out.size());
    }

    std::vector<float> out (tone.size());
    juce::AudioBuffer<float> buf (1, block);
    for (size_t pos = 0; pos + block <= tone.size(); pos += block)
    {
        s.bypass = (pos / block) % 80 >= 40;
        std::copy_n (tone.begin() + long (pos), block, buf.getWritePointer (0));
        f.engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
    }
    REQUIRE (stepMax (out, 12000, out.size()) < 1.4 * reference);
}

TEST_CASE ("Bloques de cualquier tamaño dan el mismo resultado")
{
    const auto tone = sine (int (sr), 700.0, sr, 0.3);
    const auto s = factory();

    Fixture a, b;
    const auto outA = run (a.engine, tone, s, 256);
    const auto outB = run (b.engine, tone, s, 1000);
    REQUIRE (residualDb (outA.data(), outB.data(), int (tone.size())) < -120.0);
}

// Informe de medidas (no es una prueba): `TufiTests "[.report]"`
TEST_CASE ("Informe de aliasing y CPU", "[.report]")
{
    printf ("\naliasing a 9 kHz, -6 dBFS, ajustes de fábrica (dB)\n  etapa:     triodo  pentodo   diodo\n");
    for (int os = 0; os < 4; ++os)
    {
        printf ("  %s: ", os == 0 ? " sin OS" : (os == 1 ? "    2x " : (os == 2 ? "    4x " : "    8x ")));
        for (int stage = 0; stage < 3; ++stage)
        {
            Fixture f;
            auto s = factory();
            s.stage = stage;
            s.oversample = os;
            const auto out = run (f.engine, sine (65536 + 20000, 9000.0, sr, 0.5), s);
            printf ("%8.1f ", aliasingDb (out, 20000, 9000.0, sr));
        }
        printf ("\n");
    }

    const int seconds = 10, total = int (sr) * seconds;
    const auto l = noise (total, 0.2f, 1), r = noise (total, 0.2f, 2);
    printf ("\nCPU, estéreo, 48 kHz, bloque 256, ajustes de fábrica\n");
    for (int os = 0; os < 4; ++os)
    {
        Fixture f;
        auto s = factory();
        s.oversample = os;
        juce::AudioBuffer<float> buf (2, block);
        const auto t0 = std::chrono::steady_clock::now();
        for (int pos = 0; pos + block <= total; pos += block)
        {
            std::copy_n (l.begin() + pos, block, buf.getWritePointer (0));
            std::copy_n (r.begin() + pos, block, buf.getWritePointer (1));
            f.engine.process (buf, s);
        }
        const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        printf ("  %dx: %5.2f %% de un núcleo\n", 1 << os, 100.0 * sec / seconds);
    }
}
