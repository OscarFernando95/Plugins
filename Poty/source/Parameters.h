#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace poty::params
{
inline constexpr int numBands = 5;

// Globales
inline constexpr const char* inGain    = "inGain";
inline constexpr const char* outGain   = "outGain";
inline constexpr const char* mix       = "mix";
inline constexpr const char* quality   = "quality";
inline constexpr const char* intensity = "intensity";
inline constexpr const char* mode      = "mode";
inline constexpr const char* bypass    = "bypass";

// Por banda: prefijo `bN_` (N de 1 a 5) más uno de estos sufijos.
inline constexpr const char* bandOn    = "on";
inline constexpr const char* bandFreq  = "freq";
inline constexpr const char* bandGain  = "gain";
inline constexpr const char* bandQ     = "q";
inline constexpr const char* bandType  = "type";
inline constexpr const char* bandDrive = "drive";

// Orden de los elementos de los parámetros Choice.
enum Quality { Good = 0, Better, Max };
enum Intensity { Subtle = 0, Medium, Aggressive };
enum Mode { Stereo = 0, Mid, Side };
enum SatType { Tube = 0, Tape, Solid, GermaniumDiode, Digital };

/** Identificador de un parámetro de banda; `band` va de 0 a 4 (se publica como b1_ a b5_). */
juce::String bandId (int band, const char* suffix);

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace poty::params
