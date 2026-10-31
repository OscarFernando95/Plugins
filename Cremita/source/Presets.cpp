#include "Presets.h"

#include "Parameters.h"

namespace cremita
{
namespace
{
struct P
{
    float input = 0.0f, effect = 100.0f, curve = 0.0f, bias = 0.0f;
    bool split = false;
    float xo1 = 200.0f, xo2 = 3000.0f, trimLF = 0.0f, trimMF = 0.0f, trimHF = 0.0f;
    bool clip = true, ms = false;
    int os = params::Os2x;
    bool match = false;
    float output = 0.0f;
};

ferb::PresetManager::Factory make (const char* name, const P& p)
{
    namespace k = params;
    ferb::PresetManager::Factory f;
    f.name = juce::String::fromUTF8 (name);
    f.values = { { k::input, p.input },       { k::effect, p.effect },   { k::curve, p.curve },
                 { k::bias, p.bias },         { k::split, p.split ? 1.0f : 0.0f },
                 { k::xo1, p.xo1 },           { k::xo2, p.xo2 },
                 { k::trimLF, p.trimLF },     { k::trimMF, p.trimMF },   { k::trimHF, p.trimHF },
                 { k::clip, p.clip ? 1.0f : 0.0f }, { k::ms, p.ms ? 1.0f : 0.0f },
                 { k::os, float (p.os) },     { k::match, p.match ? 1.0f : 0.0f },
                 { k::output, p.output } };
    return f;
}
} // namespace

// Valores iniciales: se afinan de oído contra un Inflator de referencia (ver la ficha).
std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    using namespace params;
    return {
        make ("Init", {}),
        make ("Mezcla suave", { .effect = 50.0f, .bias = 10.0f }),
        make ("Presencia vocal", { .effect = 70.0f, .curve = 20.0f, .bias = 20.0f, .split = true, .trimLF = -3.0f, .trimMF = 2.0f, .trimHF = 1.0f }),
        make ("Bus de batería", { .curve = 30.0f, .split = true, .xo1 = 150.0f, .xo2 = 2500.0f, .trimLF = 2.0f, .trimHF = -1.0f }),
        make ("Pegamento de master", { .effect = 60.0f, .curve = 10.0f, .bias = 5.0f, .os = Os4x, .match = true }),
        make ("Bajo grueso", { .curve = 35.0f, .bias = 40.0f, .split = true, .xo1 = 120.0f, .trimLF = 3.0f, .trimHF = -4.0f }),
        make ("Guitarra viva", { .effect = 80.0f, .curve = 25.0f, .bias = 30.0f, .split = true, .xo1 = 250.0f, .xo2 = 4000.0f, .trimLF = -2.0f, .trimMF = 2.0f, .trimHF = 2.0f }),
        make ("Clásico", { .curve = 0.0f }),
        make ("Brillo de aire", { .effect = 70.0f, .curve = 15.0f, .split = true, .xo2 = 5000.0f, .trimLF = -6.0f, .trimMF = -3.0f, .trimHF = 3.0f, .os = Os4x }),
        make ("Calidez de cinta", { .effect = 80.0f, .curve = -20.0f, .bias = 70.0f }),
    };
}
} // namespace cremita
