#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <vector>

#include <Biquad.h>
#include <ModeSwitch.h>

#include "SibilanceDetector.h"

namespace chaus
{
enum class Stereo { LeftRight = 0, Mid, Side };
enum class Shape { Bell = 0, Shelf, Auto };

struct Settings
{
    bool voiceMode = true;       // Voz sola (clasificador) o General (solo energía de la banda)
    float thresholdDb = -30.0f;  // −60 a 0
    bool adaptive = true;        // con adaptativo, −30 es «justo en la referencia»
    float rangeDb = 8.0f;        // reducción máxima, 0 a 24
    float hpHz = 4000.0f;        // 2 a 12 kHz
    float lpHz = 12000.0f;       // 5 a 20 kHz
    bool track = true;           // la campana sigue la frecuencia de la s
    bool qAuto = true;
    float q = 3.0f;              // 1 a 8 (con qAuto apagado)
    bool wideBand = false;       // Banda ancha: baja todo el audio
    Shape shape = Shape::Auto;
    float sensSoftDb = 0.0f;     // −6 a +6
    float sensHardDb = 0.0f;
    float lookaheadMs = 10.0f;   // 0 a 15 (0 si el lookahead está apagado)
    bool releaseAuto = true;
    float releaseMs = 40.0f;     // 10 a 100 (con releaseAuto apagado)
    float linkPercent = 100.0f;  // 0 a 100
    Stereo stereo = Stereo::LeftRight;
    bool externalSidechain = false;
    bool listenTrigger = false;
    bool listenSidechain = false;
    float outputDb = 0.0f;       // −12 a +12
    bool bypass = false;
};

/** Lo que el motor deja ver muestra a muestra (pruebas y, después, la pantalla). Todos los punteros son opcionales
    y, si existen, deben tener sitio para el número de muestras del bloque. Un valor por canal procesado (0 y 1). */
struct Probe
{
    std::array<float*, 2> reductionDb {};  // reducción aplicada (tras balística y enlace) en el instante del detector
    std::array<float*, 2> centreHz {};     // frecuencia de la campana
    std::array<float*, 2> levelDb {};      // nivel de la banda de sibilante
    std::array<float*, 2> thresholdDb {};  // umbral efectivo
    std::array<float*, 2> score {};
};

/** De-esser de ChauS, independiente del anfitrión para poder probarlo.

        entrada → (M/S) → ┬→ detector → nivel y puntuación → reducción → balística ─┐
                          └→ retardo de lookahead → campana o estante (o ganancia) ←─┘ → (M/S) → salida

    El audio va retrasado `lookaheadMs`; el detector no: la reducción llega antes que la s. La latencia es
    exactamente ese retardo. Todo se reserva en `prepare`. */
class ChausEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int chunkSize = 256;
    static constexpr float maxLookaheadMs = 15.0f;

    /** Latencia que declara el procesador: el lookahead en muestras enteras. */
    static int latencyFor (const Settings&, double sampleRate) noexcept;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** `sidechain` puede ser nulo (o tener otro número de canales: mono se reparte a los dos). */
    void process (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* sidechain, const Settings&, Probe* probe = nullptr) noexcept;

private:
    /** Retardo con dos tomas: al cambiar el valor pasa de una a otra con un fundido de 50 ms. */
    class LookaheadDelay
    {
    public:
        void prepare (int maxSamples, int fadeSamples);
        void reset() noexcept;
        void setDelay (int samples) noexcept;
        /** Fija el retardo sin fundido (la primera asignación). */
        void jumpTo (int samples) noexcept;
        int getDelay() const noexcept { return delay; }
        float process (float x) noexcept;

    private:
        float read (int d) const noexcept { return buffer[static_cast<size_t> ((write - d) & mask)]; }
        std::vector<float> buffer;
        int mask = 0, write = 0, delay = 0, previous = 0, fadeLen = 1, fadePos = 1;
    };

    struct Coefficients
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    };

    struct Lane
    {
        SibilanceDetector detector;
        AdaptiveReference reference;
        LookaheadDelay delay, bandDelay, gateDelay;
        ferb::Biquad bell, shelf;
        Coefficients bellNow, bellStep, shelfNow, shelfStep;

        // seguimiento de la frecuencia y del ancho
        float logCentre = 0.0f, q = 3.0f;
        std::array<float, 1024> centroids {};
        int ringHead = 0, ringCount = 0, sinceMedian = 0;
        float median = 0.0f;
        long lastFrame = 0;

        // reducción
        float rawDb = 0.0f, smoothDb = 0.0f, linkedDb = 0.0f, gate = 0.0f;
        long eventSamples = 0, lastEventSamples = 0;
        float shapeWeight = 0.0f; // 0 = campana, 1 = estante
        float lastLevelDb = -120.0f, lastThresholdDb = 0.0f, lastScore = 0.0f;
        SibilanceDetector::Output tick {};
        float band = 0.0f;
    };

    void resetLane (Lane&) noexcept;
    void configureLane (Lane&, const Settings&) noexcept;
    void updateCoefficients (Lane&, const Settings&, float reductionDb) noexcept;
    float rawReduction (Lane&, const Settings&, const SibilanceDetector::Output&) noexcept;
    void track (Lane&, const Settings&) noexcept;

    double fs = 48000.0;
    std::array<Lane, maxChannels> lanes;
    std::array<LookaheadDelay, maxChannels> passDelay; // el componente que no se procesa en M/S

    ferb::ModeSwitch modeSwitch;
    bool needsInit = true;
    int requestedDelay = -1, adoptedDelay = -1, stableSamples = 0;

    float attackCoeff = 0.0f, shapeCoeff = 0.0f, trackFastCoeff = 0.0f, trackSlowCoeff = 0.0f, gateCoeff = 0.0f;
    int activeKey = 0;
    // En doble precisión: con `float` el suavizado se estanca ~1e-4 antes del objetivo (cada paso queda bajo medio ulp).
    double bypassMix = 0.0, listenMix = 0.0, outputGain = 1.0, bypassCoeff = 0.0, listenCoeff = 0.0, outputCoeff = 0.0;
    int sinceCoefficients = 0;
    float lastHp = -1.0f, lastLp = -1.0f;
    Settings settingsInUse;
};
} // namespace chaus
