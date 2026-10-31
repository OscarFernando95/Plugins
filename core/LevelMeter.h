#pragma once

#include <atomic>

namespace ferb
{
/** Nivel de pico publicado por el hilo de audio y leído por la interfaz.

    `publishMax` conserva el mayor pico desde la última lectura con `consume`, así la interfaz, que
    lee a 30 Hz, no pierde los picos de los bloques intermedios. */
class LevelMeter
{
public:
    void publish (float peak) noexcept { value.store (peak, std::memory_order_relaxed); }

    void publishMax (float peak) noexcept
    {
        float current = value.load (std::memory_order_relaxed);
        while (peak > current && ! value.compare_exchange_weak (current, peak, std::memory_order_relaxed))
        {
        }
    }

    float get() const noexcept { return value.load (std::memory_order_relaxed); }

    /** Lee el pico acumulado y empieza de nuevo. Lo llama la interfaz. */
    float consume() noexcept { return value.exchange (0.0f, std::memory_order_relaxed); }

    void reset() noexcept { publish (0.0f); }

private:
    std::atomic<float> value { 0.0f };
};
} // namespace ferb
