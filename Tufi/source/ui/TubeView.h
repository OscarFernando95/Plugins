#pragma once

#include "../PluginProcessor.h"
#include "Style.h"

namespace tufi::ui
{
/** Ventana con la válvula encendida. El brillo sigue el nivel de salida y la forma, la etapa activa. */
class TubeView : public juce::Component, private juce::Timer
{
public:
    TubeView (TufiProcessor& p, std::function<float()> levelSource);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;

    TufiProcessor& processor;
    std::function<float()> level;
    float glow = 0.5f;
    int stage = 0;
};
} // namespace tufi::ui
