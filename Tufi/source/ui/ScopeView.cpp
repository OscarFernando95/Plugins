#include "ScopeView.h"

namespace tufi::ui
{
namespace
{
juce::Rectangle<float> screenOf (juce::Rectangle<float> frame) { return frame.reduced (14.0f); }
} // namespace

ScopeView::ScopeView (TufiProcessor& p) : processor (p)
{
    recompute();
    startTimerHz (30);
}

void ScopeView::timerCallback()
{
    const auto now = tufi::ShapeParams::fromControls (
        juce::roundToInt (processor.apvts.getRawParameterValue (tufi::params::stage)->load()),
        processor.apvts.getRawParameterValue (tufi::params::lowAmount)->load(),
        processor.apvts.getRawParameterValue (tufi::params::peakAmount)->load(),
        processor.apvts.getRawParameterValue (tufi::params::peakHardness)->load(),
        processor.apvts.getRawParameterValue (tufi::params::evenTop)->load(),
        processor.apvts.getRawParameterValue (tufi::params::evenBottom)->load());

    if (! havePath || ! (now == params))
        recompute();
}

void ScopeView::recompute()
{
    params = tufi::ShapeParams::fromControls (
        juce::roundToInt (processor.apvts.getRawParameterValue (tufi::params::stage)->load()),
        processor.apvts.getRawParameterValue (tufi::params::lowAmount)->load(),
        processor.apvts.getRawParameterValue (tufi::params::peakAmount)->load(),
        processor.apvts.getRawParameterValue (tufi::params::peakHardness)->load(),
        processor.apvts.getRawParameterValue (tufi::params::evenTop)->load(),
        processor.apvts.getRawParameterValue (tufi::params::evenBottom)->load());
    tufi::ShapeTableBuilder::build (params, result);
    havePath = true;

    const auto screen = screenOf ({ 0.0f, 0.0f, float (frameSize), float (frameSize) }).reduced (20.0f);
    curvePath.clear();
    for (int i = 0; i < numPoints; ++i)
    {
        const double x = -range + 2.0 * range * i / (numPoints - 1);
        const double y = juce::jlimit (-range, range, tufi::lookup (result.y, x));
        const float px = screen.getX() + screen.getWidth() * float ((x + range) / (2.0 * range));
        const float py = screen.getBottom() - screen.getHeight() * float ((y + range) / (2.0 * range));
        i == 0 ? curvePath.startNewSubPath (px, py) : curvePath.lineTo (px, py);
    }
    repaint();
}

void ScopeView::drawScreen (juce::Graphics& g, juce::Rectangle<float> screen)
{
    g.setColour (screenGlass);
    g.fillRoundedRectangle (screen, 16.0f);

    const auto grid = screen.reduced (20.0f);

    // Rejilla: 4 × 4 divisiones, con la cruz central algo más viva.
    for (int i = 0; i <= 4; ++i)
    {
        const bool centre = i == 2;
        g.setColour (phosphor.withAlpha (centre ? 0.34f : 0.16f));
        const float x = grid.getX() + grid.getWidth() * float (i) / 4.0f;
        const float y = grid.getY() + grid.getHeight() * float (i) / 4.0f;
        g.drawLine (x, grid.getY(), x, grid.getBottom(), 1.0f);
        g.drawLine (grid.getX(), y, grid.getRight(), y, 1.0f);
    }

    // Recta de referencia (entrada = salida).
    const float dash[] = { 3.0f, 3.0f };
    g.setColour (phosphor.withAlpha (0.28f));
    g.drawDashedLine (juce::Line<float> (grid.getX(), grid.getBottom(), grid.getRight(), grid.getY()), dash, 2, 1.0f);

    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (grid.expanded (2.0f).toNearestInt());

        // Trazo ancho translúcido (halo) y trazo fino encima.
        g.setColour (phosphor.withAlpha (0.18f));
        g.strokePath (curvePath, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (phosphor.withAlpha (0.35f));
        g.strokePath (curvePath, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (phosphor);
        g.strokePath (curvePath, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Marcas en −1 y +1 (0 dBFS de entrada).
    for (double x : { -1.0, 1.0 })
    {
        const double y = tufi::lookup (result.y, x);
        const float px = grid.getX() + grid.getWidth() * float ((x + range) / (2.0 * range));
        const float py = grid.getBottom() - grid.getHeight() * float ((y + range) / (2.0 * range));
        g.setColour (screenGlass);
        g.fillEllipse (px - 4.5f, py - 4.5f, 9.0f, 9.0f);
        g.setColour (phosphor);
        g.drawEllipse (px - 4.5f, py - 4.5f, 9.0f, 9.0f, 1.4f);
    }

    // Líneas de barrido.
    g.setColour (juce::Colours::black.withAlpha (0.14f));
    for (float y = screen.getY(); y < screen.getBottom(); y += 3.0f)
        g.drawHorizontalLine (int (y), screen.getX() + 4.0f, screen.getRight() - 4.0f);

    // Reflejo suave en el cristal.
    juce::ColourGradient glare (juce::Colours::white.withAlpha (0.07f), screen.getX(), screen.getY(),
                                juce::Colours::white.withAlpha (0.0f), screen.getCentreX(), screen.getCentreY(), false);
    g.setGradientFill (glare);
    g.fillRoundedRectangle (screen, 16.0f);
}

void ScopeView::drawCounters (juce::Graphics& g)
{
    struct Counter { juce::String value, caption; };
    const Counter counters[] = {
        { comma (result.slope, 2) + juce::String::fromUTF8 ("×"), "PENDIENTE" },
        { (result.ceilingDb < 0 ? juce::String::fromUTF8 ("−") : juce::String ("+")) + juce::String (std::abs (juce::roundToInt (result.ceilingDb))) + " dB", "TECHO" },
        { juce::String (juce::roundToInt (result.asymmetry)) + " %", juce::String::fromUTF8 ("ASIMETRÍA") },
    };
    const float xs[] = { -1.0f, 92.0f, 184.0f }; // relativos al componente (que empieza 1 px antes que el marco)
    const float ws[] = { 85.0f, 84.0f, 85.0f };

    for (int i = 0; i < 3; ++i)
    {
        const juce::Rectangle<float> r (xs[i] + 1.0f, 302.0f, ws[i], 46.0f);
        g.setColour (juce::Colour (0xff0b0806));
        g.fillRoundedRectangle (r, 8.0f);
        g.setColour (brass.withAlpha (0.5f));
        g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);

        g.setColour (amber.withAlpha (0.22f));
        g.setFont (mono (20.0f, true));
        g.drawText (counters[i].value, r, juce::Justification::centred);
        g.setColour (amber);
        g.drawText (counters[i].value, r, juce::Justification::centred);

        g.setColour (ink);
        g.setFont (label (12.0f, 0.12f));
        g.drawText (counters[i].caption, juce::Rectangle<float> (r.getX() - 6.0f, r.getBottom() + 3.0f, r.getWidth() + 12.0f, 18.0f), juce::Justification::centred);
    }
}

void ScopeView::paint (juce::Graphics& g)
{
    const juce::Rectangle<float> frame (0.0f, 0.0f, float (frameSize), float (frameSize));

    // Marco de baquelita con aro de latón.
    g.setColour (bakelite);
    g.fillRoundedRectangle (frame, 24.0f);
    g.setColour (brass);
    g.drawRoundedRectangle (frame.reduced (1.0f), 24.0f, 2.0f);

    drawScreen (g, screenOf (frame));

    g.setColour (inkSoft);
    g.setFont (label (14.0f, 0.2f));
    g.drawText ("CURVA DE TRANSFERENCIA", juce::Rectangle<float> (0.0f, 276.0f, float (frameSize), 20.0f), juce::Justification::centred);

    drawCounters (g);
}
} // namespace tufi::ui
