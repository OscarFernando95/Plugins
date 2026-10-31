#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "OversamplerN.h"
#include "TestUtils.h"

using namespace testutil;

namespace
{
constexpr int block = 128;

/** Sube y baja en bloques de 128. Devuelve la señal bajada (misma longitud). */
std::vector<float> roundTrip (int factor, bool minPhase, const std::vector<float>& x, std::vector<float>* upsampled = nullptr)
{
    ferb::UpN up;
    ferb::DownN down;
    up.prepare (block);
    down.prepare (block);
    up.setConfig (factor, minPhase);
    down.setConfig (factor, minPhase);

    std::vector<float> y (x.size()), os (size_t (block) * size_t (factor));
    if (upsampled)
        upsampled->assign (x.size() * size_t (factor), 0.0f);
    for (size_t pos = 0; pos + block <= x.size(); pos += block)
    {
        up.process (x.data() + pos, os.data(), block);
        if (upsampled)
            std::copy (os.begin(), os.end(), upsampled->begin() + long (pos) * factor);
        down.process (os.data(), y.data() + pos, block);
    }
    return y;
}
} // namespace

TEST_CASE ("OversamplerN: la latencia es entera y coincide con el pico de un impulso, de 1× a 32×, en ambas fases")
{
    for (bool minPhase : { false, true })
        for (int factor : { 1, 2, 4, 8, 16, 32 })
        {
            std::vector<float> x (4096, 0.0f);
            x[1024] = 1.0f;
            const auto y = roundTrip (factor, minPhase, x);

            size_t peak = 0;
            for (size_t i = 0; i < y.size(); ++i)
                if (std::abs (y[i]) > std::abs (y[peak]))
                    peak = i;

            const int latency = ferb::OsDesign::latencyFor (factor, minPhase);
            INFO ("factor " << factor << ", fase mínima " << minPhase << ", latencia " << latency << ", pico en " << peak - 1024);
            if (minPhase)
                CHECK (std::abs (int (peak) - 1024 - latency) <= 1);
            else
                CHECK (int (peak) - 1024 == latency);
        }
}

TEST_CASE ("OversamplerN: respuesta plana en la banda de paso (±0,05 dB lineal, ±0,15 dB mínima)")
{
    for (bool minPhase : { false, true })
        for (int factor : { 2, 4, 8, 16, 32 })
            for (double f : { 100.0, 1000.0, 10000.0, 17000.0 })
            {
                const int n = 48000;
                const auto x = sine (n, f, 48000.0, 0.5);
                const auto y = roundTrip (factor, minPhase, x);
                const double gain = toDb (measureAmplitude (y.data() + 24000, 16384, f, 48000.0) / measureAmplitude (x.data() + 24000, 16384, f, 48000.0));
                INFO ("factor " << factor << ", fase mínima " << minPhase << ", " << f << " Hz");
                CHECK (std::abs (gain) < (minPhase ? 0.15 : 0.05));
            }
}

TEST_CASE ("OversamplerN: las imágenes quedan por debajo de −80 dB")
{
    for (bool minPhase : { false, true })
        for (int factor : { 2, 4, 8, 16, 32 })
        {
            const double fs = 48000.0, f = 12000.0;
            const int n = 32768;
            std::vector<float> os;
            roundTrip (factor, minPhase, sine (n, f, fs, 0.5), &os);

            // La primera imagen cae en fs − f = 36 kHz (con 2× o más, dentro del espectro sobremuestreado).
            const double osRate = fs * factor;
            const double fund = measureAmplitude (os.data() + 8192 * factor, 8192 * factor, f, osRate);
            const double image = measureAmplitude (os.data() + 8192 * factor, 8192 * factor, fs - f, osRate);
            INFO ("factor " << factor << ", fase mínima " << minPhase << ": imagen " << toDb (image / fund) << " dB");
            CHECK (toDb (image / fund) < -80.0);
        }
}
