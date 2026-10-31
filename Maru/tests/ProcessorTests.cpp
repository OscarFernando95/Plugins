#include <catch2/catch_approx.hpp>


#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"

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

