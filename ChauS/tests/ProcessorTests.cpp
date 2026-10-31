#include <catch2/catch_approx.hpp>

#include <chrono>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"
#include "Signals.h"
#include "TestUtils.h"
#include "ui/ChausUi.h"

TEST_CASE ("ChauS: parámetros aleatorios, tasas, bloques y canales extremos") { suite::fuzz<ChauSProcessor>(); }
TEST_CASE ("ChauS: el estado se guarda y se restaura") { suite::stateRoundTrip<ChauSProcessor>(); }
TEST_CASE ("ChauS: el editor se abre, se redimensiona y se cierra") { suite::editorLifecycle<ChauSProcessor>(); }
TEST_CASE ("ChauS: configuraciones de canales") { suite::busLayouts<ChauSProcessor>(); }
TEST_CASE ("ChauS: processBlock no reserva memoria") { suite::noAllocationsInProcessBlock<ChauSProcessor> ([] { return allocguard::Scope(); }); }

TEST_CASE ("ChauS: el bypass es el del anfitrión")
{
    ChauSProcessor p;
    REQUIRE (p.getBypassParameter() != nullptr);
}

namespace
{
void set (ChauSProcessor& p, const char* id, float real)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (real));
}

float real (ChauSProcessor& p, const char* id)
{
    auto* param = p.apvts.getParameter (id);
    return param->convertFrom0to1 (param->getValue());
}
} // namespace

TEST_CASE ("ChauS: el sidechain externo se acepta apagado, mono o estéreo")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    const auto layout = [] (const juce::AudioChannelSet& sc)
    {
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (juce::AudioChannelSet::stereo());
        l.inputBuses.add (sc);
        l.outputBuses.add (juce::AudioChannelSet::stereo());
        return l;
    };
    REQUIRE (p.isBusesLayoutSupported (layout (juce::AudioChannelSet::disabled())));
    REQUIRE (p.isBusesLayoutSupported (layout (juce::AudioChannelSet::mono())));
    REQUIRE (p.isBusesLayoutSupported (layout (juce::AudioChannelSet::stereo())));
    REQUIRE_FALSE (p.isBusesLayoutSupported (layout (juce::AudioChannelSet::create5point1())));
}

TEST_CASE ("ChauS: los 24 parámetros tienen los IDs de la ficha y ninguno se perdió")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    const char* ids[] = { "mode", "thr", "adaptive", "range", "hp", "lp", "track", "q", "qAuto", "band", "shape", "sensSoft", "sensHard",
                          "lookOn", "look", "release", "releaseAuto", "link", "ms", "scExt", "listenTrig", "listenSC", "output", "bypass" };
    REQUIRE (p.getParameters().size() == int (std::size (ids)));
    for (auto* id : ids)
    {
        INFO (id);
        REQUIRE (p.apvts.getParameter (id) != nullptr);
    }
    // Valores por defecto de la ficha.
    REQUIRE (real (p, "thr") == Catch::Approx (-30.0f));
    REQUIRE (real (p, "range") == Catch::Approx (8.0f));
    REQUIRE (real (p, "hp") == Catch::Approx (4000.0f).margin (10.0f));
    REQUIRE (real (p, "lp") == Catch::Approx (12000.0f).margin (10.0f));
    REQUIRE (real (p, "look") == Catch::Approx (10.0f));
    REQUIRE (real (p, "link") == Catch::Approx (100.0f));
    REQUIRE (juce::roundToInt (real (p, "shape")) == 2);
    REQUIRE (real (p, "adaptive") > 0.5f);
    REQUIRE (real (p, "scExt") < 0.5f);
}

TEST_CASE ("ChauS: la latencia declarada es la del lookahead y coincide con la medida con un impulso")
{
    juce::ScopedJuceInitialiser_GUI gui;
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (float look : { 0.0f, 4.0f, 10.0f, 15.0f })
        {
            ChauSProcessor p;
            set (p, "look", look);
            set (p, "range", 0.0f);
            p.setPlayConfigDetails (2, 2, fs, 256);
            p.prepareToPlay (fs, 256);
            const int declared = p.getLatencySamples();
            REQUIRE (declared == int (look * 0.001 * fs + 0.5));

            juce::AudioBuffer<float> buf (2, 256);
            juce::MidiBuffer midi;
            std::vector<float> out;
            for (int b = 0; b < 40; ++b)
            {
                buf.clear();
                if (b == 10)
                    for (int ch = 0; ch < 2; ++ch)
                        buf.setSample (ch, 0, 0.1f);
                p.processBlock (buf, midi);
                for (int i = 0; i < 256; ++i)
                    out.push_back (buf.getSample (0, i));
            }
            size_t peak = 0;
            for (size_t i = 0; i < out.size(); ++i)
                if (std::abs (out[i]) > std::abs (out[peak]))
                    peak = i;
            INFO ("fs " << fs << ", lookahead " << look << ", declarada " << declared);
            REQUIRE (int (peak) == 10 * 256 + declared);
        }
}

TEST_CASE ("ChauS: la latencia se actualiza al mover el lookahead o apagarlo, y se avisa al anfitrión")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 256);
    p.prepareToPlay (48000.0, 256);
    REQUIRE (p.getLatencySamples() == 480);
    set (p, "look", 15.0f);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (p.getLatencySamples() == 720);
    set (p, "lookOn", 0.0f);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (p.getLatencySamples() == 0);
    set (p, "lookOn", 1.0f);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (p.getLatencySamples() == 720);
}

TEST_CASE ("ChauS: los 10 presets de fábrica cargan, se ven como programas del anfitrión y Init es el valor por defecto")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    REQUIRE (p.getNumPrograms() == 10);
    const char* expected[] = { "Init", "Voz masculina", "Voz femenina", "Locución", "Rap", "Coros", "Voz agresiva", "Podcast", "Batería (platillos)", "Mezcla completa" };
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        p.setCurrentProgram (i);
        INFO (p.getProgramName (i));
        REQUIRE (p.getProgramName (i) == juce::String::fromUTF8 (expected[i]));
        REQUIRE (p.getPresets().getCurrentName() == p.getProgramName (i));
        REQUIRE_FALSE (p.getPresets().isModified());
    }
    p.setCurrentProgram (6); // Voz agresiva
    REQUIRE (real (p, "thr") == Catch::Approx (-26.0f));
    REQUIRE (real (p, "range") == Catch::Approx (14.0f));
    REQUIRE (real (p, "sensHard") == Catch::Approx (2.0f));
    p.setCurrentProgram (9); // Mezcla completa: General y banda ancha
    REQUIRE (juce::roundToInt (real (p, "mode")) == 1);
    REQUIRE (juce::roundToInt (real (p, "band")) == 1);
    p.setCurrentProgram (0);
    REQUIRE (real (p, "thr") == Catch::Approx (-30.0f));
    REQUIRE (juce::roundToInt (real (p, "mode")) == 0);
    REQUIRE (juce::roundToInt (real (p, "band")) == 0);
    REQUIRE (real (p, "hp") == Catch::Approx (4000.0f).margin (10.0f));
}

TEST_CASE ("ChauS: el bypass no forma parte de los presets ni del A/B")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    set (p, "bypass", 1.0f);
    p.setCurrentProgram (3);
    REQUIRE (real (p, "bypass") > 0.5f); // cargar un preset no lo toca
    p.getAb().select (1);
    set (p, "bypass", 0.0f);
    p.getAb().select (0);
    REQUIRE (real (p, "bypass") < 0.5f);
}

TEST_CASE ("ChauS: A/B y deshacer")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    set (p, "range", 5.0f);
    p.getAb().select (1);
    set (p, "range", 12.0f);
    p.getAb().select (0);
    REQUIRE (real (p, "range") == Catch::Approx (5.0f));
    p.getAb().select (1);
    REQUIRE (real (p, "range") == Catch::Approx (12.0f));

    const auto settle = [] { juce::MessageManager::getInstance()->runDispatchLoopUntil (300); };
    set (p, "thr", -40.0f);
    settle();
    p.undoManager.beginNewTransaction();
    set (p, "thr", -20.0f);
    settle();
    REQUIRE (p.undoManager.canUndo());
    p.undoManager.undo();
    REQUIRE (real (p, "thr") == Catch::Approx (-40.0f));
}

TEST_CASE ("ChauS: con audio real el procesador reduce las eses y el bypass del anfitrión las deja pasar")
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double fs = 48000.0;
    const auto phrase = sig::makePhrase (fs, 8, 5, -14.0);
    ChauSProcessor p;
    set (p, "thr", -50.0f);
    set (p, "range", 12.0f);
    p.setPlayConfigDetails (2, 2, fs, 512);
    p.prepareToPlay (fs, 512);
    const int latency = p.getLatencySamples();

    const auto run = [&] (bool bypass)
    {
        set (p, "bypass", bypass ? 1.0f : 0.0f);
        p.reset();
        p.prepareToPlay (fs, 512);
        std::vector<float> out;
        juce::AudioBuffer<float> buf (2, 512);
        juce::MidiBuffer midi;
        for (size_t pos = 0; pos + 512 <= phrase.audio.size(); pos += 512)
        {
            for (int ch = 0; ch < 2; ++ch)
                std::copy_n (phrase.audio.begin() + long (pos), 512, buf.getWritePointer (ch));
            p.processBlock (buf, midi);
            for (int i = 0; i < 512; ++i)
                out.push_back (buf.getSample (0, i));
        }
        return out;
    };
    const auto processed = run (false), bypassed = run (true);
    double diffProcessed = 0.0, diffBypassed = 0.0;
    for (size_t i = size_t (latency + 20000); i < processed.size(); ++i)
    {
        diffProcessed = std::max (diffProcessed, double (std::abs (processed[i] - phrase.audio[i - size_t (latency)])));
        diffBypassed = std::max (diffBypassed, double (std::abs (bypassed[i] - phrase.audio[i - size_t (latency)])));
    }
    REQUIRE (diffProcessed > 0.02); // actuó
    REQUIRE (diffBypassed < 1.0e-5); // bypass: entrada retrasada
}

TEST_CASE ("Pantalla: el analizador de espectro mide el nivel eficaz de una senoidal y la sitúa en su frecuencia")
{
    juce::ScopedJuceInitialiser_GUI gui;
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (double freq : { 1000.0, 7000.0, 12000.0 })
        {
            ferb::SampleRing ring;
            ring.prepare (16384);
            const auto x = testutil::sine (8192, freq, fs, 0.1);
            ring.push (x.data(), int (x.size()));
            chaus::ui::SpectrumAnalyzer analyzer;
            std::array<float, chaus::ui::SpectrumAnalyzer::numPoints> db {};
            REQUIRE (analyzer.analyse (ring, fs, db));

            int peak = 0;
            for (int i = 0; i < int (db.size()); ++i)
                if (db[size_t (i)] > db[size_t (peak)])
                    peak = i;
            const double peakHz = chaus::ui::SpectrumAnalyzer::pointFrequency (peak);
            INFO ("fs " << fs << ", frecuencia " << freq << ", máximo en " << peakHz << " Hz con " << db[size_t (peak)] << " dB");
            REQUIRE (std::abs (std::log2 (peakHz / freq)) < 0.1);       // dentro de ~7 % (un punto de la rejilla es 1/24 de octava)
            REQUIRE (db[size_t (peak)] == Catch::Approx (-23.0f).margin (3.1)); // 0,1 de amplitud = −23 dBFS eficaces (la banda no recoge todo el lóbulo)
            // Lejos de la senoidal no queda casi nada.
            const int far = peak > 80 ? 10 : 150;
            REQUIRE (db[size_t (far)] < db[size_t (peak)] - 40.0f);
        }
}

TEST_CASE ("Pantalla: los anillos del procesador llevan la entrada y la salida (suma a mono)")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    set (p, "range", 0.0f);
    set (p, "look", 0.0f);
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    std::vector<float> expected;
    for (int b = 0; b < 8; ++b)
    {
        for (int i = 0; i < 512; ++i)
        {
            const float l = 0.3f * std::sin (0.05f * float (b * 512 + i)), r = 0.1f * std::sin (0.11f * float (b * 512 + i));
            buf.setSample (0, i, l);
            buf.setSample (1, i, r);
            expected.push_back (0.5f * (l + r));
        }
        p.processBlock (buf, midi);
    }
    std::vector<float> in (1000), out (1000);
    p.getInputRing().copyLatest (in.data(), 1000);
    p.getOutputRing().copyLatest (out.data(), 1000);
    for (int i = 0; i < 1000; ++i)
    {
        REQUIRE (in[size_t (i)] == Catch::Approx (expected[expected.size() - 1000 + size_t (i)]).margin (1.0e-6));
        REQUIRE (out[size_t (i)] == Catch::Approx (in[size_t (i)]).margin (1.0e-4)); // Rango 0 y sin lookahead: pasa igual
    }
}

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink;
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}

TEST_CASE ("ChauS: restaurar el estado devuelve cada parámetro a su valor guardado (como pluginval)")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);
    juce::Random rng (5);

    for (int round = 0; round < 6; ++round)
    {
        juce::MemoryBlock saved;
        std::vector<std::pair<juce::RangedAudioParameter*, float>> before;
        for (auto* param : p.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (param))
                before.emplace_back (r, r->getValue());
        p.getStateInformation (saved);

        for (auto& [param, value] : before)
            param->setValueNotifyingHost (rng.nextFloat());
        p.setStateInformation (saved.getData(), int (saved.getSize()));

        for (auto& [param, value] : before)
        {
            INFO ("ronda " << round << ", parámetro " << param->getName (64));
            CHECK (std::abs (param->getValue() - value) < 0.1f);
        }
        for (auto& [param, value] : before)
            param->setValueNotifyingHost (rng.nextFloat() > 0.5f ? 1.0f : 0.0f);
    }
}

TEST_CASE ("Informe: calibración del espectro de la pantalla", "[.report]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    double worst = 0.0;
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (double freq : { 300.0, 1000.0, 3000.0, 7000.0, 12000.0, 16000.0 })
        {
            ferb::SampleRing ring;
            ring.prepare (16384);
            const auto x = testutil::sine (8192, freq, fs, 0.1);
            ring.push (x.data(), int (x.size()));
            chaus::ui::SpectrumAnalyzer analyzer;
            std::array<float, chaus::ui::SpectrumAnalyzer::numPoints> db {};
            analyzer.analyse (ring, fs, db);
            worst = std::max (worst, double (std::abs (*std::max_element (db.begin(), db.end()) + 23.01f)));
        }
    std::printf ("espectro de la pantalla: peor error del máximo frente a −23,01 dBFS eficaces: %.2f dB\n", worst);
}

TEST_CASE ("Informe: costo de pintar la pantalla completa a 75, 100 y 200 %", "[.report]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);
    const auto phrase = sig::makePhrase (48000.0, 8, 3, -16.0);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    for (size_t pos = 0; pos + 512 <= phrase.audio.size(); pos += 512)
    {
        for (int ch = 0; ch < 2; ++ch)
            std::copy_n (phrase.audio.begin() + long (pos), 512, buf.getWritePointer (ch));
        p.processBlock (buf, midi);
    }
    std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
    editor->setVisible (true);
    for (double scale : { 0.75, 1.0, 2.0 })
    {
        editor->setSize (int (960 * scale), int (580 * scale));
        juce::MessageManager::getInstance()->runDispatchLoopUntil (60);
        const auto start = std::chrono::steady_clock::now();
        const int frames = 20;
        for (int i = 0; i < frames; ++i)
            editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
        const double ms = std::chrono::duration<double, std::milli> (std::chrono::steady_clock::now() - start).count() / frames;
        std::printf ("pintar toda la ventana a %3.0f %%: %.2f ms por cuadro (con la interfaz entera; a 30 Hz hay 33 ms)\n", scale * 100.0, ms);
    }
}

TEST_CASE ("ChauS: el editor se puede abrir y pintar antes de prepareToPlay (como hacen los validadores)")
{
    juce::ScopedJuceInitialiser_GUI gui;
    ChauSProcessor p; // sin prepareToPlay
    std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
    editor->setVisible (true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (150); // el temporizador lee los anillos y la telemetría
    editor->createComponentSnapshot (editor->getLocalBounds());
    SUCCEED();
}
