#include "TubeView.h"

namespace tufi::ui
{
namespace
{
const char* stageNames[] = { "TRIODO", "PENTODO", "DIODO" };
} // namespace

TubeView::TubeView (TufiProcessor& p, std::function<float()> levelSource)
    : processor (p), level (std::move (levelSource))
{
    startTimerHz (30);
}

void TubeView::timerCallback()
{
    stage = juce::jlimit (0, 2, juce::roundToInt (processor.apvts.getRawParameterValue (tufi::params::stage)->load()));
    const float target = 0.45f + 0.55f * juce::jlimit (0.0f, 1.0f, level() * 2.2f);
    glow += (target - glow) * 0.18f;
    repaint();
}

void TubeView::paint (juce::Graphics& g)
{
    const auto frame = getLocalBounds().toFloat();
    g.setColour (bakelite.darker (0.4f));
    g.fillRoundedRectangle (frame, 18.0f);

    const float cx = frame.getCentreX();

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (frame.reduced (2.0f), 16.0f);
        g.reduceClipRegion (clip);

        // Resplandor cálido detrás del cristal.
        juce::ColourGradient halo (amber.withAlpha (0.50f * glow), cx, 105.0f, juce::Colour (0x00ffb347), cx + 105.0f, 105.0f, true);
        g.setGradientFill (halo);
        g.fillRect (frame);

        // Cuerpo de vidrio: cúpula arriba, base plana abajo.
        const float topY = 17.0f, bottomY = 160.0f, halfW = 31.0f;
        juce::Path glass;
        glass.startNewSubPath (cx - halfW, bottomY);
        glass.lineTo (cx - halfW, topY + halfW);
        glass.addCentredArc (cx, topY + halfW, halfW, halfW, 0.0f, juce::degreesToRadians (-90.0f), juce::degreesToRadians (90.0f), false);
        glass.clear();
        glass.startNewSubPath (cx - halfW, bottomY);
        glass.lineTo (cx - halfW, topY + halfW);
        glass.cubicTo (cx - halfW, topY - 4.0f, cx + halfW, topY - 4.0f, cx + halfW, topY + halfW);
        glass.lineTo (cx + halfW, bottomY);
        glass.closeSubPath();

        g.setColour (juce::Colour (0xff1a120c).withAlpha (0.55f));
        g.fillPath (glass);
        g.setColour (juce::Colours::white.withAlpha (0.38f));
        g.strokePath (glass, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Brillo vertical del cristal, a la izquierda.
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRoundedRectangle (cx - halfW + 6.0f, topY + 28.0f, 5.0f, 78.0f, 2.5f);

        // Placa, con su perforado, y el filamento encendido.
        const float plateH = stage == 2 ? 52.0f : (stage == 1 ? 84.0f : 74.0f);
        const juce::Rectangle<float> plate (cx - 17.0f, 56.0f + (80.0f - plateH) * 0.5f, 34.0f, plateH);
        g.setColour (juce::Colour (0xff3b3632));
        g.fillRoundedRectangle (plate, 3.0f);
        g.setColour (juce::Colour (0xff6c655d));
        g.drawRoundedRectangle (plate, 3.0f, 1.0f);
        g.setColour (juce::Colour (0xff15110e));
        for (float yy = plate.getY() + 5.0f; yy < plate.getBottom() - 3.0f; yy += 5.0f)
        {
            g.fillEllipse (plate.getX() + 5.0f, yy, 2.4f, 2.4f);
            g.fillEllipse (plate.getRight() - 7.4f, yy, 2.4f, 2.4f);
        }

        // Pentodo: dos rejillas de malla alrededor de la placa.
        if (stage == 1)
        {
            g.setColour (juce::Colour (0xff8a8176).withAlpha (0.8f));
            g.drawRoundedRectangle (plate.expanded (6.0f, 3.0f), 6.0f, 1.0f);
            g.drawRoundedRectangle (plate.expanded (11.0f, 6.0f), 9.0f, 1.0f);
        }

        const juce::Rectangle<float> filament (cx - 2.5f, plate.getY() + 6.0f, 5.0f, plateH - 12.0f);
        juce::ColourGradient fil (amber.brighter (0.4f), cx, filament.getY(), amber.darker (0.1f), cx, filament.getBottom(), false);
        g.setColour (amber.withAlpha (0.35f * glow));
        g.fillRoundedRectangle (filament.expanded (7.0f), 8.0f);
        g.setGradientFill (fil);
        g.fillRoundedRectangle (filament, 2.5f);
        g.setColour (juce::Colours::white.withAlpha (0.6f * glow));
        g.fillRoundedRectangle (filament.reduced (1.6f, 4.0f), 1.0f);

        // Getter plateado arriba.
        g.setColour (juce::Colour (0xffd8d4cc).withAlpha (0.85f));
        g.fillEllipse (cx - 4.0f, 20.0f, 8.0f, 8.0f);

        // Zócalo y patillas.
        g.setColour (juce::Colour (0xff2a2520));
        g.fillRoundedRectangle (cx - 27.0f, bottomY, 54.0f, 14.0f, 2.0f);
        g.setColour (juce::Colour (0xff6a6259));
        g.drawRoundedRectangle (cx - 27.0f, bottomY, 54.0f, 14.0f, 2.0f, 1.0f);
        g.setColour (juce::Colour (0xffb0a898));
        for (float dx : { -19.0f, -6.5f, 6.5f, 19.0f })
            g.fillRect (cx + dx - 1.5f, bottomY + 14.0f, 3.0f, 18.0f);
    }

    g.setColour (brass);
    g.drawRoundedRectangle (frame.reduced (1.0f), 18.0f, 2.0f);

    g.setColour (cream);
    g.setFont (label (13.0f, 0.2f));
    g.drawText (juce::String::fromUTF8 ("ETAPA ACTIVA · ") + stageNames[stage],
                juce::Rectangle<float> (0.0f, frame.getBottom() - 42.0f, frame.getWidth(), 24.0f), juce::Justification::centred);
}
} // namespace tufi::ui
