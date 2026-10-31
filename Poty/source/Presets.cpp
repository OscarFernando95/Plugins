#include "Presets.h"

#include "Parameters.h"

namespace poty
{
namespace
{
struct Band
{
    float freq, gainDb, q;
    int type;
    float drive;
};

ferb::PresetManager::Factory make (const char* name, float inGain, float outGain, float mix, int quality,
                                   int intensity, int mode, const std::array<Band, params::numBands>& bands)
{
    ferb::PresetManager::Factory f;
    f.name = juce::String::fromUTF8 (name);
    f.values = { { params::inGain, inGain }, { params::outGain, outGain }, { params::mix, mix },
                 { params::quality, float (quality) }, { params::intensity, float (intensity) },
                 { params::mode, float (mode) } };

    for (int b = 0; b < params::numBands; ++b)
    {
        const auto& x = bands[size_t (b)];
        f.values.push_back ({ params::bandId (b, params::bandOn), x.gainDb > 0.0f ? 1.0f : 0.0f });
        f.values.push_back ({ params::bandId (b, params::bandFreq), x.freq });
        f.values.push_back ({ params::bandId (b, params::bandGain), x.gainDb });
        f.values.push_back ({ params::bandId (b, params::bandQ), x.q });
        f.values.push_back ({ params::bandId (b, params::bandType), float (x.type) });
        f.values.push_back ({ params::bandId (b, params::bandDrive), x.drive });
    }
    return f;
}

} // namespace

std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    using namespace params;
    constexpr int Ge = GermaniumDiode;
    return {
        // Los valores por defecto del plugin (los del bosquejo).
        make ("Brillo vocal", 0.0f, -0.5f, 70.0f, Better, Medium, Stereo,
              { { { 90.0f, 5.0f, 0.8f, Tube, 45.0f }, { 350.0f, 3.0f, 1.0f, Tape, 30.0f },
                  { 1800.0f, 6.0f, 1.1f, Solid, 55.0f }, { 5000.0f, 8.0f, 1.2f, Ge, 70.0f },
                  { 12000.0f, 7.0f, 0.7f, Digital, 40.0f } } }),

        make ("Calidez de bus", 0.0f, -0.5f, 55.0f, Better, Subtle, Stereo,
              { { { 70.0f, 4.0f, 0.8f, Tube, 50.0f }, { 250.0f, 3.0f, 0.9f, Tape, 40.0f },
                  { 1200.0f, 2.0f, 0.8f, Tape, 30.0f }, { 4000.0f, 2.0f, 1.0f, Solid, 25.0f },
                  { 10000.0f, 3.0f, 0.7f, Tape, 30.0f } } }),

        make ("Aire y brillo", 0.0f, -1.0f, 50.0f, Max, Subtle, Stereo,
              { { { 120.0f, 0.0f, 0.8f, Tube, 30.0f }, { 600.0f, 0.0f, 1.0f, Tape, 30.0f },
                  { 3000.0f, 3.0f, 0.9f, Solid, 25.0f }, { 8000.0f, 6.0f, 0.8f, Tape, 30.0f },
                  { 16000.0f, 9.0f, 0.6f, Tape, 35.0f } } }),

        make ("Graves con cuerpo", 0.0f, -1.0f, 75.0f, Better, Medium, Mid,
              { { { 55.0f, 8.0f, 0.9f, Tube, 65.0f }, { 120.0f, 5.0f, 0.9f, Tube, 55.0f },
                  { 300.0f, 2.0f, 1.0f, Tape, 30.0f }, { 2500.0f, 0.0f, 1.0f, Solid, 30.0f },
                  { 9000.0f, 0.0f, 0.8f, Tape, 30.0f } } }),

        make ("Presencia agresiva", -1.0f, -2.0f, 80.0f, Better, Aggressive, Stereo,
              { { { 150.0f, 0.0f, 0.8f, Tube, 30.0f }, { 700.0f, 2.0f, 1.0f, Solid, 35.0f },
                  { 1800.0f, 6.0f, 1.1f, Ge, 70.0f }, { 3500.0f, 8.0f, 1.2f, Ge, 75.0f },
                  { 7000.0f, 7.0f, 0.9f, Digital, 60.0f } } }),
    };
}
} // namespace poty
