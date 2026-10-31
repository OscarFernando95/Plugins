#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "../PluginProcessor.h"
#include "Style.h"

namespace tufi::ui
{
/** Osciloscopio de fósforo con la curva de transferencia, y debajo los tres contadores
    (pendiente, techo y asimetría). La curva se calcula con los mismos controles que usa el motor. */
class ScopeView : public juce::Component, private juce::Timer
{
public:
    explicit ScopeView (TufiProcessor& p);

    void paint (juce::Graphics&) override;

    static constexpr int frameSize = 268;

private:
    static constexpr int numPoints = 200;
    static constexpr double range = 1.6; // la rejilla cubre de −1,6 a +1,6

    void timerCallback() override;
    void recompute();
    void drawScreen (juce::Graphics&, juce::Rectangle<float> screen);
    void drawCounters (juce::Graphics&);

    TufiProcessor& processor;
    tufi::ShapeResult result;
    tufi::ShapeParams params;
    bool havePath = false;
    juce::Path curvePath;
    juce::Image scanlines;
};
} // namespace tufi::ui
