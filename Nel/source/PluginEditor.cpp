#include "PluginEditor.h"

using namespace ferb::vintage;
namespace k = nel::params;

namespace
{
juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

juce::String dbText (float v, int decimals = 1)
{
    const auto s = ferb::fmt::comma (std::abs (v), decimals);
    return (v > 0.0f && s.containsAnyOf ("123456789") ? "+" : (v < 0.0f && s.containsAnyOf ("123456789") ? U ("−") : "")) + s;
}

float toDb (float lin) { return lin > 1.0e-5f ? 20.0f * std::log10 (lin) : -100.0f; }

constexpr const char* scaleNames[] = { "ESTÁNDAR", "K-12", "K-14", "K-20" };
constexpr float scaleOffsets[] = { 0.0f, 12.0f, 14.0f, 20.0f };
} // namespace

/** Rótulos fijos, la placa inferior y el título del modo (con asterisco si se modificó un valor). */
class NelEditor::Captions : public juce::Component
{
public:
    explicit Captions (const Palette& p) : palette (p) { setInterceptsMouseClicks (false, false); }

    void setModeLabel (const juce::String& text)
    {
        if (text == modeLabel)
            return;
        modeLabel = text;
        repaint (280, 404, 480, 24);
    }

    void paint (juce::Graphics& g) override
    {
        // Placa inferior.
        const auto deck = juce::Rectangle<float> (24.0f, 408.0f, 992.0f, 224.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.4f), 0.0f, deck.getY(), juce::Colours::black.withAlpha (0.2f), 0.0f, deck.getBottom(), false));
        g.fillRect (deck);
        g.setColour (brassDark);
        g.fillRect (deck.getX(), deck.getY(), deck.getWidth(), 2.0f);
        g.setColour (violet.withAlpha (0.35f));
        g.fillRect (deck.getX(), deck.getY() + 2.0f, deck.getWidth(), 1.0f);

        rule (g, 410.0f, modeLabel);
        rule (g, 476.0f, U ("TIEMPOS Y ENLACE"));

        const std::pair<float, const char*> groups[] = { { 36.0f, "SOBREMUESTREO" }, { 300.0f, "FASE" }, { 431.0f, "RENDER" }, { 542.0f, "DITHER" }, { 780.0f, "PERFIL" } };
        for (const auto& [x, text] : groups)
            drawEmbossed (g, text, { x, 581.0f, 200.0f, 13.0f }, labelFont (10.0f, 1.8f), juce::Justification::centredLeft, palette);
    }

private:
    void rule (juce::Graphics& g, float y, const juce::String& text) const
    {
        const float x = 286.0f, w = 458.0f;
        const auto font = labelFont (10.5f, 2.0f);
        const float tw = juce::GlyphArrangement::getStringWidth (font, text);
        const float mid = x + w * 0.5f;
        g.setColour (palette.rule);
        g.fillRect (x, y + 7.0f, mid - x - tw * 0.5f - 8.0f, 1.0f);
        g.fillRect (mid + tw * 0.5f + 8.0f, y + 7.0f, x + w - mid - tw * 0.5f - 8.0f, 1.0f);
        drawEmbossed (g, text, { mid - tw * 0.5f - 4.0f, y, tw + 8.0f, 14.0f }, font, juce::Justification::centred, palette);
    }

    Palette palette;
    juce::String modeLabel = U ("MODO · MODERNO");
};

NelEditor::NelEditor (NelProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75), owner (p)
{
    backdrop = juce::Colour (0xff0b0a0c);
    auto param = [&p] (const char* id) -> juce::RangedAudioParameter& { return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id)); };
    const auto raw = [&p] (const char* id) { return p.apvts.getRawParameterValue (id)->load(); };

    frame = std::make_unique<Frame> (palette, true, 18);
    frame->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*frame);

    captions = std::make_unique<Captions> (palette);
    captions->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*captions);

    Header::Spec headerSpec;
    headerSpec.name = "Nel";
    headerSpec.subtitle = "LIMITADOR DE TRUE PEAK";
    headerSpec.rack = false;
    headerSpec.presetBarX = 216;
    headerSpec.width = baseWidth;
    headerSpec.height = 72;
    headerSpec.nameSize = 38.0f;
    headerSpec.displayWidth = 236;
    headerSpec.saveButton = true;
    headerSpec.lineColour = violet;
    header = std::make_unique<Header> (palette, headerSpec, p.getPresets(), p.getAb(), p.undoManager, param (k::bypass));
    header->setBounds (0, 0, baseWidth, 72);
    content.addAndMakeVisible (*header);

    inLadder = std::make_unique<nel::ui::LadderMeter> (palette, "ENTRADA", false);
    inLadder->setTopLeftPosition (40, 76);
    outLadder = std::make_unique<nel::ui::LadderMeter> (palette, "SALIDA", true);
    outLadder->setTopLeftPosition (686, 76);
    scope = std::make_unique<nel::ui::ScopeView> (palette);
    scope->setGlassBounds ({ 130, 76, 544, 264 });
    strip = std::make_unique<nel::ui::ReadoutStrip> (palette);
    strip->setTopLeftPosition (130, 348);
    strip->onScale = [this] { cycleScale(); };
    grMeter = std::make_unique<nel::ui::GrMeter> (palette);
    grMeter->setTopLeftPosition (776 - nel::ui::GrMeter::margin, 76 - nel::ui::GrMeter::margin);

    const int target = static_cast<int> (p.apvts.state.getProperty (k::uiLufsTarget, -14));
    loudness = std::make_unique<nel::ui::LoudnessPanel> (palette, target);
    loudness->setTopLeftPosition (776 - 2, 202 - 2);
    loudness->onTarget = [&p] (int t) { p.apvts.state.setProperty (k::uiLufsTarget, t, nullptr); };
    loudness->onReset = [this]
    {
        owner.getEngine().getLoudness().requestReset();
        owner.getEngine().resetOvers();
        tpMax = -100.0f;
        grMax = 0.0f;
    };
    for (auto* c : std::initializer_list<juce::Component*> { inLadder.get(), outLadder.get(), scope.get(), strip.get(), grMeter.get(), loudness.get() })
        content.addAndMakeVisible (*c);

    // Perillas.
    struct KnobDef { const char* id; int x, y, w; VKnob::Spec spec; };
    const auto ms1 = [] (float v) { return ferb::fmt::comma (v, 1) + " ms"; };
    const auto percent = [] (float v) { return juce::String (juce::roundToInt (v)) + " %"; };
    const std::vector<KnobDef> defs = {
        { k::gain, 36, 416, 112, { .label = "GANANCIA", .size = 96, .big = true, .endLow = "0", .endHigh = "+24", .format = [] (float v) { return dbText (v) + " dB"; } } },
        { k::ceiling, 156, 416, 112, { .label = "TECHO", .size = 96, .big = true, .endLow = U ("−30"), .endHigh = "0", .format = [] (float v) { return dbText (v) + " dBTP"; } } },
        { k::character, 762, 436, 96, { .label = U ("CARÁCTER"), .size = 76, .endLow = "0", .endHigh = "100", .format = percent } },
        { k::lookahead, 286, 492, 76, { .label = "LOOKAHEAD", .size = 48, .format = ms1 } },
        { k::attack, 362, 492, 76, { .label = "ATAQUE", .size = 48, .format = [] (float v) { return ferb::fmt::comma (v, v < 10.0f ? 1 : 0) + " ms"; } } },
        { k::release, 438, 492, 76, { .label = "RELEASE", .size = 48, .format = [&p] (float v) { return juce::String (juce::roundToInt (v)) + " ms" + (p.apvts.getRawParameterValue (k::releaseAuto)->load() >= 0.5f ? " A" : ""); } } },
        { k::knee, 514, 492, 76, { .label = "KNEE", .size = 48, .format = [] (float v) { return ferb::fmt::comma (v, 1) + " dB"; } } },
        { k::linkTransient, 590, 492, 76, { .label = "ENL. TRANS.", .size = 48, .format = percent } },
        { k::linkRelease, 666, 492, 76, { .label = "ENL. REL.", .size = 48, .format = percent } },
    };
    for (const auto& d : defs)
    {
        auto knob = std::make_unique<VKnob> (param (d.id), palette, d.spec, &p.undoManager);
        knob->setBounds (d.x, d.y, d.w, VKnob::heightFor (d.spec));
        content.addAndMakeVisible (*knob);
        if (juce::String (d.id) == k::release)
            releaseKnob = knob.get();
        knobs.push_back (std::move (knob));
    }

    modes = std::make_unique<nel::ui::ModeSelector> (palette, [&p] (int m) { p.selectMode (m); });
    modes->setTopLeftPosition (286, 430);
    content.addAndMakeVisible (*modes);

    const std::pair<const char*, const char*> toggleDefs[] = {
        { k::truePeak, "TRUE PEAK" }, { k::unityGain, "GANANCIA UNIT." }, { k::audition, "AUDICIÓN Δ" },
        { k::dcFilter, "FILTRO DC" }, { k::releaseAuto, "RELEASE AUTO" } };
    for (size_t i = 0; i < 5; ++i)
    {
        auto t = std::make_unique<nel::ui::LampToggle> (param (toggleDefs[i].first), palette, U (toggleDefs[i].second), &p.undoManager);
        t->setBounds (870, 416 + static_cast<int> (i) * 32, 134, 30);
        content.addAndMakeVisible (*t);
        toggles.push_back (std::move (t));
    }

    oversample = std::make_unique<VSeg> (param (k::oversample), palette, juce::StringArray { "1X", "2X", "4X", "8X", "16X", "32X" }, 36, &p.undoManager);
    oversample->setTopLeftPosition (36, 596);
    phase = std::make_unique<VSeg> (param (k::osPhase), palette, juce::StringArray { "LINEAL", U ("MÍNIMA") }, 52, &p.undoManager);
    phase->setTopLeftPosition (300, 596);
    renderToggle = std::make_unique<nel::ui::LampToggle> (param (k::osOffline), palette, U ("+1 ESCALÓN"), &p.undoManager);
    renderToggle->setCream (true);
    renderToggle->setBounds (431, 596, 84, 30);
    ditherBits = std::make_unique<VSeg> (param (k::ditherBits), palette, juce::StringArray { "NO", "16", "18", "20", "22", "24" }, 36, &p.undoManager);
    ditherBits->setTopLeftPosition (542, 596);
    ditherShape = std::make_unique<VSeg> (param (k::ditherShape), palette, juce::StringArray { "PLANO", "SUAVE", "MEDIO", "FUERTE" }, 54, &p.undoManager);
    ditherShape->setTopLeftPosition (780, 596);
    for (auto* c : std::initializer_list<juce::Component*> { oversample.get(), phase.get(), renderToggle.get(), ditherBits.get(), ditherShape.get() })
        content.addAndMakeVisible (*c);

    lastReleaseAuto = raw (k::releaseAuto) >= 0.5f;
    meters.ceilingDb = raw (k::ceiling);
    timerCallback();
    startTimerHz (30);
}

NelEditor::~NelEditor() = default;

void NelEditor::cycleScale()
{
    const int next = (static_cast<int> (owner.apvts.state.getProperty (k::uiMeterScale, 0)) + 1) % 4;
    owner.apvts.state.setProperty (k::uiMeterScale, next, nullptr);
}

void NelEditor::timerCallback()
{
    auto& engine = owner.getEngine();
    const auto raw = [this] (const char* id) { return owner.apvts.getRawParameterValue (id)->load(); };
    constexpr float dt = 1.0f / 30.0f;

    // Picos: suben al instante, bajan 40 dB/s; la retención cae 15 dB/s tras 2 s.
    for (size_t c = 0; c < 2; ++c)
    {
        const float in = toDb (engine.getInputPeak (static_cast<int> (c)).consume());
        const float out = toDb (engine.getOutputPeak (static_cast<int> (c)).consume());
        meters.inPeakDb[c] = std::max (in, meters.inPeakDb[c] - 40.0f * dt);
        meters.outPeakDb[c] = std::max (out, meters.outPeakDb[c] - 40.0f * dt);
        tpMax = std::max (tpMax, toDb (engine.getTruePeak (static_cast<int> (c)).consume()));

        for (size_t side = 0; side < 2; ++side)
        {
            auto& hold = (side == 0 ? meters.inHoldDb : meters.outHoldDb)[c];
            const float cur = side == 0 ? in : out;
            if (cur >= hold) { hold = cur; holdAge[side][c] = 2.0f; }
            else if ((holdAge[side][c] -= dt) <= 0.0f) hold -= 15.0f * dt;
        }
    }
    const auto meanRms = [&] (float a, float b) { return toDb (std::sqrt (0.5f * (a * a + b * b))); };
    meters.outRmsDb = meanRms (engine.getRms (0), engine.getRms (1));
    meters.inRmsDb = meanRms (engine.getInputRms (0), engine.getInputRms (1));
    meters.ceilingDb = raw (k::ceiling);
    meters.meterOffset = scaleOffsets[juce::jlimit (0, 3, static_cast<int> (owner.apvts.state.getProperty (k::uiMeterScale, 0)))];
    inLadder->setState (meters);
    outLadder->setState (meters);

    const float gr = std::max (engine.getReductionMeter().consume(), 0.0f);
    grMax = std::max (grMax, gr);
    vuGr += (gr - vuGr) * (1.0f - std::exp (-dt / (gr > vuGr ? 0.02f : 0.25f)));
    grMeter->setValue (vuGr);
    strip->setValues (gr, grMax, engine.getOvers());
    strip->setScaleName (U (scaleNames[juce::jlimit (0, 3, static_cast<int> (owner.apvts.state.getProperty (k::uiMeterScale, 0)))]));

    const float inPeak = std::max (meters.inPeakDb[0], meters.inPeakDb[1]);
    const float outPeak = std::max (meters.outPeakDb[0], meters.outPeakDb[1]);
    scope->setCeiling (raw (k::ceiling));
    scope->setAudition (raw (k::audition) >= 0.5f);
    scope->setLegend (raw (k::audition) >= 0.5f ? "diferencia" : (raw (k::unityGain) >= 0.5f ? U ("salida (ganancia unitaria)") : "salida"));
    scope->push (inPeak + raw (k::gain), outPeak, gr);

    nel::ui::LoudnessPanel::Values v;
    auto& lm = engine.getLoudness();
    v.momentary = lm.momentary();
    v.shortTerm = lm.shortTerm();
    v.integrated = lm.integrated();
    v.range = lm.range();
    v.elapsed = lm.elapsedSeconds();
    v.truePeakMax = tpMax;
    loudness->setValues (v);

    const int mode = juce::roundToInt (raw (k::mode));
    const bool modified = owner.isModeModified();
    if (mode != lastMode || modified != lastModified)
    {
        lastMode = mode;
        lastModified = modified;
        modes->setMode (mode);
        captions->setModeLabel (U ("MODO · ") + U (nel::modes[static_cast<size_t> (mode)].name).toUpperCase() + (modified ? "*" : ""));
    }

    const bool releaseAuto = raw (k::releaseAuto) >= 0.5f;
    if (releaseAuto != lastReleaseAuto && releaseKnob != nullptr)
    {
        lastReleaseAuto = releaseAuto;
        releaseKnob->refreshValue();
    }

    // El perfil de dither solo cuenta con dither encendido.
    const bool dither = juce::roundToInt (raw (k::ditherBits)) > 0;
    ditherShape->setAlpha (dither ? 1.0f : 0.35f);
    ditherShape->setInterceptsMouseClicks (dither, false);
}
