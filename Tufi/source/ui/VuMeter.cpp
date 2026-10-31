#include "VuMeter.h"

namespace tufi::ui
{
namespace
{
// 0 VU = −12 dBFS de pico.
constexpr float referenceDb = -12.0f;
constexpr float timeConstant = 0.065f; // ~300 ms hasta el 99 %
constexpr float refreshHz = 30.0f;
} // namespace

VuMeter::VuMeter (TufiProcessor& p) : processor (p)
{
    startTimerHz (30);
}

float VuMeter::vuToDegrees (float vu) noexcept
{
    // Marcas de la escala: (VU, grados). Entre ellas se interpola.
    static constexpr float marks[][2] = { { -20.0f, -48.0f }, { -10.0f, -26.0f }, { -7.0f, -15.0f }, { -5.0f, -7.0f },
                                          { -3.0f, 3.0f },    { 0.0f, 20.0f },    { 1.0f, 27.0f },   { 2.0f, 36.0f },
                                          { 3.0f, 48.0f } };
    vu = juce::jlimit (-20.0f, 3.0f, vu);
    for (size_t i = 1; i < std::size (marks); ++i)
        if (vu <= marks[i][0])
        {
            const float t = (vu - marks[i - 1][0]) / (marks[i][0] - marks[i - 1][0]);
            return marks[i - 1][1] + t * (marks[i][1] - marks[i - 1][1]);
        }
    return 48.0f;
}

void VuMeter::timerCallback()
{
    const float peak = processor.getOutputMeter().consume();
    const float alpha = 1.0f - std::exp (-1.0f / (refreshHz * timeConstant));
    smoothed += alpha * (peak - smoothed);

    const float vu = juce::Decibels::gainToDecibels (smoothed, -60.0f) - referenceDb;
    const float target = vuToDegrees (vu);
    // Un pequeño rebote al soltar, para que no parezca digital.
    needleDegrees += (target - needleDegrees) * 0.55f;
    repaint();
}

void VuMeter::paint (juce::Graphics& g)
{
    const auto frame = getLocalBounds().toFloat();
    g.setColour (bakelite);
    g.fillRoundedRectangle (frame, 18.0f);
    g.setColour (brass);
    g.drawRoundedRectangle (frame.reduced (1.0f), 18.0f, 2.0f);

    const auto face = frame.reduced (7.0f);
    juce::ColourGradient paper (vuFace.brighter (0.18f), face.getX(), face.getY(), vuFace.darker (0.05f), face.getRight(), face.getBottom(), false);
    g.setGradientFill (paper);
    g.fillRoundedRectangle (face, 12.0f);

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (face, 12.0f);
        g.reduceClipRegion (clip);

        // Reflejo diagonal del cristal.
        juce::Path glare;
        glare.startNewSubPath (face.getX(), face.getY());
        glare.lineTo (face.getX() + face.getWidth() * 0.62f, face.getY());
        glare.lineTo (face.getX(), face.getY() + face.getHeight() * 0.45f);
        glare.closeSubPath();
        g.setColour (juce::Colours::white.withAlpha (0.14f));
        g.fillPath (glare);

        const juce::Point<float> pivot (face.getCentreX(), face.getBottom() - 8.0f);
        const float radius = 105.0f;
        auto at = [&] (float degrees, float r)
        {
            const float a = juce::degreesToRadians (degrees);
            return juce::Point<float> (pivot.x + std::sin (a) * r, pivot.y - std::cos (a) * r);
        };

        // Arco de la escala y zona roja.
        juce::Path arc, red;
        arc.addCentredArc (pivot.x, pivot.y, radius, radius, 0.0f, juce::degreesToRadians (-48.0f), juce::degreesToRadians (48.0f), true);
        g.setColour (ink);
        g.strokePath (arc, juce::PathStrokeType (1.4f));
        red.addCentredArc (pivot.x, pivot.y, radius - 3.0f, radius - 3.0f, 0.0f, juce::degreesToRadians (20.0f), juce::degreesToRadians (48.0f), true);
        g.setColour (vuRed);
        g.strokePath (red, juce::PathStrokeType (6.0f));

        // Marcas y números.
        struct Tick { float vu; const char* text; bool major; };
        const Tick ticks[] = { { -20, "20", true }, { -15, "", false }, { -10, "10", true }, { -7, "7", true }, { -5, "5", true },
                               { -3, "3", true }, { -2, "", false }, { -1, "", false }, { 0, "0", true }, { 1, "1", true },
                               { 2, "2", true }, { 3, "3", true } };
        for (const auto& t : ticks)
        {
            const bool isRed = t.vu >= 0.0f;
            const float deg = vuToDegrees (t.vu);
            g.setColour (isRed ? vuRed : ink);
            g.drawLine (juce::Line<float> (at (deg, radius), at (deg, radius - (t.major ? 9.0f : 5.0f))), t.major ? 1.5f : 1.0f);
            if (t.text[0] != '\0')
            {
                g.setFont (label (11.5f, 0.0f));
                const auto p = at (deg, radius - 19.0f);
                g.drawText (t.text, juce::Rectangle<float> (p.x - 10.0f, p.y - 7.0f, 20.0f, 14.0f), juce::Justification::centred);
            }
        }

        g.setColour (ink);
        g.setFont (label (26.0f, 0.0f));
        g.drawText ("VU", juce::Rectangle<float> (face.getX() + 12.0f, face.getBottom() - 38.0f, 50.0f, 30.0f), juce::Justification::centredLeft);

        // Aguja y su eje.
        g.setColour (juce::Colours::black.withAlpha (0.2f));
        g.drawLine (juce::Line<float> (pivot.translated (2.0f, 2.0f), at (needleDegrees, radius - 4.0f).translated (2.0f, 2.0f)), 1.6f);
        g.setColour (ink);
        g.drawLine (juce::Line<float> (pivot, at (needleDegrees, radius - 4.0f)), 1.8f);
        g.setColour (bakelite);
        g.fillEllipse (pivot.x - 7.0f, pivot.y - 7.0f, 14.0f, 14.0f);
    }
}
} // namespace tufi::ui
