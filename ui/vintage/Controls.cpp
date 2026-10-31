#include "Controls.h"

namespace ferb::vintage
{
namespace
{
constexpr float degToRad = juce::MathConstants<float>::pi / 180.0f;

void tickLine (juce::Graphics& g, float angleDeg, float r1, float r2, float width)
{
    const float a = angleDeg * degToRad;
    g.drawLine (50.0f + r1 * std::cos (a), 50.0f + r1 * std::sin (a), 50.0f + r2 * std::cos (a), 50.0f + r2 * std::sin (a), width);
}

juce::Path roundCap (float x1, float y1, float x2, float y2) { juce::Path p; p.startNewSubPath (x1, y1); p.lineTo (x2, y2); return p; }
} // namespace

void drawEmbossed (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, const juce::Font& font,
                   juce::Justification j, const Palette& p, float alpha)
{
    if (! p.emboss.isTransparent())
    {
        g.setColour (p.emboss.withMultipliedAlpha (alpha));
        drawSpaced (g, text, area.translated (0.0f, 1.0f), font, j);
    }
    g.setColour (p.ink.withMultipliedAlpha (alpha));
    drawSpaced (g, text, area, font, j);
}

// ------------------------------------------------------------------ ValuePill

ValuePill::ValuePill (juce::RangedAudioParameter& parameter, Format format, Parse parse, const Palette& p,
                      float fontHeight, float padding, float corner)
    : ParamField (parameter, std::move (format), std::move (parse),
                  [&]
                  {
                      Style s;
                      s.font = digitFont (fontHeight);
                      s.text = p.acc;
                      s.editText = p.acc;
                      s.editBackground = p.glass;
                      s.editOutline = p.acc;
                      s.justification = juce::Justification::centred;
                      return s;
                  }()),
      glass (p.glass), pad (padding), cornerSize (corner)
{
    setBorderSize ({ 0, 0, 0, 0 });
}

void ValuePill::paint (juce::Graphics& g)
{
    if (! isBeingEdited())
    {
        const float w = std::min (static_cast<float> (getWidth()), juce::GlyphArrangement::getStringWidth (getFont(), getText()) + 2.0f * pad);
        const auto r = juce::Rectangle<float> (w, static_cast<float> (getHeight())).withCentre (getLocalBounds().toFloat().getCentre());
        g.setColour (glass);
        g.fillRoundedRectangle (r, cornerSize);
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawRoundedRectangle (r.reduced (0.5f), cornerSize, 1.0f);
    }
    ParamField::paint (g);
}

// ------------------------------------------------------------------ VKnob

class VKnob::AccessibleValue : public juce::AccessibilityValueInterface
{
public:
    explicit AccessibleValue (VKnob& k) : knob (k) {}
    bool isReadOnly() const override { return false; }
    double getCurrentValue() const override { return knob.param.convertFrom0to1 (knob.normalised); }
    void setValue (double v) override { knob.setNormalised (knob.param.convertTo0to1 (static_cast<float> (v))); }
    juce::String getCurrentValueAsString() const override { return knob.spec.format (knob.param.convertFrom0to1 (knob.normalised)); }
    void setValueAsString (const juce::String& s) override
    {
        const float current = knob.param.convertFrom0to1 (knob.normalised);
        knob.setNormalised (knob.param.convertTo0to1 (knob.spec.parse (s, current)));
    }
    AccessibleValueRange getRange() const override
    {
        const auto r = knob.param.getNormalisableRange();
        return { { static_cast<double> (r.start), static_cast<double> (r.end) }, static_cast<double> (r.interval) };
    }

private:
    VKnob& knob;
};

VKnob::VKnob (juce::RangedAudioParameter& parameter, const Palette& p, Spec s, juce::UndoManager* undo)
    : param (parameter), palette (p), spec (std::move (s)),
      attachment (parameter, [this] (float v) { normalised = param.convertTo0to1 (v); repaint(); }, undo)
{
    setWantsKeyboardFocus (true);
    setMouseClickGrabsKeyboardFocus (true);
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
    setTitle (spec.label);

    pill = std::make_unique<ValuePill> (parameter, spec.format, spec.parse, palette, spec.big ? 14.0f : 12.0f, spec.big ? 9.0f : 6.0f);
    addAndMakeVisible (*pill);
    attachment.sendInitialUpdate();
}

int VKnob::heightFor (const Spec& s) noexcept
{
    return s.size + 2 + (s.big ? 16 : 13) + 2 + (s.big ? 20 : 15);
}

juce::Rectangle<float> VKnob::dialArea() const noexcept
{
    const float size = static_cast<float> (spec.size);
    return { (static_cast<float> (getWidth()) - size) * 0.5f, 0.0f, size, size };
}

void VKnob::resized()
{
    const int labelH = spec.big ? 16 : 13, pillH = spec.big ? 20 : 15;
    pill->setBounds (0, spec.size + 2 + labelH + 2, getWidth(), pillH);
}

void VKnob::paint (juce::Graphics& g)
{
    const auto dial = dialArea();
    {
        juce::Graphics::ScopedSaveState state (g);
        const float scale = dial.getWidth() / 100.0f;
        g.addTransform (juce::AffineTransform::scale (scale).translated (dial.getX(), dial.getY()));

        // Marcas grabadas alrededor.
        g.setColour (palette.ink.withAlpha (0.75f));
        for (int i = 0; i <= 10; ++i)
            tickLine (g, 135.0f + 27.0f * static_cast<float> (i), spec.big ? 42.0f : 43.0f, i % 5 == 0 ? 49.0f : 46.5f, i % 5 == 0 ? 2.2f : 1.4f);

        if (spec.size >= 70)
        {
            g.setColour (palette.ink.withAlpha (0.8f));
            g.setFont (digitFont (9.0f));
            g.drawText (spec.endLow, juce::Rectangle<float> (2.0f, 91.0f, 30.0f, 10.0f), juce::Justification::centred);
            g.drawText (spec.endHigh, juce::Rectangle<float> (68.0f, 91.0f, 30.0f, 10.0f), juce::Justification::centred);
        }

        // Sombra y bisel.
        const float bodyR = spec.big ? 36.0f : 31.0f;
        g.setColour (juce::Colours::black.withAlpha (0.38f));
        g.fillEllipse (53.0f - bodyR, 54.0f - bodyR, 2 * bodyR, 2 * bodyR);

        if (spec.big)
        {
            juce::ColourGradient brassGradient (juce::Colour (0xfff3d98f), 14.0f, 14.0f, juce::Colour (0xff6f5120), 86.0f, 86.0f, false);
            brassGradient.addColour (0.5, juce::Colour (0xffb98f43));
            g.setGradientFill (brassGradient);
            g.fillEllipse (14.0f, 14.0f, 72.0f, 72.0f);
            g.setColour (juce::Colour (0xff2c1f0c));
            g.drawEllipse (14.0f, 14.0f, 72.0f, 72.0f, 0.8f);
            g.setColour (juce::Colour (0xff1a1420));
            g.fillEllipse (17.0f, 17.0f, 66.0f, 66.0f);
        }

        const bool dark = palette.darkKnobs;
        const juce::Rectangle<float> body (19.0f, 19.0f, 62.0f, 62.0f);
        g.setGradientFill (dark ? svgRadial (body, 0.38f, 0.32f, 0.8f, { { 0.0f, juce::Colour (0xff6b6157) }, { 0.55f, juce::Colour (0xff2f2822) }, { 1.0f, juce::Colour (0xff120e0b) } })
                                : svgRadial (body, 0.38f, 0.32f, 0.8f, { { 0.0f, juce::Colour (0xfffbf4e1) }, { 0.55f, juce::Colour (0xffe6d9b8) }, { 1.0f, juce::Colour (0xffb9a77f) } }));
        g.fillEllipse (body);
        g.setColour (dark ? juce::Colours::black : juce::Colour (0xff8d7b57));
        g.drawEllipse (body, 1.0f);

        // Anillo estriado.
        {
            juce::Path ring;
            ring.addEllipse (20.0f, 20.0f, 60.0f, 60.0f);
            juce::Path dashed;
            const float dashes[] = { 1.2f, 1.8f };
            juce::PathStrokeType (2.6f).createDashedStroke (dashed, ring, dashes, 2);
            g.setColour ((dark ? juce::Colour (0xff8a7d6e) : juce::Colour (0xffa89873)).withAlpha (0.7f));
            g.fillPath (dashed);
        }

        const juce::Rectangle<float> cap (28.0f, 28.0f, 44.0f, 44.0f);
        g.setGradientFill (dark ? svgRadial (cap, 0.40f, 0.34f, 0.75f, { { 0.0f, juce::Colour (0xff7c7165) }, { 1.0f, juce::Colour (0xff231c17) } })
                                : svgRadial (cap, 0.40f, 0.34f, 0.75f, { { 0.0f, juce::Colour (0xfffffaf0) }, { 1.0f, juce::Colour (0xffd9caa4) } }));
        g.fillEllipse (cap);
        g.setColour (dark ? juce::Colours::black : juce::Colour (0xffb8a780));
        g.drawEllipse (cap, 0.8f);

        // Puntero.
        g.addTransform (juce::AffineTransform::rotation ((-135.0f + 270.0f * normalised) * degToRad, 50.0f, 50.0f));
        g.setColour (dark ? juce::Colour (0xfff3e6c8) : juce::Colour (0xff2a1a40));
        g.strokePath (roundCap (50.0f, 40.0f, 50.0f, 19.0f), juce::PathStrokeType (4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xffffb347));
        g.fillEllipse (47.8f, 14.3f, 4.4f, 4.4f);
    }

    if (showFocus)
    {
        g.setColour (focusRing);
        g.drawEllipse (dial.expanded (3.0f), 2.0f);
    }

    const float labelY = static_cast<float> (spec.size) + 2.0f;
    drawEmbossed (g, spec.label, { 0.0f, labelY, static_cast<float> (getWidth()), spec.big ? 16.0f : 13.0f },
                  labelFont (spec.big ? 13.0f : 10.5f, spec.big ? 2.4f : 1.0f), juce::Justification::centred, palette);
}

void VKnob::setNormalised (float n)
{
    attachment.setValueAsPartOfGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, n)));
}

void VKnob::nudge (float delta)
{
    attachment.setValueAsCompleteGesture (param.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, normalised + delta)));
}

void VKnob::mouseDown (const juce::MouseEvent& e)
{
    if (! dialArea().expanded (4.0f).contains (e.position))
        return;
    dragStartNorm = normalised;
    attachment.beginGesture();
    showFocus = false;
}

void VKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (e.mouseWasDraggedSinceMouseDown() && ! dialArea().expanded (4.0f).contains (e.getMouseDownPosition().toFloat()))
        return;
    const float distance = static_cast<float> (e.getDistanceFromDragStartY());
    setNormalised (dragStartNorm - distance / 190.0f * (e.mods.isShiftDown() ? 0.25f : 1.0f));
}

void VKnob::mouseUp (const juce::MouseEvent& e)
{
    if (dialArea().expanded (4.0f).contains (e.getMouseDownPosition().toFloat()))
        attachment.endGesture();
}

void VKnob::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (dialArea().expanded (4.0f).contains (e.position))
        attachment.setValueAsCompleteGesture (param.convertFrom0to1 (param.getDefaultValue()));
}

void VKnob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& d)
{
    if (d.deltaY == 0.0f)
        return;
    nudge ((d.deltaY > 0.0f ? 1.0f : -1.0f) * (e.mods.isShiftDown() ? 0.005f : 0.02f));
}

bool VKnob::keyPressed (const juce::KeyPress& k)
{
    const float step = k.getModifiers().isShiftDown() ? 0.1f : 0.01f;
    const auto code = k.getKeyCode();
    if (code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey) nudge (step);
    else if (code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey) nudge (-step);
    else if (code == juce::KeyPress::pageUpKey) nudge (0.1f);
    else if (code == juce::KeyPress::pageDownKey) nudge (-0.1f);
    else if (code == juce::KeyPress::homeKey) attachment.setValueAsCompleteGesture (param.convertFrom0to1 (0.0f));
    else if (code == juce::KeyPress::endKey) attachment.setValueAsCompleteGesture (param.convertFrom0to1 (1.0f));
    else return false;
    return true;
}

void VKnob::focusGained (FocusChangeType cause)
{
    showFocus = cause == focusChangedByTabKey;
    repaint();
}

void VKnob::focusLost (FocusChangeType)
{
    showFocus = false;
    repaint();
}

std::unique_ptr<juce::AccessibilityHandler> VKnob::createAccessibilityHandler()
{
    return std::make_unique<juce::AccessibilityHandler> (*this, juce::AccessibilityRole::slider,
                                                         juce::AccessibilityActions(), juce::AccessibilityHandler::Interfaces { std::make_unique<AccessibleValue> (*this) });
}

// ------------------------------------------------------------------ VLever

VLever::VLever (juce::RangedAudioParameter& parameter, const Palette& p, Spec s, juce::UndoManager* undo)
    : juce::Button (s.name), palette (p), spec (std::move (s)), attachment (parameter, *this, undo)
{
    setClickingTogglesState (true);
    setSize (width, height);
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle (spec.name);
}

void VLever::paintButton (juce::Graphics& g, bool, bool)
{
    const bool up = spec.invert ? ! getToggleState() : getToggleState();
    const float w = static_cast<float> (width);

    drawEmbossed (g, spec.top, { 0.0f, 0.0f, w, 11.0f }, labelFont (9.0f, 1.2f), juce::Justification::centred, palette, up ? 1.0f : 0.55f);
    drawEmbossed (g, spec.bottom, { 0.0f, 74.0f, w, 11.0f }, labelFont (9.0f, 1.2f), juce::Justification::centred, palette, up ? 0.55f : 1.0f);
    drawEmbossed (g, spec.name, { 0.0f, 88.0f, w, 14.0f }, labelFont (10.5f, 1.8f), juce::Justification::centred, palette);

    {
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (62.0f / 64.0f).translated ((w - 40.0f * 62.0f / 64.0f) * 0.5f, 12.0f));

        const juce::Rectangle<float> nut (3.0f, 15.0f, 34.0f, 34.0f);
        g.setGradientFill (svgRadial (nut, 0.38f, 0.32f, 0.8f, { { 0.0f, juce::Colours::white }, { 0.5f, juce::Colour (0xffb9bebb) }, { 1.0f, juce::Colour (0xff5a5f5c) } }));
        g.fillEllipse (nut);
        g.setColour (juce::Colour (0xff3b403d));
        g.drawEllipse (nut, 1.0f);
        g.setColour (juce::Colour (0xff2a2f2c));
        g.fillEllipse (9.0f, 21.0f, 22.0f, 22.0f);
        g.setColour (juce::Colour (0xff8d9390));
        g.drawEllipse (9.0f, 21.0f, 22.0f, 22.0f, 1.0f);

        if (! up)
            g.addTransform (juce::AffineTransform::rotation (juce::MathConstants<float>::pi, 20.0f, 32.0f));

        juce::ColourGradient chrome (juce::Colour (0xff6b716d), 16.5f, 0.0f, juce::Colour (0xff555a57), 23.5f, 0.0f, false);
        chrome.addColour (0.35, juce::Colour (0xfffafbfa));
        chrome.addColour (0.6, juce::Colour (0xffb6bbb8));
        g.setGradientFill (chrome);
        g.fillRoundedRectangle (16.5f, 6.0f, 7.0f, 26.0f, 3.5f);
        g.setColour (juce::Colour (0xff404544));
        g.drawRoundedRectangle (16.5f, 6.0f, 7.0f, 26.0f, 3.5f, 0.6f);

        const juce::Rectangle<float> tip (14.5f, 2.5f, 11.0f, 11.0f);
        g.setGradientFill (svgRadial (tip, 0.38f, 0.32f, 0.8f, { { 0.0f, juce::Colours::white }, { 0.5f, juce::Colour (0xffb9bebb) }, { 1.0f, juce::Colour (0xff5a5f5c) } }));
        g.fillEllipse (tip);
        g.setColour (juce::Colour (0xff3b403d));
        g.drawEllipse (tip, 0.8f);
    }

    if (hasKeyboardFocus (false))
    {
        g.setColour (focusRing);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 2.0f);
    }
}

// ------------------------------------------------------------------ VSeg

VSeg::VSeg (juce::RangedAudioParameter& parameter, const Palette& p, juce::StringArray labels, int buttonWidth, juce::UndoManager* undo)
    : palette (p), items (std::move (labels)), buttonW (buttonWidth),
      attachment (parameter, [this] (float v) { selected = juce::roundToInt (v); repaint(); }, undo)
{
    setSize (buttonW * items.size(), 30);
    setWantsKeyboardFocus (true);
    setTitle (parameter.getName (64));
    attachment.sendInitialUpdate();
}

void VSeg::paint (juce::Graphics& g)
{
    for (int i = 0; i < items.size(); ++i)
    {
        const auto cell = juce::Rectangle<float> (static_cast<float> (i * buttonW), 0.0f, static_cast<float> (buttonW), 28.0f);
        const bool on = i == selected;
        const float corner = 4.0f;

        // Esquinas redondeadas solo en los extremos de la fila.
        juce::Path shape;
        shape.addRoundedRectangle (cell.getX(), cell.getY(), cell.getWidth(), cell.getHeight(), corner, corner, i == 0, i == items.size() - 1, i == 0, i == items.size() - 1);

        g.setColour (palette.edge);
        g.fillPath (shape, juce::AffineTransform::translation (0.0f, 2.0f));
        g.setGradientFill (on ? juce::ColourGradient (juce::Colour (0xffe9dcc0), 0.0f, cell.getY(), juce::Colour (0xffcbb98f), 0.0f, cell.getBottom(), false)
                              : juce::ColourGradient (palette.btnA, 0.0f, cell.getY(), palette.btnB, 0.0f, cell.getBottom(), false));
        g.fillPath (shape);
        g.setColour (palette.edge);
        g.strokePath (shape, juce::PathStrokeType (1.0f));
        if (! on)
        {
            g.setColour (juce::Colours::white.withAlpha (0.18f));
            g.drawLine (cell.getX() + 2.0f, cell.getY() + 1.5f, cell.getRight() - 2.0f, cell.getY() + 1.5f);
        }
        g.setColour (on ? juce::Colour (0xff2a1a40) : palette.btnInk);
        drawSpaced (g, items[i], cell, labelFont (10.5f, 1.2f), juce::Justification::centred);
    }

    if (hasKeyboardFocus (false))
    {
        g.setColour (focusRing);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 4.0f, 2.0f);
    }
}

void VSeg::mouseDown (const juce::MouseEvent& e)
{
    const int i = juce::jlimit (0, items.size() - 1, e.x / buttonW);
    attachment.setValueAsCompleteGesture (static_cast<float> (i));
}

bool VSeg::keyPressed (const juce::KeyPress& k)
{
    int next = selected;
    if (k.isKeyCode (juce::KeyPress::leftKey) || k.isKeyCode (juce::KeyPress::upKey)) next = selected - 1;
    else if (k.isKeyCode (juce::KeyPress::rightKey) || k.isKeyCode (juce::KeyPress::downKey)) next = selected + 1;
    else return false;
    attachment.setValueAsCompleteGesture (static_cast<float> (juce::jlimit (0, items.size() - 1, next)));
    return true;
}

// ------------------------------------------------------------------ VButton

VButton::VButton (const Palette& p, juce::String text, Icon i, bool lamp)
    : juce::Button (text), palette (p), caption (std::move (text)), icon (i), hasLamp (lamp)
{
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle (caption);
}

void VButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat().withTrimmedBottom (2.0f);
    const bool pressedState = getToggleState();
    if (down)
        r = r.translated (0.0f, 1.0f);

    g.setColour (palette.edge);
    g.fillRoundedRectangle (r.translated (0.0f, down ? 1.0f : 2.0f), 5.0f);
    const auto top = pressedState ? palette.btnB : palette.btnA, bottom = pressedState ? palette.btnB.darker (0.5f) : palette.btnB;
    g.setGradientFill (juce::ColourGradient (highlighted ? top.brighter (0.1f) : top, 0.0f, r.getY(), bottom, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (palette.edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.18f));
    g.drawLine (r.getX() + 3.0f, r.getY() + 1.5f, r.getRight() - 3.0f, r.getY() + 1.5f);

    g.setColour (palette.btnInk);
    if (icon != Icon::none)
    {
        const auto c = r.getCentre();
        juce::Path p;
        if (icon == Icon::previous) { p.startNewSubPath (c.x + 2.5f, c.y - 5.0f); p.lineTo (c.x - 2.5f, c.y); p.lineTo (c.x + 2.5f, c.y + 5.0f); }
        else if (icon == Icon::next) { p.startNewSubPath (c.x - 2.5f, c.y - 5.0f); p.lineTo (c.x + 2.5f, c.y); p.lineTo (c.x - 2.5f, c.y + 5.0f); }
        else
        {
            // Flecha curva: deshacer gira a la izquierda; rehacer es su espejo.
            const float s = icon == Icon::undo ? 1.0f : -1.0f;
            p.startNewSubPath (c.x - 5.0f * s, c.y - 1.0f);
            p.lineTo (c.x - 5.0f * s, c.y - 5.0f);
            p.startNewSubPath (c.x - 5.0f * s, c.y - 1.0f);
            p.lineTo (c.x - 1.0f * s, c.y - 1.0f);
            p.startNewSubPath (c.x - 5.0f * s, c.y - 1.0f);
            p.cubicTo (c.x - 3.0f * s, c.y - 5.0f, c.x + 5.0f * s, c.y - 5.0f, c.x + 5.0f * s, c.y + 3.0f);
        }
        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    else
    {
        auto text = r.reduced (4.0f, 0.0f);
        if (hasLamp)
        {
            const auto lampBox = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ r.getX() + 15.0f, r.getCentreY() });
            const bool lit = getToggleState();
            if (lit)
            {
                g.setColour (palette.lamp.withAlpha (0.35f));
                g.fillEllipse (lampBox.expanded (3.0f));
            }
            g.setColour (lit ? palette.lamp : juce::Colour (0xff3a2a22));
            g.fillEllipse (lampBox);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.drawEllipse (lampBox, 1.0f);
            text.removeFromLeft (14.0f);
        }
        drawSpaced (g, caption, text, labelFont (11.0f, 1.2f), juce::Justification::centred);
    }

    if (hasKeyboardFocus (false))
    {
        g.setColour (focusRing);
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.0f), 5.0f, 2.0f);
    }
}

// ------------------------------------------------------------------ ReadoutWindow

ReadoutWindow::ReadoutWindow (const Palette& p, juce::String cap) : palette (p), caption (std::move (cap))
{
    setInterceptsMouseClicks (false, false);
}

void ReadoutWindow::setText (const juce::String& t, std::optional<juce::Colour> colour)
{
    if (t == text && colour == colourOverride)
        return;
    text = t;
    colourOverride = colour;
    repaint();
}

void ReadoutWindow::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (3.0f);
    g.setColour (juce::Colour (0xff1a1a1a));
    g.fillRoundedRectangle (r.expanded (2.0f), 6.0f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawRoundedRectangle (r.expanded (2.0f).translated (0.0f, 1.0f), 6.0f, 1.0f);
    g.setColour (palette.glass);
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 2.0f);

    g.setColour (palette.ink2);
    drawSpaced (g, caption, r.reduced (8.0f, 3.0f).withHeight (10.0f), labelFont (8.5f, 1.4f), juce::Justification::centredLeft);

    const auto colour = colourOverride.value_or (palette.acc);
    const auto digits = r.reduced (8.0f, 0.0f).withTrimmedTop (13.0f).withHeight (18.0f);
    g.setColour (colour.withAlpha (0.18f));
    g.setFont (digitFont (16.0f));
    g.drawText (text, digits.translated (0.0f, 0.0f).expanded (0.5f), juce::Justification::centredRight);
    g.setColour (colour);
    g.drawText (text, digits, juce::Justification::centredRight);
}

// ------------------------------------------------------------------ GlassView

void GlassView::paint (juce::Graphics& g)
{
    const auto inner = getLocalBounds().toFloat().reduced (static_cast<float> (margin));
    paintGlassBackground (g, inner, palette);
    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (inner, 7.0f);
        g.reduceClipRegion (clip);
        g.addTransform (juce::AffineTransform::translation (inner.getX(), inner.getY()));
        paintContent (g, inner.getWidth(), inner.getHeight());
    }
    paintGlassOverlay (g, inner);
}
} // namespace ferb::vintage
