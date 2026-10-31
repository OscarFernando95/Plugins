#pragma once

#include <SegmentedControl.h>

#include "../PluginProcessor.h"
#include "ParamField.h"
#include "Style.h"

namespace poty::ui
{
/** Panel global: Entrada, Salida, Mezcla, Calidad e Intensidad. */
class GlobalPanel : public juce::Component
{
public:
    explicit GlobalPanel (PotyProcessor& p);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<ParamField> value;
        std::unique_ptr<juce::SliderParameterAttachment> attachment;
        juce::String caption;
    };

    std::array<Knob, 3> knobs;
    std::unique_ptr<ferb::SegmentedControl> quality, intensity;
};
} // namespace poty::ui
