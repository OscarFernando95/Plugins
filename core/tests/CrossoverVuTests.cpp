#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "Crossover.h"
#include "TestUtils.h"
#include "VuBallistics.h"

using namespace testutil;

TEST_CASE ("Cruce de 3 bandas: la suma de las bandas tiene magnitud plana (±0,1 dB)")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (double freq : { 40.0, 150.0, 200.0, 350.0, 1000.0, 2800.0, 3000.0, 5000.0, 12000.0 })
        {
            ferb::ThreeBandSplit split;
            split.setFrequencies (fs, 200.0, 3000.0);
            const int n = int (fs * 1.5);
            const auto in = sine (n, freq, fs, 0.5);
            std::vector<float> out (in.size());
            std::array<float, 3> bands {};
            for (size_t i = 0; i < in.size(); ++i)
            {
                split.process (in[i], bands);
                out[i] = bands[0] + bands[1] + bands[2];
            }
            const int skip = n / 2, len = n - skip;
            const double ratio = measureAmplitude (out.data() + skip, len, freq, fs) / measureAmplitude (in.data() + skip, len, freq, fs);
            INFO ("fs " << fs << ", f " << freq);
            CHECK (toDb (ratio) == Catch::Approx (0.0).margin (0.1));
        }
}

TEST_CASE ("Cruce de 3 bandas: cada banda atenúa fuera de su zona")
{
    const double fs = 48000.0;
    ferb::ThreeBandSplit split;
    split.setFrequencies (fs, 200.0, 3000.0);
    auto bandLevel = [&] (double freq, int band)
    {
        split.reset();
        const int n = int (fs);
        const auto in = sine (n, freq, fs, 0.5);
        std::vector<float> out (in.size());
        std::array<float, 3> bands {};
        for (size_t i = 0; i < in.size(); ++i)
        {
            split.process (in[i], bands);
            out[i] = bands[size_t (band)];
        }
        return toDb (measureAmplitude (out.data() + n / 2, n / 2, freq, fs) / 0.5);
    };
    CHECK (bandLevel (50.0, 0) > -0.5);
    CHECK (bandLevel (50.0, 1) < -40.0);
    CHECK (bandLevel (1000.0, 1) > -0.5);
    CHECK (bandLevel (1000.0, 0) < -20.0);
    CHECK (bandLevel (10000.0, 2) > -0.5);
    CHECK (bandLevel (10000.0, 1) < -30.0);
}

TEST_CASE ("VU: una senoidal llega al 99 % en 300 ms y marca su valor eficaz")
{
    const double fs = 48000.0;
    ferb::VuBallistics vu;
    vu.prepare (fs);

    const auto tone = sine (int (fs * 3.0), 1000.0, fs, 0.5);
    float atAbout300 = 0.0f;
    for (size_t pos = 0; pos + 48 <= tone.size(); pos += 48)
    {
        const float* ch[1] = { tone.data() + pos };
        vu.process (ch, 1, 48);
        if (pos + 48 == size_t (fs * 0.3))
            atAbout300 = vu.level();
    }
    const float final = vu.level();
    CHECK (final == Catch::Approx (0.5f / std::sqrt (2.0f)).epsilon (0.005)); // RMS de la senoidal
    CHECK (atAbout300 / final == Catch::Approx (0.99).margin (0.006));
}
