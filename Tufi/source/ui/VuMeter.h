#pragma once

#include "../PluginProcessor.h"
#include "Style.h"

namespace tufi::ui
{
/** VU con aguja: arco de −48° a +48°, el cero a +20° y zona roja de 0 a +3. La aguja sigue el
    nivel de pico de salida con una balística de unos 300 ms. */
class VuMeter : public juce::Component, private juce::Timer
{
public:
    explicit VuMeter (TufiProcessor& p);

    void paint (juce::Graphics&) override;

    /** Nivel suavizado (lineal), para que la válvula brille con el mismo nivel. */
    float getLevel() const noexcept { return smoothed; }

private:
    void timerCallback() override;

    /** Posición de la aguja, en grados desde la vertical, para un valor en VU. */
    static float vuToDegrees (float vu) noexcept;

    TufiProcessor& processor;
    float smoothed = 0.0f;
    float needleDegrees = -48.0f;
};
} // namespace tufi::ui
