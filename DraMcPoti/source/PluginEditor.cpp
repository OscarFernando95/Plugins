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
    headerSpec.saveButton = true;
    header = std::make_unique<Header> (palette, headerSpec, p.getPresets(), p.getAb(), p.undoManager, param (k::bypass));
    header->setBounds (0, 0, baseWidth, 64);
    content.addAndMakeVisible (*header);

    jewel = std::make_unique<mcpoti::ui::Jewel>();
    jewel->setBounds (1178, 6, 72, 54);
    content.addAndMakeVisible (*jewel);

    auto seg = [&] (const char* id, juce::StringArray labels, int buttonWidth, int x, int y)
    {
        auto s = std::make_unique<VSeg> (param (id), palette, std::move (labels), buttonWidth, &p.undoManager);
        s->setTopLeftPosition (x, y);
        content.addAndMakeVisible (*s);
        segments.push_back (std::move (s));
    };

    // Izquierda: palancas de Modo y Era, selector rotativo de Voicing y los tres mandos de nivel.
    addLever (k::mode, { "MODO", "VOCAL", "MASTER", {}, true }, 20, 66, 110, 142);
    addLever (k::era, { "ERA", "VINTAGE", "MODERNO", {}, true }, 136, 66, 110, 142);
    voicing = std::make_unique<mcpoti::ui::VoicingKnob> (param (k::voice), palette, [&p] { return juce::roundToInt (p.apvts.getRawParameterValue (k::era)->load()); }, &p.undoManager);
    voicing->setBounds (12, 212, 236, 166);
    content.addAndMakeVisible (*voicing);
    addKnob (k::input, "ENTRADA", 56, false, 24, 390, 78, db());
    addKnob (k::mix, "MEZCLA", 56, false, 90, 390, 78, percent());
    addKnob (k::output, "SALIDA", 56, false, 156, 390, 78, db());

    // Cuatro módulos de banda: válvula, placa con el rango, Drive, y Dinámica y Nivel.
    const char* names[] = { "CUERPO", "MEDIOS", "PRESENCIA", "AIRE" };
    for (int b = 0; b < k::numBands; ++b)
    {
        const int x = 268 + b * 196;
        auto tube = std::make_unique<mcpoti::ui::GlassTube>();
        tube->setBounds (x + 65, 68, 40, 86);
        content.addAndMakeVisible (*tube);
        tubes.push_back (std::move (tube));
        auto plate = std::make_unique<mcpoti::ui::BandPlate> (names[b]);
        plate->setBounds (x + 15, 160, 140, 38);
        content.addAndMakeVisible (*plate);
        plates.push_back (std::move (plate));
        addKnob (k::bandId ("drive", b), "DRIVE", 84, true, x + 30, 204, 110, plain());
        addKnob (k::bandId ("dyn", b), "DINÁMICA", 50, false, x + 4, 340, 80, signedInt());
        addKnob (k::bandId ("mix", b), "NIVEL", 50, false, x + 100, 340, 80, percent());
        addToggle (k::bandId ("on", b), "ON", x + 18, 428, 70);
        addToggle (k::bandId ("solo", b), "SOLO", x + 98, 428, 70);
    }
    // Los cruces van entre los módulos, a la altura del Drive.
    for (int c = 0; c < 3; ++c)
        addKnob (std::array { k::xo1, k::xo2, k::xo3 }[static_cast<size_t> (c)], (U ("CRUCE ") + juce::String (c + 1)).toRawUTF8(), 40, false, 268 + c * 196 + 148, 236, 70, hz());

    // Derecha: vúmetros y los controles de lo añadido.
    vuIn = std::make_unique<mcpoti::ui::AmberVu> ("ENTRADA");
    vuIn->setBounds (1076, 66, 124, 98);
    content.addAndMakeVisible (*vuIn);
    vuOut = std::make_unique<mcpoti::ui::AmberVu> ("SALIDA");
    vuOut->setBounds (1210, 66, 124, 98);
    content.addAndMakeVisible (*vuOut);

    const int rx = 1074;
    addKnob (k::tune, "SINTONIZ.", 50, false, rx, 166, 80, hz());
    addKnob (k::phase, "FASE", 50, false, rx + 90, 166, 80, percent());
    addKnob (k::color, "COLOR", 50, false, rx + 180, 166, 80, signedInt());
    addKnob (k::soften, "SUAVIZAR", 50, false, rx, 264, 80, hz());
    guardKnob = &addKnob (k::guard, "GUARDA", 50, false, rx + 90, 264, 80, percent());
    widthKnob = &addKnob (k::width, "ANCHO", 50, false, rx + 180, 264, 80, percent());
    airKnob = &addKnob (k::airCorner, "ESQUINA", 50, false, rx, 362, 80, hz());
    addKnob (k::link, "ENLACE", 50, false, rx + 90, 362, 80, percent());
    addKnob (k::vuRef, "REF. VU", 50, false, rx + 180, 362, 80, [] (float v) { return ferb::fmt::comma (v, 1) + " dBFS"; });

    harmonics = std::make_unique<Harmonics> (palette, p);
    harmonics->setGlassBounds ({ 268, 486, 780, 38 });
    content.addAndMakeVisible (*harmonics);

    // Pie.
    seg (k::os, { "1X", "2X", "4X", "8X" }, 40, 30, 548);
    addLever (k::linPhase, { {}, {}, {}, "FASE LINEAL", false }, 215, 538, 150, 44);
    addLever (k::delta, { {}, {}, {}, "DELTA", false }, 375, 538, 120, 44);
    addLever (k::match, { {}, {}, {}, "IGUALAR", false }, 505, 538, 130, 44);
    addLever (k::monoSafe, { {}, {}, {}, "MONO SEGURO", false }, 645, 538, 170, 44);
    monoButton = levers.back().get();

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

    timerCallback();
    startTimerHz (20);
}

DraMcPotiEditor::~DraMcPotiEditor() { stopTimer(); }

/** Comportamiento por modo y era (sección 6 de la ficha): atenúa lo que no actúa, actualiza los rangos y las válvulas. */
void DraMcPotiEditor::timerCallback()
{
    const auto raw = [this] (const char* id) { return owner.apvts.getRawParameterValue (id)->load(); };
    const bool master = juce::roundToInt (raw (k::mode)) == k::Master;
    const int era = juce::roundToInt (raw (k::era)), voice = juce::roundToInt (raw (k::voice));

    guardKnob->setAlpha (master ? 0.35f : 1.0f);   // Guarda: solo Vocal
    widthKnob->setAlpha (master ? 1.0f : 0.35f);   // Ancho y Mono seguro: solo Master
    monoButton->setAlpha (master ? 1.0f : 0.35f);
    airKnob->setAlpha (era == k::Moderno && voice == 3 ? 1.0f : 0.35f); // Esquina de aire: solo con el carácter Aire
    voicing->refresh();                              // los nombres cambian con la era

    const float edges[5] = { 40.0f, raw (k::xo1), raw (k::xo2), raw (k::xo3), 20000.0f };
    for (size_t b = 0; b < plates.size(); ++b)
        plates[b]->setRange (edges[b], edges[b + 1]);

    // Vúmetros: 0 VU = Referencia VU (dBFS), con la balística de 65 ms del motor.
    const float ref = raw (k::vuRef);
    const auto toVu = [ref] (float level) { return 20.0f * std::log10 (level + 1.0e-6f) - ref; };
    vuIn->setValue (toVu (owner.getEngine().inputVuLevel()));
    vuOut->setValue (toVu (owner.getEngine().outputVuLevel()));

    const bool bypassed = raw (k::bypass) >= 0.5f;
    jewel->setLit (! bypassed);
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
    auto knob = std::make_unique<mcpoti::ui::ScaleKnob> (parameter, palette, spec, &owner.undoManager);
    knob->setBounds (x, y, w, VKnob::heightFor (spec));
    content.addAndMakeVisible (*knob);
    knobs.push_back (std::move (knob));
    return *knobs.back();
}

void DraMcPotiEditor::addLever (const juce::String& id, mcpoti::ui::ChromeToggle::Spec spec, int x, int y, int w, int h)
{
    auto& parameter = *dynamic_cast<juce::RangedAudioParameter*> (owner.apvts.getParameter (id));
    auto lever = std::make_unique<mcpoti::ui::ChromeToggle> (parameter, palette, std::move (spec), &owner.undoManager);
    lever->setBounds (x, y, w, h);
    content.addAndMakeVisible (*lever);
    levers.push_back (std::move (lever));
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
