#pragma once

#include <algorithm>
#include <vector>

namespace ferb
{
/** Retardo fijo de un canal, en muestras enteras. Se reserva en `prepare`, nunca en el hilo de audio. */
class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        int size = 1;
        while (size < maxDelaySamples + 1)
            size <<= 1;
        buffer.assign (static_cast<size_t> (size), 0.0f);
        mask = size - 1;
        setDelay (0);
        reset();
    }

    void setDelay (int samples) noexcept { delay = std::min (samples, mask); }
    int getDelay() const noexcept { return delay; }

    void reset() noexcept
    {
        std::fill (buffer.begin(), buffer.end(), 0.0f);
        write = 0;
    }

    float process (float x) noexcept
    {
        buffer[static_cast<size_t> (write)] = x;
        const float y = buffer[static_cast<size_t> ((write - delay) & mask)];
        write = (write + 1) & mask;
        return y;
    }

    void processBlock (const float* in, float* out, int n) noexcept
    {
        for (int i = 0; i < n; ++i)
            out[i] = process (in[i]);
    }

private:
    std::vector<float> buffer;
    int mask = 0, write = 0, delay = 0;
};
} // namespace ferb
