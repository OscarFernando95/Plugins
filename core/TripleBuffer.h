#pragma once

#include <array>
#include <atomic>

namespace ferb
{
/** Intercambio sin bloqueos entre UN productor y UN consumidor: el consumidor siempre ve la última
    versión publicada completa. Los tres huecos se reservan al construir `T`.

        productor:   writeSlot() = …;  publish();
        consumidor:  if (update()) usar (read());                                             */
template <typename T>
class TripleBuffer
{
public:
    T& writeSlot() noexcept { return slots[static_cast<size_t> (writeIndex)]; }

    void publish() noexcept
    {
        const int old = middle.exchange (writeIndex | dirtyBit, std::memory_order_acq_rel);
        writeIndex = old & indexMask;
    }

    /** Toma la última versión publicada si hay una nueva. */
    bool update() noexcept
    {
        if ((middle.load (std::memory_order_acquire) & dirtyBit) == 0)
            return false;
        const int old = middle.exchange (readIndex, std::memory_order_acq_rel);
        readIndex = old & indexMask;
        return true;
    }

    const T& read() const noexcept { return slots[static_cast<size_t> (readIndex)]; }

    /** Acceso a los tres huecos para inicializarlos (antes de usarlo desde dos hilos). */
    std::array<T, 3>& all() noexcept { return slots; }

private:
    static constexpr int dirtyBit = 4, indexMask = 3;

    std::array<T, 3> slots;
    int writeIndex = 0, readIndex = 2;
    std::atomic<int> middle { 1 };
};
} // namespace ferb
