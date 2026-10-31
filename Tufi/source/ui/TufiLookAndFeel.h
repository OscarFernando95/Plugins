#pragma once

#include <numbers>

#include "Style.h"

namespace tufi::ui
{
class TufiLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TufiLookAndFeel()
    {
        setColour (juce::PopupMenu::backgroundColourId, ivory);
        setColour (juce::PopupMenu::textColourId, ink);
        setColour (juce::PopupMenu::headerTextColourId, inkSoft);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, brass.withAlpha (0.35f));
        setColour (juce::PopupMenu::highlightedTextColourId, ink);
        setColour (juce::AlertWindow::backgroundColourId, ivory);
        setColour (juce::AlertWindow::textColourId, ink);
        setColour (juce::AlertWindow::outlineColourId, brass);
        setColour (juce::TextEditor::backgroundColourId, ivory.brighter (0.25f));
        setColour (juce::TextEditor::textColourId, ink);
        setColour (juce::TextEditor::outlineColourId, brass);
        setColour (juce::TextEditor::focusedOutlineColourId, ink);
        setColour (juce::TextButton::buttonColourId, bakelite);
        setColour (juce::TextButton::textColourOffId, cream);
    }

    juce::Font getPopupMenuFont() override { return label (14.0f, 0.04f); }

    void drawPopupMenuBackground (juce::Graphics& g, int width, int height) override
    {
        g.fillAll (ivory);
        g.setColour (brass);
        g.drawRect (0, 0, width, height, 1);
    }

    /** Perilla: marcas grabadas alrededor, cuerpo de baquelita moleteado y puntero crema.
        El diámetro de la perilla es el ancho del control, marcas incluidas. */
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float startAngle,
                           float endAngle, juce::Slider&) override
    {
        const auto area = juce::Rectangle<float> (float (x), float (y), float (w), float (h));
        const float d = juce::jmin (area.getWidth(), area.getHeight());
        const auto c = area.getCentre();
        const float angle = startAngle + pos * (endAngle - startAngle);

        // Once marcas grabadas.
        const float rTick = d * 0.5f - 1.0f;
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + float (i) * (endAngle - startAngle) / 10.0f;
            const float len = (i == 0 || i == 10) ? d * 0.13f : d * 0.09f;
            const juce::Point<float> p1 (c.x + std::sin (a) * (rTick - len), c.y - std::cos (a) * (rTick - len));
            const juce::Point<float> p2 (c.x + std::sin (a) * rTick, c.y - std::cos (a) * rTick);
            g.setColour (ink);
            g.drawLine (juce::Line<float> (p1, p2), juce::jmax (1.2f, d * 0.018f));
        }

        const float rb = d * 0.34f;

        // Sombra y cuerpo.
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.fillEllipse (c.x - rb, c.y - rb + d * 0.03f, rb * 2.0f, rb * 2.0f);

        juce::ColourGradient body (juce::Colour (0xff3f3228), c.x - rb * 0.4f, c.y - rb * 0.5f,
                                   juce::Colour (0xff0f0a07), c.x + rb * 0.6f, c.y + rb * 0.8f, true);
        g.setGradientFill (body);
        g.fillEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f);

        // Moleteado del borde.
        const int knurls = juce::jmax (36, int (d * 0.6f));
        g.setColour (juce::Colour (0xff7a6650).withAlpha (0.55f));
        for (int i = 0; i < knurls; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * float (i) / float (knurls);
            g.drawLine (c.x + std::sin (a) * (rb - d * 0.035f), c.y - std::cos (a) * (rb - d * 0.035f),
                        c.x + std::sin (a) * rb, c.y - std::cos (a) * rb, 1.0f);
        }
        g.setColour (juce::Colour (0xff050302));
        g.drawEllipse (c.x - rb, c.y - rb, rb * 2.0f, rb * 2.0f, 1.2f);

        // Disco interior y su aro.
        const float ri = rb * 0.68f;
        juce::ColourGradient inner (juce::Colour (0xff2a2119), c.x - ri * 0.3f, c.y - ri * 0.5f,
                                    juce::Colour (0xff130d09), c.x + ri * 0.5f, c.y + ri * 0.7f, true);
        g.setGradientFill (inner);
        g.fillEllipse (c.x - ri, c.y - ri, ri * 2.0f, ri * 2.0f);
        g.setColour (juce::Colour (0xff4a3a2c));
        g.drawEllipse (c.x - ri, c.y - ri, ri * 2.0f, ri * 2.0f, 1.0f);

        // Puntero crema.
        juce::Path pointer;
        pointer.startNewSubPath (0.0f, -ri * 0.2f);
        pointer.lineTo (0.0f, -rb * 0.96f);
        g.setColour (cream);
        g.strokePath (pointer, juce::PathStrokeType (juce::jmax (2.2f, d * 0.055f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                      juce::AffineTransform::rotation (angle).translated (c.x, c.y));
    }
};
} // namespace tufi::ui
