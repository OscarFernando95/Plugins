#pragma once

#include <ParamField.h>

#include "Style.h"

namespace poty::ui
{
/** El campo de valor genérico con el estilo de Poty (JetBrains Mono en negrita). */
class ParamField : public ferb::ParamField
{
public:
    ParamField (juce::RangedAudioParameter& parameter, Format format, Parse parse, juce::Colour colour = text)
        : ferb::ParamField (parameter, std::move (format), std::move (parse), makeStyle (colour))
    {
    }

private:
    static Style makeStyle (juce::Colour colour)
    {
        Style s;
        s.font = mono (13.0f, true);
        s.text = colour;
        s.editText = text;
        s.editBackground = bg;
        s.editOutline = accent;
        return s;
    }
};
} // namespace poty::ui
