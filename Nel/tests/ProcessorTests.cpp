#include <catch2/catch_approx.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"
#include "dsp/Modes.h"

TEST_CASE ("Nel: parámetros aleatorios, tasas, bloques y canales extremos") { suite::fuzz<NelProcessor>(); }
TEST_CASE ("Nel: el estado se guarda y se restaura") { suite::stateRoundTrip<NelProcessor>(); }
TEST_CASE ("Nel: el editor se abre, se redimensiona y se cierra") { suite::editorLifecycle<NelProcessor>(); }
TEST_CASE ("Nel: configuraciones de canales") { suite::busLayouts<NelProcessor>(); }
TEST_CASE ("Nel: processBlock no reserva memoria") { suite::noAllocationsInProcessBlock<NelProcessor> ([] { return allocguard::Scope(); }); }

TEST_CASE ("Nel: el bypass es el del anfitrión")
{
    NelProcessor p;
    REQUIRE (p.getBypassParameter() != nullptr);
}

namespace
{
void set (NelProcessor& p, const char* id, float real)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (real));
}

float real (NelProcessor& p, const char* id)
{
    auto* param = p.apvts.getParameter (id);
    return param->convertFrom0to1 (param->getValue());
}
} // namespace

TEST_CASE ("Nel: la latencia declarada coincide con la medida con un impulso")
{
    juce::ScopedJuceInitialiser_GUI gui;
    for (int os : { 0, 2, 5 })
        for (int phase : { 0, 1 })
            for (float look : { 0.0f, 2.0f })
            {
                NelProcessor p;
                set (p, "oversample", float (os));
                set (p, "osPhase", float (phase));
                set (p, "lookahead", look);
                p.setPlayConfigDetails (2, 2, 48000.0, 256);
                p.prepareToPlay (48000.0, 256);
                const int declared = p.getLatencySamples();

                juce::AudioBuffer<float> buf (2, 256);
                juce::MidiBuffer midi;
                std::vector<float> out;
                for (int b = 0; b < 40; ++b)
                {
                    buf.clear();
                    if (b == 20)
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
                INFO ("os " << (1 << os) << ", fase " << phase << ", lookahead " << look << ", declarada " << declared);
                REQUIRE ((declared > 0) == (os > 0 || look > 0.0f));
                REQUIRE (std::abs (int (peak) - 20 * 256 - declared) <= phase);
            }
}

TEST_CASE ("Nel: la latencia cambia al soltar los controles y se avisa al anfitrión")
{
    juce::ScopedJuceInitialiser_GUI gui;
    NelProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 256);
    p.prepareToPlay (48000.0, 256);
    const int before = p.getLatencySamples();
    set (p, "oversample", 5.0f);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (p.getLatencySamples() > before);
    set (p, "oversample", 0.0f);
    set (p, "lookahead", 0.0f);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    REQUIRE (p.getLatencySamples() == 0);
}

TEST_CASE ("Nel: elegir un modo carga sus valores y modificar uno avanzado lo marca")
{
    juce::ScopedJuceInitialiser_GUI gui;
    NelProcessor p;
    for (int m = 0; m < nel::numModes; ++m)
    {
        p.selectMode (m);
        const auto& spec = nel::modes[size_t (m)];
        INFO (spec.name);
        REQUIRE (real (p, "lookahead") == Catch::Approx (spec.lookaheadMs).margin (0.06));
        REQUIRE (real (p, "attack") == Catch::Approx (spec.attackMs).epsilon (0.03));
        REQUIRE (real (p, "release") == Catch::Approx (spec.releaseMs).epsilon (0.01));
        REQUIRE (real (p, "knee") == Catch::Approx (spec.kneeDb).margin (0.06));
        REQUIRE_FALSE (p.isModeModified());
    }
    set (p, "release", 123.0f);
    REQUIRE (p.isModeModified());
}

TEST_CASE ("Nel: los 16 presets de fábrica cargan, se ven como programas del anfitrión y Init es el valor por defecto")
{
    juce::ScopedJuceInitialiser_GUI gui;
    NelProcessor p;
    REQUIRE (p.getNumPrograms() == 16);
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        p.setCurrentProgram (i);
        INFO (p.getProgramName (i));
        REQUIRE (p.getPresets().getCurrentName() == p.getProgramName (i));
        REQUIRE_FALSE (p.isModeModified());
    }
    p.setCurrentProgram (5); // Master electrónica
    REQUIRE (real (p, "gain") == Catch::Approx (8.0f));
    REQUIRE (juce::roundToInt (real (p, "mode")) == 5);
    p.setCurrentProgram (0);
    REQUIRE (real (p, "gain") == Catch::Approx (0.0f));
    REQUIRE (real (p, "ceiling") == Catch::Approx (-1.0f));
    REQUIRE (juce::roundToInt (real (p, "oversample")) == 2);
}

TEST_CASE ("Nel: A/B y deshacer")
{
    juce::ScopedJuceInitialiser_GUI gui;
    NelProcessor p;
    set (p, "gain", 5.0f);
    p.getAb().select (1);
    set (p, "gain", 9.0f);
    p.getAb().select (0);
    REQUIRE (real (p, "gain") == Catch::Approx (5.0f));
    p.getAb().select (1);
    REQUIRE (real (p, "gain") == Catch::Approx (9.0f));

    const auto settle = [] { juce::MessageManager::getInstance()->runDispatchLoopUntil (300); };
    set (p, "ceiling", -2.0f);
    settle();
    p.undoManager.beginNewTransaction();
    set (p, "ceiling", -6.0f);
    settle();
    REQUIRE (p.undoManager.canUndo());
    p.undoManager.undo();
    REQUIRE (real (p, "ceiling") == Catch::Approx (-2.0f));
}

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink;
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}
