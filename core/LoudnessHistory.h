#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace ferb
{
/** Historial de sonoridad de hasta 10 h con resolución decreciente para lo antiguo.

    Se le da un punto por segundo (M, S y true peak). Guarda tres niveles, cada uno un anillo:
        1 s  × 600 =  10 min
        10 s × 720 =  2 h
        60 s × 600 = 10 h
    Al pasar a un nivel más grueso, M y S se promedian en energía y el true peak se queda con el máximo.
    Un solo hilo (el de mensajes). La memoria se reserva al construir. */
class LoudnessHistory
{
public:
    static constexpr float noValue = -200.0f;

    struct Point
    {
        float momentary = noValue, shortTerm = noValue, truePeak = -100.0f;
    };

    struct Tier
    {
        int capacity;
        int seconds; // por punto
    };

    static constexpr std::array<Tier, 3> tiers { { { 600, 1 }, { 720, 10 }, { 600, 60 } } };

    LoudnessHistory() { clear(); }

    void clear()
    {
        for (size_t t = 0; t < tiers.size(); ++t)
        {
            rings[t].assign (static_cast<size_t> (tiers[t].capacity), Point {});
            counts[t] = 0;
            acc[t] = {};
        }
    }

    /** Un punto por segundo. Los valores sin dato (−200) cuentan como energía cero. */
    void push (float momentary, float shortTerm, float truePeakDb)
    {
        add (0, { momentary, shortTerm, truePeakDb });
    }

    /** Los puntos del nivel `tier`, del más antiguo al más reciente. */
    std::vector<Point> snapshot (size_t tier) const
    {
        const auto& ring = rings[tier];
        const int cap = tiers[tier].capacity;
        const int n = std::min (counts[tier], cap);
        std::vector<Point> out (static_cast<size_t> (n));
        for (int i = 0; i < n; ++i)
            out[static_cast<size_t> (i)] = ring[static_cast<size_t> (((counts[tier] - n + i) % cap + cap) % cap)];
        return out;
    }

    /** Segundos que cubre cada punto del nivel. */
    static int secondsPerPoint (size_t tier) { return tiers[tier].seconds; }

private:
    struct Accumulator
    {
        double energyM = 0.0, energyS = 0.0;
        float peak = -100.0f;
        int n = 0;
    };

    static double toEnergy (float lufs) { return lufs > -150.0f ? std::pow (10.0, (static_cast<double> (lufs) + 0.691) / 10.0) : 0.0; }
    static float toLufs (double energy) { return energy > 0.0 ? static_cast<float> (-0.691 + 10.0 * std::log10 (energy)) : noValue; }

    void add (size_t tier, const Point& p)
    {
        rings[tier][static_cast<size_t> (counts[tier] % tiers[tier].capacity)] = p;
        ++counts[tier];

        if (tier + 1 >= tiers.size())
            return;

        auto& a = acc[tier + 1];
        a.energyM += toEnergy (p.momentary);
        a.energyS += toEnergy (p.shortTerm);
        a.peak = std::max (a.peak, p.truePeak);
        const int ratio = tiers[tier + 1].seconds / tiers[tier].seconds;
        if (++a.n >= ratio)
        {
            const Point merged { toLufs (a.energyM / a.n), toLufs (a.energyS / a.n), a.peak };
            a = {};
            add (tier + 1, merged);
        }
    }

    std::array<std::vector<Point>, 3> rings;
    std::array<int, 3> counts {};
    std::array<Accumulator, 3> acc {};
};
} // namespace ferb
