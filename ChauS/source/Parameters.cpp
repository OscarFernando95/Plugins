#include "Parameters.h"

namespace chaus::params
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

    layout.add (std::make_unique<Choice> (pid (mode), "Modo", utf8Array ({ "Voz sola", "General" }), VoiceOnly));
    layout.add (std::make_unique<Float> (pid (thr), "Umbral", Range (-60.0f, 0.0f, 0.1f), -36.0f, attrs ("dB", 1)));
    layout.add (std::make_unique<Bool> (pid (adaptive), "Umbral adaptativo", true));
    layout.add (std::make_unique<Float> (pid (range), "Rango", Range (0.0f, 24.0f, 0.1f), 10.0f, attrs ("dB", 1)));

    layout.add (std::make_unique<Float> (pid (hp), "Filtro HP", logRange (2000.0f, 12000.0f, 10.0f), 4000.0f, attrs ("Hz", 0)));
    layout.add (std::make_unique<Float> (pid (lp), "Filtro LP", logRange (5000.0f, 20000.0f, 10.0f), 12000.0f, attrs ("Hz", 0)));
    layout.add (std::make_unique<Bool> (pid (track), "Seguir la s", true));
    layout.add (std::make_unique<Float> (pid (q), "Ancho (Q)", Range (1.0f, 8.0f, 0.01f), 3.0f, attrs ("", 2)));
    layout.add (std::make_unique<Bool> (pid (qAuto), "Ancho automático", true));
    layout.add (std::make_unique<Choice> (pid (band), "Banda", utf8Array ({ "Dividida", "Ancha" }), Split));
    layout.add (std::make_unique<Choice> (pid (shape), "Forma", utf8Array ({ "Campana", "Estante", "Auto" }), Auto));

    layout.add (std::make_unique<Float> (pid (sensSoft), "S suaves", Range (-6.0f, 6.0f, 0.1f), 0.0f, attrs ("dB", 1)));
    layout.add (std::make_unique<Float> (pid (sensHard), "S duras", Range (-6.0f, 6.0f, 0.1f), 0.0f, attrs ("dB", 1)));

    layout.add (std::make_unique<Bool> (pid (lookOn), "Lookahead activo", true));
    layout.add (std::make_unique<Float> (pid (look), "Lookahead", Range (0.0f, 15.0f, 0.1f), 10.0f, attrs ("ms", 1)));
    layout.add (std::make_unique<Float> (pid (release), juce::String::fromUTF8 ("Liberación"), logRange (10.0f, 100.0f, 1.0f), 40.0f, attrs ("ms", 0)));
    layout.add (std::make_unique<Bool> (pid (releaseAuto), juce::String::fromUTF8 ("Liberación automática"), true));

    layout.add (std::make_unique<Float> (pid (link), juce::String::fromUTF8 ("Enlace estéreo"), Range (0.0f, 100.0f, 1.0f), 100.0f, attrs ("%", 0)));
    layout.add (std::make_unique<Choice> (pid (ms), "Mid/Side", utf8Array ({ "Apagado", "Mid", "Side" }), Off));

    layout.add (std::make_unique<Bool> (pid (scExt), "Sidechain externo", false));
    layout.add (std::make_unique<Bool> (pid (listenTrig), "Escuchar disparo", false));
    layout.add (std::make_unique<Bool> (pid (listenSC), "Escuchar sidechain", false));

    layout.add (std::make_unique<Float> (pid (output), "Salida", Range (-12.0f, 12.0f, 0.1f), 0.0f, attrs ("dB", 1)));
    layout.add (std::make_unique<Bool> (pid (bypass), "Bypass", false));
    return layout;
}
} // namespace chaus::params
