#include "PluginEditor.h"

using namespace ferb::vintage;
namespace k = maru::params;

namespace
{
juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

juce::String dbText (float v, int decimals = 0)
{
    v = std::round (v * std::pow (10.0f, static_cast<float> (decimals))) / std::pow (10.0f, static_cast<float> (decimals));
    const auto s = ferb::fmt::comma (std::abs (v) < 1.0e-6f ? 0.0f : std::abs (v), decimals);
    return (v < 0.0f && s.containsAnyOf ("123456789") ? U ("−") : "") + s + " dB";
}

juce::String signedValue (float v, int decimals)
{
    v = std::round (v * std::pow (10.0f, static_cast<float> (decimals))) / std::pow (10.0f, static_cast<float> (decimals));
    const auto s = ferb::fmt::comma (std::abs (v), decimals);
    const bool zero = ! s.containsAnyOf ("123456789");
    return (zero ? "" : (v > 0.0f ? "+" : U ("−"))) + s;
}

juce::String msText (float v, int decimals) { return ferb::fmt::comma (v, decimals) + " ms"; }
} // namespace

/** Rótulos fijos: el filete «CONTROLES DEL MOTOR», «PISO DE RUIDO» y «LATENCIA». */
class MaruEditor::Captions : public juce::Component
{
public:
    explicit Captions (const Palette& p) : palette (p)
    {
        setInterceptsMouseClicks (false, false);
        setBufferedToImage (true);
    }

    void paint (juce::Graphics& g) override
    {
        drawEmbossed (g, "PISO DE RUIDO", { 62.0f, 240.0f, 150.0f, 14.0f }, labelFont (10.0f, 2.0f), juce::Justification::centred, palette);
        drawEmbossed (g, "LATENCIA", { 1070.0f, 250.0f, 130.0f, 14.0f }, labelFont (10.0f, 2.0f), juce::Justification::centredLeft, palette);

        const float x = 690.0f, w = 360.0f, y = 76.0f;
        const auto font = labelFont (10.5f, 2.0f);
        const juce::String text = U ("CONTROLES DEL MOTOR");
        const float tw = juce::GlyphArrangement::getStringWidth (font, text), mid = x + w * 0.5f;
        g.setColour (palette.rule);
        g.fillRect (x, y + 7.0f, mid - x - tw * 0.5f - 8.0f, 1.0f);
        g.fillRect (mid + tw * 0.5f + 8.0f, y + 7.0f, x + w - mid - tw * 0.5f - 8.0f, 1.0f);
        drawEmbossed (g, text, { mid - tw * 0.5f - 4.0f, y, tw + 8.0f, 14.0f }, font, juce::Justification::centred, palette);
    }

private:
    Palette palette;
};

MaruEditor::MaruEditor (MaruProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75), owner (p)
{
    backdrop = juce::Colour (0xff0b0d0c);
    auto param = [&p] (const char* id) -> juce::RangedAudioParameter& { return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id)); };

    frame = std::make_unique<Frame> (palette, true);
    frame->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*frame);

    captions = std::make_unique<Captions> (palette);
    captions->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*captions);

    Header::Spec headerSpec;
    headerSpec.name = "Maru";
    headerSpec.subtitle = U ("SUPRESOR DE RUIDO · GATE");
    headerSpec.presetBarX = 330;
    headerSpec.width = baseWidth;
    header = std::make_unique<Header> (palette, headerSpec, p.getPresets(), p.getAb(), p.undoManager, param (k::bypass));
    header->setBounds (0, 0, baseWidth, 56);
    content.addAndMakeVisible (*header);

    floorView = std::make_unique<maru::ui::FloorView> (palette);
    floorView->setGlassBounds ({ 62, 80, maru::ui::FloorView::size, maru::ui::FloorView::size });
    content.addAndMakeVisible (*floorView);

    // Palancas: motor y escuchar lo quitado; el enlace es continuo (0 a 100 %), así que lleva una perilla.
    struct LeverDef { const char* id; int x; VLever::Spec spec; };
    const std::vector<LeverDef> leverDefs = {
        { k::engine, 238, { .name = "MOTOR", .top = "SUPR.", .bottom = "GATE", .invert = true } },
        { k::listen, 308, { .name = "QUITADO", .top = "OYE", .bottom = "NO" } },
    };
    for (const auto& d : leverDefs)
    {
        auto lever = std::make_unique<VLever> (param (d.id), palette, d.spec, &p.undoManager);
        lever->setTopLeftPosition (d.x, 78);
        content.addAndMakeVisible (*lever);
        levers.push_back (std::move (lever));
    }

    amountKnob = std::make_unique<VKnob> (param (k::amount), palette,
                                          VKnob::Spec { .label = U ("SUPRESIÓN"), .size = 178, .big = true, .endLow = "0", .endHigh = "100",
                                                        .format = [] (float v) { return juce::String (juce::roundToInt (v)) + " %"; } },
                                          &p.undoManager);
    amountKnob->setBounds (460, 66, 200, VKnob::heightFor ({ .size = 178, .big = true }));
    content.addAndMakeVisible (*amountKnob);

    // Perillas pequeñas. Las del Gate y las de la Supresión se muestran según el motor.
    struct KnobDef { const char* id; int x, y; VKnob::Spec spec; bool gateOnlyKnob, suppressionOnlyKnob; };
    const auto small = [] (const juce::String& label, std::function<juce::String (float)> format) { return VKnob::Spec { .label = label, .size = 56, .format = std::move (format) }; };
    const std::vector<KnobDef> defs = {
        { k::range, 700, 96, small ("RANGO", [] (float v) { return dbText (v); }), false, false },
        { k::sens, 770, 96, small ("SENSIB.", [] (float v) { return signedValue (v, 0); }), false, true },
        { k::recovery, 840, 96, small ("RECUPERAC.", [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }), false, true },
        { k::attack, 770, 96, small ("ATAQUE", [] (float v) { return msText (v, v < 10.0f ? 1 : 0); }), true, false },
        { k::hold, 840, 96, small ("HOLD", [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }), true, false },
        { k::hyst, 910, 96, small (U ("HISTÉR."), [] (float v) { return dbText (v); }), true, false },
        { k::recovery, 980, 96, small ("RELEASE", [] (float v) { return juce::String (juce::roundToInt (v)) + " ms"; }), true, false },
        // Segunda fila, solo en Gate (la maqueta no las trae).
        { k::sens, 700, 196, small ("SENSIB.", [] (float v) { return signedValue (v, 0); }), true, false },
        { k::lookahead, 765, 196, small ("LOOKAHEAD", [] (float v) { return msText (v, 1); }), true, false },
        { k::scHP, 840, 196, small ("SC HP", [] (float v) { return juce::String (juce::roundToInt (v)) + " Hz"; }), true, false },
        { k::scLP, 910, 196, small ("SC LP", [] (float v) { return ferb::fmt::comma (v / 1000.0f, 1) + " kHz"; }), true, false },
        { k::gateThresh, 980, 196, small ("UMBRAL", [] (float v) { return dbText (v, 1); }), true, false },
    };
    for (const auto& d : defs)
    {
        auto knob = std::make_unique<VKnob> (param (d.id), palette, d.spec, &p.undoManager);
        knob->setBounds (d.x, d.y, juce::String (d.id) == k::lookahead ? 80 : 70, VKnob::heightFor (d.spec));
        content.addAndMakeVisible (*knob);
        if (d.gateOnlyKnob)
            gateOnly.push_back (knob.get());
        if (d.suppressionOnlyKnob)
            suppressionOnly.push_back (knob.get());
        if (juce::String (d.id) == k::gateThresh)
            thresholdKnob = knob.get();
        knobs.push_back (std::move (knob));
    }

    linkKnob = std::make_unique<VKnob> (param (k::link), palette, VKnob::Spec { .label = "ENLACE", .size = 50, .format = [] (float v) { return juce::String (juce::roundToInt (v)) + " %"; } }, &p.undoManager);
    linkKnob->setBounds (378, 78, 70, VKnob::heightFor ({ .size = 50 }));
    content.addAndMakeVisible (*linkKnob);

    // Interruptores del Gate (a la izquierda, bajo las palancas).
    const auto addToggle = [&] (const char* id, const char* text, juce::Rectangle<int> bounds)
    {
        auto t = std::make_unique<VButton> (palette, U (text), VButton::Icon::none, true);
        t->setClickingTogglesState (true);
        t->setBounds (bounds);
        toggleAttachments.push_back (std::make_unique<juce::ButtonParameterAttachment> (param (id), *t, &p.undoManager));
        content.addAndMakeVisible (*t);
        gateButtons.push_back (t.get());
        toggles.push_back (std::move (t));
    };
    addToggle (k::gateAuto, "UMBRAL AUTO", { 238, 204, 140, 28 });
    addToggle (k::scExt, "SIDECHAIN EXT.", { 238, 240, 140, 28 });

    // Medidor de atenuación, lámpara, ventanas y latencia.
    vu = std::make_unique<VuNeedle> (palette, U ("ATENUACIÓN"), false);
    vu->setFaceBounds ({ 1070, 70, 130, 88 });
    content.addAndMakeVisible (*vu);

    lamp = std::make_unique<maru::ui::JewelLamp> (palette);
    lamp->setBounds (1062, 166, 148, 24);
    content.addAndMakeVisible (*lamp);

    floorWindow = std::make_unique<ReadoutWindow> (palette, "PISO");
    floorWindow->setBounds (1062, 198, 72, 40);
    content.addAndMakeVisible (*floorWindow);
    reductionWindow = std::make_unique<ReadoutWindow> (palette, "REDUCC.");
    reductionWindow->setBounds (1134, 198, 72, 40);
    content.addAndMakeVisible (*reductionWindow);

    latencySeg = std::make_unique<VSeg> (param (k::latency), palette, juce::StringArray { "BAJA", "ALTA" }, 62, &p.undoManager);
    latencySeg->setTopLeftPosition (1070, 264);
    content.addAndMakeVisible (*latencySeg);

    engineWatch = std::make_unique<juce::ParameterAttachment> (param (k::engine), [this] (float v)
                                                               {
                                                                   if (gateEngine != (juce::roundToInt (v) == k::Gate))
                                                                   {
                                                                       gateEngine = juce::roundToInt (v) == k::Gate;
                                                                       layoutForEngine();
                                                                   }
                                                               });
    autoWatch = std::make_unique<juce::ParameterAttachment> (param (k::gateAuto), [this] (float v)
                                                             {
                                                                 if (thresholdKnob != nullptr)
                                                                     thresholdKnob->setAlpha (v >= 0.5f ? 0.4f : 1.0f); // con Auto el umbral manual no se usa
                                                             });
    gateEngine = juce::roundToInt (p.apvts.getRawParameterValue (k::engine)->load()) == k::Gate;
    layoutForEngine();
    autoWatch->sendInitialUpdate();

    timerCallback();
    startTimerHz (30);
}

MaruEditor::~MaruEditor() = default;

void MaruEditor::layoutForEngine()
{
    for (auto* knob : suppressionOnly)
        knob->setVisible (! gateEngine);
    for (auto* knob : gateOnly)
        knob->setVisible (gateEngine);
    for (auto* button : gateButtons)
        button->setVisible (gateEngine);
}

void MaruEditor::timerCallback()
{
    const auto& telemetry = owner.getTelemetry();
    const auto raw = [this] (const char* id) { return owner.apvts.getRawParameterValue (id)->load(); };

    maru::ui::FloorView::Data data;
    for (size_t i = 0; i < data.input.size(); ++i)
    {
        data.input[i] = telemetry.input[i].load (std::memory_order_relaxed);
        data.output[i] = telemetry.output[i].load (std::memory_order_relaxed);
        data.noise[i] = telemetry.noise[i].load (std::memory_order_relaxed);
    }
    data.listening = raw (k::listen) >= 0.5f;
    floorView->setData (data);

    const float floorDb = telemetry.noiseFloorDb.load (std::memory_order_relaxed), reduction = telemetry.reductionDb.load (std::memory_order_relaxed);
    const bool bypassed = raw (k::bypass) >= 0.5f;
    const float shown = bypassed ? 0.0f : reduction;

    // El vúmetro con la inercia de la aguja (como en la maqueta: de −20 a +3 VU, con 30 dB de atenuación a ≈ +5 VU).
    const float target = -20.0f + std::clamp (shown, 0.0f, 40.0f) / 30.0f * 25.3f;
    vuValue += (target > vuValue ? 0.4f : 0.15f) * (target - vuValue);
    vu->setValue (std::min (vuValue, 3.0f));

    const bool working = telemetry.working.load (std::memory_order_relaxed) && ! bypassed;
    if (gateEngine)
        lamp->set (! working, working ? U ("PUERTA CERRADA") : U ("PUERTA ABIERTA"));
    else
        lamp->set (working, working ? U ("TRABAJANDO") : U ("EN REPOSO"));

    floorWindow->setText (floorDb > -119.0f ? (floorDb < -0.05f ? U ("−") : juce::String()) + ferb::fmt::comma (std::abs (floorDb), 1) : juce::String::fromUTF8 ("−−"));
    reductionWindow->setText ((shown > 0.05f ? U ("−") : juce::String()) + ferb::fmt::comma (shown > 0.05f ? shown : 0.0f, 1));
}
