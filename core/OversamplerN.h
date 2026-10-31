#pragma once

#include <array>
#include <vector>

namespace ferb
{
/** Sobremuestreo por etapas de media banda de 2×, hasta 32× (5 etapas), con filtros de fase lineal
    (Kaiser, ~100 dB) o de fase mínima (los mismos filtros pasados a fase mínima por el método del
    cepstro). Cada etapa es un FIR polifásico general.

    La latencia de subir y bajar se expresa en muestras de la frecuencia más alta, con un retardo de
    relleno para que quede en un número ENTERO de muestras base. En fase mínima el retardo de cada
    etapa es el de grupo a 0 Hz, redondeado: la latencia real varía unas décimas de muestra con la
    frecuencia. */
class OsDesign
{
public:
    static constexpr int maxStages = 5;
    static constexpr int maxFactor = 32;

    struct Stage
    {
        std::vector<float> taps; // suma 1; longitud impar
        double delay = 0.0;      // retardo de grupo a 0 Hz, en muestras de la frecuencia de salida de la etapa
    };

    static const std::array<Stage, maxStages>& stages (bool minimumPhase);
    static int stagesFor (int factor) noexcept; // 1→0, 2→1, 4→2, 8→3, 16→4, 32→5

    /** Retardo de subir + bajar, en muestras de la frecuencia más alta, sin relleno. */
    static int unitsFor (int factor, bool minimumPhase);
    /** Muestras de la frecuencia más alta que completan una latencia entera en muestras base. */
    static int padFor (int factor, bool minimumPhase);
    /** Latencia total de subir + bajar, en muestras base (entera). */
    static int latencyFor (int factor, bool minimumPhase);
};

/** Sube un canal por 1, 2, 4, 8, 16 o 32. Una instancia por canal. */
class UpN
{
public:
    void prepare (int maxBlockSize);
    void reset();
    void setConfig (int factor, bool minimumPhase) noexcept; // reinicia el estado
    int getFactor() const noexcept { return factor; }

    /** `in` tiene n muestras; `out` recibe n·factor. */
    void process (const float* in, float* out, int n) noexcept;

private:
    void runStage (int stage, const float* in, float* out, int n) noexcept;

    int factor = 1;
    bool minPhase = false;
    std::array<std::vector<float>, OsDesign::maxStages> history, ext;
    std::vector<float> mid[2];
};

/** Baja un canal por 1, 2, 4, 8, 16 o 32. Una instancia por canal. */
class DownN
{
public:
    void prepare (int maxBlockSize);
    void reset();
    void setConfig (int factor, bool minimumPhase) noexcept;
    int getFactor() const noexcept { return factor; }

    /** `in` tiene n·factor muestras; `out` recibe n. Incluye el retardo de relleno. */
    void process (const float* in, float* out, int n) noexcept;

private:
    void runStage (int stage, const float* in, float* out, int n) noexcept;

    int factor = 1;
    bool minPhase = false;
    std::array<std::vector<float>, OsDesign::maxStages> history, ext;
    std::vector<float> mid[2];
    std::vector<float> padded, padHistory;
};
} // namespace ferb
