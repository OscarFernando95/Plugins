#pragma once

#include <vector>

namespace tufi
{
inline constexpr int tableSize = 4096;
inline constexpr double tableHalfRange = 2.0; // entradas de −2 a +2 (hasta +6 dBFS tras la entrada)
inline constexpr double tableStep = 2.0 * tableHalfRange / (tableSize - 1);

using Table = std::vector<double>;

/** Curva base de cada etapa, normalizada a pendiente 1 en cero. 0 = triodo, 1 = pentodo, 2 = diodo.
    Se calcula la primera vez (unos milisegundos) y se reutiliza; es seguro llamarla desde varios hilos. */
const Table& stageBase (int stage);

/** Interpolación lineal en la tabla, con extensión lineal fuera de rango. */
double lookup (const Table& table, double x) noexcept;
} // namespace tufi
