#pragma once

#include <Adaa.h>
#include <Biquad.h>
#include <Saturators.h>

#include <vector>

namespace poty
{
/** Saturador de una banda en un canal de trabajo, a la frecuencia de muestreo del camino saturado.

    Aplica g(u) = f(d·u) / d con ADAA de primer orden. La cinta lleva un estante de agudos de
    +3 dB antes de la curva y su inverso después. Al cambiar el tipo se mezclan 10 ms de la
    curva vieja y la nueva para que el cambio no suene como un golpe. */
class BandSaturator
{
public:
    /** Reserva el búfer de trabajo (una sola vez). */
    void prepare (int maxBlock) { scratch.assign (static_cast<size_t> (maxBlock), 0.0f); }

    /** Fija la frecuencia del camino saturado (sin asignar) y olvida el tipo. */
    void setSampleRate (double osSampleRate) noexcept
    {
        sampleRate = osSampleRate;
        fadeLength = static_cast<int> (0.010 * osSampleRate);
        reset();
    }

    void reset() noexcept
    {
        cur.reset();
        prev.reset();
        fadeLeft = 0;
        hasType = false;
    }

    void setType (ferb::SatType type)
    {
        if (hasType && type == cur.type)
            return;

        if (hasType)
        {
            prev = cur;
            fadeLeft = fadeLength;
        }

        cur.configure (type, sampleRate);
        hasType = true;
    }

    /** `u` entra con Δ y sale con g(Δ); `d` es el drive efectivo por muestra. */
    void process (float* u, const float* d, int n) noexcept
    {
        if (fadeLeft <= 0)
        {
            cur.run (u, d, n);
            return;
        }

        std::copy (u, u + n, scratch.begin());
        prev.run (scratch.data(), d, n);
        cur.run (u, d, n);

        for (int i = 0; i < n; ++i)
        {
            const float t = fadeLeft > 0 ? 1.0f - static_cast<float> (fadeLeft) / static_cast<float> (fadeLength) : 1.0f;
            u[i] = scratch[static_cast<size_t> (i)] * (1.0f - t) + u[i] * t;
            if (fadeLeft > 0)
                --fadeLeft;
        }
    }

private:
    struct Voice
    {
        ferb::Adaa adaa;
        ferb::Biquad pre, post;
        ferb::SatType type = ferb::SatType::Solid;

        void reset() noexcept
        {
            adaa.reset();
            pre.reset();
            post.reset();
        }

        void configure (ferb::SatType t, double rate)
        {
            type = t;
            reset();
            if (t == ferb::SatType::Tape)
            {
                pre = ferb::Biquad::highShelf (rate, 2000.0, 3.0);
                post = pre.inverse();
            }
        }

        void run (float* u, const float* d, int n) noexcept
        {
            const bool tape = type == ferb::SatType::Tape;
            for (int i = 0; i < n; ++i)
            {
                const float x = tape ? pre.process (u[i]) : u[i];
                const double drive = d[i];
                const float y = static_cast<float> (adaa.process (type, drive * x) / drive);
                u[i] = tape ? post.process (y) : y;
            }
        }
    };

    Voice cur, prev;
    std::vector<float> scratch;
    double sampleRate = 48000.0;
    int fadeLength = 480, fadeLeft = 0;
    bool hasType = false;
};
} // namespace poty
