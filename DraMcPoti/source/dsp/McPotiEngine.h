#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <atomic>
#include <vector>

#include <Biquad.h>
#include <Crossover.h>
#include <DelayLine.h>
#include <Loudness.h>
#include <OversamplerN.h>
#include <VuBallistics.h>

#include "LinearSplit.h"

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
    int osIndex = 2; // 0 a 3 = 1×, 2×, 4×, 8×
    float airCornerHz = 16000.0f;
    float phasePercent = 40.0f, colorPercent = 0.0f, softenHz = 20000.0f, widthPercent = 100.0f;
    bool monoSafe = false;
    float guardPercent = 0.0f;
    bool match = false;
    bool linearPhase = false; // cruce FIR de fase lineal: añade LinearSplit::center muestras de latencia
};

/** Fase 2 (parcial): cruce de cuatro bandas y generador de armónicos de los cuatro caracteres Vintage con
    ADAA de primer orden y sobremuestreo de 1× a 8×, dinámica por banda, los cuatro caracteres Modernos (Dinámico, Adaptativo, Seda y Aire), mezcla, Delta y Solo.
    La latencia es constante (la de 8×): el seco, Solo y los factores menores se retrasan lo que falta.
    Color, Fase, Suavizar, Ancho, Mono seguro, guarda de sibilancia e Igualar ya están. Pendiente: fase lineal. */
class McPotiEngine
{
public:
    static constexpr int maxFactor = 8;
    /** Latencia en muestras base. Con el cruce LR4 es constante (la de subir y bajar a 8× con fase lineal): el
        sobremuestreo y el carácter nunca la cambian. El cruce de fase lineal suma `LinearSplit::center` muestras y
        es el único ajuste que la cambia (el procesador avisa al anfitrión). */
    static int latencySamples (bool linearPhase = false) { return ferb::OsDesign::latencyFor (maxFactor, false) + (linearPhase ? LinearSplit::center : 0); }

    void prepare (double sampleRate, int maxBlock);
    void reset() noexcept;
    void process (juce::AudioBuffer<float>& buffer, const Settings& s) noexcept;

    /** Nivel añadido por banda (RMS suavizado, lineal), para las válvulas de la interfaz. */
    /** Ganancia que aplica Igualar, en dB (para la interfaz). */
    /** Nivel lineal de entrada y de salida con balística de VU (65 ms), para los vúmetros. */
    float inputVuLevel() const noexcept { return vuIn.level(); }
    float outputVuLevel() const noexcept { return vuOut.level(); }
    float matchGainDb() const noexcept { return matchDb.load (std::memory_order_relaxed); }
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
        LinearSplit linear;
        std::array<ferb::Biquad, nb> hpIn, hpOut, shelf;
        std::array<float, nb> env {}, gcTarget, gcSmooth;
        std::array<ferb::UpN, nb> up;
        std::array<ferb::DownN, nb> down;
        std::array<AdaaState, nb> adaa, adaaOdd; // adaaOdd: la rama con −u, para Color hacia los impares
        std::array<ferb::Lr4, 4> phaseAp;        // red de pasa-todo de lo añadido (600, 1800, 5000 y 11000 Hz)
        ferb::Biquad soften;
        std::array<ferb::Biquad, 2> kDry, kOut;   // ponderación K de la entrada y de la salida (Igualar)                      // corte superior de lo añadido
        ferb::DelayLine dryDelay;
        std::array<ferb::DelayLine, nb> bandDelay, addDelay;
        std::array<std::vector<float>, nb> bandSig, nlIn, nlOut;
    };

    void updateControl (const Settings& s) noexcept;
    void processChunk (juce::AudioBuffer<float>& buffer, int start, int n, const Settings& s) noexcept;
    void configureFactor (int factor) noexcept;
    void applyAddDelays() noexcept;
    void designShelves() noexcept;

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
    bool linearActive = false, firPending = false;
    int firCounter = 0, muteHold = 0; // muteHold: muestras que la salida sigue en silencio mientras se llenan los retardos
    Smoother outGate;               // funde TODA la salida al cambiar de cruce (cambia la latencia)
    ferb::DelayLine guardDelay;     // alinea la guarda con lo añadido cuando el cruce es de fase lineal
    void applyLinearPhase (bool linear) noexcept;
    std::array<float, 3> smoothXo {};
    float smoothTune = 4000.0f, freqCoeff = 0.0f;
    std::array<float, 3> shelfXo {};
    float smoothAir = 16000.0f, shelfAir = 0.0f, adaptGain = 1.0f, tonalCoeff = 0.0f;
    ferb::Lr4 sideSplit;       // Mono seguro: el lateral de lo añadido pierde lo que queda bajo 150 Hz
    float smoothSoften = 20000.0f, lastSoften = 0.0f;
    // Guarda de sibilancia: detector de 5–10 kHz frente a 300 Hz–3 kHz con histéresis; baja Presencia y Aire (Vocal).
    std::array<ferb::Biquad, 4> guardFilter; // paso alto 5 kHz, paso bajo 10 kHz, paso alto 300 Hz, paso bajo 3 kHz
    float guardHi = 0.0f, guardMid = 0.0f, guardDb = 0.0f, guardEnvCoeff = 0.0f, guardAttack = 0.0f, guardRelease = 0.0f;
    bool sibilant = false;
    std::vector<float> guardBuf;
    // Igualar: sonoridad K de 3 s de la entrada frente a la de la salida.
    float powerDry = 0.0f, powerOut = 0.0f, powerCoeff = 0.0f, matchLin = 1.0f;
    std::atomic<float> matchDb { 0.0f };
    ferb::VuBallistics vuIn, vuOut;
    std::array<float, 2> tonal {}; // potencia de Medios y de Presencia, promedio de 2 s
};
} // namespace mcpoti
