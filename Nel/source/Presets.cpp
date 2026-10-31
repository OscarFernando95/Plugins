#include "Presets.h"

#include "Parameters.h"
#include "dsp/Modes.h"

namespace nel
{
namespace
{
struct P
{
    int mode = 0;
    float gain = 0.0f, ceiling = -1.0f, character = 0.0f;
    int oversample = 2;
    bool releaseAuto = true;
    float linkTransient = 75.0f;
    int ditherBits = 0, ditherShape = 0;
};

/** Los parámetros avanzados salen de la tabla del modo: así un preset suena como el modo que nombra. */
ferb::PresetManager::Factory make (const char* name, const P& p)
{
    namespace k = params;
    const auto& m = modes[static_cast<size_t> (p.mode)];
    ferb::PresetManager::Factory f;
    f.name = juce::String::fromUTF8 (name);
    f.values = { { k::mode, float (p.mode) },         { k::gain, p.gain },
                 { k::ceiling, p.ceiling },           { k::character, p.character },
                 { k::lookahead, m.lookaheadMs },     { k::attack, m.attackMs },
                 { k::release, m.releaseMs },         { k::knee, m.kneeDb },
                 { k::releaseAuto, p.releaseAuto ? 1.0f : 0.0f },
                 { k::linkTransient, p.linkTransient },
                 { k::oversample, float (p.oversample) },
                 { k::ditherBits, float (p.ditherBits) }, { k::ditherShape, float (p.ditherShape) } };
    return f;
}
} // namespace

// Valores iniciales de la ficha: la ganancia depende del material, así que son un punto de partida.
std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    enum { Moderno, Transparente, Punchy, Dinamico, Versatil, Agresivo, Bus, Seguro };
    auto presets = std::vector<ferb::PresetManager::Factory> {
        make ("Init", { .mode = Moderno }),
        make ("Streaming −14", { .mode = Moderno, .gain = 4.0f, .ceiling = -1.0f, .oversample = 1 }),
        make ("Streaming prudente", { .mode = Seguro, .gain = 2.0f, .ceiling = -1.5f }),
        make ("Master pop brillante", { .mode = Dinamico, .gain = 5.0f, .character = 8.0f }),
        make ("Master rock", { .mode = Versatil, .gain = 6.0f }),
        make ("Master electrónica", { .mode = Agresivo, .gain = 8.0f, .ceiling = -0.3f, .character = 12.0f }),
        make ("Master acústico", { .mode = Seguro, .gain = 2.0f }),
        make ("Pegamento de bus", { .mode = Bus, .gain = 3.0f, .ceiling = -0.5f, .character = 20.0f }),
        make ("Batería con pegada", { .mode = Punchy, .gain = 4.0f, .ceiling = -0.5f, .linkTransient = 50.0f }),
        make ("Voz clara", { .mode = Transparente, .gain = 3.0f }),
        make ("Cinta cálida", { .mode = Versatil, .gain = 4.0f, .character = 45.0f }),
        make ("Válvula años 60", { .mode = Punchy, .gain = 5.0f, .character = 60.0f }),
        make ("Radio saturada", { .mode = Agresivo, .gain = 9.0f, .character = 80.0f }),
        make ("Broadcast R 128", { .mode = Transparente }),
        make ("Podcast −16", { .mode = Transparente, .gain = 6.0f }),
        make ("Render para 16 bits", { .mode = Moderno, .oversample = 5, .ditherBits = 1, .ditherShape = 2 }),
    };

    // Categorías de la ficha, para agrupar el menú.
    const std::pair<const char*, const char*> categories[] = {
        { "Init", "Utilidad" }, { "Render para 16 bits", "Utilidad" },
        { "Streaming −14", "Streaming" }, { "Streaming prudente", "Streaming" },
        { "Master pop brillante", "Master" }, { "Master rock", "Master" }, { "Master electrónica", "Master" }, { "Master acústico", "Master" },
        { "Pegamento de bus", "Bus" }, { "Batería con pegada", "Bus" },
        { "Voz clara", "Mezcla" },
        { "Cinta cálida", "Vintage" }, { "Válvula años 60", "Vintage" }, { "Radio saturada", "Vintage" },
        { "Broadcast R 128", "Broadcast" }, { "Podcast −16", "Voz" },
    };
    for (auto& preset : presets)
        for (const auto& [name, category] : categories)
            if (preset.name == juce::String::fromUTF8 (name))
                preset.category = juce::String::fromUTF8 (category);
    return presets;
}
} // namespace nel
