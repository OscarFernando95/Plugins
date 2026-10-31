#include "PluginEditor.h"

using namespace ferb::vintage;
namespace k = cremita::params;

namespace
{
juce::String U (const char* utf8) { return juce::String::fromUTF8 (utf8); }

/** «+1,0 dB», «−2,5 dB» y «0,0 dB», como en la maqueta (el cero no lleva signo). */
juce::String dbText (float v)
{
    const auto s = ferb::fmt::comma (std::abs (v), 1);
    return (v > 0.05f ? "+" : (v < -0.05f ? U ("−") : "")) + s + " dB";
}

juce::String signedInt (float v)
{
    const int i = juce::roundToInt (v);
    return (i > 0 ? "+" : (i < 0 ? U ("−") : "")) + juce::String (std::abs (i));
}

float toVu (float level, float referenceDb)
{
    return level > 1e-6f ? juce::Decibels::gainToDecibels (level) - referenceDb : -30.0f;
}
} // namespace

/** Rótulos fijos que no pertenecen a ningún control. */
class CremitaEditor::Captions : public juce::Component
{
public:
    explicit Captions (const Palette& p) : palette (p)
    {
        setInterceptsMouseClicks (false, false);
        setBufferedToImage (true);
    }

    void paint (juce::Graphics& g) override
    {
        drawEmbossed (g, "CURVA DE TRANSFERENCIA", { 430.0f, 246.0f, 400.0f, 14.0f }, labelFont (10.0f, 2.0f), juce::Justification::centred, palette);
        drawEmbossed (g, "SOBREMUESTREO", { 690.0f, 386.0f, 130.0f, 14.0f }, labelFont (10.0f, 2.0f), juce::Justification::centredLeft, palette);
    }

private:
    Palette palette;
};

CremitaEditor::CremitaEditor (CremitaProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75), owner (p)
{
    backdrop = juce::Colour (0xff0b0a0c);

    frame = std::make_unique<Frame> (palette, true);
    frame->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*frame);

    captions = std::make_unique<Captions> (palette);
    captions->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*captions);

    auto param = [&p] (const char* id) -> juce::RangedAudioParameter& { return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id)); };

    Header::Spec headerSpec;
    headerSpec.name = "Cremita";
    headerSpec.subtitle = U ("INFLATOR · SONORIDAD Y CALIDEZ");
    headerSpec.rack = true;
    headerSpec.presetBarX = 330;
    headerSpec.width = baseWidth;
    header = std::make_unique<Header> (palette, headerSpec, p.getPresets(), p.getAb(), p.undoManager, param (k::bypass));
    header->setBounds (0, 0, baseWidth, 56);
    content.addAndMakeVisible (*header);

    vuIn = std::make_unique<VuNeedle> (palette, "ENTRADA");
    vuIn->setFaceBounds ({ 70, 76, 320, 160 });
    content.addAndMakeVisible (*vuIn);
    vuOut = std::make_unique<VuNeedle> (palette, "SALIDA");
    vuOut->setFaceBounds ({ 870, 76, 320, 160 });
    content.addAndMakeVisible (*vuOut);

    curveView = std::make_unique<cremita::ui::CurveView> (p, palette);
    curveView->setGlassBounds ({ 430, 76, 400, 160 });
    content.addAndMakeVisible (*curveView);

    struct KnobDef { const char* id; int x, y, w; VKnob::Spec spec; };
    const std::vector<KnobDef> defs = {
        { k::input,  62, 270, 110, { .label = "INPUT", .size = 74, .format = dbText } },
        { k::effect, 172, 270, 110, { .label = "EFECTO", .size = 74, .format = [] (float v) { return juce::String (juce::roundToInt (v)) + " %"; } } },
        { k::curve,  290, 254, 140, { .label = "CURVA", .size = 104, .big = true, .endLow = U ("−50"), .endHigh = "+50", .format = signedInt } },
        { k::bias,   432, 270, 110, { .label = "SESGO", .size = 74, .format = [] (float v) { return juce::String (juce::roundToInt (v)) + " %"; } } },
        { k::output, 542, 270, 110, { .label = "OUTPUT", .size = 74, .format = dbText } },
    };
    for (const auto& d : defs)
    {
        auto knob = std::make_unique<VKnob> (param (d.id), palette, d.spec, &p.undoManager);
        knob->setBounds (d.x, d.y, d.w, VKnob::heightFor (d.spec));
        content.addAndMakeVisible (*knob);
        knobs.push_back (std::move (knob));
    }

    struct LeverDef { const char* id; int x; VLever::Spec spec; };
    const std::vector<LeverDef> leverDefs = {
        { k::split, 690, { .name = "BANDAS", .top = "3", .bottom = "1" } },
        { k::clip, 760, { .name = "CLIP 0", .top = "ON", .bottom = "OFF" } },
        { k::ms, 830, { .name = "M / S", .top = "M/S", .bottom = "L/R" } },
        { k::match, 900, { .name = "IGUALAR", .top = "ON", .bottom = "OFF" } },
        { k::inOn, 970, { .name = "IN", .top = "IN", .bottom = "OUT" } },
    };
    for (const auto& d : leverDefs)
    {
        auto lever = std::make_unique<VLever> (param (d.id), palette, d.spec, &p.undoManager);
        lever->setTopLeftPosition (d.x, 268);
        content.addAndMakeVisible (*lever);
        levers.push_back (std::move (lever));
    }

    effectWindow = std::make_unique<ReadoutWindow> (palette, "EFECTO");
    effectWindow->setBounds (1068, 262, 122, 40);
    content.addAndMakeVisible (*effectWindow);
    overloadWindow = std::make_unique<ReadoutWindow> (palette, "SOBRECARGA");
    overloadWindow->setBounds (1068, 312, 122, 40);
    content.addAndMakeVisible (*overloadWindow);

    oversample = std::make_unique<VSeg> (param (k::os), palette, juce::StringArray { "1X", "2X", "4X", "8X" }, 44, &p.undoManager);
    oversample->setTopLeftPosition (820, 380);
    content.addAndMakeVisible (*oversample);

    timerCallback();
    startTimerHz (30);
}

CremitaEditor::~CremitaEditor() = default;

void CremitaEditor::timerCallback()
{
    auto& engine = owner.getEngine();
    const float reference = k::vuReferenceDb (juce::roundToInt (owner.apvts.getRawParameterValue (k::vuRef)->load()));

    vuIn->setValue (toVu (engine.getInputLevel(), reference));
    vuOut->setValue (toVu (engine.getOutputLevel(), reference));
    curveView->setWorkingLevel (engine.getInputLevel());
    curveView->repaint();

    effectWindow->setText (dbText (engine.getEffectGainDb()));

    const double now = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    if (engine.getOverloadMeter().consume() >= 1.0f)
        overloadUntil = now + 2.0;
    const bool over = now < overloadUntil;
    overloadWindow->setText (over ? "0 dB" : juce::String::fromUTF8 ("—"), over ? std::optional<juce::Colour> (alert) : std::nullopt);
}
