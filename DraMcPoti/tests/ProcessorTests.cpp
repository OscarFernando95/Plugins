#include <catch2/catch_approx.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"

TEST_CASE ("Dra. McPoti: parámetros aleatorios, tasas, bloques y canales extremos") { suite::fuzz<DraMcPotiProcessor>(); }
TEST_CASE ("Dra. McPoti: el estado se guarda y se restaura") { suite::stateRoundTrip<DraMcPotiProcessor>(); }
TEST_CASE ("Dra. McPoti: el editor se abre, se redimensiona y se cierra") { suite::editorLifecycle<DraMcPotiProcessor>(); }
TEST_CASE ("Dra. McPoti: configuraciones de canales") { suite::busLayouts<DraMcPotiProcessor>(); }
TEST_CASE ("Dra. McPoti: processBlock no reserva memoria") { suite::noAllocationsInProcessBlock<DraMcPotiProcessor> ([] { return allocguard::Scope(); }); }

TEST_CASE ("Dra. McPoti: el bypass es el del anfitrión")
{
    DraMcPotiProcessor p;
    REQUIRE (p.getBypassParameter() != nullptr);
}

TEST_CASE ("Dra. McPoti: 44 parámetros, 21 programas y cada preset de fábrica carga")
{
    DraMcPotiProcessor p;
    CHECK (p.getParameters().size() == 44);
    CHECK (p.getNumPrograms() == 21); // 20 de la ficha + Init
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        p.setCurrentProgram (i);
        CHECK (p.getCurrentProgram() == i);
    }
}

TEST_CASE ("Dra. McPoti: el audio pasa sin cambios y sin latencia (fase 1)")
{
    juce::ScopedJuceInitialiser_GUI gui;
    DraMcPotiProcessor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 256);
    p.prepareToPlay (48000.0, 256);
    REQUIRE (p.getLatencySamples() == 0);

    juce::AudioBuffer<float> buf (2, 256), ref (2, 256);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            ref.setSample (ch, i, 0.5f * std::sin (0.05f * float (i + 7 * ch)));
    buf.makeCopyOf (ref);
    juce::MidiBuffer midi;
    p.processBlock (buf, midi);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 256; ++i)
            REQUIRE (buf.getSample (ch, i) == Catch::Approx (ref.getSample (ch, i)).margin (1.0e-9));
}
