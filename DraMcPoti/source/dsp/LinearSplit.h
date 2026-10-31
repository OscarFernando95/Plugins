#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace mcpoti
{
/** Cruce de cuatro bandas de fase lineal: tres paso-bajos FIR de 2049 coeficientes (sinc con ventana de Blackman,
    ganancia 1 en continua) en xo1, xo2 y xo3. Las bandas son diferencias de paso-bajos y la última es la señal
    retardada menos el paso-bajo más alto, así que suman EXACTAMENTE la entrada retardada `center` muestras.
    La convolución es directa (sin FFT): son 3 productos punto de 2049 muestras por muestra y canal, que el
    compilador vectoriza. Un canal por instancia. Todo se reserva en `prepare`; `design` no asigna. */
class LinearSplit
{
public:
    static constexpr int taps = 2049;
    static constexpr int center = taps / 2; // latencia, en muestras

    void prepare()
    {
        for (auto& lp : lowPass)
            lp.assign (static_cast<size_t> (taps), 0.0f);
        history.assign (static_cast<size_t> (2 * taps), 0.0f);
        window.resize (static_cast<size_t> (taps));
        for (int n = 0; n < taps; ++n)
        {
            const double t = 2.0 * std::numbers::pi * n / (taps - 1);
            window[static_cast<size_t> (n)] = 0.42 - 0.5 * std::cos (t) + 0.08 * std::cos (2.0 * t);
        }
        reset();
    }

    void reset() noexcept
    {
        std::fill (history.begin(), history.end(), 0.0f);
        pos = 0;
    }

    /** Rediseña los tres paso-bajos (sinc con ventana). Sin asignaciones. */
    void design (double sampleRate, double xo1, double xo2, double xo3) noexcept
    {
        const double cutoffs[3] = { xo1, xo2, xo3 };
        for (size_t k = 0; k < 3; ++k)
        {
            const double wc = 2.0 * std::numbers::pi * std::min (cutoffs[k], 0.45 * sampleRate) / sampleRate;
            double sum = 0.0;
            auto& h = lowPass[k];
            for (int n = 0; n < taps; ++n)
            {
                const double m = n - center;
                const double v = (n == center ? wc / std::numbers::pi : std::sin (wc * m) / (std::numbers::pi * m)) * window[static_cast<size_t> (n)];
                h[static_cast<size_t> (n)] = static_cast<float> (v);
                sum += v;
            }
            const float norm = static_cast<float> (1.0 / sum);
            for (auto& v : h)
                v *= norm;
        }
    }

    void process (float x, std::array<float, 4>& bands) noexcept
    {
        pos = pos == 0 ? taps - 1 : pos - 1;
        history[static_cast<size_t> (pos)] = history[static_cast<size_t> (pos + taps)] = x;
        const float* z = history.data() + pos;

        std::array<std::array<float, 8>, 3> acc {};
        const float* h0 = lowPass[0].data();
        const float* h1 = lowPass[1].data();
        const float* h2 = lowPass[2].data();
        int i = 0;
        for (; i + 8 <= taps; i += 8)
            for (int j = 0; j < 8; ++j)
            {
                acc[0][static_cast<size_t> (j)] += h0[i + j] * z[i + j];
                acc[1][static_cast<size_t> (j)] += h1[i + j] * z[i + j];
                acc[2][static_cast<size_t> (j)] += h2[i + j] * z[i + j];
            }
        float y[3] = { 0.0f, 0.0f, 0.0f };
        for (size_t k = 0; k < 3; ++k)
            for (float v : acc[k])
                y[k] += v;
        for (; i < taps; ++i)
        {
            y[0] += h0[i] * z[i];
            y[1] += h1[i] * z[i];
            y[2] += h2[i] * z[i];
        }

        bands[0] = y[0];
        bands[1] = y[1] - y[0];
        bands[2] = y[2] - y[1];
        bands[3] = z[center] - y[2];
    }

private:
    std::array<std::vector<float>, 3> lowPass;
    std::vector<float> history, window;
    int pos = 0;
};
} // namespace mcpoti
