#pragma once

#include <Controls.h>

#include "../PluginProcessor.h"

namespace cremita::ui
{
/** Editor de las tres bandas dentro de la ventana de cristal: los dos cruces se arrastran en horizontal
    (escala logarítmica de 40 Hz a 16 kHz) y el peso de cada banda (±6 dB) en vertical. Doble clic en una
    banda lo devuelve a 0 dB, y en un cruce, a su valor por defecto. Mayús afina. */
class BandView : public ferb::vintage::GlassView
{
public:
    BandView (CremitaProcessor&, const ferb::vintage::Palette&);

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    static constexpr float lowestHz = 40.0f, highestHz = 16000.0f, trimRangeDb = 6.0f;
    static float frequencyToX (float hz, float width) noexcept;
    static float xToFrequency (float x, float width) noexcept;

protected:
    void paintContent (juce::Graphics&, float width, float height) override;

private:
    enum class Target { none, crossover1, crossover2, bandLow, bandMid, bandHigh };

    Target targetAt (juce::Point<float> p) const;
    juce::RangedAudioParameter& paramFor (Target) const;
    juce::ParameterAttachment& attachmentFor (Target);
    float valueOf (Target) const;
    float widthInside() const noexcept { return static_cast<float> (getWidth() - 2 * margin); }

    CremitaProcessor& processor;
    std::array<std::unique_ptr<juce::ParameterAttachment>, 5> attachments; // xo1, xo2, trim LF, MF, HF
    Target dragging = Target::none;
    float dragStartValue = 0.0f;
};
} // namespace cremita::ui
