#include "Meters.h"

namespace ferb::vintage
{
namespace
{
constexpr float rad = juce::MathConstants<float>::pi / 180.0f;
const juce::Colour needleRed { 0xffa8321d };
const juce::Colour needleInk { 0xff17101f };
} // namespace

VuNeedle::VuNeedle (const Palette& p, juce::String t, bool warmFace) : palette (p), title (std::move (t)), warm (warmFace)
{
    setInterceptsMouseClicks (false, false);
    setOpaque (false);
    setTitle ("Vúmetro " + title.toLowerCase());
    setAccessible (true);
}

void VuNeedle::setValue (float vu)
{
    vu = juce::jlimit (-21.0f, 4.0f, vu);
    if (std::abs (vu - value) < 0.02f)
        return;
    value = vu;
    repaint();
}

void VuNeedle::renderFace (juce::Graphics& g, juce::Rectangle<float> f) const
{
    const float w = f.getWidth(), h = f.getHeight();
    const float cx = w * 0.5f, cy = h * 1.28f, R = h * 1.1f;
    const auto pt = [&] (float deg, float r) { return juce::Point<float> (cx + r * std::sin (deg * rad), cy - r * std::cos (deg * rad)); };
    const auto angleOf = [] (float v) { return -48.0f + 96.0f * ((v + 20.0f) / 23.0f); };

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff8ebc6), 0.0f, 0.0f, juce::Colour (0xffe2cd98), 0.0f, h, false));
    g.fillRect (0.0f, 0.0f, w, h);
    g.setColour (juce::Colour (0xffffb347).withAlpha (warm ? 0.16f : 0.08f));
    g.fillRect (0.0f, 0.0f, w, h);

    const auto ink = juce::Colour (0xff2a1d14);

    // Arco exterior y zona roja (de 0 VU al final).
    const float arcR = R * 0.99f;
    juce::Path arc, red;
    arc.addCentredArc (cx, cy, arcR, arcR, 0.0f, -48.0f * rad, 48.0f * rad, true);
    red.addCentredArc (cx, cy, arcR, arcR, 0.0f, angleOf (0.0f) * rad, 48.0f * rad, true);
    g.setColour (ink);
    g.strokePath (arc, juce::PathStrokeType (1.4f));
    g.setColour (needleRed);
    g.strokePath (red, juce::PathStrokeType (h * 0.035f));

    // Marcas y cifras.
    for (int i = 0; i <= 23; ++i)
    {
        const int v = i - 20;
        const float th = -48.0f + 96.0f * static_cast<float> (i) / 23.0f;
        const bool major = v % 5 == 0 || v == 0 || v == 3;
        const auto a = pt (th, R * (major ? 0.86f : 0.91f)), b = pt (th, R * 0.97f);
        g.setColour (ink);
        g.drawLine (a.x, a.y, b.x, b.y, major ? 1.8f : 1.0f);
    }

    const std::pair<int, const char*> numbers[] = { { -20, "20" }, { -10, "10" }, { -7, "7" }, { -5, "5" }, { -3, "3" }, { 0, "0" }, { 3, "+3" } };
    for (const auto& [v, text] : numbers)
    {
        const auto p = pt (angleOf (static_cast<float> (v)), R * 0.74f);
        g.setColour (v >= 0 ? needleRed : ink);
        g.setFont (labelFont (h * 0.11f));
        g.drawText (text, juce::Rectangle<float> (p.x - 20.0f, p.y + 4.0f - h * 0.09f, 40.0f, h * 0.12f), juce::Justification::centred);
    }

    g.setColour (ink);
    g.setFont (labelFont (h * 0.09f, 2.0f));
    g.drawText (title, juce::Rectangle<float> (0.0f, h * 0.86f - h * 0.085f, w, h * 0.11f), juce::Justification::centred);
    g.setFont (labelFont (h * 0.1f));
    g.drawText ("VU", juce::Rectangle<float> (w * 0.1f, h * 0.2f - h * 0.09f, h * 0.4f, h * 0.12f), juce::Justification::centredLeft);
}

void VuNeedle::paint (juce::Graphics& g)
{
    const auto f = face();
    const float corner = 6.0f;

    // Bisel: negro, aro del aparato y filete oscuro.
    {
        auto outer = f.expanded (7.0f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (outer.translated (0.0f, 3.0f).expanded (1.0f), corner + 7.0f);
        g.setColour (palette.bezelDark);
        g.fillRoundedRectangle (outer, corner + 7.0f);
        g.setColour (palette.bezel);
        g.fillRoundedRectangle (outer.reduced (1.0f), corner + 6.0f);
        g.setColour (juce::Colour (0xff1a1a1a));
        g.fillRoundedRectangle (f.expanded (3.0f), corner + 3.0f);
    }

    // Esfera en caché a la resolución real (incluye la escala de la ventana y la pantalla).
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (! cache.isValid() || std::abs (scale - cacheScale) > 0.001f || cache.getWidth() != juce::roundToInt (f.getWidth() * scale))
    {
        cache = juce::Image (juce::Image::ARGB, juce::jmax (1, juce::roundToInt (f.getWidth() * scale)),
                             juce::jmax (1, juce::roundToInt (f.getHeight() * scale)), true);
        juce::Graphics ig (cache);
        ig.addTransform (juce::AffineTransform::scale (scale));
        renderFace (ig, { 0.0f, 0.0f, f.getWidth(), f.getHeight() });
        cacheScale = scale;
    }

    juce::Path clip;
    clip.addRoundedRectangle (f, corner);
    g.saveState();
    g.reduceClipRegion (clip);
    g.drawImage (cache, f, juce::RectanglePlacement::stretchToFit);

    // Aguja con su sombra.
    const float w = f.getWidth(), h = f.getHeight();
    const float cx = f.getX() + w * 0.5f, cy = f.getY() + h * 1.28f, R = h * 1.1f;
    const float angle = (-48.0f + 96.0f * (juce::jlimit (-1.0f, 24.0f, value + 20.0f) / 23.0f)) * rad;
    const juce::Point<float> tip (cx + (R + 4.0f) * std::sin (angle), cy - (R + 4.0f) * std::cos (angle));
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawLine (cx + 3.0f, cy + 3.0f, tip.x + 3.0f, tip.y + 3.0f, 3.0f);
    g.setColour (needleInk);
    g.drawLine (cx, cy, tip.x, tip.y, 2.0f);

    // Brillo del cristal.
    juce::Path shine;
    shine.addTriangle (f.getX(), f.getY(), f.getRight(), f.getY(), f.getX(), f.getY() + h * 0.6f);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.fillPath (shine);
    g.restoreState();

    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawRoundedRectangle (f.reduced (0.5f), corner, 1.0f);
}
} // namespace ferb::vintage
