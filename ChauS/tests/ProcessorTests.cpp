#include <catch2/catch_approx.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"

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

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink;
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}
