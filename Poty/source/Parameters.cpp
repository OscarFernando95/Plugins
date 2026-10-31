#include "Parameters.h"

namespace poty::params
{
namespace
{
constexpr int versionHint = 1;

struct BandDefaults
{
    float freq, gain, q;
    int type;
    float drive;
};

// Valores del bosquejo aprobado.
constexpr BandDefaults bandDefaults[numBands] = {
    {   90.0f, 5.0f, 0.8f, Tube,           45.0f },
    {  350.0f, 3.0f, 1.0f, Tape,           30.0f },
    { 1800.0f, 6.0f, 1.1f, Solid,          55.0f },
    { 5000.0f, 8.0f, 1.2f, GermaniumDiode, 70.0f },
    { 12000.0f, 7.0f, 0.7f, Digital,       40.0f },
};

juce::ParameterID pid (const juce::String& id) { return { id, versionHint }; }

juce::String utf8 (const char* s) { return juce::String::fromUTF8 (s); }

juce::StringArray utf8Array (std::initializer_list<const char*> items)
{
    juce::StringArray out;
    for (auto* s : items)
        out.add (utf8 (s));
    return out;
}

juce::NormalisableRange<float> logRange (float lo, float hi, float step = 0.0f)
{
    juce::NormalisableRange<float> range (lo, hi, step);
    range.setSkewForCentre (std::sqrt (lo * hi)); // escala logarítmica
    return range;
}

juce::String frequencyText (float hz, int)
{
    return hz >= 1000.0f ? juce::String (hz / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
}

juce::String decimals (float value, int n) { return juce::String (value, n); }

void addBand (juce::AudioProcessorValueTreeState::ParameterLayout& layout, int band)
{
    const auto& d = bandDefaults[band];
    const auto label = juce::String (band + 1);

    layout.add (std::make_unique<juce::AudioParameterBool> (pid (bandId (band, bandOn)), "Banda " + label, true));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (bandId (band, bandFreq)), "Frecuencia " + label, logRange (20.0f, 20000.0f), d.freq,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (frequencyText)));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (bandId (band, bandGain)), "Ganancia " + label, juce::NormalisableRange<float> (0.0f, 12.0f, 0.1f), d.gain,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return decimals (v, 1); })));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (bandId (band, bandQ)), "Q " + label, logRange (0.3f, 4.0f), d.q,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return decimals (v, 2); })));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        pid (bandId (band, bandType)), "Tipo " + label,
        utf8Array ({ "Tubo", "Cinta", "Sólido", "Diodo Ge", "Digital" }), d.type));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (bandId (band, bandDrive)), "Drive " + label, juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), d.drive,
        juce::AudioParameterFloatAttributes().withLabel ("%")));
}
} // namespace

juce::String bandId (int band, const char* suffix)
{
    jassert (band >= 0 && band < numBands);
    return "b" + juce::String (band + 1) + "_" + suffix;
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const auto gainAttrs = juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return decimals (v, 1); });

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (inGain), "Entrada", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f, gainAttrs));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (outGain), "Salida", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), -0.5f, gainAttrs));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        pid (mix), "Mezcla", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 70.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        pid (quality), "Calidad", utf8Array ({ "Buena (2×)", "Mejor (4×)", "Máxima (8×)" }), Better));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        pid (intensity), "Intensidad", utf8Array ({ "Sutil", "Media", "Agresiva" }), Medium));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        pid (mode), "Modo", utf8Array ({ "Estéreo", "Mid", "Side" }), Stereo));
    layout.add (std::make_unique<juce::AudioParameterBool> (pid (bypass), "Bypass", false));

    for (int band = 0; band < numBands; ++band)
        addBand (layout, band);

    return layout;
}
} // namespace poty::params
