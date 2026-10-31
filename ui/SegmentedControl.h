#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ferb
{
/** Fila de botones ligada a un parámetro Choice: el elegido va relleno con el color de acento. */
class SegmentedControl : public juce::Component
{
public:
    struct Style
    {
        juce::Colour border, text, activeFill, activeText;
        juce::Font font { juce::FontOptions (13.0f) };
        float corner = 8.0f;
        float gap = 4.0f;
    };

    SegmentedControl (juce::RangedAudioParameter& parameter, juce::StringArray labels, Style s)
        : items (std::move (labels)), style (std::move (s)),
          attachment (parameter, [this] (float v) { selected = juce::roundToInt (v); repaint(); })
    {
        param = &parameter;
        attachment.sendInitialUpdate();
    }

    /** Ancho de cada segmento, para igualar la rejilla de la interfaz. Si no se llama, se reparten por igual. */
    void setSegmentWidths (std::vector<float> widths) { fixedWidths = std::move (widths); repaint(); }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < items.size(); ++i)
        {
            const auto r = segment (i);
            const bool on = i == selected;
            if (on)
            {
                g.setColour (style.activeFill);
                g.fillRoundedRectangle (r, style.corner);
            }
            else
            {
                g.setColour (style.border);
                g.drawRoundedRectangle (r.reduced (0.5f), style.corner, 1.0f);
            }
            g.setColour (on ? style.activeText : style.text);
            g.setFont (style.font);
            g.drawText (items[i], r, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < items.size(); ++i)
            if (segment (i).contains (e.position))
            {
                attachment.setValueAsCompleteGesture (static_cast<float> (i));
                return;
            }
    }

private:
    juce::Rectangle<float> segment (int i) const
    {
        const auto b = getLocalBounds().toFloat();
        float x = 0.0f;
        const float equal = (b.getWidth() - style.gap * static_cast<float> (items.size() - 1)) / static_cast<float> (items.size());
        for (int k = 0; k < i; ++k)
            x += (fixedWidths.empty() ? equal : fixedWidths[static_cast<size_t> (k)]) + style.gap;
        const float w = fixedWidths.empty() ? equal : fixedWidths[static_cast<size_t> (i)];
        return { b.getX() + x, b.getY(), w, b.getHeight() };
    }

    juce::StringArray items;
    Style style;
    std::vector<float> fixedWidths;
    juce::RangedAudioParameter* param = nullptr;
    juce::ParameterAttachment attachment;
    int selected = 0;
};
} // namespace ferb
