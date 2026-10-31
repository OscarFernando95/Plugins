#include "NelUi.h"

namespace nel::ui
{
using namespace ferb::vintage;

namespace
{
constexpr float rad = juce::MathConstants<float>::pi / 180.0f;
const juce::Colour lilacDim = lilac.withAlpha (0.62f);
const juce::Colour creamDim { 0xffc9bda8 };

juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

juce::String num (float v, int decimals)
{
    const auto s = ferb::fmt::comma (std::abs (v), decimals);
    return (v < 0.0f && s.containsAnyOf ("123456789") ? U ("−") : "") + s;
}

juce::String signedNum (float v, int decimals)
{
    const auto s = ferb::fmt::comma (std::abs (v), decimals);
    return (v > 0.0f && s.containsAnyOf ("123456789") ? "+" : (v < 0.0f && s.containsAnyOf ("123456789") ? U ("−") : "")) + s;
}

void panelButton (juce::Graphics& g, juce::Rectangle<float> r, const Palette& p, float corner, bool pressed)
{
    g.setColour (juce::Colour (0xff160b26));
    g.fillRoundedRectangle (r.translated (0.0f, pressed ? 1.0f : 2.0f), corner);
    juce::ColourGradient grad (pressed ? juce::Colour (0xff171320) : p.btnA, 0.0f, r.getY(), pressed ? juce::Colour (0xff0e0c12) : p.btnB, 0.0f, r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, corner);
    g.setColour (p.edge);
    g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
    if (! pressed)
    {
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawLine (r.getX() + 3.0f, r.getY() + 1.5f, r.getRight() - 3.0f, r.getY() + 1.5f);
    }
}

void lamp (juce::Graphics& g, juce::Point<float> c, float d, bool on, juce::Colour colour)
{
    const auto box = juce::Rectangle<float> (d, d).withCentre (c);
    if (on)
    {
        g.setColour (colour.withAlpha (0.35f));
        g.fillEllipse (box.expanded (3.0f));
    }
    g.setColour (on ? colour : juce::Colour (0xff3a2a22));
    g.fillEllipse (box);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (box, 1.0f);
}
} // namespace

// ------------------------------------------------------------------ LadderMeter

LadderMeter::LadderMeter (const Palette& p, juce::String t, bool isOutput) : palette (p), title (std::move (t)), output (isOutput)
{
    setInterceptsMouseClicks (false, false);
    setSize (width, height);
    setTitle ("Medidor de " + title.toLowerCase());
}

void LadderMeter::setState (const MeterState& s)
{
    state = s;
    repaint();
}

void LadderMeter::paint (juce::Graphics& g)
{
    constexpr int N = 48;
    constexpr float h = 272.0f, segH = h / N;
    const auto& peak = output ? state.outPeakDb : state.inPeakDb;
    const auto& hold = output ? state.outHoldDb : state.inHoldDb;
    const float rms = output ? state.outRmsDb : state.inRmsDb;

    for (int b = 0; b < 2; ++b)
    {
        const float x0 = 4.0f + 20.0f * static_cast<float> (b);
        for (int i = 0; i < N; ++i)
        {
            const float lo = -48.0f + static_cast<float> (i), y = h - static_cast<float> (i + 1) * segH + 0.8f;
            const auto colour = lo >= -3.0f ? alert : (lo >= -9.0f ? juce::Colour (0xffffb347) : lilac);
            const bool on = peak[static_cast<size_t> (b)] >= lo + 0.5f, inRms = rms >= lo + 0.5f;
            g.setColour (inRms ? colour : (on ? colour.withAlpha (0.5f) : lilac.withAlpha (0.10f)));
            g.fillRect (x0, y, 16.0f, segH - 1.6f);
        }
        const int hi = static_cast<int> (std::floor (hold[static_cast<size_t> (b)] + 48.0f));
        if (hi >= 0 && hi < N)
        {
            g.setColour (juce::Colours::white);
            g.fillRect (x0, h - static_cast<float> (hi + 1) * segH + 0.8f, 16.0f, segH - 1.6f);
        }
    }

    g.setFont (digitFont (10.0f));
    for (const int t : { 0, -3, -6, -12, -18, -24, -36, -48 })
    {
        const float y = h - static_cast<float> (t + 48) * segH;
        g.setColour (palette.ink.withAlpha (0.55f));
        g.fillRect (44.0f, y - 0.5f, 4.0f, 1.0f);
        g.setColour (palette.ink.withAlpha (0.85f));
        const float v = static_cast<float> (t) + state.meterOffset;
        g.drawText (signedNum (v, 0), juce::Rectangle<float> (50.0f, juce::jlimit (6.0f, h - 6.0f, y) - 6.0f, 28.0f, 12.0f), juce::Justification::centredLeft);
    }

    if (output)
    {
        const float cy = h - (juce::jlimit (-48.0f, 0.0f, state.ceilingDb) + 48.0f) * segH;
        g.setColour (brass);
        g.fillRect (0.0f, cy - 1.0f, 44.0f, 2.0f);
        juce::Path arrow;
        arrow.addTriangle (44.0f, cy, 52.0f, cy - 4.0f, 52.0f, cy + 4.0f);
        g.fillPath (arrow);
    }

    drawEmbossed (g, title, { 0.0f, 276.0f, static_cast<float> (width), 16.0f }, labelFont (12.0f, 2.0f), juce::Justification::centred, palette);
    g.setColour (lilac);
    g.setFont (digitFont (11.0f));
    const float peakNow = std::max (peak[0], peak[1]);
    g.drawText ("PICO " + num (peakNow, 1), juce::Rectangle<float> (0.0f, 293.0f, static_cast<float> (width), 12.0f), juce::Justification::centred);
    g.drawText ("RMS " + num (rms, 1), juce::Rectangle<float> (0.0f, 305.0f, static_cast<float> (width), 12.0f), juce::Justification::centred);
}

// ------------------------------------------------------------------ ScopeView

ScopeView::ScopeView (const Palette& p) : GlassView (p), in (historyLength, -100.0f), out (historyLength, -100.0f), gr (historyLength, 0.0f)
{
    setTitle ("Pantalla de nivel y reducción de ganancia");
}

void ScopeView::push (float inputDb, float outputDb, float reductionDb)
{
    for (auto* v : { &in, &out, &gr })
        std::rotate (v->begin(), v->begin() + 1, v->end());
    in.back() = inputDb;
    out.back() = outputDb;
    gr.back() = reductionDb;
    repaint();
}

void ScopeView::paintContent (juce::Graphics& g, float W, float H)
{
    constexpr float top = 6.0f, bottom = -30.0f, span = top - bottom;
    const float pxdb = H / span;
    const auto ydb = [&] (float d) { return (top - juce::jlimit (bottom - 3.0f, top + 3.0f, d)) / span * H; };

    g.setFont (digitFont (10.0f));
    for (const int d : { 0, -6, -12, -18, -24, -30 })
    {
        const float y = std::round (ydb (static_cast<float> (d))) + 0.5f;
        g.setColour (lilac.withAlpha (0.12f));
        g.drawLine (0.0f, y, W, y, 1.0f);
        g.setColour (lilac.withAlpha (0.6f));
        g.drawText (signedNum (static_cast<float> (d), 0).replace ("+", ""), juce::Rectangle<float> (W - 46.0f, y - 14.0f, 40.0f, 12.0f), juce::Justification::centredRight);
    }

    const int n = historyLength;
    const auto at = [&] (const std::vector<float>& a, float px) { return a[static_cast<size_t> (std::min (n - 1, static_cast<int> (px / (W - 1.0f) * static_cast<float> (n - 1))))]; };

    const auto fillArea = [&] (const std::vector<float>& a, juce::Colour fill, std::optional<juce::Colour> line)
    {
        juce::Path area, stroke;
        area.startNewSubPath (0.0f, H);
        for (float px = 0.0f; px < W; px += 2.0f)
        {
            area.lineTo (px, ydb (at (a, px)));
            px == 0.0f ? stroke.startNewSubPath (px, ydb (at (a, px))) : stroke.lineTo (px, ydb (at (a, px)));
        }
        area.lineTo (W, ydb (at (a, W - 1.0f)));
        area.lineTo (W, H);
        area.closeSubPath();
        g.setColour (fill);
        g.fillPath (area);
        if (line)
        {
            g.setColour (*line);
            g.strokePath (stroke, juce::PathStrokeType (1.4f));
        }
    };
    fillArea (in, juce::Colour (143, 114, 201).withAlpha (0.2f), std::nullopt);
    fillArea (out, audition ? juce::Colour (0xffffb347).withAlpha (0.55f) : juce::Colour (138, 92, 240).withAlpha (0.38f),
              audition ? juce::Colour (0xffffb347) : juce::Colour (0xffe6d4ff));

    // Reducción desde arriba y la etiqueta de cada máximo local.
    std::vector<float> reduction (static_cast<size_t> (W));
    for (size_t px = 0; px < reduction.size(); ++px)
        reduction[px] = at (gr, static_cast<float> (px));
    g.setColour (juce::Colour (0xffffb347).withAlpha (0.55f));
    for (float px = 0.0f; px < W; px += 2.0f)
    {
        const float hh = reduction[static_cast<size_t> (px)] * pxdb;
        if (hh > 0.4f)
            g.fillRect (px, 0.0f, 2.0f, hh);
    }
    g.setFont (digitFont (11.0f));
    int last = -100;
    for (int px = 40; px < static_cast<int> (W) - 40; ++px)
    {
        const float v = reduction[static_cast<size_t> (px)];
        if (v < 2.5f || px - last < 44)
            continue;
        bool isPeak = true;
        for (int d = -34; d <= 34 && isPeak; ++d)
        {
            const float w = reduction[static_cast<size_t> (px + d)];
            if (w > v || (std::abs (w - v) < 1.0e-6f && d < 0))
                isPeak = false;
        }
        if (isPeak)
        {
            last = px;
            g.setColour (juce::Colour (0xffffd9a0));
            g.drawText (U ("−") + ferb::fmt::comma (v, 1), juce::Rectangle<float> (static_cast<float> (px) - 25.0f, std::min (v * pxdb + 13.0f, H - 4.0f) - 11.0f, 50.0f, 13.0f), juce::Justification::centred);
        }
    }

    // Techo.
    const float cy = std::round (ydb (ceilingDb)) + 0.5f;
    juce::Path line;
    line.startNewSubPath (0.0f, cy);
    line.lineTo (W, cy);
    juce::Path dashed;
    const float pattern[] = { 6.0f, 4.0f };
    juce::PathStrokeType (1.2f).createDashedStroke (dashed, line, pattern, 2);
    g.setColour (alert);
    g.fillPath (dashed);
    g.setFont (digitFont (10.0f));
    g.setColour (juce::Colour (0xffff8aa3));
    g.drawText ("TECHO " + num (ceilingDb, 1), juce::Rectangle<float> (8.0f, std::min (cy + 11.0f, H - 4.0f) - 10.0f, 120.0f, 12.0f), juce::Justification::centredLeft);
    g.setColour (lilac.withAlpha (0.55f));
    g.drawText ("6 s", juce::Rectangle<float> (W - 48.0f, H - 16.0f, 40.0f, 12.0f), juce::Justification::centredRight);

    // Leyenda.
    const juce::String names[3] = { U ("entrada con ganancia"), outputLegend, U ("reducción") };
    const juce::Colour colours[3] = { juce::Colour (0xff8f72c9), lilac, juce::Colour (0xffffb347) };
    const auto font = labelFont (11.0f, 0.8f);
    float widthTotal = 16.0f;
    for (const auto& s : names)
        widthTotal += 15.0f + juce::GlyphArrangement::getStringWidth (font, s) + 14.0f;
    widthTotal -= 14.0f;
    const juce::Rectangle<float> box (8.0f, H - 6.0f - 22.0f, widthTotal, 22.0f);
    g.setColour (juce::Colour (14, 7, 23).withAlpha (0.78f));
    g.fillRoundedRectangle (box, 4.0f);
    float x = box.getX() + 8.0f;
    g.setFont (font);
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (colours[i]);
        g.fillRoundedRectangle (x, box.getY() + 6.0f, 10.0f, 10.0f, 2.0f);
        g.setColour (creamDim);
        const float tw = juce::GlyphArrangement::getStringWidth (font, names[i]);
        g.drawText (names[i], juce::Rectangle<float> (x + 15.0f, box.getY(), tw + 2.0f, 22.0f), juce::Justification::centredLeft);
        x += 15.0f + tw + 14.0f;
    }
}

// ------------------------------------------------------------------ GrMeter

GrMeter::GrMeter (const Palette& p) : palette (p)
{
    setInterceptsMouseClicks (false, false);
    setSize (228 + 2 * margin, 118 + 2 * margin);
    setTitle (U ("Vúmetro de reducción de ganancia"));
}

void GrMeter::setValue (float reductionDb)
{
    reductionDb = juce::jlimit (0.0f, 12.6f, reductionDb);
    if (std::abs (reductionDb - value) < 0.02f)
        return;
    value = reductionDb;
    repaint();
}

void GrMeter::paint (juce::Graphics& g)
{
    const auto f = getLocalBounds().toFloat().reduced (static_cast<float> (margin));
    const float corner = 8.0f;
    {
        auto outer = f.expanded (4.0f);
        g.setColour (brassDark);
        g.fillRoundedRectangle (outer, corner + 4.0f);
        g.setColour (juce::Colour (0xff0a0510));
        g.fillRoundedRectangle (f.expanded (2.0f), corner + 2.0f);
    }

    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (! cache.isValid() || std::abs (scale - cacheScale) > 0.001f)
    {
        cache = juce::Image (juce::Image::ARGB, juce::roundToInt (228.0f * scale), juce::roundToInt (118.0f * scale), true);
        juce::Graphics ig (cache);
        ig.addTransform (juce::AffineTransform::scale (scale));
        const float cx = 114.0f, cy = 150.0f, R = 112.0f;
        const auto pt = [&] (float deg, float r) { return juce::Point<float> (cx + r * std::sin (deg * rad), cy - r * std::cos (deg * rad)); };
        const auto ink = juce::Colour (0xff2a1a40), red = juce::Colour (0xffa8321d);

        ig.setGradientFill (juce::ColourGradient (juce::Colour (0xfff8ebc6), 0.0f, 0.0f, juce::Colour (0xffe2cd98), 0.0f, 118.0f, false));
        ig.fillRect (0.0f, 0.0f, 228.0f, 118.0f);
        ig.setColour (juce::Colour (0xffffb347).withAlpha (0.10f));
        ig.fillRect (0.0f, 0.0f, 228.0f, 118.0f);

        juce::Path arc, redArc;
        arc.addCentredArc (cx, cy, R, R, 0.0f, -48.0f * rad, 48.0f * rad, true);
        redArc.addCentredArc (cx, cy, 108.0f, 108.0f, 0.0f, -48.0f * rad, (48.0f - 96.0f * 9.0f / 12.0f) * rad, true);
        ig.setColour (ink);
        ig.strokePath (arc, juce::PathStrokeType (1.6f));
        ig.setColour (red);
        ig.strokePath (redArc, juce::PathStrokeType (5.0f));

        for (int i = 0; i <= 24; ++i)
        {
            const float gdb = static_cast<float> (i) * 0.5f, th = 48.0f - 96.0f * gdb / 12.0f;
            const bool major = i % 4 == 0, whole = i % 2 == 0;
            const auto a = pt (th, major ? 98.0f : (whole ? 103.0f : 106.0f)), b = pt (th, 112.0f);
            ig.setColour (ink);
            ig.drawLine (a.x, a.y, b.x, b.y, major ? 2.0f : 1.0f);
        }
        for (int gdb = 0; gdb <= 12; gdb += 2)
        {
            const auto p = pt (48.0f - 96.0f * static_cast<float> (gdb) / 12.0f, 84.0f);
            ig.setColour (gdb >= 10 ? red : ink);
            ig.setFont (labelFont (13.0f));
            ig.drawText (juce::String (gdb), juce::Rectangle<float> (p.x - 15.0f, p.y + 4.0f - 12.0f, 30.0f, 15.0f), juce::Justification::centred);
        }
        ig.setColour (ink);
        ig.setFont (labelFont (10.0f, 2.0f));
        ig.drawText (U ("REDUCCIÓN · dB"), juce::Rectangle<float> (0.0f, 104.0f - 10.0f, 228.0f, 12.0f), juce::Justification::centred);
        ig.setFont (labelFont (9.0f));
        ig.drawText (U ("MÁX"), juce::Rectangle<float> (150.0f, 18.0f - 10.0f, 42.0f, 12.0f), juce::Justification::centredRight);
        cacheScale = scale;
    }

    juce::Path clip;
    clip.addRoundedRectangle (f, corner);
    g.saveState();
    g.reduceClipRegion (clip);
    g.drawImage (cache, f, juce::RectanglePlacement::stretchToFit);

    const float cx = f.getX() + 114.0f, cy = f.getY() + 150.0f;
    const float angle = (48.0f - 96.0f * value / 12.0f) * rad;
    const juce::Point<float> tip (cx + 118.0f * std::sin (angle), cy - 118.0f * std::cos (angle));
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawLine (cx + 3.0f, cy + 3.0f, tip.x + 3.0f, tip.y + 3.0f, 3.0f);
    g.setColour (juce::Colour (0xff17101f));
    g.drawLine (cx, cy, tip.x, tip.y, 2.0f);

    g.setColour (value > 12.0f ? juce::Colour (0xffff3b3b) : juce::Colour (0xff5a2a2a));
    g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ f.getX() + 204.0f, f.getY() + 14.0f }));

    juce::Path shine;
    shine.addTriangle (f.getX(), f.getY(), f.getRight(), f.getY(), f.getX(), f.getY() + 70.0f);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.fillPath (shine);
    g.restoreState();
}

// ------------------------------------------------------------------ LoudnessPanel

LoudnessPanel::LoudnessPanel (const Palette& p, int initialTarget) : palette (p), target (initialTarget)
{
    setSize (width + 4, height + 4);
    setTitle ("Sonoridad");
}

void LoudnessPanel::setValues (const Values& v)
{
    values = v;
    repaint();
}

juce::Rectangle<float> LoudnessPanel::resetButton() const { return { 2.0f + 162.0f, 2.0f + 8.0f, 56.0f, 18.0f }; }

juce::Rectangle<float> LoudnessPanel::targetButton (int i) const
{
    const float w = (158.0f - 16.0f) / 5.0f;
    return { 2.0f + 60.0f + static_cast<float> (i) * (w + 4.0f), 2.0f + 147.0f, w, 20.0f };
}

void LoudnessPanel::mouseDown (const juce::MouseEvent& e)
{
    if (resetButton().contains (e.position))
    {
        if (onReset)
            onReset();
        return;
    }
    static constexpr int targets[] = { -9, -14, -16, -23, -24 };
    for (int i = 0; i < 5; ++i)
        if (targetButton (i).contains (e.position))
        {
            target = targets[i];
            if (onTarget)
                onTarget (target);
            repaint();
        }
}

void LoudnessPanel::paint (juce::Graphics& g)
{
    const auto panel = juce::Rectangle<float> (2.0f, 2.0f, static_cast<float> (width), static_cast<float> (height));
    g.setColour (brassDark);
    g.fillRoundedRectangle (panel.expanded (2.0f), 10.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff17131c), 0.0f, panel.getY(), juce::Colour (0xff0e0c12), 0.0f, panel.getBottom(), false));
    g.fillRoundedRectangle (panel, 8.0f);
    g.setColour (violet.withAlpha (0.3f));
    g.drawLine (panel.getX() + 6.0f, panel.getY() + 1.0f, panel.getRight() - 6.0f, panel.getY() + 1.0f);
    g.setColour (juce::Colour (0xff050407));
    g.drawRoundedRectangle (panel.reduced (0.5f), 8.0f, 1.0f);

    const auto origin = panel.getPosition();
    const auto at = [&] (float x, float y, float w, float h) { return juce::Rectangle<float> (origin.x + x, origin.y + y, w, h); };

    g.setColour (palette.ink);
    drawSpaced (g, "LOUDNESS", at (10.0f, 7.0f, 90.0f, 20.0f), labelFont (11.0f, 2.4f), juce::Justification::centredLeft);
    g.setColour (creamDim);
    drawSpaced (g, "EBU", at (162.0f - 8.0f - 26.0f, 7.0f, 28.0f, 20.0f), labelFont (9.0f, 1.2f), juce::Justification::centredRight);
    lamp (g, { origin.x + 162.0f - 8.0f - 34.0f, origin.y + 17.0f }, 7.0f, true, juce::Colour (0xff4cd37f));
    panelButton (g, at (162.0f, 8.0f, 56.0f, 18.0f), palette, 2.0f, false);
    g.setColour (palette.ink);
    drawSpaced (g, "RESET", at (162.0f, 8.0f, 56.0f, 18.0f), labelFont (9.0f, 1.2f), juce::Justification::centred);

    // Columnas M y S: 27 segmentos de −18 a +9 LU respecto del objetivo.
    const auto column = [&] (float x, float value)
    {
        const auto bar = at (x + 6.0f, 33.0f, 18.0f, 92.0f);
        g.setColour (juce::Colour (0xff050407));
        g.fillRoundedRectangle (bar, 2.0f);
        const float rel = value - static_cast<float> (target);
        const bool have = value > -150.0f;
        const float segH = (92.0f - 4.0f - 26.0f) / 27.0f;
        for (int i = 0; i < 27; ++i)
        {
            const int lu = -18 + i + 1;
            const float y = bar.getBottom() - 2.0f - static_cast<float> (i + 1) * segH - static_cast<float> (i);
            juce::Colour c = violet.withAlpha (0.13f);
            if (have && rel >= static_cast<float> (lu) - 0.5f)
                c = lu > 6 ? alert : (lu > 1 ? juce::Colour (0xffffb347) : (lu >= -1 ? juce::Colour (0xfff3e3b8) : violet));
            g.setColour (c);
            g.fillRoundedRectangle (bar.getX() + 2.0f, y, 14.0f, segH, 1.0f);
        }
        g.setColour (lilac);
        g.setFont (digitFont (9.5f));
        g.drawText (have ? num (value, 1) : U ("−−"), at (x - 4.0f, 33.0f + 92.0f + 1.0f, 38.0f, 12.0f), juce::Justification::centred);
    };
    column (10.0f, values.momentary);
    column (68.0f, values.shortTerm);

    g.setFont (digitFont (9.0f));
    g.setColour (palette.ink.withAlpha (0.75f));
    for (const int lu : { -18, -12, -6, 0, 6, 9 })
    {
        const float y = 35.0f + 92.0f * (1.0f - static_cast<float> (lu + 18) / 27.0f);
        g.drawText (signedNum (static_cast<float> (lu), 0), at (42.0f, y - 5.0f, 24.0f, 10.0f), juce::Justification::centred);
    }

    // Ventanas.
    const auto window = [&] (juce::Rectangle<float> r, const juce::String& label, const juce::String& text, juce::Colour colour, float digitSize)
    {
        g.setColour (juce::Colour (0xff050407));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawLine (r.getX() + 2.0f, r.getBottom() + 0.5f, r.getRight() - 2.0f, r.getBottom() + 0.5f);
        g.setColour (lilacDim);
        drawSpaced (g, label, juce::Rectangle<float> (r.getX() + 6.0f, r.getY() + 2.0f, r.getWidth() + 4.0f, 10.0f), labelFont (8.5f, 0.9f), juce::Justification::centredLeft);
        g.setColour (colour.withAlpha (0.2f));
        g.setFont (digitFont (digitSize));
        g.drawText (text, r.reduced (7.0f, 0.0f).withTrimmedTop (r.getHeight() - digitSize - 4.0f).translated (0.0f, 0.0f), juce::Justification::centredRight);
        g.setColour (colour);
        g.drawText (text, r.reduced (7.0f, 0.0f).withTrimmedTop (r.getHeight() - digitSize - 4.0f), juce::Justification::centredRight);
    };

    const bool haveI = values.integrated > -69.5f;
    const float delta = values.integrated - static_cast<float> (target);
    const bool haveLra = values.elapsed >= 3.2f && values.integrated > -69.5f;
    const bool haveTp = values.truePeakMax > -80.0f;

    window (at (108.0f, 33.0f, 110.0f, 42.0f), "INT.", haveI ? num (values.integrated, 1) : U ("−−,−"), lilac, 26.0f);
    g.setColour (lilacDim);
    drawSpaced (g, "LUFS", at (160.0f, 36.0f, 54.0f, 10.0f), labelFont (9.0f, 1.4f), juce::Justification::centredRight);
    window (at (108.0f, 79.0f, 53.0f, 30.0f), U ("Δ OBJ"), haveI ? signedNum (delta, 1) : U ("−−"), haveI && std::abs (delta) > 0.5f ? juce::Colour (0xffffb347) : lilac, 14.0f);
    window (at (165.0f, 79.0f, 53.0f, 30.0f), "LRA", haveLra ? num (values.range, 1) : U ("−−"), lilac, 14.0f);
    window (at (108.0f, 113.0f, 53.0f, 30.0f), "PLR", haveI && haveTp ? num (values.truePeakMax - values.integrated, 1) : U ("−−"), lilac, 14.0f);
    window (at (165.0f, 113.0f, 53.0f, 30.0f), U ("TP MÁX"), haveTp ? num (values.truePeakMax, 1) : U ("−−"), lilac, 14.0f);

    g.setColour (creamDim);
    drawSpaced (g, "OBJETIVO", at (10.0f, 147.0f, 60.0f, 20.0f), labelFont (9.0f, 0.9f), juce::Justification::centredLeft);
    static constexpr int targets[] = { -9, -14, -16, -23, -24 };
    for (int i = 0; i < 5; ++i)
    {
        const bool on = targets[i] == target;
        const auto r = targetButton (i);
        panelButton (g, r, palette, 2.0f, on);
        if (on)
        {
            g.setColour (violet);
            g.drawRoundedRectangle (r.translated (0.0f, 1.0f).reduced (1.0f), 2.0f, 1.0f);
        }
        g.setColour (on ? lilac : palette.ink);
        g.setFont (digitFont (10.5f));
        g.drawText (juce::String (targets[i]).replace ("-", U ("−")), r.translated (0.0f, on ? 1.0f : 0.0f), juce::Justification::centred);
    }
}

// ------------------------------------------------------------------ ModeSelector

ModeSelector::ModeSelector (const Palette& p, std::function<void (int)> onSelect) : palette (p), select (std::move (onSelect))
{
    setSize (8 * buttonW + 7 * gap, buttonH + 4);
    setWantsKeyboardFocus (true);
    setTitle ("Modo de limitación");
}

void ModeSelector::setMode (int m)
{
    if (m == mode)
        return;
    mode = m;
    repaint();
}

void ModeSelector::mouseDown (const juce::MouseEvent& e)
{
    const int i = e.x / (buttonW + gap);
    if (i >= 0 && i < 8 && e.x % (buttonW + gap) < buttonW)
        select (i);
}

bool ModeSelector::keyPressed (const juce::KeyPress& k)
{
    if (k.isKeyCode (juce::KeyPress::leftKey))
        select (juce::jmax (0, mode - 1));
    else if (k.isKeyCode (juce::KeyPress::rightKey))
        select (juce::jmin (7, mode + 1));
    else
        return false;
    return true;
}

void ModeSelector::paint (juce::Graphics& g)
{
    static const char* names[8] = { "MODERNO", "TRANSP.", "PUNCHY", "DINÁM.", "VERSÁTIL", "AGRES.", "BUS", "SEGURO" };
    for (int i = 0; i < 8; ++i)
    {
        const bool on = i == mode;
        auto r = juce::Rectangle<float> (static_cast<float> (i * (buttonW + gap)), 0.0f, static_cast<float> (buttonW), static_cast<float> (buttonH));
        if (on)
            r = r.translated (0.0f, 2.0f);
        g.setColour (juce::Colour (0xff160b26));
        g.fillRoundedRectangle (r.translated (0.0f, on ? 1.0f : 3.0f), 5.0f);
        g.setGradientFill (on ? juce::ColourGradient (juce::Colour (0xffcdbd96), 0.0f, r.getY(), juce::Colour (0xffb9a77c), 0.0f, r.getBottom(), false)
                              : juce::ColourGradient (juce::Colour (0xffe9dcc0), 0.0f, r.getY(), juce::Colour (0xffcbb98f), 0.0f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (juce::Colour (0xff1a0f2c));
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        lamp (g, { r.getCentreX(), r.getY() + 12.0f }, 7.0f, on, juce::Colour (0xffffb347));
        g.setColour (juce::Colour (0xff2a1a40));
        g.setFont (labelFont (9.0f));
        g.drawText (U (names[i]), r.withTrimmedTop (18.0f).reduced (1.0f, 0.0f), juce::Justification::centred);
    }
    if (hasKeyboardFocus (false))
    {
        g.setColour (focusRing);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 2.0f);
    }
}

// ------------------------------------------------------------------ LampToggle

LampToggle::LampToggle (juce::RangedAudioParameter& param, const Palette& p, juce::String text, juce::UndoManager* undo)
    : juce::Button (text), palette (p), caption (std::move (text)), attachment (param, *this, undo)
{
    setClickingTogglesState (true);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle (caption);
}

void LampToggle::paintButton (juce::Graphics& g, bool, bool)
{
    auto r = getLocalBounds().toFloat().withTrimmedBottom (2.0f);
    const bool on = getToggleState();
    if (cream)
    {
        const juce::Rectangle<float> cell = r;
        g.setColour (juce::Colour (0xff160b26));
        g.fillRoundedRectangle (cell.translated (0.0f, 2.0f), 4.0f);
        g.setGradientFill (on ? juce::ColourGradient (juce::Colour (0xffe9dcc0), 0.0f, cell.getY(), juce::Colour (0xffcbb98f), 0.0f, cell.getBottom(), false)
                              : juce::ColourGradient (palette.btnA, 0.0f, cell.getY(), palette.btnB, 0.0f, cell.getBottom(), false));
        g.fillRoundedRectangle (cell, 4.0f);
        g.setColour (palette.edge);
        g.drawRoundedRectangle (cell.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (on ? juce::Colour (0xff2a1a40) : palette.btnInk);
        drawSpaced (g, caption, cell, labelFont (11.0f, 0.2f), juce::Justification::centred);
    }
    else
    {
        panelButton (g, r, palette, 4.0f, on);
        lamp (g, { r.getX() + 15.0f, r.getCentreY() }, 9.0f, on, palette.lamp);
        g.setColour (palette.btnInk);
        g.setFont (labelFont (11.0f, 1.2f));
        g.drawFittedText (caption, r.withTrimmedLeft (28.0f).withTrimmedRight (6.0f).toNearestInt(), juce::Justification::centredLeft, 2, 0.9f);
    }
    if (hasKeyboardFocus (false))
    {
        g.setColour (focusRing);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 2.0f);
    }
}

// ------------------------------------------------------------------ ReadoutStrip

ReadoutStrip::ReadoutStrip (const Palette& p) : palette (p)
{
    setSize (544, 44);
    setTitle ("Lecturas de reducción");
}

void ReadoutStrip::setValues (float now, float max, int count)
{
    if (std::abs (now - grNow) < 0.05f && std::abs (max - grMax) < 0.05f && count == overs)
        return;
    grNow = now;
    grMax = max;
    overs = count;
    repaint();
}

void ReadoutStrip::setScaleName (juce::String name)
{
    scaleName = std::move (name);
    repaint();
}

juce::Rectangle<float> ReadoutStrip::cell (int i) const
{
    const float w = (544.0f - 24.0f) / 4.0f;
    return { static_cast<float> (i) * (w + 8.0f), 0.0f, w, 42.0f };
}

void ReadoutStrip::mouseDown (const juce::MouseEvent& e)
{
    if (cell (3).contains (e.position) && onScale)
        onScale();
}

void ReadoutStrip::paint (juce::Graphics& g)
{
    const juce::String labels[3] = { "GR ACTUAL", U ("GR MÁX"), "OVERS" };
    const juce::String texts[3] = { num (grNow, 1) + " dB", num (grMax, 1) + " dB", juce::String (overs) };
    for (int i = 0; i < 3; ++i)
    {
        const auto r = cell (i);
        g.setColour (palette.glass);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (juce::Colour (0xff05020a));
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawLine (r.getX() + 3.0f, r.getBottom() + 0.5f, r.getRight() - 3.0f, r.getBottom() + 0.5f);
        g.setColour (lilacDim);
        drawSpaced (g, labels[i], r.reduced (8.0f, 4.0f).withHeight (13.0f), labelFont (10.0f, 1.6f), juce::Justification::centredLeft);
        g.setColour (i == 2 && overs > 0 ? alert : lilac);
        g.setFont (digitFont (17.0f));
        g.drawText (texts[i], r.reduced (8.0f, 0.0f).withTrimmedTop (18.0f), juce::Justification::centredLeft);
    }

    const auto b = cell (3);
    panelButton (g, b, palette, 5.0f, false);
    g.setColour (creamDim);
    drawSpaced (g, "ESCALA", b.reduced (8.0f, 4.0f).withHeight (13.0f), labelFont (10.0f, 1.6f), juce::Justification::centredLeft);
    g.setColour (palette.ink);
    drawSpaced (g, scaleName, b.reduced (8.0f, 0.0f).withTrimmedTop (18.0f), labelFont (13.0f, 1.4f), juce::Justification::centredLeft);
}
} // namespace nel::ui
