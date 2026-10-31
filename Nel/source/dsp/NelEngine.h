#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <Biquad.h>
#include <DcBlocker.h>
#include <DelayLine.h>
#include <LevelMeter.h>
#include <Loudness.h>
#include <ModeSwitch.h>
#include <OversamplerN.h>

#include <array>
#include <atomic>
#include <vector>

#include "BodyStage.h"
#include "Dither.h"
#include "Modes.h"
#include "PeakStage.h"

namespace nel
{
/** Valores de los parámetros en unidades de la interfaz. Los avanzados (lookahead…) son los efectivos:
    el procesador ya los ha rellenado al elegir el modo; `mode` solo da los valores internos. */
struct Settings
{
    float gainDb = 0.0f;
    float ceilingDb = -1.0f;
    int mode = 0;
    float characterPercent = 0.0f;
    float lookaheadMs = 1.0f;
    float attackMs = 3.0f;
    float releaseMs = 500.0f;
    bool releaseAuto = true;
    float kneeDb = 3.0f;
    float linkTransient = 75.0f; // 0 a 100
    float linkRelease = 100.0f;  // 0 a 100
    bool truePeak = true;
    int oversample = 2;          // factor = 2^oversample: 0 = 1×, 1 = 2×, 2 = 4×, … 5 = 32×
    bool minimumPhase = false;
    bool offlineBoost = true;
    int ditherBits = 0;          // 0 = apagado, o 16, 18, 20, 22, 24
    int ditherShape = 0;         // 0 plano, 1 suave, 2 medio, 3 fuerte
    bool dcFilter = false;
    bool unityGain = false;
    bool audition = false;
    bool bypass = false;
    bool nonRealtime = false;
    bool loudnessFromInput = false;
};

/** Limitador de Nel, independiente del anfitrión para poder probarlo.

        entrada → ganancia → [DC] → sobremuestreo → Carácter y recorte previo → etapa B (cuerpo, a la
        frecuencia base) → etapa A (picos, con lookahead) → seguro de pico → submuestreo → dither

    La latencia es la del sobremuestreo más el lookahead, en muestras base enteras, y la declara el
    procesador. Los cambios de estructura (factor, fase, lookahead) usan un fundido a silencio de 50 ms,
    y solo se adoptan cuando el valor lleva 150 ms estable (para no encadenar fundidos al arrastrar). */
class NelEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int chunkSize = 256;

    /** Exponente efectivo del sobremuestreo según la frecuencia de muestreo y el render. */
    static int effectiveExponent (const Settings&, double sampleRate) noexcept;
    /** Muestras base de lookahead. */
    static int lookaheadSamples (const Settings&, double sampleRate) noexcept;
    /** Latencia total en muestras base: sobremuestreo + lookahead. */
    static int latencyFor (const Settings&, double sampleRate);

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept;

    /** Latencia de la estructura que está sonando ahora. */
    int getActiveLatency() const noexcept { return activeLatency.load (std::memory_order_relaxed); }

    // Medidores para la interfaz.
    ferb::LevelMeter& getInputPeak (int ch) noexcept { return inPeak[static_cast<size_t> (ch)]; }
    ferb::LevelMeter& getOutputPeak (int ch) noexcept { return outPeak[static_cast<size_t> (ch)]; }
    ferb::LevelMeter& getTruePeak (int ch) noexcept { return truePeakMeter[static_cast<size_t> (ch)]; }
    float getRms (int ch) const noexcept { return rms[static_cast<size_t> (ch)].load (std::memory_order_relaxed); }
    float getInputRms (int ch) const noexcept { return inRms[static_cast<size_t> (ch)].load (std::memory_order_relaxed); }
    ferb::LevelMeter& getReductionMeter() noexcept { return reductionMeter; } // dB, máximo desde la última lectura
    float getReductionDb() const noexcept { return reductionNow.load (std::memory_order_relaxed); }
    int getOvers() const noexcept { return overs.load (std::memory_order_relaxed); }
    void resetOvers() noexcept { overs.store (0); }
    float getSafetyMarginDb() const noexcept { return marginDb.load (std::memory_order_relaxed); }
    int getEffectiveFactor() const noexcept { return effectiveFactor.load (std::memory_order_relaxed); }
    ferb::LoudnessMeter& getLoudness() noexcept { return loudness; }

private:
    using Smoothed = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    void processChunk (juce::AudioBuffer<float>& chunk, const Settings& settings) noexcept;
    void applyStructure (int key) noexcept;
    static int makeKey (int exponent, bool minPhase, int lookaheadBase) noexcept { return exponent | (minPhase ? 8 : 0) | (lookaheadBase << 4); }

    double sampleRate = 48000.0;
    int maxLookaheadBase = 0;

    Smoothed gain, ceiling, character, bypassMix, unity, auditionMix; // unity: factor de compensación; auditionMix: 0 = salida, 1 = diferencia

    std::array<ferb::UpN, maxChannels> ups;
    std::array<ferb::DownN, maxChannels> downs;
    std::array<ferb::UpN, maxChannels> truePeakUps; // 4×, para medir (y cuidar) el true peak
    std::array<ferb::DcBlocker, maxChannels> dcBlockers;
    std::array<PeakStage, maxChannels> peakStages;
    std::array<BodyFollower, maxChannels> followers;
    std::array<ferb::DelayLine, maxChannels> rawDelay, gainedDelay, audioDelay;
    std::array<ferb::Biquad, maxChannels> detectorHighPass;
    std::array<Dither, maxChannels> dithers;
    std::array<float, maxChannels> bodyGainPrev {}, bodySmooth {}, envFast {}, envSlow {};

    ferb::ModeSwitch modeSwitch;
    ferb::LoudnessMeter loudness;
    std::array<ferb::LevelMeter, maxChannels> inPeak, outPeak, truePeakMeter;
    ferb::LevelMeter reductionMeter;
    std::array<std::atomic<float>, maxChannels> rms, inRms;
    std::array<float, maxChannels> rmsState {}, inRmsState {};
    std::atomic<float> reductionNow { 0.0f }, marginDb { 0.1f };
    std::atomic<int> overs { 0 }, effectiveFactor { 4 }, activeLatency { 0 };

    // Estructura activa.
    int exponent = 2, factor = 4, lookaheadBase = 0, latency = 0, windowOs = 1;
    bool minPhase = false;
    int requestedKey = -1, adoptedKey = -1, stableSamples = 0;
    bool asleep = false, needsInit = true;

    std::array<std::vector<float>, maxChannels> base, raw, gainedDry, os, outBase, tpScratch;
    std::vector<float> gainRamp, ceilRamp, charRamp, bypassRamp, unityRamp, auditionRamp;
    std::array<std::vector<float>, maxChannels> bodyGain; // por muestra base
};
} // namespace nel
