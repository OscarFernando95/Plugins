#include "Vintage.h"

#include "VintageFonts.h"

namespace ferb::vintage
{
Palette Palette::cremita()
{
    Palette p;
    p.panelHi = juce::Colour (0xfff1e8cf); p.panel = juce::Colour (0xffe6dabd); p.panelLo = juce::Colour (0xffcbbd9a);
    p.ink = juce::Colour (0xff2a1d14); p.ink2 = juce::Colour (0xff6b5a47);
    p.acc = juce::Colour (0xffe08a2e); p.acc2 = juce::Colour (0xffa85f17);
    p.glass = juce::Colour (0xff150e09);
    p.btnA = juce::Colour (0xff4a3b2e); p.btnB = juce::Colour (0xff2a1f17); p.btnInk = juce::Colour (0xfff1e6cc); p.edge = juce::Colour (0xff120b06);
    p.rule = juce::Colour (0xff2a1d14).withAlpha (0.4f); p.lamp = juce::Colour (0xffffb347); p.emboss = juce::Colours::white.withAlpha (0.75f);
    p.earA = juce::Colour (0xff6a4426); p.earB = juce::Colour (0xff4a2d17); p.earC = juce::Colour (0xff3a2211);
    p.bezel = juce::Colour (0xffd6b36a); p.bezelDark = juce::Colour (0xff6b4f1a);
    p.woodEars = true;
    return p;
}

Palette Palette::maru()
{
    Palette p;
    p.panelHi = juce::Colour (0xff62786b); p.panel = juce::Colour (0xff4e6155); p.panelLo = juce::Colour (0xff37463d);
    p.ink = juce::Colour (0xfff3ecd9); p.ink2 = juce::Colour (0xffcbd5c8);
    p.acc = juce::Colour (0xffd7efc9); p.acc2 = juce::Colour (0xff8fb78a);
    p.glass = juce::Colour (0xff060a07);
    p.btnA = juce::Colour (0xff2e3731); p.btnB = juce::Colour (0xff181d19); p.btnInk = juce::Colour (0xfff3ecd9); p.edge = juce::Colour (0xff050706);
    p.rule = juce::Colour (0xfff3ecd9).withAlpha (0.35f); p.lamp = juce::Colour (0xffd7efc9); p.emboss = juce::Colours::black.withAlpha (0.45f);
    p.earA = juce::Colour (0xff9ea8a2); p.earB = juce::Colour (0xff6d7872); p.earC = juce::Colour (0xff59635d);
    p.bezel = juce::Colour (0xffb9bec0); p.bezelDark = juce::Colour (0xff4b4f50);
    p.woodEars = false;
    return p;
}

Palette Palette::chaus()
{
    Palette p;
    p.panelHi = juce::Colour (0xff272229); p.panel = juce::Colour (0xff17141a); p.panelLo = juce::Colour (0xff0d0b0f);
    p.ink = juce::Colour (0xffefe5cf); p.ink2 = juce::Colour (0xffb3a9ba);
    p.acc = juce::Colour (0xffff6b5e); p.acc2 = juce::Colour (0xffb8453b);
    p.glass = juce::Colour (0xff09070b);
    p.btnA = juce::Colour (0xff3a3340); p.btnB = juce::Colour (0xff201b25); p.btnInk = juce::Colour (0xffefe5cf); p.edge = juce::Colour (0xff050407);
    p.rule = juce::Colour (0xffefe5cf).withAlpha (0.35f); p.lamp = juce::Colour (0xffff6b5e); p.emboss = juce::Colours::transparentBlack;
    p.bezel = brass; p.bezelDark = brassDark;
    p.darkKnobs = false; // perillas de baquelita crema
    return p;
}

namespace
{
juce::Typeface::Ptr load (const char* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size));
}
} // namespace

juce::Font logoFont (float height)
{
    static const auto face = load (VintageData::LimelightRegular_ttf, VintageData::LimelightRegular_ttfSize);
    return juce::Font (juce::FontOptions (face).withHeight (height));
}

juce::Font labelFont (float height, float letterSpacing)
{
    static const auto face = load (VintageData::JosefinSansSemiBold_ttf, VintageData::JosefinSansSemiBold_ttfSize);
    return juce::Font (juce::FontOptions (face).withHeight (height)).withExtraKerningFactor (letterSpacing / height);
}

juce::Font digitFont (float height, bool medium)
{
    static const auto regular = load (VintageData::OxaniumRegular_ttf, VintageData::OxaniumRegular_ttfSize);
    static const auto strong = load (VintageData::OxaniumMedium_ttf, VintageData::OxaniumMedium_ttfSize);
    return juce::Font (juce::FontOptions (medium ? strong : regular).withHeight (height));
}

void drawSpaced (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, const juce::Font& font, juce::Justification j)
{
    g.setFont (font);
    g.drawText (text, area, j, false);
}

juce::ColourGradient svgRadial (juce::Rectangle<float> box, float cx, float cy, float r,
                                std::initializer_list<std::pair<float, juce::Colour>> stops)
{
    const auto centre = juce::Point<float> (box.getX() + cx * box.getWidth(), box.getY() + cy * box.getHeight());
    const float radius = r * box.getWidth();
    juce::ColourGradient gradient (stops.begin()->second, centre, (stops.end() - 1)->second, centre.translated (radius, 0.0f), true);
    gradient.clearColours();
    for (const auto& s : stops)
        gradient.addColour (s.first, s.second);
    return gradient;
}

void paintBezel (juce::Graphics& g, juce::Rectangle<float> r, const Palette& p, float corner, bool thick)
{
    // De dentro afuera: 3 px negro, 2-3 px del color del aparato y 1 px oscuro (como en la maqueta).
    const float black = 3.0f, ring = thick ? 3.0f : 2.0f;
    auto outer = r.expanded (black + ring + 1.0f);
    g.setColour (p.bezelDark);
    g.fillRoundedRectangle (outer, corner + black + ring + 1.0f);
    outer = outer.reduced (1.0f);
    g.setColour (p.bezel);
    g.fillRoundedRectangle (outer, corner + black + ring);
    outer = outer.reduced (ring);
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRoundedRectangle (outer, corner + black);
}

void paintGlassBackground (juce::Graphics& g, juce::Rectangle<float> r, const Palette& p, float corner)
{
    paintBezel (g, r, p, corner, true);
    juce::ColourGradient gradient (juce::Colour (0xff16101d), r.getX() + r.getWidth() * 0.5f, r.getY() + r.getHeight() * 0.4f,
                                   p.glass, r.getX() + r.getWidth() * 1.1f, r.getY() + r.getHeight() * 1.0f, true);
    gradient.addColour (0.72, p.glass);
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (r, corner);
}

void paintGlassOverlay (juce::Graphics& g, juce::Rectangle<float> r, float corner)
{
    juce::Graphics::ScopedSaveState state (g);
    juce::Path clip;
    clip.addRoundedRectangle (r, corner);
    g.reduceClipRegion (clip);

    // Rayas de pantalla.
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    for (float y = r.getY(); y < r.getBottom(); y += 3.0f)
        g.fillRect (r.getX(), y, r.getWidth(), 1.0f);

    // Brillo diagonal en la esquina superior izquierda.
    juce::ColourGradient shine (juce::Colours::white.withAlpha (0.07f), r.getX(), r.getY(),
                                juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.32f, r.getY() + r.getHeight() * 0.32f, false);
    g.setGradientFill (shine);
    g.fillRect (r);

    // Viñeta.
    juce::ColourGradient vignette (juce::Colours::transparentBlack, r.getCentreX(), r.getCentreY(),
                                   juce::Colours::black.withAlpha (0.45f), r.getX(), r.getY(), true);
    vignette.clearColours();
    vignette.addColour (0.0, juce::Colours::transparentBlack);
    vignette.addColour (0.62, juce::Colours::transparentBlack);
    vignette.addColour (1.0, juce::Colours::black.withAlpha (0.45f));
    g.setGradientFill (vignette);
    g.fillRect (r);

    // Sombra interior.
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRoundedRectangle (r.reduced (1.0f), corner, 2.0f);
}
} // namespace ferb::vintage
