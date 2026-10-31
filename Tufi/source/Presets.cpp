#include "Presets.h"

#include "Parameters.h"

namespace tufi
{
namespace
{
struct Values
{
    int stage;
    float drive, low, peak, hardness, evenTop, evenBottom, trim;
    int oversample;
};

ferb::PresetManager::Factory make (const char* name, const Values& v)
{
    using namespace params;
    ferb::PresetManager::Factory f;
    f.name = juce::String::fromUTF8 (name);
    f.values = { { stage, float (v.stage) },      { drive, v.drive },        { lowAmount, v.low },
                 { peakAmount, v.peak },          { peakHardness, v.hardness }, { evenTop, v.evenTop },
                 { evenBottom, v.evenBottom },    { trim, v.trim },          { oversample, float (v.oversample) } };
    return f;
}
} // namespace

std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    using namespace params;
    return {
        // Los valores por defecto del plugin.
        make ("Triodo cálido",        { Triode,  0.0f, 3.5f, 5.5f, 4.0f, 3.0f, 4.5f,  0.0f, X4 }),
        make ("Pentodo con mordida",  { Pentode, 2.0f, 2.0f, 7.0f, 6.0f, 4.0f, 2.0f, -1.5f, X4 }),
        make ("Diodo seco",           { Diode,   3.0f, 1.0f, 8.0f, 7.0f, 2.0f, 2.0f, -2.5f, X4 }),
        make ("Bus pegamento",        { Triode,  1.0f, 5.0f, 3.0f, 2.0f, 2.0f, 3.0f, -0.5f, X4 }),
        make ("Transparente",         { Triode,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,  0.0f, X2 }),
    };
}
} // namespace tufi
