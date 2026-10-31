#include <catch2/catch_approx.hpp>


#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"
#include "Signals.h"

TEST_CASE ("Maru: parámetros aleatorios, tasas, bloques y canales extremos") { suite::fuzz<MaruProcessor>(); }
TEST_CASE ("Maru: el estado se guarda y se restaura") { suite::stateRoundTrip<MaruProcessor>(); }
TEST_CASE ("Maru: el editor se abre, se redimensiona y se cierra") { suite::editorLifecycle<MaruProcessor>(); }
TEST_CASE ("Maru: configuraciones de canales") { suite::busLayouts<MaruProcessor>(); }
TEST_CASE ("Maru: processBlock no reserva memoria") { suite::noAllocationsInProcessBlock<MaruProcessor> ([] { return allocguard::Scope(); }); }

TEST_CASE ("Maru: el bypass es el del anfitrión")
{
    MaruProcessor p;
    REQUIRE (p.getBypassParameter() != nullptr);
}


TEST_CASE ("Maru: el sidechain externo se acepta apagado, mono o estéreo")
{
    juce::ScopedJuceInitialiser_GUI gui;
    MaruProcessor p;
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

namespace
{
void set (MaruProcessor& p, const char* id, float real)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (real));
}

float real (MaruProcessor& p, const char* id)
{
    auto* param = p.apvts.getParameter (id);
    return param->convertFrom0to1 (param->getValue());
}
} // namespace

TEST_CASE ("Maru: los 19 parámetros tienen los IDs de la ficha y los valores por defecto")
{
    juce::ScopedJuceInitialiser_GUI gui;
    MaruProcessor p;
    const char* ids[] = { "amount", "engine", "range", "sens", "recovery", "gateAuto", "gateThresh", "attack", "hold", "hyst", "lookahead",
                          "scHP", "scLP", "scExt", "link", "latency", "listen", "output", "bypass" };
    REQUIRE (p.getParameters().size() == int (std::size (ids)));
    for (auto* id : ids)
    {
        INFO (id);
        REQUIRE (p.apvts.getParameter (id) != nullptr);
    }
    REQUIRE (real (p, "amount") == Catch::Approx (50.0f));
    REQUIRE (juce::roundToInt (real (p, "engine")) == 0);
    REQUIRE (real (p, "range") == Catch::Approx (-30.0f));
    REQUIRE (real (p, "recovery") == Catch::Approx (120.0f).margin (1.0f));
    REQUIRE (real (p, "gateAuto") > 0.5f);
    REQUIRE (real (p, "gateThresh") == Catch::Approx (-50.0f));
    REQUIRE (real (p, "attack") == Catch::Approx (2.0f).margin (0.1f));
    REQUIRE (real (p, "hold") == Catch::Approx (40.0f));
    REQUIRE (real (p, "hyst") == Catch::Approx (6.0f));
    REQUIRE (real (p, "lookahead") == Catch::Approx (3.0f));
    REQUIRE (real (p, "scHP") == Catch::Approx (80.0f).margin (1.0f));
    REQUIRE (real (p, "scLP") == Catch::Approx (12000.0f).margin (10.0f));
    REQUIRE (real (p, "link") == Catch::Approx (100.0f));
    REQUIRE (juce::roundToInt (real (p, "latency")) == 1);
}

TEST_CASE ("Maru: la latencia declarada es la del motor y coincide con la medida con un impulso")
{
    juce::ScopedJuceInitialiser_GUI gui;
    struct Case { int engine, latency; float lookahead; };
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (auto c : { Case { 0, 1, 3.0f }, Case { 0, 0, 3.0f }, Case { 1, 1, 0.0f }, Case { 1, 1, 6.0f } })
        {
            MaruProcessor p;
            set (p, "engine", float (c.engine));
            set (p, "latency", float (c.latency));
            set (p, "lookahead", c.lookahead);
            set (p, "amount", 0.0f); // sin efecto: la salida es la entrada retrasada
            p.setPlayConfigDetails (2, 2, fs, 256);
            p.prepareToPlay (fs, 256);
            const int declared = p.getLatencySamples();
            const int expected = c.engine == 1 ? int (std::lround (c.lookahead * 0.001 * fs)) : maru::MaruEngine::fftSizeFor (fs, c.latency == 0);
            REQUIRE (declared == expected);

            juce::AudioBuffer<float> buf (2, 256);
            juce::MidiBuffer midi;
            std::vector<float> out;
            for (int b = 0; b < 80; ++b)
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
            INFO ("fs " << fs << ", motor " << c.engine << ", calidad " << c.latency << ", lookahead " << c.lookahead << ", declarada " << declared);
            REQUIRE (int (peak) == 10 * 256 + declared);
        }
}

TEST_CASE ("Maru: la latencia se actualiza al cambiar de motor, de calidad o el lookahead")
{
    juce::ScopedJuceInitialiser_GUI gui;
    MaruProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 256);
    p.prepareToPlay (48000.0, 256);
    REQUIRE (p.getLatencySamples() == 1024);
    set (p, "latency", 0.0f);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (p.getLatencySamples() == 512);
    set (p, "engine", 1.0f);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (p.getLatencySamples() == 144); // lookahead de 3 ms
    set (p, "lookahead", 10.0f);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (p.getLatencySamples() == 480);
}

TEST_CASE ("Maru: los 10 presets de fábrica cargan, son programas del anfitrión y Init es el valor por defecto")
{
    juce::ScopedJuceInitialiser_GUI gui;
    MaruProcessor p;
    REQUIRE (p.getNumPrograms() == 10);
    const char* expected[] = { "Init", "Voz de podcast", "Locución", "Voz con ventilador", "Zumbido de sala", "Guitarra acústica",
                               "Amplificador (gate)", "Batería (gate)", "Instrumento suave", "Rescate fuerte" };
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        p.setCurrentProgram (i);
        INFO (p.getProgramName (i));
        REQUIRE (p.getProgramName (i) == juce::String::fromUTF8 (expected[i]));
        REQUIRE (p.getPresets().getCurrentName() == p.getProgramName (i));
        REQUIRE_FALSE (p.getPresets().isModified());
    }
    p.setCurrentProgram (6); // Amplificador (gate)
    REQUIRE (juce::roundToInt (real (p, "engine")) == 1);
    REQUIRE (real (p, "amount") == Catch::Approx (60.0f));
    REQUIRE (real (p, "range") == Catch::Approx (-40.0f));
    REQUIRE (real (p, "hold") == Catch::Approx (80.0f));
    REQUIRE (real (p, "hyst") == Catch::Approx (8.0f));
    p.setCurrentProgram (0);
    REQUIRE (juce::roundToInt (real (p, "engine")) == 0);
    REQUIRE (real (p, "amount") == Catch::Approx (50.0f));
    REQUIRE (real (p, "range") == Catch::Approx (-30.0f));
}

TEST_CASE ("Maru: el bypass no forma parte de los presets ni del A/B")
{
    juce::ScopedJuceInitialiser_GUI gui;
    MaruProcessor p;
    set (p, "bypass", 1.0f);
    p.setCurrentProgram (3);
    REQUIRE (real (p, "bypass") > 0.5f);
    p.getAb().select (1);
    set (p, "bypass", 0.0f);
    p.getAb().select (0);
    REQUIRE (real (p, "bypass") < 0.5f);
}

TEST_CASE ("Maru: A/B y deshacer")
{
    juce::ScopedJuceInitialiser_GUI gui;
    MaruProcessor p;
    set (p, "amount", 30.0f);
    p.getAb().select (1);
    set (p, "amount", 80.0f);
    p.getAb().select (0);
    REQUIRE (real (p, "amount") == Catch::Approx (30.0f));
    p.getAb().select (1);
    REQUIRE (real (p, "amount") == Catch::Approx (80.0f));

    const auto settle = [] { juce::MessageManager::getInstance()->runDispatchLoopUntil (300); };
    set (p, "range", -20.0f);
    settle();
    p.undoManager.beginNewTransaction();
    set (p, "range", -50.0f);
    settle();
    REQUIRE (p.undoManager.canUndo());
    p.undoManager.undo();
    REQUIRE (real (p, "range") == Catch::Approx (-20.0f));
}

TEST_CASE ("Maru: con audio real el procesador reduce el ruido y el bypass del anfitrión deja pasar la entrada retrasada")
{
    juce::ScopedJuceInitialiser_GUI gui;
    const double fs = 48000.0;
    const int n = int (fs * 6.0);
    const auto in = sig::whiteNoise (n, -40.0, 3);
    MaruProcessor p;
    set (p, "amount", 100.0f);
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
        for (size_t pos = 0; pos + 512 <= in.size(); pos += 512)
        {
            for (int ch = 0; ch < 2; ++ch)
                std::copy_n (in.begin() + long (pos), 512, buf.getWritePointer (ch));
            p.processBlock (buf, midi);
            for (int i = 0; i < 512; ++i)
                out.push_back (buf.getSample (0, i));
        }
        return out;
    };
    const auto processed = run (false), bypassed = run (true);
    double inRms = 0.0, outRms = 0.0, diffBypassed = 0.0;
    for (size_t i = size_t (fs * 4.0); i < processed.size(); ++i)
    {
        inRms += double (in[i - size_t (latency)]) * in[i - size_t (latency)];
        outRms += double (processed[i]) * processed[i];
        diffBypassed = std::max (diffBypassed, double (std::abs (bypassed[i] - in[i - size_t (latency)])));
    }
    REQUIRE (10.0 * std::log10 (inRms / outRms) > 15.0);
    REQUIRE (diffBypassed < 1.0e-5);
}

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink;
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}

TEST_CASE ("Maru: restaurar el estado devuelve cada parámetro a su valor guardado (como pluginval)")
{
    juce::ScopedJuceInitialiser_GUI gui;
    MaruProcessor p;
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

TEST_CASE ("Maru: el editor se puede abrir y pintar antes de prepareToPlay (como hacen los validadores)")
{
    juce::ScopedJuceInitialiser_GUI gui;
    MaruProcessor p; // sin prepareToPlay
    std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
    editor->setVisible (true);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (150); // el temporizador lee los anillos y la telemetría
    editor->createComponentSnapshot (editor->getLocalBounds());
    SUCCEED();
}

