#pragma once

#include <algorithm>
#include <array>

#include "Biquad.h"

namespace ferb
{
/** Cruce Linkwitz-Riley de 4.º orden (dos Butterworth de 2.º orden en cascada) de un canal.
    Paso bajo + paso alto suman con magnitud plana y sin latencia. */
class Lr4
{
public:
    void setFrequency (double sampleRate, double freq) noexcept
    {
        const auto lp = Biquad::lowPass (sampleRate, freq);
        const auto hp = Biquad::highPass (sampleRate, freq);
        lp1.setCoefficients (lp);
        lp2.setCoefficients (lp);
        hp1.setCoefficients (hp);
        hp2.setCoefficients (hp);
    }

    void reset() noexcept { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); }

    void process (float x, float& low, float& high) noexcept
    {
        low = lp2.process (lp1.process (x));
        high = hp2.process (hp1.process (x));
    }

private:
    Biquad lp1, lp2, hp1, hp2;
};

/** Tres bandas (grave, media, aguda) con cruces en `xo1` y `xo2`. La banda grave pasa además por un
    paso todo de fase en `xo2`, así que la suma de las tres tiene magnitud plana (y la misma fase que
    una señal con dos cruces). Un canal por instancia. */
class ThreeBandSplit
{
public:
    void setFrequencies (double sampleRate, double xo1, double xo2) noexcept
    {
        const double nyquistLimit = 0.45 * sampleRate;
        xo2 = std::min (xo2, nyquistLimit);
        xo1 = std::min (xo1, xo2 / 1.2);
        first.setFrequency (sampleRate, xo1);
        second.setFrequency (sampleRate, xo2);
        allpass.setFrequency (sampleRate, xo2);
    }

    void reset() noexcept { first.reset(); second.reset(); allpass.reset(); }

    void process (float x, std::array<float, 3>& bands) noexcept
    {
        float low, high;
        first.process (x, low, high);

        float apLow, apHigh;
        allpass.process (low, apLow, apHigh);
        bands[0] = apLow + apHigh;

        second.process (high, bands[1], bands[2]);
    }

private:
    Lr4 first, second, allpass;
};

/** Cuatro bandas con cruces en `xo1` < `xo2` < `xo3`. Se parte en `xo2` y cada mitad se parte de nuevo
    (`xo1` abajo, `xo3` arriba); las bandas de una mitad pasan además por el paso todo del cruce de la otra,
    así que las cuatro suman con magnitud plana. Un canal por instancia. */
class FourBandSplit
{
public:
    void setFrequencies (double sampleRate, double xo1, double xo2, double xo3) noexcept
    {
        const double nyquistLimit = 0.45 * sampleRate;
        xo3 = std::min (xo3, nyquistLimit);
        xo2 = std::min (xo2, xo3 / 1.2);
        xo1 = std::min (xo1, xo2 / 1.2);
        mid.setFrequency (sampleRate, xo2);
        lowSplit.setFrequency (sampleRate, xo1);
        highSplit.setFrequency (sampleRate, xo3);
        lowAllpass[0].setFrequency (sampleRate, xo3);
        lowAllpass[1].setFrequency (sampleRate, xo3);
        highAllpass[0].setFrequency (sampleRate, xo1);
        highAllpass[1].setFrequency (sampleRate, xo1);
    }

    void reset() noexcept
    {
        mid.reset();
        lowSplit.reset();
        highSplit.reset();
        for (auto& a : lowAllpass)
            a.reset();
        for (auto& a : highAllpass)
            a.reset();
    }

    void process (float x, std::array<float, 4>& bands) noexcept
    {
        float low, high, l0, l1, h0, h1, lo, hi;
        mid.process (x, low, high);
        lowSplit.process (low, l0, l1);
        highSplit.process (high, h0, h1);

        lowAllpass[0].process (l0, lo, hi);
        bands[0] = lo + hi;
        lowAllpass[1].process (l1, lo, hi);
        bands[1] = lo + hi;
        highAllpass[0].process (h0, lo, hi);
        bands[2] = lo + hi;
        highAllpass[1].process (h1, lo, hi);
        bands[3] = lo + hi;
    }

private:
    Lr4 mid, lowSplit, highSplit;
    std::array<Lr4, 2> lowAllpass, highAllpass;
};
} // namespace ferb
