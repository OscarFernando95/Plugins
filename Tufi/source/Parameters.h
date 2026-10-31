#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace tufi::params
{
inline constexpr const char* stage        = "stage";
inline constexpr const char* drive        = "drive";
inline constexpr const char* lowAmount    = "lowAmount";
inline constexpr const char* peakAmount   = "peakAmount";
inline constexpr const char* peakHardness = "peakHardness";
inline constexpr const char* evenTop      = "evenTop";
inline constexpr const char* evenBottom   = "evenBottom";
inline constexpr const char* trim         = "trim";
inline constexpr const char* oversample   = "oversample";
inline constexpr const char* bypass       = "bypass";

// Orden de los elementos del parámetro `stage`.
enum Stage { Triode = 0, Pentode, Diode };

// Elementos de `oversample`: factor = 2^índice.
enum Oversample { Off = 0, X2, X4, X8 };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace tufi::params
