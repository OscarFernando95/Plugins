#include "Parameters.h"

namespace maru::params
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

juce::NormalisableRange<float> logRange (float lo, float hi, float step)
{
    juce::NormalisableRange<float> range (lo, hi, step);
    range.setSkewForCentre (std::sqrt (lo * hi));
    return range;
}

juce::AudioParameterFloatAttributes attrs (const char* label, int decimals)
{
    return juce::AudioParameterFloatAttributes().withLabel (label).withStringFromValueFunction ([decimals] (float v, int) { return juce::String (v, decimals); });
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    using Choice = juce::AudioParameterChoice;
    using Range = juce::NormalisableRange<float>;

    layout.add (std::make_unique<Float> (pid (amount), juce::String::fromUTF8 ("Supresión"), Range (0.0f, 100.0f, 1.0f), 50.0f, attrs ("%", 0)));
    layout.add (std::make_unique<Choice> (pid (engine), "Motor", utf8Array ({ "Supresión", "Gate" }), Suppression));
    layout.add (std::make_unique<Float> (pid (range), "Rango", Range (-60.0f, -6.0f, 1.0f), -30.0f, attrs ("dB", 0)));
    layout.add (std::make_unique<Float> (pid (sens), "Sensibilidad", Range (-10.0f, 10.0f, 0.1f), 0.0f, attrs ("", 1)));
    layout.add (std::make_unique<Float> (pid (recovery), juce::String::fromUTF8 ("Recuperación"), logRange (10.0f, 1000.0f, 1.0f), 120.0f, attrs ("ms", 0)));

    layout.add (std::make_unique<Bool> (pid (gateAuto), "Auto", true));
    layout.add (std::make_unique<Float> (pid (gateThresh), "Umbral", Range (-80.0f, 0.0f, 0.1f), -50.0f, attrs ("dB", 1)));
    layout.add (std::make_unique<Float> (pid (attack), "Ataque", logRange (0.1f, 50.0f, 0.1f), 2.0f, attrs ("ms", 1)));
    layout.add (std::make_unique<Float> (pid (hold), "Hold", Range (0.0f, 500.0f, 1.0f), 40.0f, attrs ("ms", 0)));
    layout.add (std::make_unique<Float> (pid (hyst), juce::String::fromUTF8 ("Histéresis"), Range (0.0f, 12.0f, 0.1f), 6.0f, attrs ("dB", 1)));
    layout.add (std::make_unique<Float> (pid (lookahead), "Lookahead", Range (0.0f, 10.0f, 0.1f), 3.0f, attrs ("ms", 1)));

    layout.add (std::make_unique<Float> (pid (scHP), "Sidechain HP", logRange (20.0f, 2000.0f, 1.0f), 80.0f, attrs ("Hz", 0)));
    layout.add (std::make_unique<Float> (pid (scLP), "Sidechain LP", logRange (1000.0f, 20000.0f, 10.0f), 12000.0f, attrs ("Hz", 0)));
    layout.add (std::make_unique<Bool> (pid (scExt), "Sidechain externo", false));

    layout.add (std::make_unique<Float> (pid (link), juce::String::fromUTF8 ("Enlace estéreo"), Range (0.0f, 100.0f, 1.0f), 100.0f, attrs ("%", 0)));
    layout.add (std::make_unique<Choice> (pid (latency), "Latencia", utf8Array ({ "Baja", "Alta" }), High));
    layout.add (std::make_unique<Bool> (pid (listen), "Escuchar lo quitado", false));
    layout.add (std::make_unique<Float> (pid (output), "Salida", Range (-12.0f, 12.0f, 0.1f), 0.0f, attrs ("dB", 1)));
    layout.add (std::make_unique<Bool> (pid (bypass), "Bypass", false));
    return layout;
}
} // namespace maru::params
