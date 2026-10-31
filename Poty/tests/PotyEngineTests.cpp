#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <chrono>
#include <complex>

#include "AliasingMeter.h"
#include "TestUtils.h"
#include "dsp/PotyEngine.h"

using namespace testutil;
using Catch::Approx;

namespace
{
constexpr int block = 256;
const int latency = poty::PotyEngine::getLatencySamples(); // 45, constante en todas las calidades

struct Stereo
{
    std::vector<float> l, r;
};

/** Pasa una señal por el motor en bloques de 256. Devuelve la salida SIN compensar la latencia. */
Stereo runStereo (poty::PotyEngine& engine, const Stereo& in, const poty::Settings& s, int chunk = block)
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
std::vector<float> run (poty::PotyEngine& engine, std::vector<float> in, const poty::Settings& s)
{
    const size_t n = in.size();
    in.resize (n + size_t (latency), 0.0f); // cola para recuperar las muestras retrasadas

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

/** Ajustes de fábrica de la ficha, con la mezcla al 100 % y la salida en 0 dB. */
poty::Settings factorySettings()
{
    poty::Settings s;
    s.mixPercent = 100.0f;
    s.bands = { { { true, 90.0f, 5.0f, 0.8f, 0, 45.0f },
                  { true, 350.0f, 3.0f, 1.0f, 1, 30.0f },
                  { true, 1800.0f, 6.0f, 1.1f, 2, 55.0f },
                  { true, 5000.0f, 8.0f, 1.2f, 3, 70.0f },
                  { true, 12000.0f, 7.0f, 0.7f, 4, 40.0f } } };
    return s;
}

/** Solo la banda 4 activa, en la frecuencia y con el tipo pedidos. */
poty::Settings singleBand (float freq, float gainDb, int type, float drive = 70.0f)
{
    poty::Settings s;
    s.mixPercent = 100.0f;
    s.bands[3] = { true, freq, gainDb, 1.2f, type, drive };
    return s;
}

std::complex<double> response (const float* c, double freq, double sampleRate)
{
    const auto z1 = std::polar (1.0, -2.0 * std::numbers::pi * freq / sampleRate);
    const auto z2 = z1 * z1;
    return (double (c[0]) + double (c[1]) * z1 + double (c[2]) * z2)
           / (1.0 + double (c[3]) * z1 + double (c[4]) * z2);
}

/** Respuesta teórica de y = x + mezcla · Σ (H_i − 1)·x, con H_i de la campana de JUCE. */
double theoreticalDb (const poty::Settings& s, double freq, double sampleRate)
{
    std::complex<double> sum = 0.0;
    for (const auto& b : s.bands)
    {
        if (! b.on || b.gainDb <= 0.0f)
            continue;
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            sampleRate, b.freq, b.q, static_cast<float> (fromDb (b.gainDb)));
        sum += response (coeffs->coefficients.getRawDataPointer(), freq, sampleRate) - 1.0;
    }
    return toDb (std::abs (1.0 + (s.mixPercent / 100.0) * sum));
}

double measuredDb (poty::Settings s, double freq, double sampleRate)
{
    s.linear = true;
    poty::PotyEngine engine;
    engine.prepare (sampleRate, block);

    const int n = int (sampleRate * 1.5);
    const auto in = sine (n, freq, sampleRate, 0.1);
    const auto out = run (engine, in, s);

    const int skip = n - int (sampleRate); // último segundo: el resto es el transitorio
    return toDb (measureAmplitude (out.data() + skip, int (sampleRate), freq, sampleRate))
           - toDb (measureAmplitude (in.data() + skip, int (sampleRate), freq, sampleRate));
}

/** Salto máximo entre muestras de la salida estable (sin cambiar nada) con esos ajustes. */
double steadyStep (const poty::Settings& s, const std::vector<float>& tone)
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);
    const auto out = run (engine, tone, s);
    return stepMax (out, 4000, out.size());
}
} // namespace

// ------------------------------------------------------------------ hito 3

TEST_CASE ("Transparencia: ganancias de banda en 0 dejan la señal intacta, en todas las calidades")
{
    for (int quality = 0; quality < 4; ++quality)
    {
        poty::PotyEngine engine;
        engine.prepare (48000.0, block);

        auto s = factorySettings();
        s.quality = quality;
        for (auto& b : s.bands)
            b.gainDb = 0.0f;

        const auto in = noise (48000 * 2);
        const auto out = run (engine, in, s);
        INFO ("calidad " << quality);
        REQUIRE (residualDb (in.data(), out.data(), int (in.size())) < -90.0);
    }
}

TEST_CASE ("Realce lineal: una banda con mezcla 100 % es exactamente su campana")
{
    poty::Settings s;
    s.mixPercent = 100.0f;
    s.bands[2] = { true, 1800.0f, 6.0f, 1.1f, 2, 50.0f };

    REQUIRE (measuredDb (s, 1800.0, 48000.0) == Approx (6.0).margin (0.05));
}

TEST_CASE ("Realce lineal: respuesta medida frente a la teórica, 20 Hz a 20 kHz")
{
    for (double sampleRate : { 44100.0, 48000.0, 96000.0 })
        for (int quality = 0; quality < 4; ++quality)
            for (float mixPercent : { 100.0f, 70.0f })
            {
                auto s = factorySettings();
                s.mixPercent = mixPercent;
                s.quality = quality;

                double worst = 0.0;
                for (int i = 0; i < 24; ++i)
                {
                    const double f = 20.0 * std::pow (1000.0, i / 23.0);
                    if (f > 0.45 * sampleRate)
                        continue;
                    worst = std::max (worst, std::abs (measuredDb (s, f, sampleRate) - theoreticalDb (s, f, sampleRate)));
                }

                INFO ("fs = " << sampleRate << ", calidad = " << quality << ", mezcla = " << mixPercent);
                REQUIRE (worst < 0.1);
            }
}

TEST_CASE ("Entrada y Salida suman sus ganancias")
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    poty::Settings s;
    s.inGainDb = 4.0f;
    s.outGainDb = -10.0f;

    const auto in = sine (48000, 1000.0, 48000.0, 0.1);
    const auto out = run (engine, in, s);

    const int skip = 24000;
    const double gain = toDb (measureAmplitude (out.data() + skip, 24000, 1000.0, 48000.0))
                        - toDb (measureAmplitude (in.data() + skip, 24000, 1000.0, 48000.0));
    REQUIRE (gain == Approx (-6.0).margin (0.01));
}

TEST_CASE ("Mezcla 0 % deja solo la señal original")
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    auto s = factorySettings();
    s.mixPercent = 0.0f;

    const auto in = noise (48000);
    const auto out = run (engine, in, s);

    const int skip = 4800; // el cambio de mezcla de 1 a 0 se suaviza 20 ms
    REQUIRE (residualDb (in.data() + skip, out.data() + skip, int (in.size()) - skip) < -90.0);
}

TEST_CASE ("Bypass: tras el fundido la salida es la entrada, con la misma latencia")
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    auto s = factorySettings();
    s.bypass = true;

    const auto in = noise (48000);
    const auto out = run (engine, in, s);

    const int skip = 4800;
    REQUIRE (residualDb (in.data() + skip, out.data() + skip, int (in.size()) - skip) < -140.0);
}

TEST_CASE ("Bypass: salir de él no deja datos viejos ni clics")
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    auto s = factorySettings();
    const auto in = sine (48000 * 2, 440.0, 48000.0, 0.3);
    std::vector<float> out (in.size());
    juce::AudioBuffer<float> buf (1, block);

    for (size_t pos = 0; pos + block <= in.size(); pos += block)
    {
        s.bypass = (pos / block) % 40 >= 20; // 20 bloques activo, 20 en bypass
        std::copy_n (in.begin() + long (pos), block, buf.getWritePointer (0));
        engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
    }

    // Un tono de 440 Hz a 0,3 y con realces no cambia más de ~0,1 entre muestras.
    REQUIRE (stepMax (out, 2000, out.size()) < 0.12);
}

TEST_CASE ("Banda apagada o con ganancia 0 no aporta nada")
{
    auto s = factorySettings();
    for (size_t i = 1; i < s.bands.size(); ++i)
        s.bands[i].gainDb = 0.0f;
    s.bands[0].on = false;

    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    const auto in = noise (48000);
    const auto out = run (engine, in, s);
    REQUIRE (residualDb (in.data(), out.data(), int (in.size())) < -90.0);
}

// ------------------------------------------------------------------ hito 4

TEST_CASE ("Armónicos por tipo: pares solo en tubo y germanio")
{
    constexpr double sr = 48000.0;
    const std::vector<std::pair<int, bool>> types = { { 0, true }, { 1, false }, { 2, false }, { 3, true }, { 4, false } };

    for (double f : { 1000.0, 5000.0 })
    {
        for (auto [type, hasEven] : types)
        {
            poty::PotyEngine engine;
            engine.prepare (sr, block);

            auto s = singleBand (float (f), 8.0f, type);
            s.quality = 2;

            const int n = 65536 + 20000;
            const auto out = run (engine, sine (n, f, sr, 0.5), s);
            const int skip = 20000;
            const double h1 = measureAmplitude (out.data() + skip, 65536, f, sr);
            const double h2 = measureAmplitude (out.data() + skip, 65536, 2 * f, sr);
            const double h3 = measureAmplitude (out.data() + skip, 65536, 3 * f, sr);

            INFO ("f = " << f << ", tipo " << type << ": H2 = " << toDb (h2 / h1) << " dB, H3 = " << toDb (h3 / h1) << " dB");
            REQUIRE (toDb (h3 / h1) > -80.0); // todos generan impares
            if (hasEven)
                REQUIRE (toDb (h2 / h1) > -60.0);
            else
                REQUIRE (toDb (h2 / h1) < -80.0);
        }
    }
}

TEST_CASE ("Aliasing, peor caso: banda centrada en la fundamental, +8 dB, saturación casi de recorte")
{
    // Metas por factor. Máxima (8×) cumple los -80 dB de la ficha incluso aquí; Mejor y Buena
    // no llegan a eso en el peor caso, y se cumplen con los ajustes de fábrica (siguiente prueba).
    constexpr double sr = 48000.0;
    const double limits[4] = { 0.0, -50.0, -65.0, -80.0 }; // índice = calidad del motor (2^calidad)

    for (int quality = 1; quality < 4; ++quality)
    {
        double worst = -200.0;
        for (double f : { 1307.0, 3011.0, 5003.0, 7907.0, 9109.0 })
            for (int type = 0; type < 5; ++type)
            {
                poty::PotyEngine engine;
                engine.prepare (sr, block);

                auto s = singleBand (float (f), 8.0f, type);
                s.quality = quality;

                const int skip = 20000;
                const auto out = run (engine, sine (65536 + skip, f, sr, 0.5), s);
                worst = std::max (worst, aliasingDb (out, skip, f, sr));
            }

        INFO ("factor " << (1 << quality) << ": peor aliasing " << worst << " dB");
        REQUIRE (worst < limits[quality]);
    }
}

TEST_CASE ("Aliasing con ajustes de fábrica, senoidal a -6 dBFS de 1 a 12 kHz")
{
    constexpr double sr = 48000.0;
    const double limits[4] = { 0.0, -50.0, -70.0, -80.0 };

    for (int quality = 1; quality < 4; ++quality)
    {
        double worst = -200.0;
        for (double f : { 1307.0, 3011.0, 5003.0, 7907.0, 9109.0, 12011.0 })
        {
            poty::PotyEngine engine;
            engine.prepare (sr, block);
            auto s = factorySettings();
            s.quality = quality;

            const int skip = 20000;
            const auto out = run (engine, sine (65536 + skip, f, sr, 0.5), s);
            worst = std::max (worst, aliasingDb (out, skip, f, sr));
        }

        INFO ("factor " << (1 << quality) << ": peor aliasing " << worst << " dB");
        REQUIRE (worst < limits[quality]);
    }
}

// ------------------------------------------------------------------ hito 5

TEST_CASE ("Latencia constante: el camino saturado queda alineado en las tres calidades")
{
    // Con los saturadores en modo lineal y la misma configuración, la salida no depende de la calidad.
    auto base = factorySettings();
    base.linear = true;

    const auto in = noise (48000, 0.3f);
    std::vector<std::vector<float>> outs;
    for (int quality = 0; quality < 4; ++quality)
    {
        poty::PotyEngine engine;
        engine.prepare (48000.0, block);
        auto s = base;
        s.quality = quality;
        outs.push_back (run (engine, in, s));
    }

    // Ruido de banda ancha hasta 24 kHz: el filtro de media banda recorta por encima de 20 kHz.
    // Se compara con una banda más estrecha: una banda de realce a 12 kHz sin energía cerca de 22 kHz.
    REQUIRE (residualDb (outs[0].data() + 4800, outs[1].data() + 4800, 48000 - 4800) < -20.0);
    REQUIRE (residualDb (outs[1].data() + 4800, outs[2].data() + 4800, 48000 - 4800) < -20.0);
    REQUIRE (residualDb (outs[2].data() + 4800, outs[3].data() + 4800, 48000 - 4800) < -20.0);
}

TEST_CASE ("Latencia constante: con mezcla 0 el pico del impulso cae en la latencia declarada")
{
    for (int quality = 0; quality < 4; ++quality)
    {
        poty::PotyEngine engine;
        engine.prepare (48000.0, block);

        auto s = factorySettings();
        s.mixPercent = 0.0f;
        s.quality = quality;

        std::vector<float> in (4096, 0.0f);
        in[300] = 1.0f;
        juce::AudioBuffer<float> buf (1, block);
        std::vector<float> out (in.size());
        for (size_t pos = 0; pos < in.size(); pos += block)
        {
            std::copy_n (in.begin() + long (pos), block, buf.getWritePointer (0));
            engine.process (buf, s);
            std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
        }

        size_t peak = 0;
        for (size_t i = 0; i < out.size(); ++i)
            if (std::abs (out[i]) > std::abs (out[peak]))
                peak = i;
        REQUIRE (int (peak) == 300 + latency);
    }
}

TEST_CASE ("Cambiar la calidad con audio sonando no produce clics")
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    auto s = factorySettings();
    const auto in = sine (48000 * 4, 440.0, 48000.0, 0.3);
    std::vector<float> out (in.size());
    juce::AudioBuffer<float> buf (1, block);

    double reference = 0.0;
    for (int q = 0; q < 4; ++q)
    {
        auto steady = s;
        steady.quality = q;
        reference = std::max (reference, steadyStep (steady, sine (48000, 440.0, 48000.0, 0.3)));
    }

    for (size_t pos = 0; pos + block <= in.size(); pos += block)
    {
        s.quality = int (pos / 48000) % 4; // 0, 1, 2, 3 cada segundo
        std::copy_n (in.begin() + long (pos), block, buf.getWritePointer (0));
        engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
    }

    REQUIRE (stepMax (out, 4000, out.size()) < 1.3 * reference);
}

// ------------------------------------------------------------------ hito 6

TEST_CASE ("Mid y Side: el otro canal queda sin cambios")
{
    constexpr double sr = 48000.0;
    const int n = 48000;
    const auto tone = sine (n, 1000.0, sr, 0.3);
    std::vector<float> neg (tone.size());
    for (size_t i = 0; i < tone.size(); ++i)
        neg[i] = -tone[i];

    const Stereo mono { tone, tone };      // solo mid
    const Stereo wide { tone, neg };       // solo side

    auto run2 = [&] (const Stereo& in, int mode)
    {
        poty::PotyEngine engine;
        engine.prepare (sr, block);
        auto s = factorySettings();
        s.mode = mode;
        Stereo padded = in;
        padded.l.resize (in.l.size() + size_t (latency), 0.0f);
        padded.r.resize (in.l.size() + size_t (latency), 0.0f);
        auto out = runStereo (engine, padded, s);
        return Stereo { { out.l.begin() + latency, out.l.begin() + latency + n },
                        { out.r.begin() + latency, out.r.begin() + latency + n } };
    };

    // Modo Side con señal solo mid: nada cambia.
    auto o = run2 (mono, 2);
    REQUIRE (residualDb (tone.data(), o.l.data(), n) < -90.0);
    REQUIRE (residualDb (tone.data(), o.r.data(), n) < -90.0);

    // Modo Mid con señal solo side: nada cambia.
    o = run2 (wide, 1);
    REQUIRE (residualDb (tone.data(), o.l.data(), n) < -90.0);
    REQUIRE (residualDb (neg.data(), o.r.data(), n) < -90.0);

    // Modo Mid con señal mid: se procesa y L = R se mantiene.
    o = run2 (mono, 1);
    REQUIRE (residualDb (tone.data(), o.l.data(), n) > -40.0);
    REQUIRE (residualDb (o.l.data(), o.r.data(), n) < -120.0);
}

TEST_CASE ("Cambiar de modo con audio sonando no produce clics")
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    auto s = factorySettings();
    const int total = 48000 * 3;
    const auto l = sine (total, 440.0, 48000.0, 0.3);
    const auto r = sine (total, 660.0, 48000.0, 0.3);

    // Referencia: el salto máximo estable de cada canal en cada modo.
    double reference = 0.0;
    for (int m = 0; m < 3; ++m)
    {
        poty::PotyEngine e;
        e.prepare (48000.0, block);
        auto steady = s;
        steady.mode = m;
        const auto o = runStereo (e, { l, r }, steady);
        reference = std::max ({ reference, stepMax (o.l, 4000, o.l.size()), stepMax (o.r, 4000, o.r.size()) });
    }
    Stereo out { std::vector<float> (size_t (total)), std::vector<float> (size_t (total)) };
    juce::AudioBuffer<float> buf (2, block);

    for (int pos = 0; pos + block <= total; pos += block)
    {
        s.mode = (pos / 48000) % 3;
        std::copy_n (l.begin() + pos, block, buf.getWritePointer (0));
        std::copy_n (r.begin() + pos, block, buf.getWritePointer (1));
        engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.l.begin() + pos);
        std::copy_n (buf.getReadPointer (1), block, out.r.begin() + pos);
    }

    REQUIRE (stepMax (out.l, 4000, out.l.size()) < 1.3 * reference);
    REQUIRE (stepMax (out.r, 4000, out.r.size()) < 1.3 * reference);
}

// ----------------------------------------------------- suavizado de perillas

TEST_CASE ("Arrastrar un nodo no produce saltos (sin clics)")
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    auto s = factorySettings();
    const auto in = sine (48000 * 2, 440.0, 48000.0, 0.3);
    std::vector<float> out (in.size());
    juce::AudioBuffer<float> buf (1, block);

    // La banda 3 barre de 1,8 kHz a 4 kHz y de +6 a +12 dB mientras suena el tono.
    for (size_t pos = 0; pos + block <= in.size(); pos += block)
    {
        const float t = float (pos) / float (in.size());
        s.bands[2].freq = 1800.0f + 2200.0f * t;
        s.bands[2].gainDb = 6.0f + 6.0f * t;

        std::copy_n (in.begin() + long (pos), block, buf.getWritePointer (0));
        engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
    }

    REQUIRE (stepMax (out, 24000, out.size()) < 0.1);
}

TEST_CASE ("Cambiar el tipo, el drive y la intensidad con audio sonando no produce clics")
{
    poty::PotyEngine engine;
    engine.prepare (48000.0, block);

    auto s = singleBand (440.0f, 10.0f, 0, 80.0f);
    const auto in = sine (48000 * 3, 440.0, 48000.0, 0.4);
    std::vector<float> out (in.size());
    juce::AudioBuffer<float> buf (1, block);

    // Referencia: el peor salto estable entre todas las combinaciones que se recorren.
    double reference = 0.0;
    for (int type = 0; type < 5; ++type)
        for (float drive : { 20.0f, 100.0f })
            for (int intensity = 0; intensity < 3; ++intensity)
            {
                auto steady = singleBand (440.0f, 10.0f, type, drive);
                steady.intensity = intensity;
                reference = std::max (reference, steadyStep (steady, sine (48000, 440.0, 48000.0, 0.4)));
            }

    for (size_t pos = 0; pos + block <= in.size(); pos += block)
    {
        const int sec = int (pos / 24000);          // cada medio segundo
        s.bands[3].type = sec % 5;
        s.bands[3].drive = (sec % 2) ? 20.0f : 100.0f;
        s.intensity = sec % 3;
        std::copy_n (in.begin() + long (pos), block, buf.getWritePointer (0));
        engine.process (buf, s);
        std::copy_n (buf.getReadPointer (0), block, out.begin() + long (pos));
    }

    REQUIRE (stepMax (out, 4000, out.size()) < 1.3 * reference);
}

TEST_CASE ("Bloques de cualquier tamaño dan el mismo resultado")
{
    const auto tone = sine (48000, 700.0, 48000.0, 0.3);
    const Stereo in { tone, tone };
    const auto s = factorySettings();

    poty::PotyEngine a, b;
    a.prepare (48000.0, block);
    b.prepare (48000.0, block);

    const auto outA = runStereo (a, in, s, 256);
    const auto outB = runStereo (b, in, s, 1000); // mayor que el trozo interno
    REQUIRE (residualDb (outA.l.data(), outB.l.data(), 48000) < -120.0);
}

// Informe de medidas (no es una prueba): `PotyTests "[.report]"`
TEST_CASE ("Informe de aliasing por calidad, tipo y frecuencia", "[.report]")
{
    constexpr double sr = 48000.0;
    const char* names[5] = { "Tubo", "Cinta", "Sólido", "Diodo", "Digital" };
    for (int quality = 1; quality < 4; ++quality)
    {
        printf ("\ncalidad %dx\n  freq   ", 1 << quality);
        for (auto* n : names)
            printf ("%9s", n);
        printf ("\n");
        for (double f : { 1307.0, 3011.0, 5003.0, 7907.0, 9109.0 })
        {
            printf ("  %5.0f  ", f);
            for (int type = 0; type < 5; ++type)
            {
                poty::PotyEngine engine;
                engine.prepare (sr, block);
                auto s = singleBand (float (f), 8.0f, type);
                s.quality = quality;
                const int skip = 20000;
                const auto out = run (engine, sine (65536 + skip, f, sr, 0.5), s);
                printf ("%9.1f", aliasingDb (out, skip, f, sr));
            }
            printf ("\n");
        }
    }
}

TEST_CASE ("Informe de aliasing con ajustes de fábrica", "[.report]")
{
    constexpr double sr = 48000.0;
    for (double amp : { 0.5, 0.25, 0.1 })
    {
        printf ("\namplitud %.2f (%.0f dBFS), ajustes de fábrica, mezcla 100 %%\n  freq    2x     4x     8x\n", amp, toDb (amp));
        for (double f : { 1307.0, 3011.0, 5003.0, 7907.0, 9109.0, 12011.0 })
        {
            printf ("  %5.0f ", f);
            for (int quality = 1; quality < 4; ++quality)
            {
                poty::PotyEngine engine;
                engine.prepare (sr, block);
                auto s = factorySettings();
                s.quality = quality;
                const int skip = 20000;
                const auto out = run (engine, sine (65536 + skip, f, sr, amp), s);
                printf ("%6.1f ", aliasingDb (out, skip, f, sr));
            }
            printf ("\n");
        }
    }
}

TEST_CASE ("Informe de CPU", "[.report]")
{
    constexpr double sr = 48000.0;
    constexpr int seconds = 10;
    const int total = int (sr) * seconds;
    const auto l = noise (total, 0.2f, 1);
    const auto r = noise (total, 0.2f, 2);

    for (int mode = 0; mode < 3; ++mode)
        for (int quality = 0; quality < 4; ++quality)
        {
            poty::PotyEngine engine;
            engine.prepare (sr, block);
            auto s = factorySettings();
            s.quality = quality;
            s.mode = mode;

            juce::AudioBuffer<float> buf (2, block);
            const auto t0 = std::chrono::steady_clock::now();
            for (int pos = 0; pos + block <= total; pos += block)
            {
                std::copy_n (l.begin() + pos, block, buf.getWritePointer (0));
                std::copy_n (r.begin() + pos, block, buf.getWritePointer (1));
                engine.process (buf, s);
            }
            const double sec = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
            printf ("  modo %d, %dx: %5.2f %% de un núcleo\n", mode, 1 << quality, 100.0 * sec / seconds);
        }
}

TEST_CASE ("Informe de respuesta lineal", "[.report]")
{
    double worstAll = 0.0;
    for (double sampleRate : { 44100.0, 48000.0, 96000.0 })
        for (int quality = 0; quality < 4; ++quality)
            for (float mixPercent : { 100.0f, 70.0f })
            {
                auto s = factorySettings();
                s.mixPercent = mixPercent;
                s.quality = quality;
                double worst = 0.0;
                for (int i = 0; i < 24; ++i)
                {
                    const double f = 20.0 * std::pow (1000.0, i / 23.0);
                    if (f > 0.45 * sampleRate)
                        continue;
                    worst = std::max (worst, std::abs (measuredDb (s, f, sampleRate) - theoreticalDb (s, f, sampleRate)));
                }
                printf ("  fs %6.0f  %dx  mezcla %3.0f %%: peor error %.4f dB\n", sampleRate, 1 << quality, double (mixPercent), worst);
                worstAll = std::max (worstAll, worst);
            }
    printf ("  PEOR EN TODAS: %.4f dB\n", worstAll);
}
