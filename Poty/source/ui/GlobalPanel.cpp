#include "GlobalPanel.h"

namespace poty::ui
{
namespace
{
ferb::SegmentedControl::Style segmentStyle()
{
    ferb::SegmentedControl::Style s;
    s.border = border;
    s.text = text;
    s.activeFill = accent;
    s.activeText = bg;
    s.font = ui (13.0f, true);
    s.corner = 8.0f;
    s.gap = 4.0f;
    return s;
}

juce::RangedAudioParameter& get (PotyProcessor& p, const char* id)
{
    return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id));
}
} // namespace

GlobalPanel::GlobalPanel (PotyProcessor& p)
{
    const char* ids[] = { params::inGain, params::outGain, params::mix };
    const char* captions[] = { "ENTRADA", "SALIDA", "MEZCLA" };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.caption = captions[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        k.slider.setMouseDragSensitivity (220);
        addAndMakeVisible (k.slider);
        k.attachment = std::make_unique<juce::SliderParameterAttachment> (get (p, ids[i]), k.slider, nullptr);

        const bool percent = i == 2;
        k.value = std::make_unique<ParamField> (
            get (p, ids[i]),
            [percent] (float v) { return percent ? juce::String (juce::roundToInt (v)) + " %" : comma (v, 1) + " dB"; },
            [] (const juce::String& s, float old) { return parseNumber (s, old); }, accent);
        k.value->setFont (mono (12.0f, true));
        k.value->setJustificationType (juce::Justification::centred);
        addAndMakeVisible (*k.value);
    }

    quality = std::make_unique<ferb::SegmentedControl> (get (p, params::quality),
                                                        juce::StringArray { "Buena", "Mejor", juce::String::fromUTF8 ("Máxima") }, segmentStyle());
    quality->setSegmentWidths ({ 66.0f, 62.0f, 74.0f });
    addAndMakeVisible (*quality);

    intensity = std::make_unique<ferb::SegmentedControl> (get (p, params::intensity),
                                                          juce::StringArray { "Sutil", "Media", "Agresiva" }, segmentStyle());
    intensity->setSegmentWidths ({ 56.0f, 64.0f, 80.0f });
    addAndMakeVisible (*intensity);
}

void GlobalPanel::resized()
{
    const float centres[] = { 67.0f, 167.0f, 267.0f };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        knobs[i].slider.setBounds (juce::Rectangle<int> (66, 66).withCentre ({ int (centres[i]), 44 }));
        knobs[i].value->setBounds (juce::Rectangle<int> (90, 18).withCentre ({ int (centres[i]), 107 }));
    }
    quality->setBounds (101, 131, 210, 44);
    intensity->setBounds (101, 193, 208, 44);
}

void GlobalPanel::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (b, 14.0f);
    g.setColour (border);
    g.drawRoundedRectangle (b.reduced (0.5f), 14.0f, 1.0f);

    const float centres[] = { 67.0f, 167.0f, 267.0f };
    g.setColour (text);
    g.setFont (ui (12.0f, true).withExtraKerningFactor (0.08f));
    for (size_t i = 0; i < knobs.size(); ++i)
        g.drawText (knobs[i].caption, juce::Rectangle<float> (centres[i] - 45.0f, 77.0f, 90.0f, 20.0f), juce::Justification::centred);

    g.setColour (textDim);
    g.setFont (ui (11.5f, true).withExtraKerningFactor (0.1f));
    g.drawText ("CALIDAD", juce::Rectangle<float> (15.0f, 142.0f, 80.0f, 22.0f), juce::Justification::centredLeft);
    g.drawText ("INTENSIDAD", juce::Rectangle<float> (15.0f, 204.0f, 90.0f, 22.0f), juce::Justification::centredLeft);
}
} // namespace poty::ui
