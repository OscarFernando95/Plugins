#include "BandView.h"

namespace cremita::ui
{
using namespace ferb::vintage;
namespace k = cremita::params;

namespace
{
constexpr float handleGrab = 9.0f;
constexpr float trimPixels = 55.0f; // alto del recorrido de ±6 dB, a cada lado del cero

const char* const names[] = { "GRAVES", "MEDIOS", "AGUDOS" };

juce::String dbText (float v)
{
    const auto s = ferb::fmt::comma (std::abs (v), 1);
    return (v > 0.05f ? "+" : (v < -0.05f ? juce::String::fromUTF8 ("−") : "")) + s + " dB";
}

juce::String hzText (float hz)
{
    return hz >= 1000.0f ? ferb::fmt::comma (hz / 1000.0, 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
}
} // namespace

float BandView::frequencyToX (float hz, float width) noexcept
{
    return width * std::log (hz / lowestHz) / std::log (highestHz / lowestHz);
}

float BandView::xToFrequency (float x, float width) noexcept
{
    return lowestHz * std::pow (highestHz / lowestHz, x / width);
}

BandView::BandView (CremitaProcessor& p, const Palette& pal) : GlassView (pal), processor (p)
{
    setInterceptsMouseClicks (true, false);
    setTitle ("Editor de bandas");
    const char* ids[] = { k::xo1, k::xo2, k::trimLF, k::trimMF, k::trimHF };
    for (size_t i = 0; i < attachments.size(); ++i)
        attachments[i] = std::make_unique<juce::ParameterAttachment> (*dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (ids[i])),
                                                                      [this] (float) { repaint(); }, &p.undoManager);
}

juce::RangedAudioParameter& BandView::paramFor (Target t) const
{
    const char* id = t == Target::crossover1 ? k::xo1 : t == Target::crossover2 ? k::xo2
                   : t == Target::bandLow ? k::trimLF : t == Target::bandMid ? k::trimMF : k::trimHF;
    return *dynamic_cast<juce::RangedAudioParameter*> (processor.apvts.getParameter (id));
}

juce::ParameterAttachment& BandView::attachmentFor (Target t)
{
    return *attachments[static_cast<size_t> (t) - 1];
}

float BandView::valueOf (Target t) const
{
    return processor.apvts.getRawParameterValue (paramFor (t).paramID)->load();
}

BandView::Target BandView::targetAt (juce::Point<float> p) const
{
    const float w = widthInside();
    const float x = p.x - static_cast<float> (margin);
    const float x1 = frequencyToX (valueOf (Target::crossover1), w), x2 = frequencyToX (valueOf (Target::crossover2), w);

    if (std::abs (x - x1) <= handleGrab && std::abs (x - x1) <= std::abs (x - x2))
        return Target::crossover1;
    if (std::abs (x - x2) <= handleGrab)
        return Target::crossover2;
    return x < x1 ? Target::bandLow : (x < x2 ? Target::bandMid : Target::bandHigh);
}

void BandView::mouseDown (const juce::MouseEvent& e)
{
    dragging = targetAt (e.position);
    dragStartValue = paramFor (dragging).convertTo0to1 (valueOf (dragging));
    attachmentFor (dragging).beginGesture();
}

void BandView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging == Target::none)
        return;

    auto& param = paramFor (dragging);
    const float fine = e.mods.isShiftDown() ? 0.25f : 1.0f;
    float normalised;
    if (dragging == Target::crossover1 || dragging == Target::crossover2)
    {
        // El cursor lleva la frecuencia; con Mayús el movimiento se frena.
        const float w = widthInside();
        const float startX = frequencyToX (param.convertFrom0to1 (dragStartValue), w);
        const float x = startX + static_cast<float> (e.getDistanceFromDragStartX()) * fine;
        normalised = param.convertTo0to1 (xToFrequency (juce::jlimit (0.0f, w, x), w));
    }
    else
    {
        normalised = dragStartValue - static_cast<float> (e.getDistanceFromDragStartY()) * fine / (2.0f * trimPixels);
    }
    attachmentFor (dragging).setValueAsPartOfGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, normalised)));
}

void BandView::mouseUp (const juce::MouseEvent&)
{
    if (dragging != Target::none)
        attachmentFor (dragging).endGesture();
    dragging = Target::none;
}

void BandView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const auto t = targetAt (e.position);
    auto& param = paramFor (t);
    attachmentFor (t).setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
}

void BandView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& d)
{
    if (d.deltaY == 0.0f)
        return;
    const auto t = targetAt (e.position);
    auto& param = paramFor (t);
    const float step = (d.deltaY > 0.0f ? 1.0f : -1.0f) * (e.mods.isShiftDown() ? 0.005f : 0.02f);
    attachmentFor (t).setValueAsCompleteGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, param.convertTo0to1 (valueOf (t)) + step)));
}

void BandView::paintContent (juce::Graphics& g, float width, float height)
{
    const float xo[2] = { valueOf (Target::crossover1), valueOf (Target::crossover2) };
    const float trims[3] = { valueOf (Target::bandLow), valueOf (Target::bandMid), valueOf (Target::bandHigh) };
    const float edges[4] = { 0.0f, frequencyToX (xo[0], width), frequencyToX (xo[1], width), width };
    const float zeroY = 84.0f;
    const auto cream = juce::Colour (0xffefe5cf);

    // Rejilla logarítmica: 100 Hz, 1 kHz y 10 kHz.
    g.setFont (digitFont (9.0f));
    for (const auto& [hz, text] : { std::pair<float, const char*> { 100.0f, "100" }, { 1000.0f, "1k" }, { 10000.0f, "10k" } })
    {
        const float x = frequencyToX (hz, width);
        g.setColour (cream.withAlpha (0.10f));
        g.drawLine (x, 6.0f, x, height - 18.0f);
        g.setColour (cream.withAlpha (0.45f));
        g.drawText (text, juce::Rectangle<float> (x - 20.0f, height - 16.0f, 40.0f, 12.0f), juce::Justification::centred);
    }

    for (int b = 0; b < 3; ++b)
    {
        const float x0 = edges[b], x1 = edges[b + 1];
        const float centre = (x0 + x1) * 0.5f;
        const float trim = trims[b];

        g.setColour (palette.acc.withAlpha (b == 1 ? 0.07f : 0.04f));
        g.fillRect (x0, 0.0f, x1 - x0, height);

        // Barra del peso de la banda, desde el cero.
        const float barY = zeroY - trim / trimRangeDb * trimPixels;
        const auto bar = juce::Rectangle<float> (x0 + 6.0f, std::min (barY, zeroY), std::max (4.0f, x1 - x0 - 12.0f), std::max (2.0f, std::abs (barY - zeroY)));
        g.setColour (palette.acc.withAlpha (0.55f));
        g.fillRoundedRectangle (bar, 2.0f);
        g.setColour (palette.acc);
        g.drawLine (bar.getX(), barY, bar.getRight(), barY, 2.0f);

        g.setColour (cream.withAlpha (0.8f));
        g.setFont (labelFont (9.0f, 1.4f));
        g.drawText (names[b], juce::Rectangle<float> (x0, 8.0f, x1 - x0, 12.0f), juce::Justification::centred);
        g.setColour (palette.acc);
        g.setFont (digitFont (12.0f));
        g.drawText (dbText (trim), juce::Rectangle<float> (centre - 40.0f, 22.0f, 80.0f, 14.0f), juce::Justification::centred);
    }

    // Línea de 0 dB.
    juce::Path zero;
    zero.startNewSubPath (0.0f, zeroY);
    zero.lineTo (width, zeroY);
    juce::Path dashed;
    const float pattern[] = { 3.0f, 3.0f };
    juce::PathStrokeType (1.0f).createDashedStroke (dashed, zero, pattern, 2);
    g.setColour (cream.withAlpha (0.35f));
    g.fillPath (dashed);

    // Cruces con su asa y su frecuencia.
    for (int i = 0; i < 2; ++i)
    {
        const float x = edges[i + 1];
        const bool active = dragging == (i == 0 ? Target::crossover1 : Target::crossover2);
        g.setColour (cream.withAlpha (active ? 1.0f : 0.8f));
        g.drawLine (x, 0.0f, x, height - 18.0f, 2.0f);
        g.fillRoundedRectangle (x - 5.0f, height - 54.0f, 10.0f, 26.0f, 3.0f);
        g.setColour (palette.glass);
        g.drawLine (x - 1.5f, height - 48.0f, x - 1.5f, height - 34.0f, 1.0f);
        g.drawLine (x + 1.5f, height - 48.0f, x + 1.5f, height - 34.0f, 1.0f);

        g.setColour (cream.withAlpha (0.9f));
        g.setFont (digitFont (10.0f));
        const bool left = x > width - 60.0f;
        g.drawText (hzText (xo[i]), juce::Rectangle<float> (left ? x - 66.0f : x + 8.0f, height - 52.0f, 60.0f, 12.0f),
                    left ? juce::Justification::centredRight : juce::Justification::centredLeft);
    }
}
} // namespace cremita::ui
