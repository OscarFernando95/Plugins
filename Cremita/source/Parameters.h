#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace cremita::params
{
inline constexpr const char* input   = "input";
inline constexpr const char* effect  = "effect";
inline constexpr const char* curve   = "curve";
inline constexpr const char* bias    = "bias";
inline constexpr const char* split   = "split";
inline constexpr const char* xo1     = "xo1";
inline constexpr const char* xo2     = "xo2";
inline constexpr const char* trimLF  = "trimLF";
inline constexpr const char* trimMF  = "trimMF";
inline constexpr const char* trimHF  = "trimHF";
inline constexpr const char* clip    = "clip";
inline constexpr const char* ms      = "ms";
inline constexpr const char* os      = "os";
inline constexpr const char* match   = "match";
inline constexpr const char* output  = "output";
inline constexpr const char* vuRef   = "vuRef";
inline constexpr const char* inOn    = "inOn";
inline constexpr const char* bypass  = "bypass"; // el del anfitrión; no forma parte de un preset

// Orden de los elementos de los parámetros Choice.
enum Oversample { Os1x = 0, Os2x, Os4x, Os8x };
enum VuReference { Vu18 = 0, Vu20, Vu14 };

/** Nivel en dBFS que corresponde a 0 VU para cada opción de `vuRef`. */
inline float vuReferenceDb (int choice) noexcept
{
    constexpr float values[] = { -18.0f, -20.0f, -14.0f };
    return values[choice < 0 ? 0 : (choice > 2 ? 2 : choice)];
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace cremita::params
