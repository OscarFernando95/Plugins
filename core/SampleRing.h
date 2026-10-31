#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

namespace ferb
{
/** Anillo de muestras de un productor (hilo de audio) y un lector (interfaz), sin bloqueos.
    El lector copia las últimas N muestras. El anillo es mucho mayor que la ventana que se lee, así
    que el productor no llega a pisar lo que se está copiando salvo que la interfaz se pare ~medio segundo. */
class SampleRing
{
public:
    void prepare (int capacityPowerOfTwo)
    {
        buffer.assign (static_cast<size_t> (capacityPowerOfTwo), 0.0f);
        mask = static_cast<uint64_t> (capacityPowerOfTwo) - 1;
        writePos.store (0);
    }

    void push (const float* samples, int n) noexcept
    {
        uint64_t pos = writePos.load (std::memory_order_relaxed);
        for (int i = 0; i < n; ++i)
            buffer[static_cast<size_t> ((pos + static_cast<uint64_t> (i)) & mask)] = samples[i];
        writePos.store (pos + static_cast<uint64_t> (n), std::memory_order_release);
    }

    /** Las últimas `count` muestras, de la más antigua a la más reciente. */
    void copyLatest (float* dest, int count) const noexcept
    {
        const uint64_t end = writePos.load (std::memory_order_acquire);
        for (int i = 0; i < count; ++i)
            dest[i] = buffer[static_cast<size_t> ((end - static_cast<uint64_t> (count) + static_cast<uint64_t> (i)) & mask)];
    }

private:
    std::vector<float> buffer;
    uint64_t mask = 0;
    std::atomic<uint64_t> writePos { 0 };
};
} // namespace ferb
