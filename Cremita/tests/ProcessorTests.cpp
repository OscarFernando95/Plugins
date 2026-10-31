#include <catch2/catch_approx.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"

TEST_CASE ("Cremita: parámetros aleatorios, tasas, bloques y canales extremos") { suite::fuzz<CremitaProcessor>(); }
TEST_CASE ("Cremita: el estado se guarda y se restaura") { suite::stateRoundTrip<CremitaProcessor>(); }
TEST_CASE ("Cremita: el editor se abre, se redimensiona y se cierra") { suite::editorLifecycle<CremitaProcessor>(); }
TEST_CASE ("Cremita: latencia constante y bypass del anfitrión") { suite::constantLatency<CremitaProcessor>(); }
TEST_CASE ("Cremita: configuraciones de canales") { suite::busLayouts<CremitaProcessor>(); }
TEST_CASE ("Cremita: processBlock no reserva memoria") { suite::noAllocationsInProcessBlock<CremitaProcessor> ([] { return allocguard::Scope(); }); }

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink; // evita que el compilador elimine la reserva
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}

namespace
{
float real (CremitaProcessor& p, const char* id)
{
    auto* param = p.apvts.getParameter (id);
    return param->convertFrom0to1 (param->getValue());
}

void set (CremitaProcessor& p, const char* id, float value)
{
    auto* param = p.apvts.getParameter (id);
    param->setValueNotifyingHost (param->convertTo0to1 (value));
}
} // namespace

TEST_CASE ("Cremita: los 10 presets de fábrica cargan y Init equivale a los valores por defecto")
{
    juce::ScopedJuceInitialiser_GUI gui;
    CremitaProcessor p;
    const auto names = p.getPresets().getFactoryNames();
    REQUIRE (names.size() == 10);
    REQUIRE (names[0] == "Init");

    for (const auto& name : names)
    {
        INFO (name);
        REQUIRE (p.getPresets().load (name));
        REQUIRE (p.getPresets().getCurrentName() == name);
        REQUIRE_FALSE (p.getPresets().isModified());
    }

    REQUIRE (p.getPresets().load ("Presencia vocal"));
    REQUIRE (real (p, "curve") == Catch::Approx (10.0f));
    REQUIRE (p.getPresets().load ("Init"));
    REQUIRE (real (p, "curve") == Catch::Approx (0.0f));
    REQUIRE (real (p, "effect") == Catch::Approx (100.0f));
    REQUIRE (p.apvts.getParameter ("bypass")->getValue() == 0.0f);
}

TEST_CASE ("Cremita: A/B guarda dos ajustes distintos y los recupera")
{
    juce::ScopedJuceInitialiser_GUI gui;
    CremitaProcessor p;
    set (p, "curve", 20.0f);
    set (p, "bypass", 1.0f);

    p.getAb().select (1); // B nace como copia de A
    REQUIRE (real (p, "curve") == Catch::Approx (20.0f));
    set (p, "curve", -30.0f);
    set (p, "effect", 40.0f);

    p.getAb().select (0);
    REQUIRE (real (p, "curve") == Catch::Approx (20.0f));
    REQUIRE (real (p, "effect") == Catch::Approx (100.0f));
    p.getAb().select (1);
    REQUIRE (real (p, "curve") == Catch::Approx (-30.0f));
    REQUIRE (real (p, "effect") == Catch::Approx (40.0f));
    REQUIRE (p.apvts.getParameter ("bypass")->getValue() == 1.0f); // el bypass no cambia con A/B
}

TEST_CASE ("Cremita: deshacer y rehacer devuelven el valor anterior")
{
    juce::ScopedJuceInitialiser_GUI gui;
    CremitaProcessor p;
    // El APVTS vuelca los cambios de parámetros al árbol (y al historial) con un temporizador.
    const auto settle = [] { juce::MessageManager::getInstance()->runDispatchLoopUntil (300); };

    set (p, "curve", 10.0f);
    settle();
    p.undoManager.beginNewTransaction();
    set (p, "curve", 40.0f);
    settle();
    p.undoManager.beginNewTransaction();
    set (p, "curve", -5.0f);
    settle();

    REQUIRE (p.undoManager.canUndo());
    p.undoManager.undo();
    CHECK (real (p, "curve") == Catch::Approx (40.0f));
    p.undoManager.undo();
    CHECK (real (p, "curve") == Catch::Approx (10.0f));
    p.undoManager.redo();
    CHECK (real (p, "curve") == Catch::Approx (40.0f));
}
