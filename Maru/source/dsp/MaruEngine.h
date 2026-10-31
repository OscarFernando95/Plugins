#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <memory>
#include <vector>

#include <DelayLine.h>
#include <ModeSwitch.h>

namespace maru
{
struct Settings
{
    bool gate = false;           // Motor: Supresión (espectral) o Gate
    float amount = 0.5f;         // Supresión, 0 a 1
    float rangeDb = 30.0f;       // reducción máxima, 6 a 60 (el parámetro `range` es su negativo)
    float sens = 0.0f;           // −10 a +10
    float recoveryMs = 120.0f;   // 10 a 1000
    bool gateAuto = true;        // Gate: umbral = piso de ruido + margen
    float gateThreshDb = -50.0f;
    float attackMs = 2.0f, holdMs = 40.0f, hystDb = 6.0f, lookaheadMs = 3.0f;
    float scHpHz = 80.0f, scLpHz = 12000.0f;
    bool scExt = false;
    float linkPercent = 100.0f;  // enlace estéreo
    bool lowLatency = false;     // N de 512 en lugar de 1024 (a 48 kHz)
    bool listen = false;         // Escuchar lo quitado
    float outputDb = 0.0f;
    bool bypass = false;
};

/** Datos del motor para la pantalla (los escribe el hilo de audio, los lee la interfaz). */
struct Telemetry
{
    static constexpr int bands = 32; // de 100 Hz a 16 kHz, en escala logarítmica
    std::atomic<float> noiseFloorDb { -120.0f };  // piso de ruido estimado, dBFS eficaces
    std::atomic<float> reductionDb { 0.0f };      // reducción de nivel (positiva), media ponderada por la potencia de entrada
    std::atomic<bool> working { false };          // Supresión: reduciendo; Gate: puerta cerrada
    std::array<std::atomic<float>, bands> input {}, output {}, noise {}; // dB por banda
};

/** Estimador de piso de ruido por bin: mínimos con probabilidad de presencia de voz (MCRA, Cohen y Berdugo 2002). */
class NoiseEstimator
{
public:
    void prepare (int numBins, double hopSeconds);
    void reset() noexcept;

    /** `power`: |X(k)|² de un cuadro. Actualiza y devuelve el espectro de ruido (potencia por bin). */
    const float* update (const float* power) noexcept;
    const float* noise() const noexcept { return lambda.data(); }
    int frames() const noexcept { return frameCount; }

private:
    int bins = 0, frameCount = 0, subFrames = 1, subCount = 0;
    float alphaS = 0.0f, alphaD = 0.95f, alphaP = 0.2f, delta = 5.0f;
    std::vector<float> smoothed, minimum, minTemp, minOld, windowMin, presence, lambda, scratch;
};

/** Ganancia espectral (LSA de Ephraim y Malah con decision-directed) de un cuadro. */
class SuppressionGain
{
public:
    void prepare (int numBins, double hopSeconds);
    void reset() noexcept;

    struct Params
    {
        float overestimate = 2.0f;     // β
        float floorGain = 0.25f;       // ganancia mínima (lineal)
        float recoverySeconds = 0.12f;
        float sensGain = 1.0f;         // factor de potencia sobre el ruido
    };

    /** `power` y `noise` por bin → `gain` por bin (suavizada en frecuencia y en tiempo, con protección de transitorios). */
    void compute (const float* power, const float* noise, const Params&, float* gain) noexcept;

private:
    static float expint1 (float x) noexcept;

    int bins = 0;
    double hop = 0.005;
    float attackCoeff = 0.0f;
    int boostFrames = 0, boostLeft = 0;
    std::vector<float> previousGain, previousGamma, previousMag, smoothedGain, target;
};

/** Motor de Maru, independiente del anfitrión.

        Supresión:  entrada → STFT (√Hann, N muestras, salto N/4) → estimador de ruido (sobre la suma mid) →
                    ganancia por bin (la misma para L y R con enlace del 100 %) → STFT inversa y solape-suma → salida
        Latencia = N muestras exactas (1024 a 44,1 y 48 kHz; el doble y el cuádruple a 96 y 192 kHz; la mitad con latencia baja). */
class MaruEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int chunkSize = 256;

    /** Tamaño de la FFT según la frecuencia de muestreo y la calidad. */
    static int fftSizeFor (double sampleRate, bool lowLatency) noexcept;
    /** Latencia que declara el procesador. */
    static int latencyFor (const Settings&, double sampleRate) noexcept;

    MaruEngine();
    ~MaruEngine();

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    const Telemetry& getTelemetry() const noexcept { return telemetry; }

    void process (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* sidechain, const Settings&) noexcept;

private:
    struct Stft;
    void applyStructure (int key);
    int structureLatency (int key) const noexcept;

    double fs = 48000.0;
    std::array<std::unique_ptr<Stft>, 2> stft; // [0] = calidad alta, [1] = latencia baja
    int lookaheadSamples = 0;
    std::array<ferb::DelayLine, maxChannels> dryDelay;
    ferb::ModeSwitch modeSwitch;
    bool needsInit = true;
    int activeKey = -1;
    // En doble precisión: con `float` el suavizado se estanca ~1e-4 antes del objetivo.
    double bypassMix = 0.0, listenMix = 0.0, outputGain = 1.0;
    double bypassCoeff = 0.0, listenCoeff = 0.0, outputCoeff = 0.0;
    Telemetry telemetry;
};
} // namespace maru
