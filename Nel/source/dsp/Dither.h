#pragma once

#include <array>
#include <cmath>
#include <cstdint>

namespace nel
{
/** Dither triangular (TPDF) con cuantización a `bits` y formador de ruido por realimentación del
    error. Un canal por instancia. Perfiles: 0 plano, 1 suave, 2 medio, 3 fuerte. */
class Dither
{
public:
    static constexpr int maxTaps = 5;

    void reset (std::uint32_t seed) noexcept { error.fill (0.0f); state = seed * 2654435761u + 1u; }

    void configure (int bitDepth, int shape) noexcept
    {
        bits = bitDepth;
        static constexpr float tables[4][maxTaps] = {
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
            { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f },
            { 1.623f, -0.982f, 0.109f, 0.0f, 0.0f },
            { 2.033f, -2.165f, 1.959f, -1.590f, 0.6149f },
        };
        for (int i = 0; i < maxTaps; ++i)
            coefs[static_cast<size_t> (i)] = tables[shape < 0 ? 0 : (shape > 3 ? 3 : shape)][i];
    }

    bool active() const noexcept { return bits > 0; }

    float process (float x) noexcept
    {
        if (bits <= 0)
            return x;

        const float lsb = std::ldexp (1.0f, 1 - bits);
        float shaped = x;
        for (size_t k = 0; k < static_cast<size_t> (maxTaps); ++k)
            shaped -= coefs[k] * error[k];

        const float d = (uniform() + uniform() - 1.0f) * lsb;
        float q = std::round ((shaped + d) / lsb) * lsb;
        q = q > 1.0f ? 1.0f : (q < -1.0f ? -1.0f : q);

        for (size_t k = maxTaps - 1; k > 0; --k)
            error[k] = error[k - 1];
        error[0] = q - shaped;
        return q;
    }

private:
    float uniform() noexcept
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return static_cast<float> (state >> 8) * (1.0f / 16777216.0f);
    }

    int bits = 0;
    std::array<float, maxTaps> coefs {}, error {};
    std::uint32_t state = 1u;
};
} // namespace nel
