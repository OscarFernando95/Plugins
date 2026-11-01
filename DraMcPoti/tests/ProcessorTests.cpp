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

TEST_CASE ("Dra. McPoti: con Mezcla 0 % el audio pasa sin cambios, retrasado la latencia declarada")
{
    juce::ScopedJuceInitialiser_GUI gui;
    DraMcPotiProcessor p;
    p.apvts.getParameter ("mix")->setValueNotifyingHost (0.0f);
    p.setPlayConfigDetails (2, 2, 48000.0, 256);
    p.prepareToPlay (48000.0, 256);
    const int latency = p.getLatencySamples();
    REQUIRE (latency == mcpoti::McPotiEngine::latencySamples());
    REQUIRE (latency > 0);

    juce::MidiBuffer midi;
    std::vector<float> in, out;
    for (int b = 0; b < 40; ++b)
    {
        juce::AudioBuffer<float> buf (2, 256);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < 256; ++i)
                buf.setSample (ch, i, 0.5f * std::sin (0.05f * float (b * 256 + i)));
        for (int i = 0; i < 256; ++i)
            in.push_back (buf.getSample (0, i));
        p.processBlock (buf, midi);
        for (int i = 0; i < 256; ++i)
            out.push_back (buf.getSample (0, i));
    }
    for (size_t i = 8000; i < in.size() - size_t (latency); ++i) // tras el suavizado de 20 ms
        REQUIRE (out[i + size_t (latency)] == Catch::Approx (in[i]).margin (1.0e-5));
}

TEST_CASE ("Dra. McPoti: la latencia declarada coincide con la medida con un impulso, con y sin fase lineal")
{
    juce::ScopedJuceInitialiser_GUI gui;
    for (int os : { 0, 2, 3 })
        for (int lin : { 0, 1 })
        {
            DraMcPotiProcessor p;
            p.apvts.getParameter ("mix")->setValueNotifyingHost (0.0f);
            auto set = [&p] (const char* id, float real)
            {
                auto* param = p.apvts.getParameter (id);
                param->setValueNotifyingHost (param->convertTo0to1 (real));
            };
            set ("os", float (os));
            set ("linPhase", float (lin));
            p.setPlayConfigDetails (2, 2, 48000.0, 256);
            p.prepareToPlay (48000.0, 256);
            const int declared = p.getLatencySamples();
            CHECK (declared == mcpoti::McPotiEngine::latencySamples (lin != 0));

            juce::AudioBuffer<float> buf (2, 256);
            juce::MidiBuffer midi;
            int peakAt = -1;
            for (int b = 0; b < 12; ++b)
            {
                buf.clear();
                if (b == 0)
                    for (int ch = 0; ch < 2; ++ch)
                        buf.setSample (ch, 0, 0.5f);
                p.processBlock (buf, midi);
                for (int i = 0; i < 256; ++i)
                    if (std::abs (buf.getSample (0, i)) > 0.4f)
                        peakAt = b * 256 + i;
            }
            INFO ("os " << os << ", fase lineal " << lin);
            CHECK (peakAt == declared);
        }
}

TEST_CASE ("Dra. McPoti: el cambio de modo aplica su plantilla a lo que no se tocó y respeta lo tocado")
{
    juce::ScopedJuceInitialiser_GUI gui;
    DraMcPotiProcessor p;
    auto set = [&p] (const char* id, float real)
    {
        auto* param = p.apvts.getParameter (id);
        param->setValueNotifyingHost (param->convertTo0to1 (real));
    };
    auto get = [&p] (const char* id)
    {
        auto* param = p.apvts.getParameter (id);
        return param->convertFrom0to1 (param->getValue());
    };
    REQUIRE (get ("tune") == Catch::Approx (4000.0f).margin (1.0f));
    set ("guard", 30.0f); // tocado: se respeta
    set ("mode", 1.0f);   // Master
    p.syncModeTemplate();
    CHECK (get ("tune") == Catch::Approx (5000.0f).margin (2.0f));
    CHECK (get ("guard") == Catch::Approx (30.0f).margin (0.6f));
    CHECK (get ("monoSafe") > 0.5f);
    CHECK (get ("xo2") == Catch::Approx (1600.0f).margin (2.0f));

    set ("os", 3.0f);     // un solo paso de diferencia (8× frente a 4×) ya es un valor tocado
    set ("mode", 0.0f);   // de vuelta a Vocal: tune vuelve; guard sigue como lo dejó el usuario
    p.syncModeTemplate();
    CHECK (get ("tune") == Catch::Approx (4000.0f).margin (2.0f));
    CHECK (get ("monoSafe") < 0.5f);
    CHECK (get ("guard") == Catch::Approx (30.0f).margin (0.6f));
    CHECK (get ("os") > 2.5f); // el 8× elegido se respeta
}
