#include "Parameters.h"

namespace mcpoti::params
{
namespace
{
constexpr int versionHint = 1;
juce::ParameterID pid (const juce::String& id) { return { id, versionHint }; }

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

juce::AudioParameterFloatAttributes attrs (const char* label)
{
    return juce::AudioParameterFloatAttributes().withLabel (label);
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using Float = juce::AudioParameterFloat;
    using Bool = juce::AudioParameterBool;
    using Choice = juce::AudioParameterChoice;
    using Range = juce::NormalisableRange<float>;

    layout.add (std::make_unique<Choice> (pid (mode), "Modo", juce::StringArray { "Vocal", "Master" }, Vocal));
    layout.add (std::make_unique<Choice> (pid (era), "Era", juce::StringArray { "Vintage", "Moderno" }, Vintage));
    layout.add (std::make_unique<Choice> (pid (voice), "Voicing", juce::StringArray { "1", "2", "3", "4" }, 0));

    layout.add (std::make_unique<Float> (pid (input), "Entrada", Range (-12.0f, 12.0f, 0.1f), 0.0f, attrs ("dB")));
    layout.add (std::make_unique<Float> (pid (output), "Salida", Range (-12.0f, 12.0f, 0.1f), 0.0f, attrs ("dB")));
    layout.add (std::make_unique<Float> (pid (mix), juce::String::fromUTF8 ("Mezcla"), Range (0.0f, 100.0f, 1.0f), 100.0f, attrs ("%")));

    layout.add (std::make_unique<Float> (pid (tune), juce::String::fromUTF8 ("Sintonización"), logRange (1500.0f, 12000.0f, 1.0f), 4000.0f, attrs ("Hz")));
    layout.add (std::make_unique<Float> (pid (phase), "Fase", Range (0.0f, 100.0f, 1.0f), 40.0f, attrs ("%")));
    layout.add (std::make_unique<Float> (pid (color), "Color", Range (-100.0f, 100.0f, 1.0f), 0.0f));
    layout.add (std::make_unique<Float> (pid (soften), "Suavizar", logRange (4000.0f, 20000.0f, 10.0f), 20000.0f, attrs ("Hz")));
    layout.add (std::make_unique<Float> (pid (airCorner), juce::String::fromUTF8 ("Esquina de aire"), logRange (8000.0f, 40000.0f, 100.0f), 16000.0f, attrs ("Hz")));
    layout.add (std::make_unique<Float> (pid (guard), "Guarda", Range (0.0f, 100.0f, 1.0f), 60.0f, attrs ("%")));
    layout.add (std::make_unique<Float> (pid (width), "Ancho", Range (0.0f, 200.0f, 1.0f), 100.0f, attrs ("%")));
    layout.add (std::make_unique<Bool> (pid (monoSafe), "Mono seguro", false));
    layout.add (std::make_unique<Float> (pid (link), "Enlace", Range (0.0f, 100.0f, 1.0f), 100.0f, attrs ("%")));

    layout.add (std::make_unique<Float> (pid (xo1), "Cruce 1", logRange (80.0f, 400.0f, 1.0f), 150.0f, attrs ("Hz")));
    layout.add (std::make_unique<Float> (pid (xo2), "Cruce 2", logRange (600.0f, 4000.0f, 1.0f), 1800.0f, attrs ("Hz")));
    layout.add (std::make_unique<Float> (pid (xo3), "Cruce 3", logRange (3000.0f, 12000.0f, 1.0f), 7000.0f, attrs ("Hz")));

    constexpr float driveDefault[numBands] = { 20.0f, 25.0f, 40.0f, 35.0f };
    constexpr const char* bandName[numBands] = { "Cuerpo", "Medios", "Presencia", "Aire" };
    for (int b = 0; b < numBands; ++b)
    {
        const auto name = juce::String (bandName[b]);
        layout.add (std::make_unique<Float> (pid (bandId ("drive", b)), "Drive " + name, Range (0.0f, 100.0f, 1.0f), driveDefault[b]));
        layout.add (std::make_unique<Float> (pid (bandId ("dyn", b)), juce::String::fromUTF8 ("Dinámica ") + name, Range (-50.0f, 50.0f, 1.0f), 0.0f));
        layout.add (std::make_unique<Float> (pid (bandId ("mix", b)), "Nivel " + name, Range (0.0f, 100.0f, 1.0f), 100.0f, attrs ("%")));
        layout.add (std::make_unique<Bool> (pid (bandId ("on", b)), "Activar " + name, true));
        layout.add (std::make_unique<Bool> (pid (bandId ("solo", b)), "Solo " + name, false));
    }

    layout.add (std::make_unique<Choice> (pid (os), "Oversampling", utf8Array ({ "1×", "2×", "4×", "8×" }), 2));
    layout.add (std::make_unique<Bool> (pid (linPhase), "Fase lineal", false));
    layout.add (std::make_unique<Bool> (pid (delta), "Delta", false));
    layout.add (std::make_unique<Bool> (pid (match), "Igualar", true));
    layout.add (std::make_unique<Float> (pid (vuRef), juce::String::fromUTF8 ("Referencia VU"), Range (-24.0f, -12.0f, 0.5f), -18.0f, attrs ("dBFS")));
    layout.add (std::make_unique<Bool> (pid (bypass), "Bypass", false));
    return layout;
}
} // namespace mcpoti::params
