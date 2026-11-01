#include "PluginEditor.h"

#include <Format.h>
#include <juce_dsp/juce_dsp.h>

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

/** Ventana de armónicos: espectro de la entrada (tenue) y de la salida (ámbar) con una FFT de 2048 puntos en el hilo de
    interfaz, los cruces como cortes verticales y la Sintonización como línea punteada. */
class DraMcPotiEditor::Harmonics : public GlassView, private juce::Timer
{
public:
    Harmonics (const Palette& palette, DraMcPotiProcessor& p) : GlassView (palette), owner (p)
    {
        inDb.fill (floorDb);
        outDb.fill (floorDb);
        startTimerHz (30);
    }
    ~Harmonics() override { stopTimer(); }

private:
    static constexpr int order = 11, size = 1 << order;
    static constexpr float floorDb = -90.0f;
    static constexpr double minFreq = 40.0, maxFreq = 20000.0;

    void timerCallback() override
    {
        const double sr = owner.getSampleRate() > 0.0 ? owner.getSampleRate() : 48000.0;
        analyze (owner.getInputRing(), inDb, sr);
        analyze (owner.getOutputRing(), outDb, sr);
        repaint();
    }

    void analyze (const ferb::SampleRing& ring, std::array<float, 400>& db, double sr)
    {
        ring.copyLatest (samples.data(), size);
        window.multiplyWithWindowingTable (samples.data(), size);
        std::fill (data.begin(), data.end(), 0.0f);
        std::copy (samples.begin(), samples.end(), data.begin());
        fft.performFrequencyOnlyForwardTransform (data.data());
        for (size_t x = 0; x < db.size(); ++x)
        {
            const double f0 = minFreq * std::pow (maxFreq / minFreq, double (x) / double (db.size())),
                         f1 = minFreq * std::pow (maxFreq / minFreq, double (x + 1) / double (db.size()));
            const int b0 = juce::jlimit (1, size / 2 - 1, int (f0 / sr * size)), b1 = juce::jlimit (b0, size / 2 - 1, int (f1 / sr * size));
            float peak = 0.0f;
            for (int b = b0; b <= b1; ++b)
                peak = std::max (peak, data[static_cast<size_t> (b)]);
            const float level = 20.0f * std::log10 (peak / (0.5f * size * 0.5f) + 1.0e-7f); // ventana de Hann: ganancia coherente 0,5
            db[x] = db[x] * 0.6f + std::max (level, floorDb) * 0.4f;
        }
    }

    float freqToX (double f, float w) const { return float (std::log (f / minFreq) / std::log (maxFreq / minFreq)) * w; }

    juce::Path makePath (const std::array<float, 400>& db, float w, float h) const
    {
        juce::Path path;
        for (size_t x = 0; x < db.size(); ++x)
        {
            const float px = w * float (x) / float (db.size() - 1), py = h * (1.0f - (db[x] - floorDb) / -floorDb);
            x == 0 ? path.startNewSubPath (px, py) : path.lineTo (px, py);
        }
        return path;
    }

    void paintContent (juce::Graphics& g, float w, float h) override
    {
        g.fillAll (juce::Colour (0xff120d09));
        const auto raw = [this] (const char* id) { return owner.apvts.getRawParameterValue (id)->load(); };
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        for (double f : { 100.0, 1000.0, 10000.0 })
            g.fillRect (freqToX (f, w), 0.0f, 1.0f, h);
        g.setColour (juce::Colour (0xffffa030).withAlpha (0.35f)); // cortes de las bandas
        for (const char* id : { k::xo1, k::xo2, k::xo3 })
            g.fillRect (freqToX (raw (id), w), 0.0f, 1.0f, h);
        const float tuneX = freqToX (raw (k::tune), w); // Sintonización, punteada
        for (float y = 0.0f; y < h; y += 6.0f)
            g.fillRect (tuneX, y, 1.0f, 3.0f);

        g.setColour (juce::Colours::white.withAlpha (0.3f));
        g.strokePath (makePath (inDb, w, h), juce::PathStrokeType (1.0f));
        g.setColour (juce::Colour (0xffffa030));
        g.strokePath (makePath (outDb, w, h), juce::PathStrokeType (1.4f));
    }

    DraMcPotiProcessor& owner;
    juce::dsp::FFT fft { order };
    juce::dsp::WindowingFunction<float> window { static_cast<size_t> (size), juce::dsp::WindowingFunction<float>::hann };
    std::array<float, size> samples {};
    std::array<float, 2 * size> data {};
    std::array<float, 400> inDb, outDb;
};

DraMcPotiEditor::DraMcPotiEditor (DraMcPotiProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75), owner (p)
{
    auto param = [&p] (const juce::String& id) -> juce::RangedAudioParameter& { return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (id)); };

    frame = std::make_unique<Frame> (palette, true, 44); // orejas de acero con ranuras, como la maqueta
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
    seg (k::os, { "1X", "2X", "4X", "8X" }, 40, 30, 548);
    addToggle (k::linPhase, "FASE LINEAL", 220, 548, 110);
    addToggle (k::delta, "DELTA", 340, 548, 80);
    addToggle (k::match, "IGUALAR", 430, 548, 90);
    addToggle (k::monoSafe, "MONO SEGURO", 530, 548, 136);
    monoButton = buttons.back().get();

    // Chips de tamaño: el redimensionado de 75 % a 200 % también se hace arrastrando la esquina de la ventana.
    int chipX = 1130;
    for (const auto& [text, scale] : { std::pair { "75 %", 0.75 }, std::pair { "100 %", 1.0 }, std::pair { "150 %", 1.5 }, std::pair { "200 %", 2.0 } })
    {
        auto chip = std::make_unique<VButton> (palette, U (text));
        chip->setBounds (chipX, 548, 56, 30);
        chip->onClick = [this, scale = scale] { setSize (juce::roundToInt (baseWidth * scale), juce::roundToInt (baseHeight * scale)); };
        content.addAndMakeVisible (*chip);
        buttons.push_back (std::move (chip));
        chipX += 62;
    }

    harmonics = std::make_unique<Harmonics> (palette, p);
    harmonics->setGlassBounds ({ 268, 492, 780, 44 });
    content.addAndMakeVisible (*harmonics);

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

    const bool bypassed = raw (k::bypass) >= 0.5f;
    for (size_t b = 0; b < tubes.size(); ++b)
    {
        const float level = owner.getEngine().addedLevel (static_cast<int> (b));
        const float db = 20.0f * std::log10 (level + 1.0e-6f);
        // Con Bypass las válvulas quedan a media luz (el filamento sigue caliente, sin añadir nada).
        tubes[b]->setGlow (bypassed ? 0.5f : juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 40.0f));
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
