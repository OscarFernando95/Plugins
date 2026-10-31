#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>

#include <Biquad.h>
#include <Crossover.h>

namespace mcpoti
{
inline constexpr int numBands = 4;

struct BandSettings
{
    float drive = 0.0f;  // 0 a 100
    float dyn = 0.0f;    // −50 a +50
    float level = 100.0f; // 0 a 100 %
    bool on = true, solo = false;
};

/** Ajustes que el procesador lee de los parámetros en cada bloque. El motor no sabe de JUCE ni de APVTS. */
struct Settings
{
    float inputDb = 0.0f, outputDb = 0.0f, mixPercent = 100.0f;
    bool master = false; // Modo: false = Vocal
    int era = 0;         // 0 Vintage, 1 Moderno
    int voice = 0;       // 0 a 3 dentro de la era
    float tuneHz = 4000.0f, linkPercent = 100.0f;
    float xo1 = 150.0f, xo2 = 1800.0f, xo3 = 7000.0f;
    std::array<BandSettings, numBands> bands;
    bool delta = false;
};

/** Fase 2 (parcial): cruce de cuatro bandas, generador de armónicos de los cuatro caracteres Vintage con
    dinámica por banda, mezcla, Delta y Solo. Pendiente: sobremuestreo y ADAA, caracteres Modernos, Color,
    Fase, Suavizar, guarda, M/S, fase lineal, Igualar y latencia. */
class McPotiEngine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer, const Settings& s) noexcept;

    /** Nivel añadido por banda (RMS suavizado, lineal), para las válvulas de la interfaz. */
    float addedLevel (int band) const noexcept { return added[static_cast<size_t> (band)].load (std::memory_order_relaxed); }

private:
    static constexpr int maxChannels = 2;
    static constexpr int controlInterval = 16;

    struct Smoother
    {
        float value = 0.0f, coeff = 0.0f;
        void snap (float v) noexcept { value = v; }
        float step (float target) noexcept { value += coeff * (target - value); return value; }
    };

    struct Channel
    {
        ferb::FourBandSplit split;
        std::array<ferb::Biquad, numBands> hpIn, hpOut;
        std::array<float, numBands> env {};
    };

    void updateControl (const Settings& s) noexcept;

    double fs = 48000.0;
    std::array<Channel, maxChannels> channels;
    std::array<float, numBands> dynFactor {};
    std::array<float, numBands> lastHp {};
    std::array<float, 3> lastXo {};
    std::array<Smoother, numBands> driveSmooth, levelSmooth;
    Smoother inSmooth, outSmooth, mixSmooth;
    std::array<float, numBands> addedPower {};
    std::array<std::atomic<float>, numBands> added {};
    float envAttack = 0.0f, envRelease = 0.0f;
    bool primed = false, snapControl = true;
    std::array<float, 3> smoothXo {};
    float smoothTune = 4000.0f, freqCoeff = 0.0f;
};
} // namespace mcpoti
