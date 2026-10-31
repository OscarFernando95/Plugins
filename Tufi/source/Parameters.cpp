#include "Parameters.h"

namespace tufi::params
{
namespace
{
constexpr int versionHint = 1;

juce::ParameterID pid (const char* id) { return { id, versionHint }; }

juce::String oneDecimal (float value, int) { return juce::String (value, 1); }

std::unique_ptr<juce::AudioParameterFloat> makeAmount (const char* id, const char* name, float defaultValue)
{
    return std::make_unique<juce::AudioParameterFloat> (
        pid (id), juce::String::fromUTF8 (name),
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.1f), defaultValue,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (oneDecimal));
}

std::unique_ptr<juce::AudioParameterFloat> makeGain (const char* id, const char* name)
{
    return std::make_unique<juce::AudioParameterFloat> (
        pid (id), juce::String::fromUTF8 (name),
        juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction (oneDecimal));
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        pid (stage), "Etapa", juce::StringArray { "Triodo", "Pentodo", "Diodo" }, Triode));

    layout.add (makeGain (drive, "Entrada"));
    layout.add (makeAmount (lowAmount, "Nivel bajo · Cantidad", 3.5f));
    layout.add (makeAmount (peakAmount, "Picos · Cantidad", 5.5f));
    layout.add (makeAmount (peakHardness, "Picos · Dureza", 4.0f));
    layout.add (makeAmount (evenTop, "Pares · Arriba", 3.0f));
    layout.add (makeAmount (evenBottom, "Pares · Abajo", 4.5f));
    layout.add (makeGain (trim, "Salida"));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        pid (oversample), "Sobremuestreo", juce::StringArray { "Desactivado", juce::String::fromUTF8 ("2×"), juce::String::fromUTF8 ("4×"), juce::String::fromUTF8 ("8×") }, X4));
    layout.add (std::make_unique<juce::AudioParameterBool> (pid (bypass), "Bypass", false));

    return layout;
}
} // namespace tufi::params
