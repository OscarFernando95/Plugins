#include "PluginEditor.h"

using namespace ferb::vintage;
namespace k = chaus::params;

namespace
{
juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

/** Entero si lo es, y con un decimal si no. */
juce::String compact (float v)
{
    v = std::round (v * 10.0f) / 10.0f;
    return std::abs (v - std::round (v)) < 0.05f ? juce::String (juce::roundToInt (v)) : ferb::fmt::comma (v, 1);
}

juce::String signedDb (float v, int decimals)
{
    const float scale = std::pow (10.0f, static_cast<float> (decimals));
    v = std::round (v * scale) / scale; // sin restos como 8,9e-08 de la conversión normalizada
    const auto s = ferb::fmt::comma (std::abs (v) < 1.0e-6f ? 0.0f : std::abs (v), decimals);
    const bool zero = ! s.containsAnyOf ("123456789");
    return (zero ? "" : (v > 0.0f ? "+" : U ("−"))) + s + " dB";
}
} // namespace

/** Rótulos fijos de los selectores. */
class ChauSEditor::Captions : public juce::Component
{
public:
    explicit Captions (const Palette& p) : palette (p) { setInterceptsMouseClicks (false, false); }

    void paint (juce::Graphics& g) override
    {
        const std::pair<int, const char*> labels[] = { { 250, "MODO" }, { 294, "BANDA" }, { 338, "ESTEREO" } };
        for (const auto& [y, text] : labels)
            drawEmbossed (g, text, { 716.0f, static_cast<float> (y), 120.0f, 13.0f }, labelFont (10.0f, 1.8f), juce::Justification::centredLeft, palette);
    }

private:
    Palette palette;
};

ChauSEditor::ChauSEditor (ChauSProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75), owner (p)
{
    backdrop = juce::Colour (0xff0b090d);
    auto param = [&p] (const char* id) -> juce::RangedAudioParameter& { return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id)); };
    const auto raw = [&p] (const char* id) { return p.apvts.getRawParameterValue (id)->load(); };

    frame = std::make_unique<Frame> (palette, false);
    frame->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*frame);

    captions = std::make_unique<Captions> (palette);
    captions->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*captions);

    Header::Spec headerSpec;
    headerSpec.name = "ChauS";
    headerSpec.subtitle = U ("DE-ESSER · EQ DINÁMICO");
    headerSpec.rack = false;
    headerSpec.presetBarX = 236;
    headerSpec.width = baseWidth;
    headerSpec.height = 56;
    header = std::make_unique<Header> (palette, headerSpec, p.getPresets(), p.getAb(), p.undoManager, param (k::bypass));
    header->setBounds (0, 0, baseWidth, 56);
    content.addAndMakeVisible (*header);

    // Pantallas.
    screen = std::make_unique<chaus::ui::ScreenView> (palette);
    screen->setGlassBounds ({ 20, 72, chaus::ui::ScreenView::width, chaus::ui::ScreenView::height });
    timeline = std::make_unique<chaus::ui::TimelineView> (palette);
    timeline->setGlassBounds ({ 20, 372, chaus::ui::TimelineView::width, chaus::ui::TimelineView::height });
    content.addAndMakeVisible (*screen);
    content.addAndMakeVisible (*timeline);
    points.resize (chaus::ui::TimelineView::numPoints);

    // Perillas.
    struct KnobDef { const char* id; int x, y, w; VKnob::Spec spec; };
    const auto kHz = [] (float v) { return ferb::fmt::comma (v / 1000.0f, 1) + " kHz"; };
    const std::vector<KnobDef> defs = {
        { k::thr, 714, 80, 150, { .label = "UMBRAL", .size = 122, .big = true, .endLow = U ("−60"), .endHigh = "0", .format = [] (float v) { return signedDb (v, 0).replace ("+", ""); } } },
        { k::range, 864, 96, 90, { .label = "RANGO", .size = 78, .format = [] (float v) { return compact (v) + " dB"; } } },
        { k::hp, 20, 476, 92, { .label = "FILTRO HP", .size = 50, .format = kHz } },
        { k::lp, 112, 476, 92, { .label = "FILTRO LP", .size = 50, .format = kHz } },
        { k::look, 204, 476, 92, { .label = "LOOKAHEAD", .size = 50, .format = [&p] (float v) { return p.apvts.getRawParameterValue (k::lookOn)->load() >= 0.5f ? compact (v) + " ms" : U ("APAGADO"); } } },
        { k::sensSoft, 296, 476, 92, { .label = "S SUAVES", .size = 50, .format = [] (float v) { return signedDb (v, 0); } } },
        { k::sensHard, 388, 476, 92, { .label = "S DURAS", .size = 50, .format = [] (float v) { return signedDb (v, 0); } } },
        { k::link, 480, 476, 92, { .label = "ENLACE", .size = 50, .format = [] (float v) { return juce::String (juce::roundToInt (v)) + " %"; } } },
        { k::output, 572, 476, 92, { .label = "SALIDA", .size = 50, .format = [] (float v) { return signedDb (v, 1); } } },
    };
    for (const auto& d : defs)
    {
        auto knob = std::make_unique<VKnob> (param (d.id), palette, d.spec, &p.undoManager);
        knob->setBounds (d.x, d.y, d.w, VKnob::heightFor (d.spec));
        content.addAndMakeVisible (*knob);
        knobs.push_back (std::move (knob));
    }

    // Selectores.
    const auto addSegment = [&] (const char* id, juce::StringArray labels, int buttonWidth, int y)
    {
        auto seg = std::make_unique<VSeg> (param (id), palette, std::move (labels), buttonWidth, &p.undoManager);
        seg->setTopLeftPosition (716, y);
        content.addAndMakeVisible (*seg);
        segments.push_back (std::move (seg));
    };
    addSegment (k::mode, { "VOZ SOLA", "GENERAL" }, 80, 262);
    addSegment (k::band, { "DIVIDIDA", "ANCHA" }, 80, 306);
    addSegment (k::ms, { "L/R", "MID", "SIDE" }, 56, 350);

    // Interruptores con lámpara.
    const auto addToggle = [&] (const char* id, const char* text, juce::Rectangle<int> bounds)
    {
        auto t = std::make_unique<VButton> (palette, U (text), VButton::Icon::none, true);
        t->setClickingTogglesState (true);
        t->setBounds (bounds);
        toggleAttachments.push_back (std::make_unique<juce::ButtonParameterAttachment> (param (id), *t, &p.undoManager));
        content.addAndMakeVisible (*t);
        toggles.push_back (std::move (t));
    };
    addToggle (k::listenTrig, "ESCUCHAR DISPARO", { 716, 392, 200, 30 });
    addToggle (k::adaptive, "UMBRAL ADAPTATIVO", { 716, 424, 200, 30 });
    addToggle (k::track, "SEGUIR LA S", { 716, 456, 200, 30 });
    // Añadidos sobre la maqueta: lo que la maqueta no tiene perilla (ver «Estado y cambios sobre la ficha»).
    addToggle (k::scExt, "SC EXT.", { 716, 492, 98, 26 });
    addToggle (k::listenSC, "OÍR SC", { 818, 492, 98, 26 });

    advancedButton = std::make_unique<VButton> (palette, U ("AJUSTES AVANZADOS"));
    advancedButton->setBounds (716, 522, 200, 26);
    advancedButton->onClick = [this] { showAdvancedMenu(); };
    advancedButton->setTitle (U ("Ajustes avanzados: forma, ancho y liberación"));
    content.addAndMakeVisible (*advancedButton);

    screenData.input.fill (-120.0f);
    screenData.output.fill (-120.0f);
    rawIn.fill (-120.0f);
    rawOut.fill (-120.0f);
    (void) raw;
    timerCallback();
    startTimerHz (30);
}

ChauSEditor::~ChauSEditor() = default;

void ChauSEditor::setParameter (const char* id, float realValue)
{
    if (auto* p = owner.apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
        p->endChangeGesture();
    }
}

void ChauSEditor::showAdvancedMenu()
{
    const auto& apvts = owner.apvts;
    const auto raw = [&apvts] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    juce::PopupMenu menu, shapeMenu, qMenu, releaseMenu;
    const int shape = juce::roundToInt (raw (k::shape));
    const char* shapeNames[] = { "Campana", "Estante", "Auto" };
    for (int i = 0; i < 3; ++i)
        shapeMenu.addItem (juce::PopupMenu::Item (shapeNames[i]).setTicked (shape == i).setAction ([this, i] { setParameter (k::shape, static_cast<float> (i)); }));

    const bool qAuto = raw (k::qAuto) >= 0.5f;
    qMenu.addItem (juce::PopupMenu::Item ("Auto").setTicked (qAuto).setAction ([this] { setParameter (k::qAuto, 1.0f); }));
    for (float q : { 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f })
        qMenu.addItem (juce::PopupMenu::Item (ferb::fmt::comma (q, 1)).setTicked (! qAuto && std::abs (raw (k::q) - q) < 0.25f)
                           .setAction ([this, q] { setParameter (k::qAuto, 0.0f); setParameter (k::q, q); }));

    const bool releaseAuto = raw (k::releaseAuto) >= 0.5f;
    releaseMenu.addItem (juce::PopupMenu::Item ("Auto").setTicked (releaseAuto).setAction ([this] { setParameter (k::releaseAuto, 1.0f); }));
    for (float r : { 10.0f, 20.0f, 40.0f, 60.0f, 100.0f })
        releaseMenu.addItem (juce::PopupMenu::Item (juce::String (juce::roundToInt (r)) + " ms").setTicked (! releaseAuto && std::abs (raw (k::release) - r) < 5.0f)
                                 .setAction ([this, r] { setParameter (k::releaseAuto, 0.0f); setParameter (k::release, r); }));

    menu.addSubMenu (U ("Forma"), shapeMenu);
    menu.addSubMenu (U ("Ancho (Q)"), qMenu);
    menu.addSubMenu (U ("Liberación"), releaseMenu);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (advancedButton.get()));
}

void ChauSEditor::timerCallback()
{
    const auto& engine = owner.getEngine();
    const double fs = owner.getSampleRate();

    // Espectros (entrada y salida) con ataque rápido y caída lenta.
    inputAnalyzer.analyse (owner.getInputRing(), fs, rawIn);
    outputAnalyzer.analyse (owner.getOutputRing(), fs, rawOut);
    for (size_t i = 0; i < screenData.input.size(); ++i)
    {
        auto follow = [] (float& state, float target) { state += (target > state ? 0.55f : 0.18f) * (target - state); };
        follow (screenData.input[i], rawIn[i]);
        follow (screenData.output[i], rawOut[i]);
    }

    const auto& telemetry = engine.getTelemetry();
    const auto raw = [this] (const char* id) { return owner.apvts.getRawParameterValue (id)->load(); };
    screenData.hpHz = raw (k::hp);
    screenData.lpHz = raw (k::lp);
    screenData.centreHz = telemetry.centreHz.load (std::memory_order_relaxed);
    screenData.q = telemetry.q.load (std::memory_order_relaxed);
    screenData.reductionDb = telemetry.reductionDb.load (std::memory_order_relaxed);
    screenData.shapeWeight = telemetry.shapeWeight.load (std::memory_order_relaxed);
    screenData.sampleRate = fs;
    screenData.listening = raw (k::listenTrig) >= 0.5f || raw (k::listenSC) >= 0.5f;
    screen->setData (screenData);

    telemetry.copyLatest (points.data(), static_cast<int> (points.size()));
    timeline->setPoints (points);
}
