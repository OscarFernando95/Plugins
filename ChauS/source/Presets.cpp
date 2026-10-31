#include "Presets.h"

#include "Parameters.h"

namespace chaus
{
namespace
{
namespace k = params;

struct P
{
    float thr = -36.0f, range = 10.0f, hp = 4000.0f, lp = 12000.0f, sensSoft = 0.0f, sensHard = 0.0f, look = 10.0f, link = 100.0f;
    int mode = k::VoiceOnly, band = k::Split;
    bool adaptive = true, track = true;
};

ferb::PresetManager::Factory make (const char* name, const P& p)
{
    ferb::PresetManager::Factory f;
    f.name = juce::String::fromUTF8 (name);
    f.values = { { k::mode, float (p.mode) },     { k::thr, p.thr },           { k::adaptive, p.adaptive ? 1.0f : 0.0f },
                 { k::range, p.range },           { k::hp, p.hp },             { k::lp, p.lp },
                 { k::track, p.track ? 1.0f : 0.0f }, { k::band, float (p.band) },
                 { k::sensSoft, p.sensSoft },     { k::sensHard, p.sensHard }, { k::look, p.look },
                 { k::link, p.link } };
    return f;
}
} // namespace

// Los presets de voz salen de la maqueta con el Umbral 6 dB más bajo y el Rango 2 dB más alto (la reducción de la maqueta casi no actuaba
// sobre una voz real: ver la ficha); Batería y Mezcla completa (modo General) quedan como en la maqueta. El resto son valores iniciales.
std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    return {
        make ("Init", {}),
        make ("Voz masculina", { .thr = -36.0f, .range = 10.0f, .hp = 3500.0f, .lp = 11000.0f }),
        make ("Voz femenina", { .thr = -36.0f, .range = 11.0f, .hp = 5000.0f, .lp = 14000.0f }),
        make ("Locución", { .thr = -40.0f, .range = 8.0f, .hp = 4000.0f, .lp = 12000.0f, .sensSoft = -1.0f, .look = 12.0f }),
        make ("Rap", { .thr = -38.0f, .range = 12.0f, .hp = 4000.0f, .lp = 13000.0f, .sensHard = 1.0f, .look = 8.0f }),
        make ("Coros", { .thr = -38.0f, .range = 9.0f, .hp = 4500.0f, .lp = 12000.0f }),
        make ("Voz agresiva", { .thr = -32.0f, .range = 16.0f, .hp = 4500.0f, .lp = 13000.0f, .sensHard = 2.0f, .look = 8.0f }),
        make ("Podcast", { .thr = -39.0f, .range = 9.0f, .hp = 3500.0f, .lp = 11000.0f, .look = 12.0f }),
        make ("Batería (platillos)", { .thr = -26.0f, .range = 6.0f, .hp = 6000.0f, .lp = 16000.0f, .look = 5.0f, .mode = k::General, .adaptive = false, .track = false }),
        make ("Mezcla completa", { .thr = -28.0f, .range = 5.0f, .hp = 6000.0f, .lp = 16000.0f, .look = 6.0f, .mode = k::General, .band = k::Wide }),
    };
}
} // namespace chaus
