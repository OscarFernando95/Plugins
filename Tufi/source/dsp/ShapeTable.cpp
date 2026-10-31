#include "ShapeTable.h"

namespace tufi
{
void ShapeTableBuilder::build (const ShapeParams& p, ShapeResult& out)
{
    const auto& base = stageBase (p.stage);

    for (int i = 0; i < tableSize; ++i)
        out.y[static_cast<size_t> (i)] = shape (-tableHalfRange + i * tableStep, p, base);

    const auto at = [&] (double x) { return lookup (out.y, x); };

    constexpr double h = 0.01;
    out.slope = static_cast<float> ((at (h) - at (-h)) / (2.0 * h));

    // Techo de picos: el umbral T que fija la dureza, en dB.
    out.ceilingDb = static_cast<float> (20.0 * std::log10 (std::max (1.0e-6, static_cast<double> (p.ceiling))));

    const double up = at (1.0), down = at (-1.0);
    out.asymmetry = static_cast<float> (100.0 * std::abs (up + down) / std::max (1.0e-9, std::abs (up) + std::abs (down)));
}
} // namespace tufi
