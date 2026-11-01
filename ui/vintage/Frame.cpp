#include "Frame.h"

namespace ferb::vintage
{
namespace
{
const juce::Image& grain()
{
    static const juce::Image image = []
    {
        juce::Image img (juce::Image::ARGB, 160, 160, true);
        juce::Random rng (7);
        for (int y = 0; y < img.getHeight(); ++y)
            for (int x = 0; x < img.getWidth(); ++x)
                img.setPixelAt (x, y, juce::Colours::white.withAlpha (rng.nextFloat() * 0.07f));
        return img;
    }();
    return image;
}
} // namespace

// ------------------------------------------------------------------ Frame

Frame::Frame (const Palette& p, bool isRack, int ears) : palette (p), rack (isRack), earWidth (ears)
{
    setInterceptsMouseClicks (false, false);
    setBufferedToImage (true);
}

void Frame::paintScrew (juce::Graphics& g, juce::Point<float> c, float slotDegrees)
{
    const auto box = juce::Rectangle<float> (18.0f, 18.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (box.translated (0.0f, 1.0f));
    g.setGradientFill (svgRadial (box, 0.36f, 0.30f, 1.0f, { { 0.0f, juce::Colours::white }, { 0.45f, juce::Colour (0xffbfc4c0) }, { 1.0f, juce::Colour (0xff6b716d) } }));
    g.fillEllipse (box);

    juce::Graphics::ScopedSaveState state (g);
    g.addTransform (juce::AffineTransform::rotation (slotDegrees * juce::MathConstants<float>::pi / 180.0f, c.x, c.y));
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRect (c.x - 6.0f, c.y - 1.0f, 12.0f, 2.0f);
}

void Frame::paintEar (juce::Graphics& g, juce::Rectangle<float> r, bool left) const
{
    if (palette.woodEars)
    {
        g.setGradientFill (juce::ColourGradient (palette.earA, r.getX(), 0.0f, palette.earC, r.getRight(), 0.0f, false));
        g.fillRect (r);
        juce::ColourGradient wood (palette.earA, r.getX(), 0.0f, palette.earC, r.getRight(), 0.0f, false);
        wood.addColour (0.7, palette.earB);
        g.setGradientFill (wood);
        g.fillRect (r);
        // Vetas verticales.
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        for (float x = r.getX() + 1.0f; x < r.getRight(); x += 7.0f)
            g.fillRect (x, r.getY(), 2.0f, r.getHeight());
    }
    else
    {
        juce::ColourGradient metal (palette.earA, r.getX(), 0.0f, palette.earC, r.getRight(), 0.0f, false);
        metal.addColour (0.6, palette.earB);
        g.setGradientFill (metal);
        g.fillRect (r);
    }

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (left ? r.getRight() - 2.0f : r.getX(), r.getY(), 2.0f, r.getHeight());

    const float cx = r.getCentreX() - 0.0f;
    if (r.getWidth() < 30.0f)
    {
        // Oreja estrecha: tornillos pequeños.
        for (const float y : { r.getY() + 14.0f, r.getBottom() - 14.0f })
        {
            const auto box = juce::Rectangle<float> (10.0f, 10.0f).withCentre ({ cx, y });
            g.setGradientFill (svgRadial (box, 0.36f, 0.30f, 1.0f, { { 0.0f, juce::Colours::white }, { 0.45f, juce::Colour (0xffbfc4c0) }, { 1.0f, juce::Colour (0xff6b716d) } }));
            g.fillEllipse (box);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawLine (cx - 3.0f, y + 1.0f, cx + 3.0f, y - 1.0f, 1.2f);
        }
        return;
    }
    paintScrew (g, { cx, r.getY() + 23.0f }, left ? 20.0f : -35.0f);
    paintScrew (g, { cx, r.getBottom() - 23.0f }, left ? -50.0f : 15.0f);
}

void Frame::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float corner = rack ? 3.0f : 10.0f;

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (r, corner);
    r = r.reduced (1.0f);

    juce::ColourGradient face (palette.panelHi, 0.0f, r.getY(), palette.panelLo, 0.0f, r.getBottom(), false);
    face.addColour (0.45, palette.panel);
    g.setGradientFill (face);
    g.fillRoundedRectangle (r, corner);

    {
        juce::Graphics::ScopedSaveState state (g);
        juce::Path clip;
        clip.addRoundedRectangle (r, corner);
        g.reduceClipRegion (clip);

        g.setTiledImageFill (grain(), 0, 0, 1.0f);
        g.fillRect (r);

        if (rack)
        {
            // Brochado: líneas claras y oscuras de 1 px.
            for (float y = r.getY(); y < r.getBottom(); y += 2.0f)
            {
                g.setColour (juce::Colours::white.withAlpha (0.04f));
                g.fillRect (r.getX(), y, r.getWidth(), 1.0f);
                g.setColour (juce::Colours::black.withAlpha (0.045f));
                g.fillRect (r.getX(), y + 1.0f, r.getWidth(), 1.0f);
            }
            g.setColour (juce::Colours::white.withAlpha (0.35f));
            g.fillRect (r.getX(), r.getY(), r.getWidth(), 1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillRect (r.getX(), r.getBottom() - 2.0f, r.getWidth(), 2.0f);

            paintEar (g, r.withWidth (static_cast<float> (earWidth)), true);
            paintEar (g, r.withLeft (r.getRight() - static_cast<float> (earWidth)), false);
        }
    }
}

// ------------------------------------------------------------------ Header

/** Nombre del preset en una ventana de cristal; un clic abre el menú. */
class Header::PresetDisplay : public PresetBar
{
public:
    PresetDisplay (PresetManager& m, const Palette& p, bool numbered)
        : PresetBar (m, Style {}), palette (p), numberedNames (numbered)
    {
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().withTrimmedBottom (0.0f);
        g.setColour (palette.glass);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawLine (r.getX() + 4.0f, r.getBottom() - 0.5f, r.getRight() - 4.0f, r.getBottom() - 0.5f);

        auto name = manager.getCurrentName();
        if (name.isEmpty())
            name = "Sin preset";
        else if (numberedNames)
        {
            // «01 · RADIO SETENTAS»: el número es la posición entre los de fábrica; «U» para uno del usuario.
            const int index = manager.getFactoryNames().indexOf (name);
            name = (index >= 0 ? juce::String (index + 1).paddedLeft ('0', 2) : juce::String ("U")) + juce::String::fromUTF8 (" · ") + name.toUpperCase();
            if (manager.isModified())
                name += " *";
        }
        else if (manager.isModified())
            name += " *";
        g.setColour (palette.acc);
        g.setFont (digitFont (14.0f));
        g.drawText (name, r.reduced (12.0f, 0.0f), juce::Justification::centredLeft, true);
    }

    void mouseDown (const juce::MouseEvent&) override { showMenu(); }

    /** Guarda el estado actual: sobrescribe si el preset actual es del usuario; si no, pide un nombre. */
    void saveCurrent()
    {
        const auto current = manager.getCurrentName();
        if (current.isNotEmpty() && ! manager.isFactory (current) && manager.getUserNames().contains (current))
            manager.save (current);
        else
            askName (juce::String());
    }

private:
    Palette palette;
    bool numberedNames = false;
};

/** Selector A/B: la ranura activa va en crema. */
class Header::AbSwitch : public juce::Component
{
public:
    AbSwitch (const Palette& p, AbCompare& c) : palette (p), ab (c)
    {
        setSize (64, 30);
        ab.onChange = [this] { repaint(); };
        setTitle ("Comparar A y B");
    }

    ~AbSwitch() override { ab.onChange = nullptr; }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < 2; ++i)
        {
            const auto cell = juce::Rectangle<float> (static_cast<float> (i * 32), 0.0f, 32.0f, 28.0f);
            const bool on = ab.getActive() == i;
            juce::Path shape;
            shape.addRoundedRectangle (cell.getX(), cell.getY(), cell.getWidth(), cell.getHeight(), 4.0f, 4.0f, i == 0, i == 1, i == 0, i == 1);
            g.setColour (palette.edge);
            g.fillPath (shape, juce::AffineTransform::translation (0.0f, 2.0f));
            g.setGradientFill (on ? juce::ColourGradient (juce::Colour (0xffe9dcc0), 0.0f, 0.0f, juce::Colour (0xffcbb98f), 0.0f, 28.0f, false)
                                  : juce::ColourGradient (palette.btnA, 0.0f, 0.0f, palette.btnB, 0.0f, 28.0f, false));
            g.fillPath (shape);
            g.setColour (palette.edge);
            g.strokePath (shape, juce::PathStrokeType (1.0f));
            g.setColour (on ? juce::Colour (0xff2a1a40) : palette.btnInk);
            drawSpaced (g, i == 0 ? "A" : "B", cell, labelFont (11.0f, 1.2f), juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        // Mayús + clic copia la ranura activa a la otra.
        if (e.mods.isShiftDown())
            ab.copyActiveToOther();
        else
            ab.select (e.x < 32 ? 0 : 1);
    }

private:
    Palette palette;
    AbCompare& ab;
};

Header::Header (const Palette& p, Spec s, PresetManager& presets, AbCompare& abCompare, juce::UndoManager& undoManager,
                juce::RangedAudioParameter& bypass)
    : palette (p), spec (std::move (s)), ab (abCompare), undo (undoManager),
      previous (p, "Preset anterior", VButton::Icon::previous), next (p, "Preset siguiente", VButton::Icon::next),
      undoButton (p, "Deshacer", VButton::Icon::undo), redoButton (p, "Rehacer", VButton::Icon::redo),
      bypassButton (p, "BYPASS", VButton::Icon::none, true), saveButton (p, "GUARDAR"), copyAbButton (p, juce::String::fromUTF8 ("A→B")),
      bypassAttachment (bypass, bypassButton, &undoManager)
{
    display = std::make_unique<PresetDisplay> (presets, palette, spec.numberedPresets);
    display->setTitle ("Elegir preset");
    abSwitch = std::make_unique<AbSwitch> (palette, ab);

    previous.onClick = [&presets] { presets.loadRelative (-1); };
    next.onClick = [&presets] { presets.loadRelative (1); };
    undoButton.onClick = [this] { undo.undo(); };
    redoButton.onClick = [this] { undo.redo(); };
    bypassButton.setClickingTogglesState (true);

    saveButton.onClick = [this] { display->saveCurrent(); };
    for (auto* c : std::initializer_list<juce::Component*> { &previous, display.get(), &next, abSwitch.get(), &undoButton, &redoButton, &bypassButton })
        addAndMakeVisible (*c);
    if (spec.saveButton)
        addAndMakeVisible (saveButton);
    if (spec.copyAbButton)
    {
        copyAbButton.onClick = [this] { ab.copyActiveToOther(); };
        addAndMakeVisible (copyAbButton);
    }
}

Header::~Header() = default;

void Header::resized()
{
    const int bh = spec.buttonHeight;
    int x = spec.presetBarX;
    previous.setBounds (x, 10, bh - 2, bh);       x += bh - 2 + 6;
    display->setBounds (x, 10, spec.displayWidth, bh - 2); x += spec.displayWidth + 6;
    next.setBounds (x, 10, bh - 2, bh);           x += bh - 2 + 6;
    if (spec.saveButton)
    {
        saveButton.setBounds (x, 10, 96, bh);
        x += 96 + 6;
    }
    abSwitch->setTopLeftPosition (x, 12);         x += 64 + 4;
    if (spec.copyAbButton)
    {
        copyAbButton.setBounds (x, 10, 58, bh);
        x += 58 + 4;
    }
    x += 4;
    undoButton.setBounds (x, 10, bh - 2, bh);     x += bh - 2 + 4;
    redoButton.setBounds (x, 10, bh - 2, bh);
    bypassButton.setBounds (spec.width - (spec.rack ? 60 : 14) - 90, 10, 90, bh);
}

void Header::paint (juce::Graphics& g)
{
    const float left = spec.rack ? 62.0f : 18.0f;

    // Logotipo con relieve.
    const auto logo = spec.italicLogo ? juce::Font (juce::FontOptions ("Georgia", spec.nameSize * 0.95f, juce::Font::bold | juce::Font::italic)) : logoFont (spec.nameSize);
    const float nameW = juce::GlyphArrangement::getStringWidth (logo, spec.name) + 2.0f;
    drawEmbossed (g, spec.name, { left, spec.rack ? 6.0f : 4.0f, nameW, spec.rack ? 40.0f : 42.0f }, logo, juce::Justification::centredLeft, palette);

    // Placa «BY FERB».
    const auto plate = juce::Rectangle<float> (left + nameW + 12.0f, 16.0f, 75.0f, 21.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9dcc0), 0.0f, plate.getY(), juce::Colour (0xffcbb98f), 0.0f, plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 3.0f);
    g.setColour (brass);
    g.drawRoundedRectangle (plate.reduced (0.75f), 3.0f, 1.5f);
    g.setColour (juce::Colour (0xff2a1a40));
    drawSpaced (g, "BY FERB", plate, labelFont (10.0f, 2.0f), juce::Justification::centred);

    g.setColour (palette.ink2);
    drawSpaced (g, spec.subtitle, { left + 2.0f, spec.rack ? 41.0f : 44.0f, 420.0f, 12.0f }, labelFont (spec.rack ? 9.0f : 10.0f, 2.2f), juce::Justification::centredLeft);

    if (spec.rack)
    {
        // Filete fino al pie de la cabecera.
        juce::ColourGradient line (juce::Colours::transparentBlack, 56.0f, 0.0f, juce::Colours::transparentBlack, static_cast<float> (spec.width - 56), 0.0f, false);
        line.addColour (0.5, palette.rule);
        g.setGradientFill (line);
        g.fillRect (56.0f, 54.0f, static_cast<float> (spec.width - 112), 2.0f);
    }
    else
    {
        const auto line = spec.lineColour.value_or (palette.acc);
        const float y = static_cast<float> (spec.height) - 2.0f;
        g.setColour (line.withAlpha (0.25f));
        g.fillRect (0.0f, y - 2.0f, static_cast<float> (spec.width), 5.0f);
        g.setColour (line);
        g.fillRect (0.0f, y, static_cast<float> (spec.width), 2.0f);
    }
}
} // namespace ferb::vintage
