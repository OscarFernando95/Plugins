#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <Crossover.h>
#include <DcBlocker.h>
#include <DelayLine.h>
#include <LevelMeter.h>
#include <Loudness.h>
#include <ModeSwitch.h>
#include <Oversampler.h>
#include <VuBallistics.h>

#include <array>
#include <vector>

#include "CremitaCurve.h"

namespace cremita
{
/** Valores de los parámetros en unidades de la interfaz. */
struct Settings
{
    float inputDb = 0.0f;
    float effectPercent = 100.0f;
    float curve = 0.0f;       // −50 a +50
    float biasPercent = 0.0f; // 0 a 100
    bool split = false;
    float crossover1 = 200.0f;
    float crossover2 = 3000.0f;
    std::array<float, 3> trimDb { 0.0f, 0.0f, 0.0f };
    bool clip = true;
    bool midSide = false;
    int oversample = 1; // factor = 2^oversample: 0 = 1×, 1 = 2×, 2 = 4×, 3 = 8×
    bool match = false;
    float outputDb = 0.0f;
    bool inOn = true;
    bool bypass = false; // el del anfitrión
};

/** Inflator de Cremita, independiente del anfitrión para poder probarlo.

        entrada → Input → [M/S] → [3 bandas LR4] → sobremuestreo → curva con ADAA → submuestreo
                → y = seco + Efecto · Σ peso_b · (procesado_b − seco_b) → DC → [M/S⁻¹]
                → igualar sonoridad → Output → Clip

    - Procesador estático: sin detector ni envolvente, así que no bombea.
    - Latencia constante: la del factor máximo (8×). Los demás factores retrasan lo que falta, de modo
      que cambiar el sobremuestreo no cambia la alineación ni obliga al anfitrión a recalcularla.
    - La señal seca de cada banda se retrasa lo mismo que la procesada: con Efecto = 0 la salida es la
      entrada retrasada, bit a bit (con Band Split, la entrada pasada por los cruces, de magnitud plana).
    - Cambios de estructura (sobremuestreo, bandas, M/S, clip): fundido a silencio, cambio y vuelta. */
class CremitaEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int numBands = 3;
    static constexpr int chunkSize = 256;

    /** Latencia constante que debe declarar el plugin, en muestras: la del factor máximo (8×). */
    static int getLatencySamples() noexcept { return ferb::Oversampler::latencyFor (ferb::Oversampler::maxFactor); }

    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept;

    /** Medidores para la interfaz (valores lineales, calibrados como un VU: una senoidal marca su RMS). */
    float getInputLevel() const noexcept { return inputVu.level(); }
    float getOutputLevel() const noexcept { return outputVu.level(); }
    /** Media de |salida − seco|, con la misma balística. */
    float getEffectLevel() const noexcept { return effectVu.level(); }
    /** Pico antes del recorte final: la interfaz enciende la luz de sobrecarga con valores ≥ 1. */
    ferb::LevelMeter& getOverloadMeter() noexcept { return overloadMeter; }
    /** Pico de la entrada ya con Input: el punto de trabajo del gráfico de la curva. */
    ferb::LevelMeter& getWorkMeter() noexcept { return workMeter; }
    /** Ganancia (dB) que aplica «Igualar sonoridad» en este momento. */
    float getMatchGainDb() const noexcept { return matchGainDb.load (std::memory_order_relaxed); }

private:
    using Smoothed = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    void processChunk (juce::AudioBuffer<float>& chunk, const Settings& settings) noexcept;
    void applyStructure (int key);
    void resetDsp() noexcept;
    void updateCrossovers (int n) noexcept;

    double sampleRate = 48000.0;

    Smoothed inGain, outGain, effect, bypassMix; // bypassMix: 0 = procesado, 1 = entrada retrasada
    Smoothed curveC, biasP, xo1, xo2;
    std::array<Smoothed, numBands> trim;

    std::array<std::array<ferb::UpSampler, numBands>, maxChannels> ups;
    std::array<std::array<ferb::DownSampler, numBands>, maxChannels> downs;
    std::array<std::array<CurveAdaa, numBands>, maxChannels> adaas;
    std::array<ferb::ThreeBandSplit, maxChannels> splits;
    std::array<ferb::DcBlocker, maxChannels> dcBlockers;
    std::array<ferb::DelayLine, maxChannels> rawDelay;
    std::array<std::array<ferb::DelayLine, numBands>, maxChannels> dryDelay, wetDelay;

    ferb::ModeSwitch modeSwitch;
    ferb::VuBallistics inputVu, outputVu, effectVu;
    ferb::LevelMeter overloadMeter, workMeter;
    ferb::LoudnessMeter loudnessIn, loudnessOut;
    std::atomic<float> matchGainDb { 0.0f };
    float matchState = 0.0f; // dB, ya suavizado

    // Estructura activa (la fija ModeSwitch en silencio).
    int factor = 2;
    bool splitOn = false, midSideOn = false, clipOn = true;
    int latencyPad = 0;
    bool asleep = false;
    bool needsInit = true;
    float lastXo1 = 0.0f, lastXo2 = 0.0f;

    // Búferes de trabajo, reservados en prepare().
    std::array<std::vector<float>, maxChannels> work, dryAP, acc, rawOut, outBuf;
    std::array<std::vector<float>, numBands> bandIn, trimRamp;
    std::vector<float> drySig, osIn, osOut, wetBase, effectRect;
    std::vector<CurveParams> params;
    std::vector<float> inRamp, outRamp, effectRamp, bypassRamp, matchRamp;
};
} // namespace cremita
