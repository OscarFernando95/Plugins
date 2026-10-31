#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <DelayLine.h>
#include <LevelMeter.h>
#include <ModeSwitch.h>
#include <Oversampler.h>

#include <array>
#include <vector>

#include "BandFilter.h"
#include "BandSaturator.h"

namespace poty
{
inline constexpr int numBands = 5;

struct BandSettings
{
    bool on = true;
    float freq = 1000.0f;
    float gainDb = 0.0f;
    float q = 1.0f;
    int type = 0;        // ferb::SatType
    float drive = 50.0f; // 0 a 100 %
};

/** Valores de los parámetros en unidades de la interfaz. */
struct Settings
{
    float inGainDb = 0.0f;
    float outGainDb = 0.0f;
    float mixPercent = 100.0f;
    bool bypass = false;
    int quality = 1;   // factor de sobremuestreo = 2^quality: 0 = 1×, 1 = 2×, 2 = 4×, 3 = 8×
    int intensity = 1; // 0 = sutil, 1 = media, 2 = agresiva
    int mode = 0;      // 0 = estéreo, 1 = mid, 2 = side
    bool linear = false; // saturadores desactivados: realce en paralelo puro (pruebas)
    std::array<BandSettings, numBands> bands;
};

/** Enhancer multibanda en paralelo, independiente del anfitrión para poder probarlo.

        y = x + mezcla · Σ g_i(Δ_i),   Δ_i = realce_i(x) − x

    Los realces corren a la frecuencia base; cada Δ_i se sube, se satura con ADAA y la suma se
    baja una sola vez. La señal original se retrasa lo mismo que el camino saturado.

    La latencia declarada es SIEMPRE la del modo máximo (8×): los demás modos retrasan el camino
    saturado lo que falta. Así cambiar la calidad no cambia la alineación ni obliga al
    anfitrión a recalcular la compensación de retardo. */
class PotyEngine
{
public:
    static constexpr int maxChannels = BandFilter::maxChannels;
    static constexpr int chunkSize = 256;

    /** Latencia constante que debe declarar el plugin, en muestras: la del factor máximo (8×). */
    static int getLatencySamples() noexcept { return ferb::Oversampler::latencyFor (ferb::Oversampler::maxFactor); }

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept;

    const ferb::LevelMeter& getOutputMeter() const noexcept { return meter; }

private:
    using Smoothed = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    void processChunk (juce::AudioBuffer<float>& chunk, const Settings& settings) noexcept;
    void applyStructure (int key);
    void resetDsp() noexcept;

    double sampleRate = 48000.0;

    Smoothed inGain, outGain, mix, bypassMix; // bypassMix: 0 = procesado, 1 = seco
    std::array<Smoothed, numBands> drive;     // drive efectivo d = 1 + 19·drive·escala

    std::array<BandFilter, numBands> filters;
    std::array<std::array<ferb::UpSampler, maxChannels>, numBands> ups;
    std::array<std::array<BandSaturator, maxChannels>, numBands> sats;
    std::array<ferb::DownSampler, maxChannels> downs;

    std::array<ferb::DelayLine, maxChannels> rawDelay, dryDelay, wetDelay;
    ferb::ModeSwitch modeSwitch;
    ferb::LevelMeter meter;

    // Estructura activa (la fija ModeSwitch en silencio).
    int factor = 2;
    int workMode = 0;
    int latencyPad = 0;
    std::array<bool, numBands> bandActive {};
    bool asleep = false;
    bool needsInit = true; // el primer bloque adopta la estructura pedida sin fundido

    // Búferes de trabajo, reservados en prepare().
    std::array<std::vector<float>, maxChannels> work, delta, sum, osSum, wet, dryOut, rawOut;
    std::vector<float> up, dBase, dOs;
    std::vector<float> inRamp, outRamp, mixRamp, bypassRamp;
};
} // namespace poty
