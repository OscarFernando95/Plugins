#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

namespace ferb
{
/** Valor de un parámetro como texto: se arrastra en vertical (Mayús = fino), un doble clic permite
    escribirlo, y Alt + clic lo devuelve a su valor por defecto. */
class ParamField : public juce::Label
{
public:
    using Format = std::function<juce::String (float)>;
    using Parse = std::function<float (const juce::String&, float)>;

    struct Style
    {
        juce::Font font { juce::FontOptions (13.0f) };
        juce::Colour text, editBackground, editOutline, editText;
        juce::Justification justification = juce::Justification::centredRight;
    };

    ParamField (juce::RangedAudioParameter& parameter, Format format, Parse parse, const Style& style)
        : param (parameter), formatValue (std::move (format)), parseValue (std::move (parse)),
          attachment (parameter, [this] (float v) { setText (formatValue (v), juce::dontSendNotification); })
    {
        setFont (style.font);
        setColour (juce::Label::textColourId, style.text);
        setColour (juce::Label::textWhenEditingColourId, style.editText);
        setColour (juce::Label::backgroundWhenEditingColourId, style.editBackground);
        setColour (juce::Label::outlineWhenEditingColourId, style.editOutline);
        setJustificationType (style.justification);
        setEditable (false, true, false);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        setBorderSize ({ 0, 0, 0, 2 });
        attachment.sendInitialUpdate();

        onTextChange = [this]
        {
            const float current = param.convertFrom0to1 (param.getValue());
            attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.convertTo0to1 (parseValue (getText(), current))));
        };
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isAltDown())
        {
            attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
            return;
        }
        if (isBeingEdited())
            return Label::mouseDown (e);

        dragStart = param.getValue();
        attachment.beginGesture();
        Label::mouseDown (e);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (isBeingEdited() || e.mods.isAltDown())
            return;
        const float speed = e.mods.isShiftDown() ? 0.0006f : 0.004f;
        const float v = juce::jlimit (0.0f, 1.0f, dragStart - static_cast<float> (e.getDistanceFromDragStartY()) * speed);
        attachment.setValueAsPartOfGesture (param.convertFrom0to1 (v));
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        attachment.endGesture();
        Label::mouseUp (e);
    }

private:
    juce::RangedAudioParameter& param;
    Format formatValue;
    Parse parseValue;
    juce::ParameterAttachment attachment;
    float dragStart = 0.0f;
};
} // namespace ferb
