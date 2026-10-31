#include "KnobUnit.h"

namespace tufi::ui
{
KnobUnit::KnobUnit (juce::RangedAudioParameter& parameter, Spec s) : spec (std::move (s))
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (220);
    addAndMakeVisible (slider);
    attachment = std::make_unique<juce::SliderParameterAttachment> (parameter, slider, nullptr);

    ferb::ParamField::Style style;
    style.font = mono (spec.valueSize * 1.3f);
    style.text = inkSoft;
    style.editText = ink;
    style.editBackground = ivory.brighter (0.3f);
    style.editOutline = brass;
    style.justification = juce::Justification::centred;

    value = std::make_unique<ferb::ParamField> (parameter, spec.format,
                                                [] (const juce::String& t, float old) { return parseNumber (t, old); }, style);
    addAndMakeVisible (*value);
}

void KnobUnit::resized()
{
    const auto c = knobCentre();
    slider.setBounds (juce::Rectangle<int> (spec.diameter, spec.diameter).withCentre (c.toInt()));
    const float w = float (getWidth());
    value->setBounds (juce::Rectangle<float> (0.0f, c.y + spec.valueOffset - spec.valueSize * 0.7f, w, spec.valueSize * 1.5f).toNearestInt());
}

void KnobUnit::paint (juce::Graphics& g)
{
    const auto c = knobCentre();
    const float d = float (spec.diameter);

    // «0» y «10» al pie de los extremos del arco.
    g.setColour (ink);
    g.setFont (label (spec.scaleSize, 0.0f));
    for (int end = 0; end < 2; ++end)
    {
        const float a = juce::MathConstants<float>::pi * (end == 0 ? 1.25f : 2.75f);
        const float r = d * 0.5f + spec.scaleSize * 0.2f;
        const juce::Point<float> p (c.x + std::sin (a) * r * 0.82f, c.y - std::cos (a) * r * 0.82f + spec.scaleSize * 0.7f);
        g.drawText (end == 0 ? "0" : "10", juce::Rectangle<float> (p.x - 12.0f, p.y - 6.0f, 24.0f, 12.0f), juce::Justification::centred);
    }

    g.setColour (ink);
    g.setFont (label (spec.captionSize, 0.1f));
    g.drawText (spec.caption.toUpperCase(), juce::Rectangle<float> (0.0f, c.y + spec.captionOffset - spec.captionSize, float (getWidth()), spec.captionSize * 2.0f),
                juce::Justification::centred);
}
} // namespace tufi::ui
