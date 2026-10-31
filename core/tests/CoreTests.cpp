#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "Adaa.h"
#include "Biquad.h"
#include "DelayLine.h"
#include "ModeSwitch.h"
#include "Oversampler.h"
#include "TestUtils.h"

using namespace testutil;

namespace
{
/** Pasa x por subir + bajar, en bloques de 128, y devuelve la salida (mismo largo). */
std::vector<float> roundTrip (int factor, const std::vector<float>& x)
{
    constexpr int block = 128;
    ferb::UpSampler up;
    ferb::DownSampler down;
    up.prepare (factor, block);
    down.prepare (factor, block);

    std::vector<float> y (x.size()), os (size_t (block) * factor);
    for (size_t pos = 0; pos + block <= x.size(); pos += block)
    {
        up.process (x.data() + pos, os.data(), block);
        down.process (os.data(), y.data() + pos, block);
    }
    return y;
}
} // namespace

TEST_CASE ("Sobremuestreo: la latencia es un número entero y coincide con la declarada")
{
    for (int factor : { 1, 2, 4, 8 })
    {
        std::vector<float> x (2048, 0.0f);
        x[256] = 1.0f;
        const auto y = roundTrip (factor, x);

        size_t peak = 0;
        for (size_t i = 0; i < y.size(); ++i)
            if (std::abs (y[i]) > std::abs (y[peak]))
                peak = i;

        INFO ("factor " << factor);
        REQUIRE (int (peak) - 256 == ferb::Oversampler::latencyFor (factor));

        // Ganancia en continua = suma de la respuesta al impulso = 1.
        double sum = 0.0;
        for (float v : y)
            sum += v;
        REQUIRE (sum == Catch::Approx (1.0).margin (1e-4));
    }
}

TEST_CASE ("Sobremuestreo: banda de paso plana hasta 20 kHz")
{
    constexpr double sr = 48000.0;
    for (int factor : { 2, 4, 8 })
    {
        for (double f : { 100.0, 1000.0, 8000.0, 15000.0, 20000.0 })
        {
            const auto x = sine (int (sr), f, sr, 0.5);
            const auto y = roundTrip (factor, x);
            const int skip = 8192;
            const double gainDb = toDb (measureAmplitude (y.data() + skip, int (sr) - skip, f, sr))
                                  - toDb (measureAmplitude (x.data() + skip, int (sr) - skip, f, sr));
            INFO ("factor " << factor << ", " << f << " Hz");
            REQUIRE (std::abs (gainDb) < 0.02);
        }
    }
}

TEST_CASE ("Sobremuestreo: las imágenes quedan 90 dB por debajo")
{
    constexpr double sr = 48000.0;
    constexpr int block = 128;
    const double f = 10000.0;
    const auto x = sine (int (sr), f, sr, 0.5);

    for (int factor : { 2, 4, 8 })
    {
        ferb::UpSampler up;
        up.prepare (factor, block);
        std::vector<float> os (size_t (x.size()) * factor);
        for (size_t pos = 0; pos + block <= x.size(); pos += block)
            up.process (x.data() + pos, os.data() + pos * factor, block);

        const double fsOs = sr * factor;
        const int skip = 16384, len = 32768;
        const double tone = measureAmplitude (os.data() + skip, len, f, fsOs);

        // Primeras imágenes: sr − f, sr + f.
        for (double image : { sr - f, sr + f })
        {
            if (image > fsOs / 2)
                continue;
            const double a = measureAmplitude (os.data() + skip, len, image, fsOs);
            INFO ("factor " << factor << ", imagen " << image << " Hz");
            REQUIRE (toDb (a) - toDb (tone) < -90.0);
        }
    }
}

TEST_CASE ("Línea de retardo")
{
    ferb::DelayLine d;
    d.prepare (100);
    d.setDelay (45);

    std::vector<float> out;
    for (int i = 0; i < 100; ++i)
        out.push_back (d.process (i == 0 ? 1.0f : 0.0f));

    for (int i = 0; i < 100; ++i)
        REQUIRE (out[size_t (i)] == (i == 45 ? 1.0f : 0.0f));
}

TEST_CASE ("Saturadores: pendiente 1 en cero y F es la antiderivada de f")
{
    using ferb::SatType;
    namespace s = ferb::saturators;

    for (int t = 0; t < ferb::numSatTypes; ++t)
    {
        const auto type = static_cast<SatType> (t);
        INFO ("tipo " << t);

        REQUIRE (s::f (type, 0.0) == Catch::Approx (0.0).margin (1e-12));

        const double h = 1e-6;
        REQUIRE ((s::f (type, h) - s::f (type, -h)) / (2.0 * h) == Catch::Approx (1.0).epsilon (1e-4));

        // F' = f lejos de los codos.
        for (double v : { -3.0, -1.7, -0.4, 0.3, 0.8, 2.2, 6.0 })
        {
            if (type == SatType::Digital && std::abs (std::abs (v) - 1.0) < 0.01)
                continue;
            const double d = (s::F (type, v + h) - s::F (type, v - h)) / (2.0 * h);
            REQUIRE (d == Catch::Approx (s::f (type, v)).margin (1e-5));
        }
    }

    // La antiderivada del diodo de germanio es continua en 0.
    REQUIRE (s::F (SatType::GermaniumDiode, 1e-9) == Catch::Approx (s::F (SatType::GermaniumDiode, -1e-9)).margin (1e-7));
    // logCosh estable con valores enormes.
    REQUIRE (std::isfinite (s::logCosh (1000.0)));
    REQUIRE (s::logCosh (1000.0) == Catch::Approx (1000.0 - 0.6931471805599453).epsilon (1e-12));
}

TEST_CASE ("Saturadores: asimetría de tubo y germanio, simetría de cinta y sólido")
{
    using ferb::SatType;
    namespace s = ferb::saturators;
    REQUIRE (std::abs (s::f (SatType::Tube, 1.0) + s::f (SatType::Tube, -1.0)) > 0.01);
    REQUIRE (std::abs (s::f (SatType::GermaniumDiode, 1.0) + s::f (SatType::GermaniumDiode, -1.0)) > 0.01);
    REQUIRE (s::f (SatType::Tape, 1.0) + s::f (SatType::Tape, -1.0) == Catch::Approx (0.0).margin (1e-12));
    REQUIRE (s::f (SatType::Solid, 1.0) + s::f (SatType::Solid, -1.0) == Catch::Approx (0.0).margin (1e-12));
}

TEST_CASE ("ADAA: con paso fino coincide con f y con paso grueso es el promedio exacto")
{
    using ferb::SatType;
    namespace s = ferb::saturators;

    for (int t = 0; t < ferb::numSatTypes; ++t)
    {
        const auto type = static_cast<SatType> (t);
        ferb::Adaa a;

        double v = 0.3;
        a.process (type, v);
        double y = a.process (type, v + 1e-6);
        REQUIRE (y == Catch::Approx (s::f (type, v)).margin (1e-4));

        // Promedio de f entre dos valores, integrado numéricamente (Simpson, 2000 pasos).
        ferb::Adaa b;
        b.process (type, -0.8);
        const double got = b.process (type, 2.5);
        double integral = 0.0;
        const int steps = 2000;
        const double h = (2.5 + 0.8) / steps;
        for (int i = 0; i <= steps; ++i)
        {
            const double w = (i == 0 || i == steps) ? 1.0 : (i % 2 ? 4.0 : 2.0);
            integral += w * s::f (type, -0.8 + i * h);
        }
        integral *= h / 3.0;
        INFO ("tipo " << t);
        REQUIRE (got == Catch::Approx (integral / 3.3).margin (2e-4));
    }
}

TEST_CASE ("Estante de agudos de la cinta: el filtro inverso lo deshace")
{
    const auto pre = ferb::Biquad::highShelf (96000.0, 2000.0, 3.0);
    auto a = pre;
    auto b = pre.inverse();

    const auto x = noise (96000);
    double worst = 0.0;
    for (size_t i = 0; i < x.size(); ++i)
    {
        const float y = b.process (a.process (x[i]));
        if (i > 2000)
            worst = std::max (worst, std::abs (double (y) - x[i]));
    }
    REQUIRE (toDb (worst) < -110.0);

    // +3 dB en agudos.
    auto s = pre;
    const auto t = sine (96000, 20000.0, 96000.0, 0.1);
    std::vector<float> o (t.size());
    for (size_t i = 0; i < t.size(); ++i)
        o[i] = s.process (t[i]);
    REQUIRE (toDb (measureAmplitude (o.data() + 20000, 60000, 20000.0, 96000.0))
             - toDb (measureAmplitude (t.data() + 20000, 60000, 20000.0, 96000.0))
             == Catch::Approx (3.0).margin (0.2));
}

TEST_CASE ("ModeSwitch: el modo cambia solo en silencio y la ganancia es continua")
{
    ferb::ModeSwitch sw;
    sw.prepare (48000.0, 0.005, 256);
    sw.reset (0);

    const int n = 128;
    int switches = 0;
    float last = 1.0f;
    int modeBefore = 0;

    for (int i = 0; i < 40; ++i)
    {
        const auto step = sw.update (i < 2 ? 0 : 1, n);

        REQUIRE (std::abs (step.gainStart - last) < 1e-6f); // continua entre trozos
        last = step.gainEnd;

        if (step.switched)
        {
            ++switches;
            REQUIRE (step.gainStart == 0.0f);
            REQUIRE (step.gainEnd == 0.0f);
            REQUIRE (step.mode == 1);
        }
        else if (step.mode != modeBefore)
        {
            FAIL ("el modo cambió sin pasar por el silencio");
        }
        modeBefore = step.mode;
    }

    REQUIRE (switches == 1);
    REQUIRE (last == 1.0f); // terminó abierto
}
