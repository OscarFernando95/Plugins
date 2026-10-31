#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace maru::params
{
inline constexpr const char* amount     = "amount";     // Supresión, 0 a 100 %
inline constexpr const char* engine     = "engine";     // Supresión / Gate
inline constexpr const char* range      = "range";      // Rango, −60 a −6 dB
inline constexpr const char* sens       = "sens";       // Sensibilidad, −10 a +10
inline constexpr const char* recovery   = "recovery";   // Recuperación (Supresión) o release (Gate), 10 a 1000 ms
inline constexpr const char* gateAuto   = "gateAuto";
inline constexpr const char* gateThresh = "gateThresh"; // −80 a 0 dB
inline constexpr const char* attack     = "attack";     // 0,1 a 50 ms
inline constexpr const char* hold       = "hold";       // 0 a 500 ms
inline constexpr const char* hyst       = "hyst";       // 0 a 12 dB
inline constexpr const char* lookahead  = "lookahead";  // 0 a 10 ms
inline constexpr const char* scHP       = "scHP";       // 20 a 2000 Hz
inline constexpr const char* scLP       = "scLP";       // 1 a 20 kHz
inline constexpr const char* scExt      = "scExt";
inline constexpr const char* link       = "link";       // enlace estéreo, 0 a 100 %
inline constexpr const char* latency    = "latency";    // Baja / Alta
inline constexpr const char* listen     = "listen";     // Escuchar lo quitado
inline constexpr const char* output     = "output";     // −12 a +12 dB
inline constexpr const char* bypass     = "bypass";     // el del anfitrión; no forma parte de un preset

enum Engine { Suppression = 0, Gate };
enum Latency { Low = 0, High };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace maru::params
