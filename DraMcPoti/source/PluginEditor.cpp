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

    addKnob (k::input, "ENTRADA", 56, false, 24, 214, 78, db());
    addKnob (k::mix, "MEZCLA", 56, false, 90, 214, 78, percent());
    addKnob (k::output, "SALIDA", 56, false, 156, 214, 78, db());
    addKnob (k::vuRef, "REF. VU", 44, false, 90, 340, 78, [] (float v) { return ferb::fmt::comma (v, 1) + " dBFS"; });

    // Cuatro módulos de banda.
    const char* names[] = { "CUERPO", "MEDIOS", "PRESENCIA", "AIRE" };
    for (int b = 0; b < k::numBands; ++b)
    {
        const int x = 268 + b * 196;
        addKnob (k::bandId ("drive", b), names[b], 84, true, x + 30, 84, 110, plain());
        addKnob (k::bandId ("dyn", b), "DINÁMICA", 50, false, x + 4, 250, 80, signedInt());
        addKnob (k::bandId ("mix", b), "NIVEL", 50, false, x + 100, 250, 80, percent());
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
    addKnob (k::guard, "GUARDA", 50, false, rx + 90, 200, 80, percent());
    addKnob (k::width, "ANCHO", 50, false, rx + 180, 200, 80, percent());
    addKnob (k::airCorner, "ESQUINA", 50, false, rx, 316, 80, hz());
    addKnob (k::link, "ENLACE", 50, false, rx + 90, 316, 80, percent());

    // Pie.
    seg (k::os, { "1X", "2X", "4X", "8X" }, 40, 30, 488);
    addToggle (k::linPhase, "FASE LINEAL", 220, 488, 110);
    addToggle (k::delta, "DELTA", 340, 488, 80);
    addToggle (k::match, "IGUALAR", 430, 488, 90);
    addToggle (k::monoSafe, "MONO SEGURO", 530, 488, 136);
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
