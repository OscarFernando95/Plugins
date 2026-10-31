#include "Presets.h"

#include "Parameters.h"

namespace mcpoti
{
namespace
{
// Índices de voice según la era (sección 4 de la ficha): Vintage = Clásico, Cinta, Válvula, Transistor;
// Moderno = Dinámico, Adaptativo, Seda, Aire.
enum Voice { Clasico, Cinta, Valvula, Transistor, Dinamico = 0, Adaptativo, Seda, Aire };

struct P
{
    const char* name;
    int mode;
    int era;
    int voice;
    float drive[4];
    float tune;
    float width = 100.0f;
    int os = 1;
    float airCorner = 16000.0f;
};

ferb::PresetManager::Factory make (const P& p)
{
    namespace k = params;
    ferb::PresetManager::Factory f;
    f.name = juce::String::fromUTF8 (p.name);
    f.category = p.mode == k::Vocal ? "Vocal" : "Master";
    f.values = { { k::mode, float (p.mode) }, { k::era, float (p.era) }, { k::voice, float (p.voice) },
                 { k::tune, p.tune }, { k::width, p.width }, { k::os, float (p.os) }, { k::airCorner, p.airCorner },
                 { k::guard, p.mode == k::Vocal ? 60.0f : 0.0f }, { k::monoSafe, p.mode == k::Master ? 1.0f : 0.0f } };
    for (int b = 0; b < k::numBands; ++b)
        f.values.emplace_back (k::bandId ("drive", b), p.drive[b]);
    return f;
}
} // namespace

// Tabla de la sección 7 de la ficha: puntos de partida que se afinan escuchando.
std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    using namespace params;
    const P table[] = {
        { "Init", Vocal, Vintage, Clasico, { 20, 25, 40, 35 }, 4000 },
        { "Radio setentas", Vocal, Vintage, Clasico, { 10, 20, 55, 40 }, 4500 },
        { "Susurro cálido", Vocal, Vintage, Valvula, { 25, 30, 35, 25 }, 3500 },
        { "Presencia de cinta", Vocal, Vintage, Cinta, { 15, 25, 45, 30 }, 4000 },
        { "Rap al frente", Vocal, Vintage, Transistor, { 10, 30, 55, 35 }, 5000 },
        { "Aire de estudio", Vocal, Vintage, Clasico, { 0, 15, 35, 60 }, 6000 },
        { "Seda pop", Vocal, Moderno, Seda, { 0, 10, 35, 50 }, 4000 },
        { "Dinámico limpio", Vocal, Moderno, Dinamico, { 15, 25, 40, 40 }, 4000 },
        { "Adaptativo coro", Vocal, Moderno, Adaptativo, { 10, 20, 35, 35 }, 4000 },
        { "Locución nítida", Vocal, Moderno, Dinamico, { 0, 35, 45, 20 }, 3000 },
        { "Aire a 20 kHz", Vocal, Moderno, Aire, { 0, 0, 20, 55 }, 6000 },
        { "Cinta suave", Master, Vintage, Cinta, { 15, 15, 25, 30 }, 5000 },
        { "Válvula calidez", Master, Vintage, Valvula, { 30, 20, 20, 20 }, 4000 },
        { "Transistor firme", Master, Vintage, Transistor, { 20, 25, 35, 25 }, 5000 },
        { "Clásico apertura", Master, Vintage, Clasico, { 0, 10, 30, 45 }, 6000 },
        { "Cuerpo y brillo", Master, Vintage, Cinta, { 40, 20, 30, 35 }, 5000 },
        { "Apertura limpia", Master, Moderno, Seda, { 0, 10, 30, 50 }, 5000 },
        { "Aire 40 kHz", Master, Moderno, Aire, { 0, 0, 15, 60 }, 6000, 100.0f, 3, 30000.0f },
        { "Adaptativo balance", Master, Moderno, Adaptativo, { 10, 20, 30, 30 }, 5000 },
        { "Dinámico mastering", Master, Moderno, Dinamico, { 10, 20, 30, 35 }, 5000 },
        { "Ancho controlado", Master, Moderno, Dinamico, { 5, 15, 30, 40 }, 5000, 130.0f },
    };
    std::vector<ferb::PresetManager::Factory> out;
    for (const auto& p : table)
        out.push_back (make (p));
    out.front().category = "Utilidad";
    return out;
}
} // namespace mcpoti
