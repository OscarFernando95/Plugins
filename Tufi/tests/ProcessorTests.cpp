#include <catch2/catch_approx.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"

TEST_CASE ("Tufi: parámetros aleatorios, tasas, bloques y canales extremos") { suite::fuzz<TufiProcessor>(); }
TEST_CASE ("Tufi: el estado se guarda y se restaura") { suite::stateRoundTrip<TufiProcessor>(); }
TEST_CASE ("Tufi: el editor se abre, se redimensiona y se cierra") { suite::editorLifecycle<TufiProcessor>(); }
TEST_CASE ("Tufi: latencia constante y bypass del anfitrión") { suite::constantLatency<TufiProcessor>(); }
TEST_CASE ("Tufi: configuraciones de canales") { suite::busLayouts<TufiProcessor>(); }
TEST_CASE ("Tufi: processBlock no reserva memoria") { suite::noAllocationsInProcessBlock<TufiProcessor> ([] { return allocguard::Scope(); }); }

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink; // evita que el compilador elimine la reserva
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}
