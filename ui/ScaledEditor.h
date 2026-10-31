#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ferb
{
/** Editor redimensionable. Todo se dibuja en `content`, con las coordenadas del tamaño base; al
    cambiar el tamaño de la ventana, `content` se escala entero. La proporción se mantiene y el
    tamaño se recuerda en el estado del plugin (propiedad `uiScale`, no es un parámetro). */
class ScaledEditor : public juce::AudioProcessorEditor
{
public:
    static constexpr double minScale = 0.6, maxScale = 2.0;

    ScaledEditor (juce::AudioProcessor& p, juce::ValueTree stateTree, int baseWidth, int baseHeight, double minimumScale = minScale)
        : AudioProcessorEditor (p), state (std::move (stateTree)), baseW (baseWidth), baseH (baseHeight), lowestScale (minimumScale)
    {
        addAndMakeVisible (content);
        content.setBounds (0, 0, baseW, baseH);

        setResizable (true, true);
        setResizeLimits (juce::roundToInt (baseW * lowestScale), juce::roundToInt (baseH * lowestScale),
                         juce::roundToInt (baseW * maxScale), juce::roundToInt (baseH * maxScale));
        getConstrainer()->setFixedAspectRatio (static_cast<double> (baseW) / baseH);

        const double saved = juce::jlimit (lowestScale, maxScale, static_cast<double> (state.getProperty ("uiScale", 1.0)));
        setSize (juce::roundToInt (baseW * saved), juce::roundToInt (baseH * saved));
        ready = true;
    }

    void paint (juce::Graphics& g) override { g.fillAll (backdrop); }

    void resized() override
    {
        const float scale = static_cast<float> (getWidth()) / static_cast<float> (baseW);
        content.setTransform (juce::AffineTransform::scale (scale));
        content.setBounds (0, 0, baseW, baseH);

        if (ready && std::abs (scale - static_cast<float> (static_cast<double> (state.getProperty ("uiScale", 1.0)))) > 0.002f)
            state.setProperty ("uiScale", static_cast<double> (scale), nullptr);
    }

    /** Escala actual respecto del tamaño base. */
    float getScale() const noexcept { return static_cast<float> (getWidth()) / static_cast<float> (baseW); }

protected:
    juce::Component content;
    juce::Colour backdrop { 0xff000000 };

private:
    juce::ValueTree state;
    int baseW, baseH;
    double lowestScale;
    bool ready = false;
};
} // namespace ferb
