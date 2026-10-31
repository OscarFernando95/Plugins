#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <functional>

#include "../PluginProcessor.h"
#include "Style.h"

namespace poty::ui
{
/** Analizador de espectro, curva del EQ y nodos arrastrables.

    Eje X: 20 Hz a 20 kHz en escala logarítmica. Eje Y: 0 a −80 dB. Los nodos viven en la misma
    escala: la ganancia de una banda (0 a +12 dB) se dibuja entre −68 y −25 dB, como en el bosquejo. */
class SpectrumView : public juce::Component, private juce::Timer
{
public:
    SpectrumView (PotyProcessor& p, int& selectedBand, std::function<void (int)> onSelect);
    ~SpectrumView() override;

    void paint (juce::Graphics&) override;
    void paintOverlay (juce::Graphics&);
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    /** Rango de dibujo del plano, en coordenadas de este componente. */
    static constexpr float plotX = 57.0f, plotY = 55.0f, plotW = 980.0f, plotH = 230.0f;

private:
    static constexpr int fftOrder = 13, fftSize = 1 << fftOrder, numPoints = 280, curvePoints = 400;
    static constexpr double minFreq = 20.0, maxFreq = 20000.0;
    static constexpr float floorDb = -80.0f;
    static constexpr double curveOffset = -68.0, curveScale = 3.6; // dB del plano = −68 + 3,6 · ganancia

    struct Node { int band; juce::Point<float> pos; };

    void timerCallback() override;
    void analyze (const ferb::SampleRing& ring, std::array<float, numPoints>& dbOut);
    void rebuildCurve();
    juce::Path makePath (const std::array<float, numPoints>& db, bool closed) const;

    float freqToX (double f) const noexcept;
    double xToFreq (float x) const noexcept;
    float dbToY (double db) const noexcept;
    double yToDb (float y) const noexcept;

    juce::RangedAudioParameter* param (int band, const char* suffix) const;
    float realValue (int band, const char* suffix) const;
    void setReal (int band, const char* suffix, float real);
    int bandAt (juce::Point<float> p) const;
    juce::Point<float> nodePosition (int band) const;
    void drawNodes (juce::Graphics&);
    void drawLabel (juce::Graphics&, int band);

    PotyProcessor& processor;
    int& selected;
    std::function<void (int)> onSelect;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { static_cast<size_t> (fftSize), juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> samples, fftData;
    std::array<float, numPoints> inDb, outDb;
    juce::Path curve;
    std::array<float, 5 * 4> lastParams {};
    double lastSampleRate = 0.0;

    class Grid;
    class Overlay;
    std::unique_ptr<Grid> grid;
    std::unique_ptr<Overlay> overlay;

    int dragBand = -1;
    juce::Point<float> dragOffset;
    int hoverBand = -1;
};
} // namespace poty::ui
