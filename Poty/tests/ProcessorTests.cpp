#include <catch2/catch_approx.hpp>

#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "AllocGuard.h"
#include "ProcessorSuite.h"

TEST_CASE ("Poty: parámetros aleatorios, tasas, bloques y canales extremos") { suite::fuzz<PotyProcessor>(); }
TEST_CASE ("Poty: el estado se guarda y se restaura") { suite::stateRoundTrip<PotyProcessor>(); }
TEST_CASE ("Poty: el editor se abre, se redimensiona y se cierra") { suite::editorLifecycle<PotyProcessor>(); }
TEST_CASE ("Poty: latencia constante y bypass del anfitrión") { suite::constantLatency<PotyProcessor>(); }
TEST_CASE ("Poty: configuraciones de canales") { suite::busLayouts<PotyProcessor>(); }
TEST_CASE ("Poty: processBlock no reserva memoria") { suite::noAllocationsInProcessBlock<PotyProcessor> ([] { return allocguard::Scope(); }); }

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink; // evita que el compilador elimine la reserva
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}
