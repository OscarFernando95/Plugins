#pragma once

#include <functional>

#include "../PluginProcessor.h"
#include "ParamField.h"
#include "Style.h"

namespace poty::ui
{
/** Barra de drive: se arrastra en horizontal. */
class DriveBar : public juce::Component
{
public:
    DriveBar (juce::RangedAudioParameter& parameter, juce::Colour colour)
        : param (parameter), tint (colour),
          attachment (parameter, [this] (float) { repaint(); })
    {
        setMouseCursor (juce::MouseCursor::LeftRightResizeCursor);
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        const auto bar = getLocalBounds().toFloat().withSizeKeepingCentre (static_cast<float> (getWidth()), 6.0f);
        g.setColour (border);
        g.fillRoundedRectangle (bar, 3.0f);
        g.setColour (tint);
        g.fillRoundedRectangle (bar.withWidth (juce::jmax (6.0f, bar.getWidth() * param.getValue())), 3.0f);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isAltDown())
        {
            attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
            return;
        }
        attachment.beginGesture();
        set (e);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (! e.mods.isAltDown()) set (e); }
    void mouseUp (const juce::MouseEvent&) override { attachment.endGesture(); }

private:
    void set (const juce::MouseEvent& e)
    {
        const float v = juce::jlimit (0.0f, 1.0f, e.position.x / static_cast<float> (getWidth()));
        attachment.setValueAsPartOfGesture (param.convertFrom0to1 (v));
    }

    juce::RangedAudioParameter& param;
    juce::Colour tint;
    juce::ParameterAttachment attachment;
};

/** Tarjeta de una banda: encendido, frecuencia, ganancia, Q, drive y tipo de saturación. */
class BandCard : public juce::Component
{
public:
    BandCard (PotyProcessor& p, int bandIndex, const int& selectedBand, std::function<void (int)> onSelect);

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    PotyProcessor& processor;
    int band;
    const int& selected;
    std::function<void (int)> select;

    juce::ParameterAttachment onAttachment;
    bool isOn = true;

    ParamField freq, gain, q;
    DriveBar drive;
    juce::ComboBox type;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
};
} // namespace poty::ui
