#include "Presets.h"

#include "Parameters.h"

namespace maru
{
namespace
{
namespace k = params;

struct P
{
    float amount = 50.0f;
    int engine = k::Suppression;
    float range = -30.0f, sens = 0.0f, recovery = 120.0f, attack = 2.0f, hold = 40.0f, hyst = 6.0f;
};

ferb::PresetManager::Factory make (const char* name, const P& p)
{
    ferb::PresetManager::Factory f;
    f.name = juce::String::fromUTF8 (name);
    f.values = { { k::amount, p.amount }, { k::engine, float (p.engine) }, { k::range, p.range }, { k::sens, p.sens },
                 { k::recovery, p.recovery }, { k::attack, p.attack }, { k::hold, p.hold }, { k::hyst, p.hyst } };
    return f;
}
} // namespace

// Voz de podcast, Locución, Voz con ventilador, Amplificador y Batería salen de la maqueta; el resto son valores iniciales.
std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    return {
        make ("Init", {}),
        make ("Voz de podcast", { .amount = 55.0f, .range = -30.0f, .recovery = 120.0f }),
        make ("Locución", { .amount = 40.0f, .range = -24.0f, .sens = -2.0f, .recovery = 200.0f }),
        make ("Voz con ventilador", { .amount = 75.0f, .range = -36.0f, .sens = 2.0f, .recovery = 90.0f }),
        make ("Zumbido de sala", { .amount = 65.0f, .range = -32.0f, .sens = 1.0f, .recovery = 150.0f }),
        make ("Guitarra acústica", { .amount = 45.0f, .range = -24.0f, .recovery = 180.0f }),
        make ("Amplificador (gate)", { .amount = 60.0f, .engine = k::Gate, .range = -40.0f, .recovery = 80.0f, .attack = 1.0f, .hold = 80.0f, .hyst = 8.0f }),
        make ("Batería (gate)", { .amount = 70.0f, .engine = k::Gate, .range = -50.0f, .sens = 1.0f, .recovery = 60.0f, .attack = 0.5f, .hold = 30.0f, .hyst = 6.0f }),
        make ("Instrumento suave", { .amount = 30.0f, .range = -18.0f, .recovery = 250.0f }),
        make ("Rescate fuerte", { .amount = 90.0f, .range = -45.0f, .sens = 3.0f, .recovery = 80.0f }),
    };
}
} // namespace maru
