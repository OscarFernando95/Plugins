#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace nel::params
{
inline constexpr const char* gain          = "gain";
inline constexpr const char* ceiling       = "ceiling";
inline constexpr const char* mode          = "mode";
inline constexpr const char* character     = "character";
inline constexpr const char* lookahead     = "lookahead";
inline constexpr const char* attack        = "attack";
inline constexpr const char* release       = "release";
inline constexpr const char* releaseAuto   = "releaseAuto";
inline constexpr const char* knee          = "knee";
inline constexpr const char* linkTransient = "linkTransient";
inline constexpr const char* linkRelease   = "linkRelease";
inline constexpr const char* truePeak      = "truePeak";
inline constexpr const char* oversample    = "oversample";
inline constexpr const char* osPhase       = "osPhase";
inline constexpr const char* osOffline     = "osOffline";
inline constexpr const char* ditherBits    = "ditherBits";
inline constexpr const char* ditherShape   = "ditherShape";
inline constexpr const char* dcFilter      = "dcFilter";
inline constexpr const char* unityGain     = "unityGain";
inline constexpr const char* audition      = "audition";
inline constexpr const char* bypass        = "bypass"; // el del anfitrión; no forma parte de un preset

// Estado de la interfaz (propiedades del árbol de estado, no parámetros).
inline constexpr const char* uiLufsTarget = "lufsTarget";
inline constexpr const char* uiLufsScale = "lufsScale";
inline constexpr const char* uiMeterScale = "meterScale";
inline constexpr const char* uiLoudnessSource = "loudnessSource";
inline constexpr const char* uiRmsRef = "rmsRef";
inline constexpr const char* uiAdvancedOpen = "advancedOpen";
inline constexpr const char* uiScopeSpan = "scopeSpan"; // 0 = 6 s en vivo, 1 = 10 min, 2 = 2 h, 3 = 10 h

enum Phase { Linear = 0, Minimum };
inline constexpr int ditherBitsFor[] = { 0, 16, 18, 20, 22, 24 };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace nel::params
