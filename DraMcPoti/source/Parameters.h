#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace mcpoti::params
{
// Globales. Los IDs de la sección 5 de la ficha no se renombran.
inline constexpr const char* mode = "mode";
inline constexpr const char* era = "era";
inline constexpr const char* voice = "voice";
inline constexpr const char* input = "input";
inline constexpr const char* output = "output";
inline constexpr const char* mix = "mix";
inline constexpr const char* tune = "tune";
inline constexpr const char* phase = "phase";
inline constexpr const char* color = "color";
inline constexpr const char* soften = "soften";
inline constexpr const char* airCorner = "airCorner";
inline constexpr const char* guard = "guard";
inline constexpr const char* width = "width";
inline constexpr const char* monoSafe = "monoSafe";
inline constexpr const char* link = "link";
inline constexpr const char* xo1 = "xo1";
inline constexpr const char* xo2 = "xo2";
inline constexpr const char* xo3 = "xo3";
inline constexpr const char* os = "os";
inline constexpr const char* linPhase = "linPhase";
inline constexpr const char* delta = "delta";
inline constexpr const char* match = "match";
inline constexpr const char* vuRef = "vuRef";
inline constexpr const char* bypass = "bypass"; // el del anfitrión; no forma parte de un preset

inline constexpr int numBands = 4;
inline constexpr const char* bandSuffix[numBands] = { "Lo", "Md", "Hi", "Ar" };

/** ID de un parámetro por banda: bandId ("drive", 2) → "driveHi". */
inline juce::String bandId (const char* base, int band) { return juce::String (base) + bandSuffix[band]; }

enum Mode { Vocal = 0, Master };
enum Era { Vintage = 0, Moderno };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace mcpoti::params
