#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <functional>

#include "AllocGuard.h"
#include "Signals.h"
#include "TestUtils.h"
#include "dsp/MaruEngine.h"

using Catch::Approx;
using namespace maru;

namespace
{
struct Out
{
    std::vector<float> l, r;
};

/** Pasa audio (mono si `right` está vacío) por el motor en trozos de `chunk`. La salida va sin compensar la latencia. */
Out run (const std::vector<float>& left, const std::vector<float>& right, const Settings& s, double fs, int chunk = 256, MaruEngine* external = nullptr)
{
    const int channels = right.empty() ? 1 : 2;
    MaruEngine local;
    MaruEngine& engine = external != nullptr ? *external : local;
    if (external == nullptr)
        engine.prepare (fs, chunk);
    Out out;
    out.l.assign (left.size(), 0.0f);
    if (channels == 2)
        out.r.assign (left.size(), 0.0f);
    juce::AudioBuffer<float> buf (channels, chunk);
    for (size_t pos = 0; pos < left.size(); pos += size_t (chunk))
    {
        const int n = int (std::min<size_t> (size_t (chunk), left.size() - pos));
        buf.setSize (channels, n, false, false, true);
        std::copy_n (left.begin() + long (pos), n, buf.getWritePointer (0));
        if (channels == 2)
            std::copy_n (right.begin() + long (pos), n, buf.getWritePointer (1));
        engine.process (buf, nullptr, s);
        std::copy_n (buf.getReadPointer (0), n, out.l.begin() + long (pos));
        if (channels == 2)
            std::copy_n (buf.getReadPointer (1), n, out.r.begin() + long (pos));
    }
    return out;
}

Settings base (float amount = 0.5f)
{
    Settings s;
    s.amount = amount;
    return s;
}

std::vector<float> delayed (const std::vector<float>& x, int d)
{
    std::vector<float> y (x.size(), 0.0f);
    for (size_t i = size_t (d); i < x.size(); ++i)
        y[i] = x[i - size_t (d)];
    return y;
}

/** Reducción de nivel (dB, positiva) entre la entrada retrasada y la salida, en [from, to). */
double reductionDb (const std::vector<float>& in, const std::vector<float>& out, int latency, int from, int to)
{
    const auto ref = delayed (in, latency);
    return sig::toDb (sig::rms (ref, from, to)) - sig::toDb (sig::rms (out, from, to));
}
} // namespace

// ───────────────────────────── Estructura y latencia ─────────────────────────────

TEST_CASE ("Tamaño de la FFT y latencia: N muestras exactas, escalado con la frecuencia y con latencia baja")
{
    REQUIRE (MaruEngine::fftSizeFor (44100.0, false) == 1024);
    REQUIRE (MaruEngine::fftSizeFor (48000.0, false) == 1024);
    REQUIRE (MaruEngine::fftSizeFor (96000.0, false) == 2048);
    REQUIRE (MaruEngine::fftSizeFor (192000.0, false) == 4096);
    REQUIRE (MaruEngine::fftSizeFor (48000.0, true) == 512);
    REQUIRE (MaruEngine::fftSizeFor (192000.0, true) == 2048);
    Settings s;
    REQUIRE (MaruEngine::latencyFor (s, 48000.0) == 1024);
    s.lowLatency = true;
    REQUIRE (MaruEngine::latencyFor (s, 48000.0) == 512);
}

TEST_CASE ("Supresión 0 %: la salida es la entrada retrasada N muestras y un impulso aparece en la muestra declarada")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (bool low : { false, true })
        {
            auto s = base (0.0f);
            s.lowLatency = low;
            const int L = MaruEngine::latencyFor (s, fs);

            const auto x = sig::voice (int (fs * 1.5), fs, -20.0);
            const auto noise = sig::whiteNoise (int (fs * 1.5), -50.0, 5);
            const auto in = sig::add (x, noise);
            const auto y = run (in, {}, s, fs);
            const auto ref = delayed (in, L);
            const int from = L + 4096;
            INFO ("fs " << fs << ", baja " << low << ", latencia " << L);
            REQUIRE (testutil::residualDb (y.l.data() + from, ref.data() + from, int (y.l.size()) - from) < -110.0);

            std::vector<float> impulse (size_t (fs * 0.5), 0.0f);
            impulse[3000] = 0.5f;
            const auto z = run (impulse, {}, s, fs);
            size_t peak = 0;
            for (size_t i = 0; i < z.l.size(); ++i)
                if (std::abs (z.l[i]) > std::abs (z.l[peak]))
                    peak = i;
            REQUIRE (int (peak) == 3000 + L);
        }
}

// ───────────────────────────── Estimador de ruido ─────────────────────────────

TEST_CASE ("Estimador de ruido: converge a ±1,5 dB del nivel de un ruido blanco en 3 s")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
        for (double level : { -70.0, -50.0, -30.0 })
        {
            const auto noise = sig::whiteNoise (int (fs * 3.0), level, unsigned (level * -1.0));
            MaruEngine engine;
            engine.prepare (fs, 256);
            run (noise, {}, base (0.5f), fs, 256, &engine);
            const double floor = engine.getTelemetry().noiseFloorDb.load();
            INFO ("fs " << fs << ", ruido " << level << " dBFS, estimado " << floor);
            REQUIRE (floor == Approx (level).margin (1.5));
        }
}

TEST_CASE ("Estimador de ruido: con voz encima su mediana de los últimos 4 s queda dentro de 6 dB del piso real (la fuga de la voz bajo 5 veces el ruido se toma como ruido)")
{
    for (double fs : { 44100.0, 48000.0 })
    {
        const int n = int (fs * 10.0);
        const auto in = sig::add (sig::voice (n, fs, -22.0), sig::whiteNoise (n, -52.0, 9));
        MaruEngine engine;
        engine.prepare (fs, 256);
        juce::AudioBuffer<float> buf (1, 256);
        const auto s = base (0.5f);
        std::vector<float> floors;
        for (size_t pos = 0; pos + 256 <= in.size(); pos += 256)
        {
            std::copy_n (in.begin() + long (pos), 256, buf.getWritePointer (0));
            engine.process (buf, nullptr, s);
            if (pos > size_t (fs * 6.0))
                floors.push_back (engine.getTelemetry().noiseFloorDb.load());
        }
        std::nth_element (floors.begin(), floors.begin() + long (floors.size() / 2), floors.end());
        const double median = floors[floors.size() / 2];
        INFO ("fs " << fs << ", mediana " << median << " dB (real −52)");
        REQUIRE (median == Approx (-52.0).margin (6.0));
    }
}

TEST_CASE ("Estimador de ruido: sigue un cambio de nivel del ruido (de −55 a −45 dBFS) en pocos segundos")
{
    const double fs = 48000.0;
    auto a = sig::whiteNoise (int (fs * 3.0), -55.0, 1), b = sig::whiteNoise (int (fs * 5.0), -45.0, 2);
    a.insert (a.end(), b.begin(), b.end());
    MaruEngine engine;
    engine.prepare (fs, 256);
    run (a, {}, base (0.5f), fs, 256, &engine);
    REQUIRE (engine.getTelemetry().noiseFloorDb.load() == Approx (-45.0).margin (2.0));
}

// ───────────────────────────── Reducción ─────────────────────────────

TEST_CASE ("Reducción estacionaria: 20 dB o más con ruido blanco a −40 dBFS al 100 %, y no crece al bajar la perilla")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        const auto noise = sig::whiteNoise (int (fs * 6.0), -40.0, 11);
        double previous = -1.0;
        for (float a : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
        {
            auto s = base (a);
            const auto y = run (noise, {}, s, fs);
            const double r = reductionDb (noise, y.l, MaruEngine::latencyFor (s, fs), int (fs * 4.0), int (fs * 6.0));
            INFO ("fs " << fs << ", Supresión " << a << ": " << r << " dB");
            REQUIRE (r >= previous - 0.3); // no baja al subir la perilla
            previous = r;
            if (a == 0.0f)
                REQUIRE (std::abs (r) < 0.01);
            if (a == 1.0f)
                REQUIRE (r >= 20.0);
        }
    }
}

TEST_CASE ("Reducción: el Rango la limita")
{
    const double fs = 48000.0;
    const auto noise = sig::whiteNoise (int (fs * 6.0), -40.0, 12);
    for (float range : { 6.0f, 12.0f, 24.0f })
    {
        auto s = base (1.0f);
        s.rangeDb = range;
        const auto y = run (noise, {}, s, fs);
        const double r = reductionDb (noise, y.l, MaruEngine::latencyFor (s, fs), int (fs * 4.0), int (fs * 6.0));
        INFO ("Rango " << range << ": " << r << " dB");
        REQUIRE (r <= range + 1.0);
        REQUIRE (r >= range - 6.0); // el LSA no llega al piso en cada bin con ruido blanco
    }
}

TEST_CASE ("Voz con ruido: al 50 % la voz no baja más de 1 dB y el ruido sí baja")
{
    for (double fs : { 44100.0, 48000.0 })
    {
        const int n = int (fs * 8.0);
        const auto v = sig::voice (n, fs, -22.0);
        const auto noise = sig::whiteNoise (n, -50.0, 21);
        const auto in = sig::add (v, noise);
        auto s = base (0.5f);
        const int L = MaruEngine::latencyFor (s, fs);
        const auto y = run (in, {}, s, fs);

        // Componente de voz de la salida: proyección sobre la voz limpia retrasada.
        const auto ref = delayed (v, L);
        double num = 0.0, den = 0.0;
        for (int i = int (fs * 3.0); i < n; ++i)
        {
            num += double (y.l[size_t (i)]) * ref[size_t (i)];
            den += double (ref[size_t (i)]) * ref[size_t (i)];
        }
        const double voiceGainDb = sig::toDb (num / den);
        // Ruido en las pausas: tramos donde la voz es casi cero.
        double sumIn = 0.0, sumOut = 0.0;
        int count = 0;
        for (int i = int (fs * 3.0); i < n; ++i)
            if (std::abs (ref[size_t (i)]) < 1.0e-5f)
            {
                sumIn += double (delayed (in, L)[size_t (i)]) * delayed (in, L)[size_t (i)];
                sumOut += double (y.l[size_t (i)]) * y.l[size_t (i)];
                ++count;
                if (count > int (fs * 1.0))
                    break;
            }
        INFO ("fs " << fs << ": ganancia de la voz " << voiceGainDb << " dB; ruido en pausas " << 10.0 * std::log10 (sumIn / std::max (sumOut, 1e-20)) << " dB menos");
        REQUIRE (voiceGainDb > -1.0);
        REQUIRE (count > 1000);
        REQUIRE (10.0 * std::log10 (sumIn / std::max (sumOut, 1.0e-20)) > 6.0);
    }
}

TEST_CASE ("Transitorios: una pulsación brillante sobre ruido mantiene su pico dentro de 1,5 dB al 50 %")
{
    const double fs = 48000.0;
    const int n = int (fs * 5.0);
    auto in = sig::whiteNoise (n, -50.0, 31);
    // «Tecla»: ruido de banda alta de 8 ms con caída rápida, a −18 dBFS de pico.
    std::mt19937 rng (77);
    std::normal_distribution<float> dist (0.0f, 1.0f);
    std::vector<float> click (int (0.008 * fs));
    float prev = 0.0f;
    for (size_t i = 0; i < click.size(); ++i)
    {
        const float w = dist (rng), hp = w - 0.7f * prev; // paso alto simple
        prev = w;
        click[i] = hp * float (std::exp (-double (i) / (0.002 * fs)));
    }
    float peak = 0.0f;
    for (float v : click) peak = std::max (peak, std::abs (v));
    const float g = float (sig::dbToLin (-18.0)) / peak;
    const int at = int (fs * 3.5);
    for (size_t i = 0; i < click.size(); ++i)
        in[size_t (at) + i] += click[i] * g;

    auto s = base (0.5f);
    const int L = MaruEngine::latencyFor (s, fs);
    const auto y = run (in, {}, s, fs);
    float inPeak = 0.0f, outPeak = 0.0f;
    for (int i = at; i < at + int (click.size()); ++i)
    {
        inPeak = std::max (inPeak, std::abs (in[size_t (i)]));
        outPeak = std::max (outPeak, std::abs (y.l[size_t (i + L)]));
    }
    INFO ("pico de entrada " << inPeak << ", de salida " << outPeak << " (" << sig::toDb (outPeak / inPeak) << " dB)");
    REQUIRE (std::abs (sig::toDb (outPeak / inPeak)) < 1.5);
}

TEST_CASE ("Ruido musical: el residuo no tiene picos aislados en el plano tiempo-frecuencia más allá de lo que tiene el propio ruido")
{
    const double fs = 48000.0;
    const int n = int (fs * 8.0);
    const auto noise = sig::whiteNoise (n, -40.0, 41);
    auto s = base (1.0f);
    const auto y = run (noise, {}, s, fs);

    // Espectrograma de potencia (cuadros de 1024 con salto de 512, Hann) y fracción de celdas 10 dB por encima de la mediana de sus vecinas.
    const auto isolatedPeaks = [&] (const std::vector<float>& x, int from)
    {
        constexpr int N = 1024, H = 512;
        const int frames = (int (x.size()) - from - N) / H;
        std::vector<std::vector<double>> P (size_t (frames), std::vector<double> (N / 2 + 1));
        for (int f = 0; f < frames; ++f)
            for (int k = 8; k < N / 2 - 8; ++k)
            {
                std::complex<double> acc = 0.0;
                for (int i = 0; i < N; i += 1)
                {
                    const double w = 0.5 - 0.5 * std::cos (2.0 * std::numbers::pi * i / N);
                    acc += w * x[size_t (from + f * H + i)] * std::polar (1.0, -2.0 * std::numbers::pi * k * i / N);
                }
                P[size_t (f)][size_t (k)] = std::norm (acc);
            }
        long cells = 0, peaks = 0;
        for (int f = 1; f < frames - 1; ++f)
            for (int k = 9; k < N / 2 - 9; k += 3) // submuestreo en frecuencia para acortar la prueba
            {
                std::vector<double> nb;
                for (int df = -1; df <= 1; ++df)
                    for (int dk = -1; dk <= 1; ++dk)
                        if (df != 0 || dk != 0)
                            nb.push_back (P[size_t (f + df)][size_t (k + dk)]);
                std::nth_element (nb.begin(), nb.begin() + 4, nb.end());
                ++cells;
                peaks += P[size_t (f)][size_t (k)] > 10.0 * nb[4] ? 1 : 0;
            }
        return double (peaks) / double (cells);
    };
    const double reference = isolatedPeaks (noise, int (fs * 4.0)), processed = isolatedPeaks (y.l, int (fs * 4.0) + MaruEngine::latencyFor (s, fs));
    INFO ("celdas con pico aislado: ruido original " << 100.0 * reference << " %, procesado " << 100.0 * processed << " %");
    REQUIRE (processed < 3.0 * reference + 0.002);
}

// ───────────────────────────── Estéreo ─────────────────────────────

TEST_CASE ("Estéreo: con enlace del 100 % la misma ganancia en L y R conserva la imagen; con 0 % cada canal es independiente")
{
    const double fs = 48000.0;
    const int n = int (fs * 6.0);
    const auto v = sig::voice (n, fs, -22.0);
    const auto noise = sig::whiteNoise (n, -48.0, 51);
    const auto left = sig::add (v, noise);
    std::vector<float> right (left.size());
    for (size_t i = 0; i < right.size(); ++i)
        right[i] = left[i] * 0.5f; // −6 dB

    auto s = base (0.6f);
    s.linkPercent = 100.0f;
    const int L = MaruEngine::latencyFor (s, fs);
    const auto y = run (left, right, s, fs);
    const double ratio = sig::toDb (sig::rms (y.l, int (fs * 4.0), n)) - sig::toDb (sig::rms (y.r, int (fs * 4.0), n));
    REQUIRE (ratio == Approx (6.02).margin (0.1));

    // Enlace 0 %: un canal en silencio no se contagia del otro y el otro se reduce.
    s.linkPercent = 0.0f;
    const std::vector<float> silent (left.size(), 0.0f);
    const auto z = run (noise, silent, s, fs);
    REQUIRE (sig::rms (z.r, 0, n) < 1.0e-6);
    REQUIRE (reductionDb (noise, z.l, L, int (fs * 4.0), n) > 8.0);

    // Canales idénticos, salidas idénticas.
    s.linkPercent = 50.0f;
    const auto same = run (left, left, s, fs);
    for (size_t i = 0; i < same.l.size(); i += 101)
        REQUIRE (same.l[i] == Approx (same.r[i]).margin (1.0e-6));
}

// ───────────────────────────── Robustez ─────────────────────────────

TEST_CASE ("Robustez: silencio, continua, NaN, infinitos y subnormales; todas las tasas y bloques")
{
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int chunk : { 1, 7, 64, 255, 256, 257, 1024, 4096 })
        {
            const int n = int (fs * 0.35);
            const auto check = [&] (const std::vector<float>& x)
            {
                auto s = base (1.0f);
                const auto y = run (x, x, s, fs, chunk);
                for (size_t i = 0; i < x.size(); ++i)
                {
                    INFO ("fs " << fs << ", bloque " << chunk << ", muestra " << i);
                    REQUIRE (std::isfinite (y.l[i]));
                    REQUIRE (std::isfinite (y.r[i]));
                    REQUIRE (std::abs (y.l[i]) < 8.0f);
                }
            };
            check (std::vector<float> (static_cast<size_t> (n), 0.0f));
            check (std::vector<float> (static_cast<size_t> (n), 0.5f));
            check (std::vector<float> (static_cast<size_t> (n), 1.0e-40f));
            std::vector<float> bad (static_cast<size_t> (n), 0.1f);
            for (size_t i = 0; i < bad.size(); i += 53)
                bad[i] = std::numeric_limits<float>::quiet_NaN();
            for (size_t i = 7; i < bad.size(); i += 211)
                bad[i] = std::numeric_limits<float>::infinity();
            check (bad);
        }
}

TEST_CASE ("El tamaño del bloque no cambia el resultado")
{
    const double fs = 48000.0;
    const int n = int (fs * 3.0);
    const auto in = sig::add (sig::voice (n, fs, -22.0), sig::whiteNoise (n, -50.0, 61));
    auto s = base (0.7f);
    const auto a = run (in, {}, s, fs, 256), b = run (in, {}, s, fs, 1), c = run (in, {}, s, fs, 1000);
    REQUIRE (testutil::residualDb (a.l.data(), b.l.data(), n) < -100.0);
    REQUIRE (testutil::residualDb (a.l.data(), c.l.data(), n) < -100.0);
}

TEST_CASE ("MaruEngine::process no reserva memoria, ni con controles y estructura cambiando")
{
    for (int channels : { 1, 2 })
        for (int block : { 64, 512, 1000 })
        {
            const double fs = 48000.0;
            MaruEngine engine;
            engine.prepare (fs, block);
            juce::AudioBuffer<float> buf (channels, block), side (2, block);
            const auto in = sig::add (sig::voice (int (fs * 2.0), fs, -22.0), sig::whiteNoise (int (fs * 2.0), -50.0, 71));
            std::mt19937 rng (3);
            long total = 0;
            size_t pos = 0;
            for (int b = 0; b < 400; ++b)
            {
                Settings s;
                if (b % 7 == 0)
                {
                    s.amount = float (rng() % 101) / 100.0f;
                    s.lowLatency = rng() % 2 == 0;
                    s.gate = rng() % 3 == 0;
                    s.rangeDb = 6.0f + float (rng() % 55);
                    s.listen = rng() % 4 == 0;
                    s.linkPercent = float (rng() % 101);
                    s.bypass = rng() % 5 == 0;
                }
                for (int ch = 0; ch < channels; ++ch)
                    for (int i = 0; i < block; ++i)
                        buf.setSample (ch, i, in[(pos + size_t (i)) % in.size()]);
                pos += size_t (block);
                allocguard::Scope scope;
                engine.process (buf, &side, s);
                total += scope.allocations();
            }
            INFO ("canales " << channels << ", bloque " << block);
            REQUIRE (total == 0);
        }
}

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink;
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}

TEST_CASE ("Informe: cifras de M1 y CPU", "[.report]")
{
    const double fs = 48000.0;
    const auto noise = sig::whiteNoise (int (fs * 6.0), -40.0, 11);
    for (float range : { 30.0f, 60.0f })
        for (float a : { 0.0f, 0.25f, 0.5f, 0.75f, 1.0f })
        {
            auto s = base (a);
            s.rangeDb = range;
            const auto y = run (noise, {}, s, fs);
            std::printf ("ruido blanco −40 dBFS, Rango %.0f, Supresión %3.0f %%: reducción %5.1f dB\n", range, a * 100.0f, reductionDb (noise, y.l, MaruEngine::latencyFor (s, fs), int (fs * 4.0), int (fs * 6.0)));
        }
    // Voz con ruido.
    const int n = int (fs * 8.0);
    const auto v = sig::voice (n, fs, -22.0);
    const auto in = sig::add (v, sig::whiteNoise (n, -50.0, 21));
    for (float a : { 0.25f, 0.5f, 0.75f, 1.0f })
    {
        auto s = base (a);
        const int L = MaruEngine::latencyFor (s, fs);
        const auto y = run (in, {}, s, fs);
        const auto ref = delayed (v, L);
        double num = 0.0, den = 0.0;
        for (int i = int (fs * 3.0); i < n; ++i) { num += double (y.l[size_t (i)]) * ref[size_t (i)]; den += double (ref[size_t (i)]) * ref[size_t (i)]; }
        std::printf ("voz −22 dBFS con ruido −50 dBFS, Supresión %3.0f %%: ganancia de la voz %+.2f dB\n", a * 100.0f, sig::toDb (num / den));
    }
    // CPU.
    for (double rate : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (bool low : { false, true })
        {
            const auto x = sig::add (sig::voice (int (rate * 20.0), rate, -22.0), sig::whiteNoise (int (rate * 20.0), -50.0, 3));
            MaruEngine engine;
            engine.prepare (rate, 512);
            auto s = base (0.5f);
            s.lowLatency = low;
            juce::AudioBuffer<float> buf (2, 512);
            const auto start = std::chrono::steady_clock::now();
            size_t pos = 0;
            for (; pos + 512 <= x.size(); pos += 512)
            {
                std::copy_n (x.begin() + long (pos), 512, buf.getWritePointer (0));
                std::copy_n (x.begin() + long (pos), 512, buf.getWritePointer (1));
                engine.process (buf, nullptr, s);
            }
            const double seconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();
            std::printf ("CPU Supresión estéreo %6.0f Hz, %s: %.2f %% de un núcleo\n", rate, low ? "latencia baja" : "latencia alta", 100.0 * seconds / (double (pos) / rate));
        }
}
