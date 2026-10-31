#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace chaus::params
{
inline constexpr const char* mode        = "mode";        // Voz sola / General
inline constexpr const char* thr         = "thr";         // Umbral, −60 a 0 dB
inline constexpr const char* adaptive    = "adaptive";
inline constexpr const char* range       = "range";       // Rango, 0 a 24 dB
inline constexpr const char* hp          = "hp";          // Filtro HP, 2 a 12 kHz
inline constexpr const char* lp          = "lp";          // Filtro LP, 5 a 20 kHz
inline constexpr const char* track       = "track";       // Seguir la s
inline constexpr const char* q           = "q";           // Ancho, 1 a 8
inline constexpr const char* qAuto       = "qAuto";
inline constexpr const char* band        = "band";        // Dividida / Ancha
inline constexpr const char* shape       = "shape";       // Campana / Estante / Auto
inline constexpr const char* sensSoft    = "sensSoft";
inline constexpr const char* sensHard    = "sensHard";
inline constexpr const char* lookOn      = "lookOn";
inline constexpr const char* look        = "look";        // Lookahead, 0 a 15 ms
inline constexpr const char* release     = "release";     // 10 a 100 ms
inline constexpr const char* releaseAuto = "releaseAuto";
inline constexpr const char* link        = "link";        // Enlace estéreo, 0 a 100 %
inline constexpr const char* ms          = "ms";          // Apagado / Mid / Side
inline constexpr const char* scExt       = "scExt";
inline constexpr const char* listenTrig  = "listenTrig";
inline constexpr const char* listenSC    = "listenSC";
inline constexpr const char* output      = "output";      // −12 a +12 dB
inline constexpr const char* bypass      = "bypass";      // el del anfitrión; no forma parte de un preset

// Estado de la interfaz (propiedades del árbol de estado, no parámetros).
inline constexpr const char* uiDisplayZoom = "displayZoom";

enum Mode { VoiceOnly = 0, General };
enum Band { Split = 0, Wide };
enum Shape { Bell = 0, Shelf, Auto };
enum Stereo { Off = 0, Mid, Side };

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace chaus::params
