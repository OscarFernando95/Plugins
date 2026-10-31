#include "CurveView.h"

namespace cremita::ui
{
using namespace ferb::vintage;

CurveView::CurveView (CremitaProcessor& p, const Palette& pal) : GlassView (pal), processor (p)
{
    setTitle ("Curva de transferencia");
}

void CurveView::paintContent (juce::Graphics& g, float width, float height)
{
    namespace k = cremita::params;
    const auto raw = [this] (const char* id) { return processor.apvts.getRawParameterValue (id)->load(); };
    const auto cp = CurveParams::make (raw (k::curve), raw (k::bias), true);
    const double effect = raw (k::effect) / 100.0;
    const bool inOn = raw (k::inOn) >= 0.5f && raw (k::bypass) < 0.5f;

    constexpr float size = 130.0f;
    const float x0 = (width - size) * 0.5f, y0 = (height - size) * 0.5f;
    const auto X = [&] (double x) { return x0 + static_cast<float> ((x + 1.0) / 2.0) * size; };
    const auto Y = [&] (double y) { return y0 + static_cast<float> (1.0 - (y + 1.0) / 2.0) * size; };

    g.setColour (juce::Colour (0xffefe5cf).withAlpha (0.14f));
    g.drawLine (X (-1), Y (0), X (1), Y (0));
    g.drawLine (X (0), Y (-1), X (0), Y (1));

    juce::Path diagonal;
    diagonal.startNewSubPath (X (-1), Y (-1));
    diagonal.lineTo (X (1), Y (1));
    juce::Path dashed;
    const float pattern[] = { 3.0f, 3.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, diagonal, pattern, 2);
    g.setColour (juce::Colour (0xffefe5cf).withAlpha (0.4f));
    g.fillPath (dashed);

    const auto transfer = [&] (double x) { return inOn ? (1.0 - effect) * x + effect * curveF (cp, x) : x; };

    juce::Path curve;
    for (int i = 0; i <= 80; ++i)
    {
        const double x = -1.0 + i / 40.0;
        const auto point = juce::Point<float> (X (x), Y (transfer (x)));
        i == 0 ? curve.startNewSubPath (point) : curve.lineTo (point);
    }
    g.setColour (palette.lamp);
    g.strokePath (curve, juce::PathStrokeType (2.2f));

    const double wx = juce::jlimit (0.0, 1.0, static_cast<double> (working));
    g.setColour (juce::Colours::white);
    g.fillEllipse (juce::Rectangle<float> (7.2f, 7.2f).withCentre ({ X (wx), Y (transfer (wx)) }));

    g.setColour (juce::Colour (0xffefe5cf).withAlpha (0.75f));
    g.setFont (digitFont (11.0f));
    g.drawText ("k  " + ferb::fmt::comma (cp.k, 2), juce::Rectangle<float> (16.0f, 12.0f, 80.0f, 14.0f), juce::Justification::centredLeft);
    g.drawText ("b  " + ferb::fmt::comma (cp.b, 2), juce::Rectangle<float> (16.0f, 28.0f, 80.0f, 14.0f), juce::Justification::centredLeft);
    g.drawText (juce::String::fromUTF8 ("entrada →"), juce::Rectangle<float> (width - 116.0f, height - 22.0f, 100.0f, 14.0f), juce::Justification::centredRight);
    g.drawText (juce::String::fromUTF8 ("↑ salida"), juce::Rectangle<float> (width - 116.0f, 12.0f, 100.0f, 14.0f), juce::Justification::centredRight);
}
} // namespace cremita::ui
