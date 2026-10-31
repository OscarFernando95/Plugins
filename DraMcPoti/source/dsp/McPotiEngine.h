#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <vector>

#include <Biquad.h>
#include <Crossover.h>
#include <DelayLine.h>
#include <OversamplerN.h>

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
    int osIndex = 1; // 0 a 3 = 1×, 2×, 4×, 8×
};

/** Fase 2 (parcial): cruce de cuatro bandas y generador de armónicos de los cuatro caracteres Vintage con
    ADAA de primer orden y sobremuestreo de 1× a 8×, dinámica por banda, mezcla, Delta y Solo.
    La latencia es constante (la de 8×): el seco, Solo y los factores menores se retrasan lo que falta.
    Pendiente: caracteres Modernos, Color, Fase, Suavizar, guarda, M/S, fase lineal e Igualar. */
class McPotiEngine
{
public:
    static constexpr int maxFactor = 8;
    /** Latencia constante en muestras base: la de subir y bajar a 8× con filtros de fase lineal. */
    static int latencySamples() { return ferb::OsDesign::latencyFor (maxFactor, false); }

    void prepare (double sampleRate, int maxBlock);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer, const Settings& s) noexcept;

    /** Nivel añadido por banda (RMS suavizado, lineal), para las válvulas de la interfaz. */
    float addedLevel (int band) const noexcept { return added[static_cast<size_t> (band)].load (std::memory_order_relaxed); }

private:
    static constexpr int maxChannels = 2;
    static constexpr int controlInterval = 16;
    static constexpr size_t nb = numBands;

    struct Smoother
    {
        float value = 0.0f, coeff = 0.0f;
        void snap (float v) noexcept { value = v; }
        float step (float target) noexcept { value += coeff * (target - value); return value; }
    };

    struct AdaaState
    {
        double vPrev = 0.0, FPrev = 0.0;
        bool primed = false;
    };

    struct Channel
    {
        ferb::FourBandSplit split;
        std::array<ferb::Biquad, nb> hpIn, hpOut;
        std::array<float, nb> env {};
        std::array<ferb::UpN, nb> up;
        std::array<ferb::DownN, nb> down;
        std::array<AdaaState, nb> adaa;
        ferb::DelayLine dryDelay;
        std::array<ferb::DelayLine, nb> bandDelay, addDelay;
        std::array<std::vector<float>, nb> bandSig, nlIn, nlOut;
    };

    void updateControl (const Settings& s) noexcept;
    void processChunk (juce::AudioBuffer<float>& buffer, int start, int n, const Settings& s) noexcept;
    void configureFactor (int factor) noexcept;

    double fs = 48000.0;
    int maxBlockSize = 0, factor = 0; // factor 0 = sin configurar
    int activeVoice = -1;             // era·4 + voicing en uso; cambia solo con lo añadido en silencio
    std::array<Channel, maxChannels> channels;
    std::vector<float> hiBuffer;
    std::array<std::vector<float>, nb> driveBuf, levelBuf;
    std::array<float, nb> dynFactor {}, lastHp {};
    std::array<float, 3> lastXo {};
    std::array<Smoother, nb> driveSmooth, levelSmooth;
    Smoother inSmooth, outSmooth, mixSmooth, gate;
    std::array<float, nb> addedPower {};
    std::array<std::atomic<float>, nb> added {};
    float envAttack = 0.0f, envRelease = 0.0f;
    bool primed = false, snapControl = true;
    std::array<float, 3> smoothXo {};
    float smoothTune = 4000.0f, freqCoeff = 0.0f;
};
} // namespace mcpoti
