// Vuelca las curvas de transferencia reales de Tufi, Poty y Cremita (el mismo código del plugin,
// sin JUCE) para dibujar las figuras de la documentación. Lo compila docs/construir.py.
//   salida: «serie x y» por línea (x de −2 a +2 en Tufi, de −3 a +3 en Poty, de −1 a +1 en Cremita).
#include <cmath>
#include <cstdio>

#include "dsp/ShapeCurve.h"          // Tufi
#include "dsp/CremitaCurve.h"        // Cremita
#include <Saturators.h>              // Poty (core)
#include "dsp/BodyStage.h"           // Nel (codo suave de la etapa de cuerpo)

int main()
{
    constexpr int n = 401;

    // Tufi: ajustes de fábrica (los del preset «Triodo cálido»), una curva por etapa.
    const char* stageName[] = { "tufi_triodo", "tufi_pentodo", "tufi_diodo" };
    for (int s = 0; s < 3; ++s)
    {
        const auto p = tufi::ShapeParams::fromControls (s, 3.5f, 5.5f, 4.0f, 3.0f, 4.5f);
        const auto& base = tufi::stageBase (s);
        for (int i = 0; i < n; ++i)
        {
            const double x = -2.0 + 4.0 * i / (n - 1); // el rango de la tabla: hasta +6 dBFS tras la entrada
            std::printf ("%s %.6f %.6f\n", stageName[s], x, tufi::shape (x, p, base));
        }
    }

    // Poty: los cinco tipos con f'(0) = 1, tal como los usa el motor (g(u) = f(d·u)/d con d = 8).
    const char* satName[] = { "poty_tubo", "poty_cinta", "poty_solido", "poty_germanio", "poty_digital" };
    for (int t = 0; t < 5; ++t)
        for (int i = 0; i < n; ++i)
        {
            const double v = -3.0 + 6.0 * i / (n - 1);
            std::printf ("%s %.6f %.6f\n", satName[t], v, ferb::saturators::f (static_cast<ferb::SatType> (t), v));
        }

    // Nel: reducción de ganancia (dB) de la etapa de cuerpo frente al nivel sobre el umbral (−12 a +12 dB).
    for (const float knee : { 0.0f, 3.0f, 6.0f, 10.0f })
        for (int i = 0; i < n; ++i)
        {
            const double x = -12.0 + 24.0 * i / (n - 1);
            std::printf ("nel_knee_%.0f %.6f %.6f\n", static_cast<double> (knee), x, static_cast<double> (nel::softKnee (static_cast<float> (x), knee)));
        }

    // Cremita: Curva −50, 0 y +50 sin sesgo, y Curva 0 con Sesgo 100 %.
    struct C { const char* name; double curve, bias; };
    for (const C c : { C { "crem_m50", -50, 0 }, C { "crem_0", 0, 0 }, C { "crem_p50", 50, 0 }, C { "crem_0_sesgo", 0, 100 } })
    {
        const auto p = cremita::CurveParams::make (c.curve, c.bias, true);
        for (int i = 0; i < n; ++i)
        {
            const double x = -1.0 + 2.0 * i / (n - 1);
            std::printf ("%s %.6f %.6f\n", c.name, x, cremita::curveF (p, x));
        }
    }
}
