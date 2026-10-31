#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

#include "Style.h"

namespace tufi::ui
{
/** Tecla de baquelita con lámpara piloto. Tres usos: elegir un valor de un Choice (Radio), recorrer
    sus valores (Cycle) o encender y apagar un parámetro Bool (Toggle). */
class KeyButton : public juce::Component
{
public:
    enum class Kind { Radio, Cycle, Toggle };

    static constexpr int lampHeight = 16;

    /** `label` devuelve el texto de la tecla para el valor actual (índice del Choice, o 0/1 del Bool). */
    KeyButton (juce::RangedAudioParameter& parameter, Kind k, int index, std::function<juce::String (int)> label)
        : param (parameter), kind (k), radioIndex (index), labelFor (std::move (label)),
          attachment (parameter, [this] (float v) { value = juce::roundToInt (v); repaint(); })
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        attachment.sendInitialUpdate();
    }

    bool isLit() const noexcept
    {
        switch (kind)
        {
            case Kind::Radio:  return value == radioIndex;
            case Kind::Cycle:  return value != 0;
            case Kind::Toggle: return value != 0;
        }
        return false;
    }

    void paint (juce::Graphics& g) override
    {
        const bool lit = isLit();
        const bool pressed = kind == Kind::Radio ? lit : false;
        auto b = getLocalBounds().toFloat();

        // Lámpara piloto.
        const juce::Point<float> lamp (b.getCentreX(), 6.0f);
        if (lit)
        {
            juce::ColourGradient glow (amber.withAlpha (0.55f), lamp.x, lamp.y, amber.withAlpha (0.0f), lamp.x + 12.0f, lamp.y, true);
            g.setGradientFill (glow);
            g.fillEllipse (lamp.x - 12.0f, lamp.y - 12.0f, 24.0f, 24.0f);
        }
        g.setColour (lit ? amber : juce::Colour (0xff4a3b2d));
        g.fillEllipse (lamp.x - 4.5f, lamp.y - 4.5f, 9.0f, 9.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (lamp.x - 4.5f, lamp.y - 4.5f, 9.0f, 9.0f, 1.0f);

        // Tecla: zócalo, cara y texto. La pulsada se hunde un par de píxeles.
        auto key = b.withTrimmedTop (float (lampHeight)).withTrimmedBottom (3.0f);
        g.setColour (juce::Colour (0xff070403));
        g.fillRoundedRectangle (key.translated (0.0f, 3.0f), 8.0f);

        if (pressed)
            key = key.translated (0.0f, 2.0f);

        juce::ColourGradient face (pressed ? juce::Colour (0xff4b392a) : juce::Colour (0xff2b2219), key.getX(), key.getY(),
                                   pressed ? juce::Colour (0xff2e2217) : juce::Colour (0xff120d0a), key.getX(), key.getBottom(), false);
        g.setGradientFill (face);
        g.fillRoundedRectangle (key, 8.0f);
        g.setColour (juce::Colour (0xff6d563d).withAlpha (pressed ? 0.5f : 0.35f));
        g.drawRoundedRectangle (key.reduced (1.0f), 7.0f, 1.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (key.reduced (0.5f), 8.0f, 1.0f);

        g.setColour (cream);
        g.setFont (label (14.0f, 0.18f));
        g.drawText (labelFor (value), key, juce::Justification::centred);
    }

    void mouseDown (const juce::MouseEvent&) override
    {
        const int count = param.getNumSteps();
        switch (kind)
        {
            case Kind::Radio:  attachment.setValueAsCompleteGesture (float (radioIndex)); break;
            case Kind::Cycle:  attachment.setValueAsCompleteGesture (float ((value + 1) % count)); break;
            case Kind::Toggle: attachment.setValueAsCompleteGesture (value != 0 ? 0.0f : 1.0f); break;
        }
    }

private:
    juce::RangedAudioParameter& param;
    Kind kind;
    int radioIndex;
    std::function<juce::String (int)> labelFor;
    juce::ParameterAttachment attachment;
    int value = 0;
};
} // namespace tufi::ui
