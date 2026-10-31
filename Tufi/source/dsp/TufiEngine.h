#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <DcBlocker.h>
#include <DelayLine.h>
#include <LevelMeter.h>
#include <ModeSwitch.h>
#include <Oversampler.h>

#include <array>
#include <memory>
#include <vector>

#include "ShapeTable.h"

namespace tufi
{
/** Valores de los parámetros en unidades de la interfaz (dB, 0 a 10). */
struct Settings
{
    int stage = 0;      // 0 = triodo, 1 = pentodo, 2 = diodo
    float driveDb = 0.0f;
    float trimDb = 0.0f;
    float lowAmount = 0.0f;
    float peakAmount = 0.0f;
    float peakHardness = 0.0f;
    float evenTop = 0.0f;
    float evenBottom = 0.0f;
    int oversample = 1; // factor = 2^oversample: 0 = sin sobremuestreo, 1 = 2×, 2 = 4×, 3 = 8×
    bool bypass = false;
};

/** Cadena de procesado de Tufi, independiente del anfitrión para poder probarla.

        entrada → [sobremuestreo] → curva de transferencia con ADAA → [submuestreo] → filtro de DC → salida

    Cada canal se procesa por separado, sin enlace estéreo.

    - Latencia constante: la del factor máximo (8×). Los demás modos retrasan la señal lo que falta,
      así que cambiar el sobremuestreo no cambia la alineación ni obliga al anfitrión a recalcularla.
    - Cambios sin clics: los controles de forma y la etapa mueven la curva de forma suave (la tabla se
      acerca a la nueva en unos 15 ms). Cambiar el sobremuestreo o pasar por cero los controles de forma
      cruza por la ruta de ganancia simple, con fundidos cortos. */
class TufiEngine
{
public:
    static constexpr int maxChannels = 2;
    static constexpr int chunkSize = 256;

    TufiEngine();
    ~TufiEngine();

    /** Latencia constante que debe declarar el plugin, en muestras: la del factor máximo (8×). */
    static int getLatencySamples() noexcept { return ferb::Oversampler::latencyFor (ferb::Oversampler::maxFactor); }

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    /** Pruebas y renderizado sin conexión: las tablas se calculan en el propio hilo, sin hilo secundario. */
    void setSynchronousTables (bool synchronous) noexcept { synchronousTables = synchronous; }

    void process (juce::AudioBuffer<float>& buffer, const Settings& settings) noexcept;

    const ferb::LevelMeter& getMeter() const noexcept { return meter; }
    ferb::LevelMeter& getMeter() noexcept { return meter; } // la interfaz consume el máximo acumulado

    /** Contadores de la interfaz: pendiente en cero, techo (dB) y asimetría (%). */
    float getSlope() const noexcept { return slope.load (std::memory_order_relaxed); }
    float getCeilingDb() const noexcept { return ceilingDb.load (std::memory_order_relaxed); }
    float getAsymmetry() const noexcept { return asymmetry.load (std::memory_order_relaxed); }

private:
    using Smoothed = juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>;

    void processChunk (juce::AudioBuffer<float>& chunk, const Settings& settings) noexcept;
    void applyStructure (int key);
    void updateShape (const Settings& s) noexcept;
    void publishTable (const ShapeResult& r) noexcept;

    double sampleRate = 48000.0;

    Smoothed drive, trim, bypassMix; // bypassMix: 0 = procesado, 1 = seco

    // Curva de trabajo, que se acerca a la tabla de destino sin saltos.
    ShapeLut lut;
    Table target;
    ShapeResult initial;
    ShapeParams lastRequested;
    bool converged = true;
    unsigned lutVersion = 0; // cambia cada vez que se recalcula la curva de trabajo
    std::unique_ptr<ShapeTableBuilder> builder;
    bool synchronousTables = false;
    std::atomic<float> slope { 1.0f }, ceilingDb { -3.0f }, asymmetry { 0.0f };

    std::array<ferb::UpSampler, maxChannels> ups;
    std::array<ferb::DownSampler, maxChannels> downs;
    std::array<LutAdaa, maxChannels> adaas;
    std::array<ferb::DcBlocker, maxChannels> dcBlockers;
    std::array<ferb::DelayLine, maxChannels> rawDelay, neutralDelay, wetDelay;

    ferb::ModeSwitch modeSwitch; // clave 0 = ganancia simple; 1…4 = curva con sobremuestreo 1×, 2×, 4×, 8×
    ferb::LevelMeter meter;

    int factor = 1;
    bool asleep = false;
    bool needsInit = true;

    std::array<std::vector<float>, maxChannels> x, neutral, wet, rawOut;
    std::vector<float> os;
    std::vector<float> driveRamp, trimRamp, bypassRamp;
};
} // namespace tufi
