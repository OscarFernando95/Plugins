#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "PresetManager.h"

using Catch::Approx;

namespace
{
juce::String U (const char* utf8) { return juce::String::fromUTF8 (utf8); }

/** Procesador mínimo con parámetros de cada tipo, para probar los presets sin un plugin real. */
struct Dummy : juce::AudioProcessor
{
    Dummy()
        : AudioProcessor (BusesProperties().withInput ("i", juce::AudioChannelSet::stereo()).withOutput ("o", juce::AudioChannelSet::stereo())),
          apvts (*this, nullptr, "S", layout())
    {
    }

    static juce::AudioProcessorValueTreeState::ParameterLayout layout()
    {
        juce::AudioProcessorValueTreeState::ParameterLayout l;
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "gain", 1 }, "Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "freq", 1 }, "Freq", juce::NormalisableRange<float> (20.0f, 20000.0f, 0.0f, 0.3f), 1000.0f));
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "type", 1 }, "Type", juce::StringArray { "A", "B", "C" }, 1));
        l.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
        return l;
    }

    const juce::String getName() const override { return "Dummy"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;

    float real (const char* id) { auto* p = apvts.getParameter (id); return p->convertFrom0to1 (p->getValue()); }
    void set (const char* id, float real) { auto* p = apvts.getParameter (id); p->setValueNotifyingHost (p->convertTo0to1 (real)); }
};

struct TempDir
{
    juce::File dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("ferb-preset-test-" + juce::String (juce::Random::getSystemRandom().nextInt (1000000)));
    ~TempDir() { dir.deleteRecursively(); }
};

ferb::PresetManager make (Dummy& d, const TempDir& t)
{
    return ferb::PresetManager (d.apvts, "Dummy",
                                { { U ("Cálido"), { { "gain", 6.0f }, { "type", 2.0f } } }, { "Plano", {} } },
                                { "bypass" }, t.dir);
}
} // namespace

TEST_CASE ("Presets de fábrica: se cargan, y lo que no mencionan vuelve al valor por defecto")
{
    Dummy d;
    TempDir t;
    auto pm = make (d, t);

    d.set ("freq", 5000.0f);
    REQUIRE (pm.load (U ("Cálido")));
    REQUIRE (d.real ("gain") == Approx (6.0f).margin (0.05f));
    REQUIRE (d.real ("type") == Approx (2.0f));
    REQUIRE (d.real ("freq") == Approx (1000.0f).epsilon (0.01)); // no mencionado: por defecto
    REQUIRE (pm.getCurrentName() == U ("Cálido"));
    REQUIRE_FALSE (pm.isModified());

    d.set ("gain", -3.0f);
    REQUIRE (pm.isModified());
}

TEST_CASE ("Presets de usuario: guardar, listar, cargar y eliminar")
{
    Dummy d;
    TempDir t;
    auto pm = make (d, t);

    REQUIRE (pm.getUserNames().isEmpty());

    d.set ("gain", -4.5f);
    d.set ("freq", 3200.0f);
    d.set ("type", 0.0f);
    REQUIRE (pm.save (U ("Mi sonido ñandú")));
    REQUIRE (pm.getUserNames() == juce::StringArray { U ("Mi sonido ñandú") });
    REQUIRE (pm.getCurrentName() == U ("Mi sonido ñandú"));
    REQUIRE_FALSE (pm.isModified());

    d.set ("gain", 9.0f);
    d.set ("freq", 100.0f);
    REQUIRE (pm.load (U ("Mi sonido ñandú")));
    REQUIRE (d.real ("gain") == Approx (-4.5f).margin (0.05f));
    REQUIRE (d.real ("freq") == Approx (3200.0f).epsilon (0.01));
    REQUIRE (d.real ("type") == Approx (0.0f));

    REQUIRE (pm.remove (U ("Mi sonido ñandú")));
    REQUIRE (pm.getUserNames().isEmpty());
    REQUIRE (pm.getCurrentName().isEmpty());
}

TEST_CASE ("Los presets no tocan el bypass")
{
    Dummy d;
    TempDir t;
    auto pm = make (d, t);

    d.set ("bypass", 1.0f);
    pm.load ("Plano");
    REQUIRE (d.real ("bypass") == Approx (1.0f)); // sigue activado

    pm.save ("X");
    d.set ("bypass", 0.0f);
    pm.load ("X");
    REQUIRE (d.real ("bypass") == Approx (0.0f));
}

TEST_CASE ("Nombres: no se puede pisar uno de fábrica y se limpian los caracteres inválidos")
{
    Dummy d;
    TempDir t;
    auto pm = make (d, t);

    REQUIRE_FALSE (pm.save (U ("Cálido")));
    REQUIRE_FALSE (pm.save ("   "));
    REQUIRE_FALSE (pm.remove (U ("Cálido")));

    REQUIRE (pm.save ("a/b:c*d"));
    REQUIRE (pm.getUserNames() == juce::StringArray { "abcd" });
    REQUIRE (ferb::PresetManager::sanitize ("  hola\n") == "hola");
}

TEST_CASE ("Anterior y siguiente recorren fábrica y usuario, y dan la vuelta")
{
    Dummy d;
    TempDir t;
    auto pm = make (d, t);
    pm.save ("Zeta");

    pm.load (U ("Cálido"));
    pm.loadRelative (1);
    REQUIRE (pm.getCurrentName() == "Plano");
    pm.loadRelative (1);
    REQUIRE (pm.getCurrentName() == "Zeta");
    pm.loadRelative (1);
    REQUIRE (pm.getCurrentName() == U ("Cálido"));
    pm.loadRelative (-1);
    REQUIRE (pm.getCurrentName() == "Zeta");
}

TEST_CASE ("Un archivo roto no se carga ni rompe nada")
{
    Dummy d;
    TempDir t;
    auto pm = make (d, t);

    t.dir.createDirectory();
    t.dir.getChildFile ("roto.ferbpreset").replaceWithText ("esto no es xml <<<");
    REQUIRE_FALSE (pm.load ("roto"));
    REQUIRE (pm.getUserNames() == juce::StringArray { "roto" }); // se lista, pero no carga
}

TEST_CASE ("El nombre del preset va en el estado y se conserva al restaurarlo")
{
    Dummy a, b;
    TempDir t;
    auto pa = make (a, t);
    auto pb = make (b, t);

    pa.load (U ("Cálido"));
    b.apvts.replaceState (a.apvts.copyState());
    REQUIRE (pb.getCurrentName() == U ("Cálido"));
    REQUIRE_FALSE (pb.isModified());
}
