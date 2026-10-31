#include "Parameters.h"

#include "dsp/Modes.h"

namespace nel::params
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
    const auto& moderno = modes[0];

    layout.add (std::make_unique<Float> (pid (gain), "Ganancia", Range (0.0f, 24.0f, 0.1f), 0.0f, attrs ("dB", 1)));
    layout.add (std::make_unique<Float> (pid (ceiling), "Techo", Range (-30.0f, 0.0f, 0.1f), -1.0f, attrs ("dBTP", 1)));

    juce::StringArray modeNames;
    for (const auto& m : modes)
        modeNames.add (juce::String::fromUTF8 (m.name));
    layout.add (std::make_unique<Choice> (pid (mode), "Modo", modeNames, 0));
    layout.add (std::make_unique<Float> (pid (character), juce::String::fromUTF8 ("Carácter"), Range (0.0f, 100.0f, 1.0f), 0.0f, attrs ("%", 0)));

    layout.add (std::make_unique<Float> (pid (lookahead), "Lookahead", Range (0.0f, 5.0f, 0.1f), moderno.lookaheadMs, attrs ("ms", 1)));
    layout.add (std::make_unique<Float> (pid (attack), "Ataque", logRange (0.1f, 50.0f, 0.1f), moderno.attackMs, attrs ("ms", 1)));
    layout.add (std::make_unique<Float> (pid (release), "Release", logRange (10.0f, 2000.0f, 1.0f), moderno.releaseMs, attrs ("ms", 0)));
    layout.add (std::make_unique<Bool> (pid (releaseAuto), "Release automático", true));
    layout.add (std::make_unique<Float> (pid (knee), "Knee", Range (0.0f, 12.0f, 0.1f), moderno.kneeDb, attrs ("dB", 1)));
    layout.add (std::make_unique<Float> (pid (linkTransient), "Enlace de transitorios", Range (0.0f, 100.0f, 1.0f), 75.0f, attrs ("%", 0)));
    layout.add (std::make_unique<Float> (pid (linkRelease), "Enlace de release", Range (0.0f, 100.0f, 1.0f), 100.0f, attrs ("%", 0)));

    layout.add (std::make_unique<Bool> (pid (truePeak), "True Peak", true));
    layout.add (std::make_unique<Choice> (pid (oversample), "Sobremuestreo", utf8Array ({ "1×", "2×", "4×", "8×", "16×", "32×" }), 2));
    layout.add (std::make_unique<Choice> (pid (osPhase), "Fase del sobremuestreo", utf8Array ({ "Lineal", "Mínima" }), Linear));
    layout.add (std::make_unique<Bool> (pid (osOffline), juce::String::fromUTF8 ("Más calidad en render"), true));
    layout.add (std::make_unique<Choice> (pid (ditherBits), "Dither", utf8Array ({ "Apagado", "16 bits", "18 bits", "20 bits", "22 bits", "24 bits" }), 0));
    layout.add (std::make_unique<Choice> (pid (ditherShape), "Perfil de dither", utf8Array ({ "Plano", "Suave", "Medio", "Fuerte" }), 0));
    layout.add (std::make_unique<Bool> (pid (dcFilter), "Filtro de DC", false));

    layout.add (std::make_unique<Bool> (pid (unityGain), "Ganancia unitaria", false));
    layout.add (std::make_unique<Bool> (pid (audition), juce::String::fromUTF8 ("Audición"), false));
    layout.add (std::make_unique<Bool> (pid (bypass), "Bypass", false));
    return layout;
}
} // namespace nel::params
