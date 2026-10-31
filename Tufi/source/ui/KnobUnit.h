#pragma once

#include <ParamField.h>

#include <functional>

#include "Style.h"

namespace tufi::ui
{
/** Perilla con su rótulo, sus extremos «0» y «10» y el valor editable debajo. */
class KnobUnit : public juce::Component
{
public:
    struct Spec
    {
        int diameter = 64;       // perilla con marcas
        float captionOffset = 40.0f; // desde el centro de la perilla
        float valueOffset = 55.0f;
        float captionSize = 13.0f;
        float valueSize = 12.0f;
        float scaleSize = 8.0f;
        juce::String caption;
        std::function<juce::String (float)> format;
    };

    KnobUnit (juce::RangedAudioParameter& parameter, Spec s);

    void paint (juce::Graphics&) override;
    void resized() override;

    /** Dónde queda el centro de la perilla dentro de este componente. */
    juce::Point<float> knobCentre() const noexcept { return { float (getWidth()) * 0.5f, float (spec.diameter) * 0.5f + 2.0f }; }

private:
    Spec spec;
    juce::Slider slider;
    std::unique_ptr<juce::SliderParameterAttachment> attachment;
    std::unique_ptr<ferb::ParamField> value;
};
} // namespace tufi::ui
