#include "PluginEditor.h"

using namespace tufi::ui;

namespace
{
juce::RangedAudioParameter& get (TufiProcessor& p, const char* id)
{
    return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id));
}

/** Rótulo de grupo: texto centrado con un filete a cada lado. */
void drawGroupHeader (juce::Graphics& g, const juce::String& text, float x0, float x1, float y, bool rules = true)
{
    g.setFont (label (12.5f, 0.2f));
    const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), text) + 4.0f;
    const float cx = 0.5f * (x0 + x1);

    g.setColour (ink);
    g.drawText (text, juce::Rectangle<float> (cx - w * 0.5f, y - 9.0f, w, 18.0f), juce::Justification::centred);
    g.setColour (inkSoft.withAlpha (0.7f));
    if (! rules)
        return;
    if (cx - w * 0.5f - 8.0f > x0)
        g.drawLine (x0, y, cx - w * 0.5f - 8.0f, y, 1.0f);
    if (cx + w * 0.5f + 8.0f < x1)
        g.drawLine (cx + w * 0.5f + 8.0f, y, x1, y, 1.0f);
}

void drawScrew (juce::Graphics& g, juce::Point<float> c, float angle)
{
    juce::ColourGradient metal (juce::Colour (0xffd8cdb0), c.x - 3.0f, c.y - 3.0f, juce::Colour (0xff7d7058), c.x + 4.0f, c.y + 4.0f, true);
    g.setGradientFill (metal);
    g.fillEllipse (c.x - 6.0f, c.y - 6.0f, 12.0f, 12.0f);
    g.setColour (juce::Colour (0xff4a3f2e));
    g.drawEllipse (c.x - 6.0f, c.y - 6.0f, 12.0f, 12.0f, 1.0f);
    const float dx = std::cos (angle) * 4.2f, dy = std::sin (angle) * 4.2f;
    g.drawLine (c.x - dx, c.y - dy, c.x + dx, c.y + dy, 1.4f);
}
} // namespace

/** Chasis: nogal en los laterales, panel marfil, marco de latón, tornillos, cabecera y rótulos fijos. */
class TufiEditor::Chassis : public juce::Component
{
public:
    Chassis() { setInterceptsMouseClicks (false, false); }

    void paint (juce::Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        constexpr float side = 38.0f;

        // Laterales de nogal, con veta.
        for (bool left : { true, false })
        {
            const auto r = juce::Rectangle<float> (left ? 0.0f : all.getRight() - side, 0.0f, side, all.getHeight());
            juce::ColourGradient wood (walnut.brighter (0.12f), r.getX(), 0.0f, walnut.darker (0.45f), r.getRight(), 0.0f, false);
            if (! left)
                wood = juce::ColourGradient (walnut.darker (0.45f), r.getX(), 0.0f, walnut.brighter (0.12f), r.getRight(), 0.0f, false);
            g.setGradientFill (wood);
            g.fillRect (r);

            juce::Random rng (left ? 11 : 29);
            for (int i = 0; i < 46; ++i)
            {
                const float x = r.getX() + rng.nextFloat() * r.getWidth();
                g.setColour ((rng.nextBool() ? juce::Colours::black : juce::Colour (0xff8a5a36)).withAlpha (0.10f + rng.nextFloat() * 0.16f));
                juce::Path grain;
                grain.startNewSubPath (x, 0.0f);
                grain.cubicTo (x + rng.nextFloat() * 6.0f - 3.0f, all.getHeight() * 0.3f, x + rng.nextFloat() * 6.0f - 3.0f,
                               all.getHeight() * 0.7f, x + rng.nextFloat() * 4.0f - 2.0f, all.getHeight());
                g.strokePath (grain, juce::PathStrokeType (0.5f + rng.nextFloat() * 1.2f));
            }
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRect (left ? r.getRight() - 1.0f : r.getX(), 0.0f, 1.0f, all.getHeight());
        }

        // Panel marfil con un leve degradado.
        const auto panel = all.withTrimmedLeft (side).withTrimmedRight (side);
        juce::ColourGradient ivoryFill (ivory.brighter (0.06f), panel.getCentreX(), 0.0f, ivory.darker (0.07f), panel.getCentreX(), panel.getBottom(), false);
        g.setGradientFill (ivoryFill);
        g.fillRect (panel);

        // Marco de latón y tornillos.
        g.setColour (brass.withAlpha (0.75f));
        g.drawRoundedRectangle (panel.reduced (14.0f), 14.0f, 1.0f);
        drawScrew (g, { panel.getX() + 12.0f, 12.0f }, 0.5f);
        drawScrew (g, { panel.getRight() - 12.0f, 12.0f }, 2.2f);
        drawScrew (g, { panel.getX() + 12.0f, all.getBottom() - 12.0f }, 1.3f);
        drawScrew (g, { panel.getRight() - 12.0f, all.getBottom() - 12.0f }, 0.1f);

        // Nombre, placa del creador y subtítulo.
        g.setColour (ink);
        g.setFont (serif (72.0f));
        g.drawText ("Tufi", juce::Rectangle<float> (64.0f, 22.0f, 130.0f, 80.0f), juce::Justification::centredLeft);

        const juce::Rectangle<float> plate (172.0f, 53.0f, 97.0f, 22.0f);
        g.setColour (ivory.brighter (0.28f));
        g.fillRoundedRectangle (plate, 3.0f);
        g.setColour (brass);
        g.drawRoundedRectangle (plate.reduced (0.5f), 3.0f, 1.2f);
        g.setColour (inkSoft);
        g.setFont (label (12.0f, 0.28f));
        g.drawText ("BY FERB", plate, juce::Justification::centred);

        g.setColour (inkSoft);
        g.setFont (label (13.5f, 0.22f));
        g.drawText (juce::String::fromUTF8 ("SATURADOR ARMÓNICO · MODELO 1"), juce::Rectangle<float> (68.0f, 91.0f, 400.0f, 20.0f), juce::Justification::centredLeft);

        // Separador entre las teclas de etapa y las de opciones.
        g.setColour (inkSoft.withAlpha (0.45f));
        g.drawLine (816.0f, 59.0f, 816.0f, 102.0f, 1.0f);

        // Grupos de controles.
        drawGroupHeader (g, "NIVEL BAJO", 353.0f, 447.0f, 134.0f, false);
        drawGroupHeader (g, "PRESET", 353.0f, 747.0f, 505.0f);
        drawGroupHeader (g, "PICOS", 456.0f, 598.0f, 134.0f);
        drawGroupHeader (g, "PARES", 607.0f, 747.0f, 134.0f);
        drawGroupHeader (g, "GANANCIA", 353.0f, 747.0f, 280.0f);

        g.setColour (inkSoft);
        g.setFont (label (14.0f, 0.2f));
        g.drawText ("NIVEL DE SALIDA", juce::Rectangle<float> (762.0f, 278.0f, 248.0f, 20.0f), juce::Justification::centred);
    }
};

TufiEditor::TufiEditor (TufiProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight)
{
    backdrop = ivory;
    setLookAndFeel (&lookAndFeel);

    chassis = std::make_unique<Chassis>();
    chassis->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*chassis);

    namespace pr = tufi::params;

    // Selector de preset: en el hueco libre del centro, bajo las perillas de ganancia.
    {
        ferb::PresetBar::Style s;
        s.background = ivory.brighter (0.22f);
        s.border = brass;
        s.text = ink;
        s.textDim = inkSoft;
        s.accent = brass;
        s.font = label (14.0f, 0.04f);
        s.boldFont = label (14.0f, 0.04f);
        s.corner = 6.0f;
        presetBar = std::make_unique<ferb::PresetBar> (p.getPresets(), s);
        presetBar->setBounds (452, 528, 246, 34);
        content.addAndMakeVisible (*presetBar);
    }

    // Teclas: etapa (radio), sobremuestreo (recorre) y bypass (conmuta).
    const char* stageNames[] = { "TRIODO", "PENTODO", "DIODO" };
    const int stageX[] = { 494, 598, 702 };
    for (int i = 0; i < 3; ++i)
    {
        const juce::String name (stageNames[i]);
        keys[size_t (i)] = std::make_unique<KeyButton> (get (p, pr::stage), KeyButton::Kind::Radio, i, [name] (int) { return name; });
        keys[size_t (i)]->setBounds (stageX[i], 59 - KeyButton::lampHeight + 5, 96, 43 + KeyButton::lampHeight);
    }
    keys[3] = std::make_unique<KeyButton> (get (p, pr::oversample), KeyButton::Kind::Cycle, 0, [] (int v)
    {
        static const char* names[] = { "OS OFF", "2x OS", "4x OS", "8x OS" };
        return juce::String::fromUTF8 (names[juce::jlimit (0, 3, v)]);
    });
    keys[3]->setBounds (835, 59 - KeyButton::lampHeight + 5, 84, 43 + KeyButton::lampHeight);
    keys[4] = std::make_unique<KeyButton> (get (p, pr::bypass), KeyButton::Kind::Toggle, 0, [] (int) { return juce::String ("BYPASS"); });
    keys[4]->setBounds (927, 59 - KeyButton::lampHeight + 5, 84, 43 + KeyButton::lampHeight);
    for (auto& k : keys)
        content.addAndMakeVisible (*k);

    scope = std::make_unique<ScopeView> (p);
    scope->setBounds (70, 130, 270, 380);
    content.addAndMakeVisible (*scope);

    // Perillas: (parámetro, centro, diámetro, rótulo, distancias, formato).
    struct K { const char* id; int cx, cy, diameter; const char* caption; float capOff, valOff, capSize, valSize; int decimals; bool db; };
    const K specs[] = {
        { pr::lowAmount,    400, 190, 84,  "Cantidad", 49.0f, 64.0f, 13.0f, 13.0f, 1, false },
        { pr::peakAmount,   494, 180, 64,  "Cantidad", 39.0f, 54.0f, 12.0f, 11.5f, 1, false },
        { pr::peakHardness, 564, 180, 64,  "Dureza",   39.0f, 54.0f, 12.0f, 11.5f, 1, false },
        { pr::evenTop,      641, 180, 64,  "Arriba",   39.0f, 54.0f, 12.0f, 11.5f, 1, false },
        { pr::evenBottom,   712, 180, 64,  "Abajo",    39.0f, 54.0f, 12.0f, 11.5f, 1, false },
        { pr::drive,        451, 357, 118, "Entrada",  70.0f, 88.0f, 17.0f, 14.5f, 1, true },
        { pr::trim,         649, 357, 118, "Salida",   70.0f, 88.0f, 17.0f, 14.5f, 1, true },
    };

    for (size_t i = 0; i < std::size (specs); ++i)
    {
        const auto& k = specs[i];
        KnobUnit::Spec s;
        s.diameter = k.diameter;
        s.caption = k.caption;
        s.captionOffset = k.capOff;
        s.valueOffset = k.valOff;
        s.captionSize = k.capSize;
        s.valueSize = k.valSize;
        s.scaleSize = k.diameter > 100 ? 14.0f : (k.diameter > 70 ? 10.0f : 8.5f);
        const bool db = k.db;
        const int decimals = k.decimals;
        s.format = [db, decimals] (float v) { return db ? signedDb (v, decimals) : comma (v, decimals); };

        knobs[i] = std::make_unique<KnobUnit> (get (p, k.id), s);
        const int w = k.diameter + 56;
        const int h = int (float (k.diameter) * 0.5f + 2.0f + k.valOff + k.valSize + 6.0f);
        knobs[i]->setBounds (k.cx - w / 2, k.cy - k.diameter / 2 - 2, w, h);
        content.addAndMakeVisible (*knobs[i]);
    }

    vu = std::make_unique<VuMeter> (p);
    vu->setBounds (762, 130, 248, 140);
    content.addAndMakeVisible (*vu);

    tube = std::make_unique<TubeView> (p, [this] { return vu->getLevel(); });
    tube->setBounds (762, 305, 248, 263);
    content.addAndMakeVisible (*tube);
}

TufiEditor::~TufiEditor()
{
    setLookAndFeel (nullptr);
}
