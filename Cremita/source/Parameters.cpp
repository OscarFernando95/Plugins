#include "Parameters.h"

namespace cremita::params
{
namespace
{
constexpr int versionHint = 1;

juce::ParameterID pid (const char* id) { return { id, versionHint }; }

juce::StringArray utf8Array (std::initializer_list<const char*> items)
{
    juce::StringArray out;
    for (auto* s : items)
        out.add (juce::String::fromUTF8 (s));
    return out;
}

juce::NormalisableRange<float> logRange (float lo, float hi)
{
    juce::NormalisableRange<float> range (lo, hi, 1.0f);
    range.setSkewForCentre (std::sqrt (lo * hi)); // escala logarítmica
    return range;
}

juce::String frequencyText (float hz, int)
{
    return hz >= 1000.0f ? juce::String (hz / 1000.0f, 2) + " kHz" : juce::String (juce::roundToInt (hz)) + " Hz";
}

juce::AudioParameterFloatAttributes dbAttrs()
{
    return juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); });
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using Float = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;

    layout.add (std::make_unique<Float> (pid (input), "Input", Range (-12.0f, 12.0f, 0.1f), 0.0f, dbAttrs()));
    layout.add (std::make_unique<Float> (pid (effect), "Efecto", Range (0.0f, 100.0f, 1.0f), 100.0f,
                                         juce::AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<Float> (pid (curve), "Curva", Range (-50.0f, 50.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<Float> (pid (bias), "Sesgo", Range (0.0f, 100.0f, 1.0f), 0.0f,
                                         juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterBool> (pid (split), "Band Split", false));
    layout.add (std::make_unique<Float> (pid (xo1), "Cruce 1", logRange (80.0f, 500.0f), 200.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (frequencyText)));
    layout.add (std::make_unique<Float> (pid (xo2), "Cruce 2", logRange (1500.0f, 8000.0f), 3000.0f,
                                         juce::AudioParameterFloatAttributes().withStringFromValueFunction (frequencyText)));
    layout.add (std::make_unique<Float> (pid (trimLF), "Ajuste graves", Range (-6.0f, 6.0f, 0.1f), 0.0f, dbAttrs()));
    layout.add (std::make_unique<Float> (pid (trimMF), "Ajuste medios", Range (-6.0f, 6.0f, 0.1f), 0.0f, dbAttrs()));
    layout.add (std::make_unique<Float> (pid (trimHF), "Ajuste agudos", Range (-6.0f, 6.0f, 0.1f), 0.0f, dbAttrs()));

    layout.add (std::make_unique<juce::AudioParameterBool> (pid (clip), "Clip 0 dB", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (pid (ms), "Mid/Side", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> (pid (os), "Sobremuestreo", utf8Array ({ "1×", "2×", "4×", "8×" }), Os2x));
    layout.add (std::make_unique<juce::AudioParameterBool> (pid (match), "Igualar sonoridad", false));
    layout.add (std::make_unique<Float> (pid (output), "Output", Range (-12.0f, 12.0f, 0.1f), 0.0f, dbAttrs()));
    layout.add (std::make_unique<juce::AudioParameterChoice> (pid (vuRef), "Referencia VU", utf8Array ({ "−18 dBFS", "−20 dBFS", "−14 dBFS" }), Vu18));
    layout.add (std::make_unique<juce::AudioParameterBool> (pid (inOn), "In", true));
    layout.add (std::make_unique<juce::AudioParameterBool> (pid (bypass), "Bypass", false));

    return layout;
}
} // namespace cremita::params
