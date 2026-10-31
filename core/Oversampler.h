#pragma once

#include <array>
#include <vector>

namespace ferb
{
/** Sobremuestreo por etapas de media banda FIR de fase lineal (Kaiser, ~100 dB de rechazo).

    Un factor 2 es una etapa; 4, dos; 8, tres. La latencia de subir y bajar es un número ENTERO
    de muestras de la frecuencia base: en los factores 4 y 8 se añade un pequeño retardo a la
    frecuencia más alta para conseguirlo. Eso permite sumar la señal original retrasada con la
    procesada sin desfases, que es lo que necesita Poty. */
class Oversampler
{
public:
    static constexpr int maxFactor = 8;
    static constexpr int maxStages = 3;

    /** Latencia de la cadena subir + bajar, en muestras base. Factor 1, 2, 4 u 8. */
    static int latencyFor (int factor) noexcept;

    /** Muestras de retardo a la frecuencia más alta que completan una latencia entera. */
    static int padFor (int factor) noexcept;
};

/** Sube un canal por un factor de 1, 2, 4 u 8. Una instancia por canal y por señal. */
class UpSampler
{
public:
    /** Reserva memoria para `maxFactor`; el factor activo es ese mismo. */
    void prepare (int maxFactor, int maxBlockSize);
    void reset();

    /** Cambia el factor activo (≤ maxFactor) sin asignar memoria. Reinicia el estado. */
    void setFactor (int newFactor) noexcept;
    int getFactor() const noexcept { return factor; }

    /** `in` tiene n muestras; `out` recibe n·factor. */
    void process (const float* in, float* out, int n) noexcept;

private:
    struct Stage
    {
        std::vector<float> taps; // 2·h[2p], p = 0..M
        std::vector<float> history;
        std::vector<float> ext;
        int M = 0;
    };

    void runStage (Stage& s, const float* in, float* out, int n) noexcept;

    int factor = 1, maxFactor = 1;
    std::array<Stage, Oversampler::maxStages> stages;
    std::vector<float> midA, midB;
};

/** Baja un canal por un factor de 1, 2, 4 u 8. `in` tiene n·factor muestras; `out` recibe n. */
class DownSampler
{
public:
    void prepare (int maxFactor, int maxBlockSize);
    void reset();
    void setFactor (int newFactor) noexcept;
    int getFactor() const noexcept { return factor; }

    void process (const float* in, float* out, int n) noexcept;

private:
    struct Stage
    {
        std::vector<float> taps; // h[2p], p = 0..M
        std::vector<float> history;
        std::vector<float> ext;
        float center = 0.5f;
        int M = 0;
    };

    void runStage (Stage& s, const float* in, float* out, int n) noexcept;

    int factor = 1, maxFactor = 1;
    std::array<Stage, Oversampler::maxStages> stages;
    std::vector<float> midA, midB, padded;
    std::array<float, 8> padHistory {};
};
} // namespace ferb
