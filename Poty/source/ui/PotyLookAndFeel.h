#pragma once

#include "Style.h"

namespace poty::ui
{
class PotyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PotyLookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, panel);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::headerTextColourId, textDim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.18f));
        setColour (juce::PopupMenu::highlightedTextColourId, text);
        setColour (juce::AlertWindow::backgroundColourId, panel);
        setColour (juce::AlertWindow::textColourId, text);
        setColour (juce::AlertWindow::outlineColourId, border);
        setColour (juce::TextEditor::backgroundColourId, bg);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::outlineColourId, border);
        setColour (juce::TextEditor::focusedOutlineColourId, accent);
        setColour (juce::TextButton::buttonColourId, bg);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::ComboBox::textColourId, text);
    }

    // Perilla: pista y arco de valor en el color de acento, indicador blanco.
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float startAngle,
                           float endAngle, juce::Slider&) override
    {
        const auto bounds = juce::Rectangle<float> (float (x), float (y), float (w), float (h)).reduced (3.0f);
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);

        juce::Path track, value;
        track.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, endAngle, true);
        value.addCentredArc (c.x, c.y, r, r, 0.0f, startAngle, angle, true);

        g.setColour (border);
        g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (value, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float inner = r - 7.0f;
        g.setColour (juce::Colour (0xff10151c));
        g.fillEllipse (c.x - inner, c.y - inner, inner * 2.0f, inner * 2.0f);
        g.setColour (border);
        g.drawEllipse (c.x - inner, c.y - inner, inner * 2.0f, inner * 2.0f, 1.0f);

        juce::Path pointer;
        pointer.startNewSubPath (0.0f, -inner * 0.25f);
        pointer.lineTo (0.0f, -inner * 0.92f);
        g.setColour (text);
        g.strokePath (pointer, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }

    // Selector de tipo: borde fino, texto en negrita y flecha.
    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override
    {
        const auto b = juce::Rectangle<float> (0.0f, 0.0f, float (width), float (height));
        g.setColour (bg.withAlpha (0.55f));
        g.fillRoundedRectangle (b, 8.0f);
        g.setColour (box.hasKeyboardFocus (true) ? accent : border);
        g.drawRoundedRectangle (b.reduced (0.5f), 8.0f, 1.0f);

        juce::Path chevron;
        const float cx = float (width) - 16.0f, cy = float (height) * 0.5f;
        chevron.startNewSubPath (cx - 4.0f, cy - 2.0f);
        chevron.lineTo (cx, cy + 2.0f);
        chevron.lineTo (cx + 4.0f, cy - 2.0f);
        g.setColour (textDim);
        g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return ui (13.0f, true); }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (11, 1, box.getWidth() - 32, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    juce::Font getPopupMenuFont() override { return ui (13.0f); }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (panel);
        g.setColour (border);
        g.drawRect (0, 0, width, height, 1);
    }

    // Botón de bypass: borde fino; encendido, color de acento.
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool) override
    {
        const auto r = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        g.setColour (on ? accent.withAlpha (0.16f) : (over ? panel.brighter (0.05f) : bg.withAlpha (0.55f)));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (on ? accent : border);
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
        g.setColour (on ? accent : text);
        g.setFont (ui (12.0f, true).withExtraKerningFactor (0.08f));
        g.drawText (b.getButtonText(), r, juce::Justification::centred);
    }
};
} // namespace poty::ui
