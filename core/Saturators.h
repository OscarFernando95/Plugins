#pragma once

namespace ferb
{
/** Tipos de saturación de Poty. El orden coincide con el parámetro `bN_type`. */
enum class SatType { Tube = 0, Tape, Solid, GermaniumDiode, Digital };

inline constexpr int numSatTypes = 5;

/** f(v) y su antiderivada F(v), con f'(0) = 1. Doble precisión: ADAA resta valores casi iguales.
    La cinta usa tanh; su pre y de-énfasis son filtros aparte (ver BandSaturator). */
namespace saturators
{
double f (SatType type, double v) noexcept;
double F (SatType type, double v) noexcept;

/** log cosh(v) en forma estable para |v| grande. */
double logCosh (double v) noexcept;
} // namespace saturators
} // namespace ferb
