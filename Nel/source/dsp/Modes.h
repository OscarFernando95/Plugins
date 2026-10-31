#pragma once

#include <array>

namespace nel
{
/** Los ocho modos de limitación. Cada uno es un conjunto de números medibles (valores iniciales, se
    afinan de oído): los cuatro primeros se escriben en los parámetros visibles al elegir el modo y el
    resto son internos. */
struct ModeSpec
{
    const char* name;
    float lookaheadMs, attackMs, releaseMs, kneeDb;
    float preClip;        // recorte suave previo, 0 a 1
    float bodyMarginDb;   // la etapa de cuerpo actúa por debajo del techo
    float extraSmoothing; // suavizado adicional de la ganancia del cuerpo, en ms (0 = ninguno)
    float transientBoostDb; // realce de transitorios antes de limitar
    bool detectorHighPass;  // detector del cuerpo con paso alto a 80 Hz
    float fixedCharacter;   // saturación fija, 0 a 1
    float peakMarginDb;     // margen extra del techo interno
};

inline constexpr int numModes = 8;

inline constexpr std::array<ModeSpec, numModes> modes {{
    //   nombre          look  atk  rel   knee  pre   body  smooth boost hp     char  margin
    { "Moderno",       1.0f,  3.0f,  500.0f,  3.0f, 0.20f, 1.0f, 0.0f, 0.0f, false, 0.00f, 0.0f },
    { "Transparente",  3.0f, 10.0f, 1200.0f,  6.0f, 0.00f, 1.0f, 3.0f, 0.0f, false, 0.00f, 0.0f },
    { "Punchy",        1.0f, 25.0f,  300.0f,  1.0f, 0.10f, 0.0f, 0.0f, 0.0f, false, 0.00f, 0.0f },
    { "Dinámico",      2.0f,  6.0f,  700.0f,  4.0f, 0.00f, 1.0f, 0.0f, 2.0f, false, 0.00f, 0.0f },
    { "Versátil",      2.0f,  8.0f,  800.0f,  4.0f, 0.05f, 1.0f, 0.0f, 0.0f, false, 0.00f, 0.0f },
    { "Agresivo",      0.2f,  1.0f,  250.0f,  0.0f, 0.45f, 0.0f, 0.0f, 0.0f, false, 0.00f, 0.0f },
    { "Bus",           1.5f, 20.0f,  600.0f,  8.0f, 0.00f, 1.0f, 0.0f, 0.0f, true,  0.10f, 0.0f },
    { "Seguro",        5.0f, 15.0f, 2000.0f, 10.0f, 0.00f, 2.0f, 0.0f, 0.0f, false, 0.00f, 0.1f },
}};
} // namespace nel
