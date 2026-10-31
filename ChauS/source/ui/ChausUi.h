#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <vector>

#include <Controls.h>
#include <SampleRing.h>

#include "../dsp/ChausEngine.h"

namespace chaus::ui
{
/** Espectro por bandas de 1/6 de octava (nivel eficaz en dBFS) para la pantalla. Corre en el hilo de la
    interfaz: toma las últimas muestras del anillo, hace FFT de 2048 con solape del 75 % (cuatro ventanas
    por lectura), promedia la potencia y la integra en `numPoints` puntos de 200 Hz a 20 kHz. */
class SpectrumAnalyzer
{
public:
    static constexpr int fftOrder = 11, fftSize = 1 << fftOrder, numPoints = 160;
    static constexpr float minFreq = 200.0f, maxFreq = 20000.0f;

    SpectrumAnalyzer();

    /** Frecuencia de cada punto de salida. */
    static float pointFrequency (int index) noexcept;

    /** Devuelve false si el anillo aún no tiene muestras suficientes. */
    bool analyse (const ferb::SampleRing& ring, double sampleRate, std::array<float, numPoints>& dbOut);

private:
    juce::dsp::FFT fft { fftOrder };
    // Sin normalizar: la ventana de Hann normalizada de JUCE sube 6 dB y la calibración (media cuadrática) deja de valer.
    juce::dsp::WindowingFunction<float> window { static_cast<size_t> (fftSize), juce::dsp::WindowingFunction<float>::hann, false };
    std::vector<float> samples, fftData;
    std::vector<double> power, cumulative;
};

/** Ventana superior: espectro de entrada (violeta) y salida (crema), la zona de sibilante entre HP y LP,
    lo que se quita (coral), y la campana dinámica con su manejador, su frecuencia y su reducción. */
class ScreenView : public ferb::vintage::GlassView
{
public:
    static constexpr int width = 660, height = 290;

    struct Data
    {
        std::array<float, SpectrumAnalyzer::numPoints> input, output;
        float hpHz = 4000.0f, lpHz = 12000.0f;
        float centreHz = 7000.0f, q = 3.0f, reductionDb = 0.0f, shapeWeight = 0.0f;
        double sampleRate = 48000.0;
        bool listening = false;
    };

    explicit ScreenView (const ferb::vintage::Palette& p) : GlassView (p, true) { data.input.fill (-120.0f); data.output.fill (-120.0f); }
    void setData (const Data& d) { data = d; repaint(); }

protected:
    void paintContent (juce::Graphics&, float w, float h) override;

private:
    Data data;
};

/** Ventana inferior: unos 3 s de historia con el nivel de la banda de sibilante, el umbral efectivo, la
    reducción (desde arriba, en ámbar) y una marca sobre cada evento detectado. */
class TimelineView : public ferb::vintage::GlassView
{
public:
    static constexpr int width = 660, height = 84;
    static constexpr int numPoints = 375; // 3 s a 8 ms

    explicit TimelineView (const ferb::vintage::Palette& p) : GlassView (p, true) { points.resize (numPoints); }
    void setPoints (const std::vector<Telemetry::Point>& p) { points = p; repaint(); }

protected:
    void paintContent (juce::Graphics&, float w, float h) override;

private:
    std::vector<Telemetry::Point> points;
};
} // namespace chaus::ui
