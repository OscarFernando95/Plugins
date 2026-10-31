#include "MaruUi.h"

using namespace ferb::vintage;

namespace maru::ui
{
namespace
{
const juce::Colour cream { 0xfff3ecd9 };
const juce::Colour pale { 0xffd7efc9 };
const juce::Colour amber { 0xffffb347 };
} // namespace

void FloorView::paintContent (juce::Graphics& g, float w, float h)
{
    const auto y = [h] (float db) { return h - std::clamp ((db + 80.0f) / 70.0f, 0.0f, 1.0f) * (h - 8.0f) - 4.0f; };
    const float bw = w / static_cast<float> (bands);

    for (int i = 0; i < bands; ++i)
    {
        const auto idx = static_cast<size_t> (i);
        const float x = static_cast<float> (i) * bw + 1.0f, yIn = y (data.input[idx]), yOut = y (data.output[idx]);
        g.setColour (cream.withAlpha (0.22f));
        g.fillRect (x, yIn, bw - 2.0f, h - yIn);
        if (data.listening)
        {
            g.setColour (amber.withAlpha (0.9f));
            g.fillRect (x, yIn, bw - 2.0f, std::max (1.0f, yOut - yIn));
        }
        else
        {
            g.setColour (pale.withAlpha (0.85f));
            g.fillRect (x, yOut, bw - 2.0f, h - yOut);
        }
    }

    // El piso por banda, discontinuo.
    juce::Path floor;
    for (int i = 0; i < bands; ++i)
    {
        const float yy = y (data.noise[static_cast<size_t> (i)]);
        i == 0 ? floor.startNewSubPath (0.0f, yy) : floor.lineTo (static_cast<float> (i) * bw, yy);
        floor.lineTo (static_cast<float> (i + 1) * bw, yy);
    }
    juce::Path dashed;
    const float dashes[] = { 4.0f, 3.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, floor, dashes, 2);
    g.setColour (amber);
    g.fillPath (dashed);

    g.setFont (digitFont (10.0f));
    g.setColour (cream.withAlpha (0.7f));
    g.drawText ("entrada", juce::Rectangle<float> (6.0f, 3.0f, 70.0f, 12.0f), juce::Justification::centredLeft, false);
    g.setColour (pale.withAlpha (0.95f));
    g.drawText (data.listening ? "quitado" : "salida", juce::Rectangle<float> (6.0f, 15.0f, 70.0f, 12.0f), juce::Justification::centredLeft, false);
}

void JewelLamp::set (bool lit, const juce::String& text)
{
    if (lit == on && text == caption)
        return;
    on = lit;
    caption = text;
    repaint();
}

void JewelLamp::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const auto jewel = juce::Rectangle<float> (16.0f, 16.0f).withCentre ({ 14.0f, r.getCentreY() });
    if (on)
    {
        g.setColour (amber.withAlpha (0.35f));
        g.fillEllipse (jewel.expanded (5.0f));
    }
    g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xffffd9a0) : juce::Colour (0xff5a4a30), jewel.getX() + 5.0f, jewel.getY() + 4.0f,
                                             on ? juce::Colour (0xffe07a10) : juce::Colour (0xff1f160b), jewel.getRight(), jewel.getBottom(), true));
    g.fillEllipse (jewel);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawEllipse (jewel, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (on ? 0.7f : 0.25f));
    g.fillEllipse (jewel.getX() + 3.5f, jewel.getY() + 2.5f, 4.5f, 3.0f);
    drawEmbossed (g, caption, r.withTrimmedLeft (30.0f), labelFont (11.0f, 1.2f), juce::Justification::centredLeft, palette);
}
} // namespace maru::ui
