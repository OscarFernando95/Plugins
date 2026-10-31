#include "PluginEditor.h"

#include <Format.h>

using namespace ferb::vintage;
namespace k = mcpoti::params;

namespace
{
juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

std::function<juce::String (float)> percent()
{
    return [] (float v) { return ferb::fmt::comma (v, 0) + " %"; };
}

std::function<juce::String (float)> plain()
{
    return [] (float v) { return ferb::fmt::comma (v, 0); };
}

std::function<juce::String (float)> signedInt()
{
    return [] (float v) { return (v > 0.0f ? juce::String ("+") : juce::String()) + ferb::fmt::comma (v, 0); };
}

std::function<juce::String (float)> db()
{
    return [] (float v) { return ferb::fmt::comma (v, 1) + " dB"; };
}

std::function<juce::String (float)> hz()
{
    return [] (float v) { return v >= 1000.0f ? ferb::fmt::comma (v / 1000.0f, 1) + " kHz" : ferb::fmt::comma (v, 0) + " Hz"; };
}
} // namespace

/** Válvula de vidrio: el filamento brilla con lo que añade su banda (−60 a −20 dB de RMS absoluto; escala inicial). */
class DraMcPotiEditor::Tube : public juce::Component
{
public:
    void setGlow (float g)
    {
        if (std::abs (g - glow) > 0.01f)
        {
            glow = g;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0xff1b1410));
        g.fillRoundedRectangle (r, r.getWidth() * 0.5f);
        const auto amber = juce::Colour (0xffffa030);
        g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.15f + 0.8f * glow), r.getCentreX(), r.getBottom() - 4.0f,
                                                 amber.withAlpha (0.0f), r.getCentreX(), r.getY(), false));
        g.fillRoundedRectangle (r.reduced (2.0f), r.getWidth() * 0.5f);
        g.setColour (juce::Colours::white.withAlpha (0.15f));
        g.drawRoundedRectangle (r, r.getWidth() * 0.5f, 1.2f);
    }

private:
    float glow = 0.0f;
};

DraMcPotiEditor::DraMcPotiEditor (DraMcPotiProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75), owner (p)
{
    auto param = [&p] (const juce::String& id) -> juce::RangedAudioParameter& { return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id)); };

    frame = std::make_unique<Frame> (palette, true, 18);
    frame->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*frame);

    Header::Spec headerSpec;
    headerSpec.name = "Dra. McPoti";
    headerSpec.subtitle = U ("EXCITADOR ARMÓNICO");
    headerSpec.width = baseWidth;
    headerSpec.height = 64;
    header = std::make_unique<Header> (palette, headerSpec, p.getPresets(), p.getAb(), p.undoManager, param (k::bypass));
    header->setBounds (0, 0, baseWidth, 64);
    content.addAndMakeVisible (*header);

    auto seg = [&] (const char* id, juce::StringArray labels, int buttonWidth, int x, int y)
    {
        auto s = std::make_unique<VSeg> (param (id), palette, std::move (labels), buttonWidth, &p.undoManager);
        s->setTopLeftPosition (x, y);
        content.addAndMakeVisible (*s);
        segments.push_back (std::move (s));
    };
    seg (k::mode, { "VOCAL", "MASTER" }, 78, 30, 84);
    seg (k::era, { "VINTAGE", "MODERNO" }, 78, 30, 124);
    seg (k::voice, { "1", "2", "3", "4" }, 39, 30, 164);

    voiceName.setBounds (30, 192, 160, 16);
    voiceName.setJustificationType (juce::Justification::centredLeft);
    voiceName.setFont (juce::FontOptions (12.0f));
    voiceName.setColour (juce::Label::textColourId, juce::Colour (0xff2b2118));
    content.addAndMakeVisible (voiceName);

    addKnob (k::input, "ENTRADA", 56, false, 24, 214, 78, db());
    addKnob (k::mix, "MEZCLA", 56, false, 90, 214, 78, percent());
    addKnob (k::output, "SALIDA", 56, false, 156, 214, 78, db());
    vuIn = std::make_unique<VuNeedle> (palette, "ENTRADA");
    vuIn->setFaceBounds ({ 32, 322, 96, 62 });
    content.addAndMakeVisible (*vuIn);
    vuOut = std::make_unique<VuNeedle> (palette, "SALIDA");
    vuOut->setFaceBounds ({ 136, 322, 96, 62 });
    content.addAndMakeVisible (*vuOut);

    addKnob (k::vuRef, "REF. VU", 44, false, 90, 410, 78, [] (float v) { return ferb::fmt::comma (v, 1) + " dBFS"; });

    // Cuatro módulos de banda.
    const char* names[] = { "CUERPO", "MEDIOS", "PRESENCIA", "AIRE" };
    for (int b = 0; b < k::numBands; ++b)
    {
        const int x = 268 + b * 196;
        addKnob (k::bandId ("drive", b), names[b], 84, true, x + 30, 84, 110, plain());
        addKnob (k::bandId ("dyn", b), "DINÁMICA", 50, false, x + 4, 250, 80, signedInt());
        addKnob (k::bandId ("mix", b), "NIVEL", 50, false, x + 100, 250, 80, percent());
        auto tube = std::make_unique<Tube>();
        tube->setBounds (x + 74, 212, 26, 36);
        content.addAndMakeVisible (*tube);
        tubes.push_back (std::move (tube));
        addToggle (k::bandId ("on", b), "ON", x + 18, 360, 70);
        addToggle (k::bandId ("solo", b), "SOLO", x + 98, 360, 70);
    }
    addKnob (k::xo1, "CRUCE 1", 40, false, 268 + 150, 400, 70, hz());
    addKnob (k::xo2, "CRUCE 2", 40, false, 268 + 150 + 196, 400, 70, hz());
    addKnob (k::xo3, "CRUCE 3", 40, false, 268 + 150 + 392, 400, 70, hz());

    // Columna derecha.
    const int rx = 1074;
    addKnob (k::tune, "SINTONIZ.", 50, false, rx, 84, 80, hz());
    addKnob (k::phase, "FASE", 50, false, rx + 90, 84, 80, percent());
    addKnob (k::color, "COLOR", 50, false, rx + 180, 84, 80, signedInt());
    addKnob (k::soften, "SUAVIZAR", 50, false, rx, 200, 80, hz());
    guardKnob = &addKnob (k::guard, "GUARDA", 50, false, rx + 90, 200, 80, percent());
    widthKnob = &addKnob (k::width, "ANCHO", 50, false, rx + 180, 200, 80, percent());
    airKnob = &addKnob (k::airCorner, "ESQUINA", 50, false, rx, 316, 80, hz());
    addKnob (k::link, "ENLACE", 50, false, rx + 90, 316, 80, percent());

    // Pie.
    seg (k::os, { "1X", "2X", "4X", "8X" }, 40, 30, 488);
    addToggle (k::linPhase, "FASE LINEAL", 220, 488, 110);
    addToggle (k::delta, "DELTA", 340, 488, 80);
    addToggle (k::match, "IGUALAR", 430, 488, 90);
    addToggle (k::monoSafe, "MONO SEGURO", 530, 488, 136);
    monoButton = buttons.back().get();

    timerCallback();
    startTimerHz (20);
}

DraMcPotiEditor::~DraMcPotiEditor() { stopTimer(); }

/** Comportamiento por modo y era (sección 6 de la ficha): atenúa lo que no actúa y rotula el carácter. */
void DraMcPotiEditor::timerCallback()
{
    const auto raw = [this] (const char* id) { return owner.apvts.getRawParameterValue (id)->load(); };
    const bool master = juce::roundToInt (raw (k::mode)) == k::Master;
    const int era = juce::roundToInt (raw (k::era)), voice = juce::roundToInt (raw (k::voice));
    constexpr const char* names[2][4] = { { "CLÁSICO", "CINTA", "VÁLVULA", "TRANSISTOR" }, { "DINÁMICO", "ADAPTATIVO", "SEDA", "AIRE" } };
    voiceName.setText (U (names[juce::jlimit (0, 1, era)][juce::jlimit (0, 3, voice)]), juce::dontSendNotification);

    guardKnob->setAlpha (master ? 0.35f : 1.0f);   // Guarda: solo Vocal
    widthKnob->setAlpha (master ? 1.0f : 0.35f);   // Ancho y Mono seguro: solo Master
    monoButton->setAlpha (master ? 1.0f : 0.35f);
    airKnob->setAlpha (era == k::Moderno && voice == 3 ? 1.0f : 0.35f); // Esquina de aire: solo con el carácter Aire

    // Vúmetros: 0 VU = Referencia VU (dBFS), con la balística de 65 ms del motor.
    const float ref = raw (k::vuRef);
    const auto toVu = [ref] (float level) { return 20.0f * std::log10 (level + 1.0e-6f) - ref; };
    vuIn->setValue (toVu (owner.getEngine().inputVuLevel()));
    vuOut->setValue (toVu (owner.getEngine().outputVuLevel()));

    for (size_t b = 0; b < tubes.size(); ++b)
    {
        const float level = owner.getEngine().addedLevel (static_cast<int> (b));
        const float db = 20.0f * std::log10 (level + 1.0e-6f);
        tubes[b]->setGlow (juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 40.0f));
    }
}

VKnob& DraMcPotiEditor::addKnob (const juce::String& id, const char* label, int size, bool big, int x, int y, int w,
                                 std::function<juce::String (float)> format)
{
    VKnob::Spec spec;
    spec.label = U (label);
    spec.size = size;
    spec.big = big;
    spec.format = std::move (format);
    auto& parameter = *dynamic_cast<juce::RangedAudioParameter*> (owner.apvts.getParameter (id));
    auto knob = std::make_unique<VKnob> (parameter, palette, spec, &owner.undoManager);
    knob->setBounds (x, y, w, VKnob::heightFor (spec));
    content.addAndMakeVisible (*knob);
    knobs.push_back (std::move (knob));
    return *knobs.back();
}

void DraMcPotiEditor::addToggle (const juce::String& id, const char* text, int x, int y, int w)
{
    auto button = std::make_unique<VButton> (palette, U (text), VButton::Icon::none, true);
    button->setClickingTogglesState (true);
    button->setBounds (x, y, w, 30);
    content.addAndMakeVisible (*button);
    auto& parameter = *dynamic_cast<juce::RangedAudioParameter*> (owner.apvts.getParameter (id));
    buttonAttachments.push_back (std::make_unique<juce::ButtonParameterAttachment> (parameter, *button, &owner.undoManager));
    buttons.push_back (std::move (button));
}
