#pragma once

#include <array>
#include <complex>
#include <vector>

#include <Biquad.h>

namespace chaus
{
/** Los rasgos de un cuadro de análisis: los cinco de la ficha, el «aire» (ver abajo) y la dispersión (que usa el seguimiento del ancho). */
struct Features
{
    float levelDb = -120.0f;   // energía de la banda de sibilante (HP a LP), dBFS eficaces
    float ratioDb = -40.0f;    // banda frente a la zona de 300 Hz a 3 kHz
    float flatness = 0.0f;     // planitud espectral en la banda: 1 = ruido blanco ideal, 0 = un tono
    float centroidHz = 0.0f;   // centroide espectral de la banda
    float slopeDbOct = 0.0f;   // pendiente de 2 a 4 kHz, en dB por octava (positiva = sube hacia los agudos)
    float airDb = -40.0f;      // densidad espectral de 12 a 18 kHz frente a la de 5 a 9 kHz (platillos y hi-hats son planos hasta arriba; una s cae)
    float spreadOct = 0.0f;    // dispersión del espectro de la banda, en octavas (da el ancho Q)
};

/** Regresión logística sobre los cinco rasgos. Los pesos son valores iniciales ajustados con señales sintéticas. */
struct Classifier
{
    static constexpr int numFeatures = 6;
    std::array<float, numFeatures> weights {};
    float bias = 0.0f;

    static Classifier initial();

    /** Rasgos llevados a una escala parecida. El centroide entra como cercanía al centro geométrico de la
        banda (un clasificador lineal no puede, si no, rechazar a la vez lo más agudo y lo más grave). */
    static std::array<float, numFeatures> normalise (const Features&, float bandCentreHz) noexcept;

    float logit (const Features&, float bandCentreHz) const noexcept;
    float probability (const Features&, float bandCentreHz) const noexcept;
};

/** Rasgos de un espectro de potencia (bins 0 a N/2 de una FFT de N puntos con ventana de Hann). */
Features extractFeatures (const double* power, int fftSize, double sampleRate, float hpHz, float lpHz) noexcept;

/** Detector de sibilantes de un canal. Dos caminos en paralelo:
    - nivel de la banda en el dominio del tiempo (HP + LP y envolvente de 1 ms), muestra a muestra;
    - clasificador por cuadros (512 muestras con salto de 128 a 44,1 o 48 kHz; el doble y el cuádruple a 96 y 192 kHz),
      con puntuación suavizada 5 ms e histéresis (enciende en 0,6, apaga en 0,4).
    Todo se reserva en `prepare`; `tick` no asigna memoria. */
class SibilanceDetector
{
public:
    struct Output
    {
        float levelDb;  // nivel de la banda de sibilante en este instante
        float score;    // puntuación 0 a 1 (suavizada)
        bool active;    // con histéresis
        float band;     // la señal filtrada entre HP y LP (lo que «oye» el detector)
    };

    static constexpr float onThreshold = 0.6f, offThreshold = 0.4f;
    static constexpr float scoreSmoothingMs = 5.0f;
    static constexpr float silenceFloorDb = -75.0f; // por debajo, el cuadro vale cero

    void prepare (double sampleRate);
    void reset() noexcept;
    void setBand (float hpHz, float lpHz) noexcept;
    void setClassifier (const Classifier& c) noexcept { classifier = c; }
    /** Apagado (modo General), no hay clasificador: la puntuación vale 1 y no se calcula la FFT. */
    void setClassifierEnabled (bool on) noexcept { classifierOn = on; }

    Output tick (float x) noexcept;

    int frameSize() const noexcept { return frameLength; }
    int hopSize() const noexcept { return hop; }
    /** Rasgos del último cuadro analizado. */
    const Features& features() const noexcept { return lastFeatures; }
    float probability() const noexcept { return lastProbability; }
    /** Número de cuadros analizados desde el último `reset`. */
    long frameCount() const noexcept { return frames; }

private:
    void analyse() noexcept;

    double fs = 48000.0;
    int frameLength = 512, hop = 128;
    float hpHz = 4000.0f, lpHz = 12000.0f;
    Classifier classifier = Classifier::initial();
    bool classifierOn = true;

    ferb::Biquad highPass, lowPass;
    double envelope = 0.0, envelopeCoeff = 0.0;

    std::vector<float> ring;
    std::vector<double> window, power;
    std::vector<std::complex<double>> spectrum;
    int writePos = 0, sinceHop = 0;
    long frames = 0;

    Features lastFeatures;
    float lastProbability = 0.0f, score = 0.0f, scoreCoeff = 0.0f;
    bool active = false;
};

/** Nivel de referencia de las sibilantes: percentil 90 de los picos de banda de los eventos de los últimos 5 s.
    El umbral efectivo es el parámetro Umbral más este nivel. Sin eventos en 5 s conserva el último valor. */
class AdaptiveReference
{
public:
    static constexpr float windowSeconds = 5.0f;
    static constexpr float minEventMs = 10.0f;      // un cruce más corto no cuenta como sibilante
    static constexpr float smoothingSeconds = 0.25f; // el nivel de referencia sigue al percentil con esta constante

    void prepare (double sampleRate);
    void reset (float initialDb = initialReferenceDb) noexcept;

    /** Una muestra: si la sibilante está activa y su nivel de banda. */
    void tick (bool active, float levelDb) noexcept;

    float referenceDb() const noexcept { return smoothed; }
    /** Percentil 90 sin suavizar (el valor al que tiende). */
    float targetDb() const noexcept { return target; }
    int eventCount() const noexcept { return count; }

    static constexpr float initialReferenceDb = -30.0f;

private:
    static constexpr int capacity = 64;

    void closeEvent() noexcept;
    void recompute() noexcept;

    double fs = 48000.0;
    long now = 0, windowSamples = 0, minEventSamples = 0, eventStart = 0;
    float smoothingCoeff = 0.0f;
    bool inEvent = false;
    float eventPeak = -200.0f;
    struct Event { long end = 0; float peakDb = 0.0f; };
    std::array<Event, capacity> events {};
    int head = 0, count = 0;
    float target = initialReferenceDb, smoothed = initialReferenceDb;
};
} // namespace chaus
