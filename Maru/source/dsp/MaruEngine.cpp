#include "MaruEngine.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace maru
{
namespace
{
constexpr float tiny = 1.0e-20f;
constexpr double structureFadeSeconds = 0.005, bypassSeconds = 0.020, listenSeconds = 0.010, outputSeconds = 0.020;

double coeff (double seconds, double fs) { return 1.0 - std::exp (-1.0 / (seconds * fs)); }
float sane (float x) noexcept { return std::isfinite (x) ? x : 0.0f; }
} // namespace

// ───────────────────────────────── Estimador de ruido ─────────────────────────────────

void NoiseEstimator::prepare (int numBins, double hopSeconds)
{
    bins = numBins;
    alphaS = static_cast<float> (std::exp (-hopSeconds / 0.03));
    subFrames = std::max (1, static_cast<int> (std::lround (0.5 / hopSeconds))); // tres subventanas de 0,5 s: el mínimo abarca de 1 a 1,5 s
    for (auto* v : { &smoothed, &minimum, &minTemp, &minOld, &windowMin, &presence, &lambda, &scratch })
        v->assign (static_cast<size_t> (bins), 0.0f);
    reset();
}

void NoiseEstimator::reset() noexcept
{
    frameCount = subCount = 0;
    for (auto* v : { &smoothed, &minimum, &minTemp, &minOld, &windowMin, &presence, &lambda, &scratch })
        std::fill (v->begin(), v->end(), 0.0f);
}

const float* NoiseEstimator::update (const float* power) noexcept
{
    const int last = bins - 1;
    if (frameCount == 0)
    {
        for (int k = 0; k < bins; ++k)
        {
            smoothed[static_cast<size_t> (k)] = minimum[static_cast<size_t> (k)] = minTemp[static_cast<size_t> (k)] = minOld[static_cast<size_t> (k)] = lambda[static_cast<size_t> (k)] = power[k];
            presence[static_cast<size_t> (k)] = 0.0f;
        }
        frameCount = 1;
        return lambda.data();
    }

    // Potencia suavizada en frecuencia (3 bins) y en tiempo.
    for (int k = 0; k <= last; ++k)
        scratch[static_cast<size_t> (k)] = 0.25f * power[std::max (k - 1, 0)] + 0.5f * power[k] + 0.25f * power[std::min (k + 1, last)];
    for (int k = 0; k < bins; ++k)
    {
        auto& s = smoothed[static_cast<size_t> (k)];
        s = alphaS * s + (1.0f - alphaS) * scratch[static_cast<size_t> (k)];
        minTemp[static_cast<size_t> (k)] = std::min (minTemp[static_cast<size_t> (k)], s); // mínimo de la subventana en curso
    }
    if (++subCount >= subFrames)
    {
        subCount = 0;
        for (int k = 0; k < bins; ++k)
        {
            minOld[static_cast<size_t> (k)] = minimum[static_cast<size_t> (k)]; // «minimum» guarda el mínimo de la subventana anterior
            minimum[static_cast<size_t> (k)] = minTemp[static_cast<size_t> (k)];
            minTemp[static_cast<size_t> (k)] = smoothed[static_cast<size_t> (k)];
        }
    }
    // Mínimo de las tres últimas subventanas.
    for (int k = 0; k < bins; ++k)
        windowMin[static_cast<size_t> (k)] = std::min ({ minimum[static_cast<size_t> (k)], minOld[static_cast<size_t> (k)], minTemp[static_cast<size_t> (k)] });

    // Probabilidad de presencia de voz y promedio recursivo del ruido solo donde no hay voz.
    const float warm = std::min (alphaD, 1.0f - 1.0f / static_cast<float> (frameCount + 1)); // al principio, promedio simple
    for (int k = 0; k < bins; ++k)
    {
        const auto i = static_cast<size_t> (k);
        const float ratio = smoothed[i] / std::max (windowMin[i], tiny);
        // Voz presente si la potencia suavizada pasa `delta` veces el mínimo o si el bin está 10 veces (10 dB) sobre el ruido estimado:
        // lo segundo reacciona al principio de una sílaba, cuando la potencia suavizada aún no ha subido.
        const bool speech = ratio > delta || scratch[i] > 10.0f * lambda[i];
        presence[i] = alphaP * presence[i] + (1.0f - alphaP) * (speech ? 1.0f : 0.0f);
        const float a = warm + (1.0f - warm) * presence[i];
        lambda[i] = a * lambda[i] + (1.0f - a) * power[k];
    }
    ++frameCount;
    return lambda.data();
}

// ───────────────────────────────── Ganancia espectral ─────────────────────────────────

void SuppressionGain::prepare (int numBins, double hopSeconds)
{
    bins = numBins;
    hop = hopSeconds;
    attackCoeff = static_cast<float> (std::exp (-hopSeconds / 0.002));
    boostFrames = std::max (1, static_cast<int> (std::lround (0.03 / hopSeconds)));
    for (auto* v : { &previousGain, &previousGamma, &previousMag, &smoothedGain, &target })
        v->assign (static_cast<size_t> (bins), 0.0f);
    reset();
}

void SuppressionGain::reset() noexcept
{
    std::fill (previousGain.begin(), previousGain.end(), 1.0f);
    std::fill (previousGamma.begin(), previousGamma.end(), 1.0f);
    std::fill (previousMag.begin(), previousMag.end(), 0.0f);
    std::fill (smoothedGain.begin(), smoothedGain.end(), 1.0f);
    std::fill (target.begin(), target.end(), 1.0f);
    boostLeft = 0;
}

/** Integral exponencial E1(x), x > 0 (aproximaciones 5.1.53 y 5.1.56 de Abramowitz y Stegun). */
float SuppressionGain::expint1 (float x) noexcept
{
    if (x <= 1.0f)
        return -std::log (x) - 0.57721566f + x * (0.99999193f + x * (-0.24991055f + x * (0.05519968f + x * (-0.00976004f + x * 0.00107857f))));
    return std::exp (-x) / x * ((x * x + 2.334733f * x + 0.250621f) / (x * x + 3.330657f * x + 1.681534f));
}

void SuppressionGain::compute (const float* power, const float* noise, const Params& p, float* gain) noexcept
{
    // Flujo espectral: un ataque (pulsación, consonante) sube el piso 6 dB durante unos 30 ms.
    float flux = 0.0f, previousSum = 0.0f;
    for (int k = 0; k < bins; ++k)
    {
        const float m = std::sqrt (power[k]), before = previousMag[static_cast<size_t> (k)];
        flux += std::max (0.0f, m - before);
        previousSum += before;
        previousMag[static_cast<size_t> (k)] = m;
    }
    if (previousSum > 1.0e-10f && flux > 0.6f * previousSum)
        boostLeft = boostFrames;
    const bool transient = boostLeft > 0;
    const float floorGain = transient ? std::min (1.0f, p.floorGain * 2.0f) : p.floorGain;
    if (transient)
        --boostLeft;

    // Durante un ataque el decision-directed se apoya menos en la trama anterior (si no, aplasta el pico).
    const float ddAlpha = transient ? 0.3f : 0.98f;
    constexpr float xiMin = 1.0e-3f;
    for (int k = 0; k < bins; ++k)
    {
        const auto i = static_cast<size_t> (k);
        const float lambda = std::max (noise[k] * p.overestimate * p.sensGain, tiny);
        const float gamma = power[k] / lambda;
        const float xi = std::max (ddAlpha * previousGain[i] * previousGain[i] * previousGamma[i] + (1.0f - ddAlpha) * std::max (gamma - 1.0f, 0.0f), xiMin);
        const float v = std::max (xi / (1.0f + xi) * gamma, 1.0e-6f);
        const float g = std::min (1.0f, xi / (1.0f + xi) * std::exp (0.5f * expint1 (v)));
        previousGain[i] = g;
        previousGamma[i] = gamma;
        target[i] = std::max (g, floorGain);
    }

    // Suavizado en frecuencia (3 bins) y en tiempo. Como en la maqueta: cuando aparece la señal la ganancia sube rápido (2 ms) para no
    // ablandar el principio de una sílaba, y la supresión «recupera» (la ganancia baja) con la constante de Recuperación.
    const float recover = static_cast<float> (std::exp (-hop / std::max (p.recoverySeconds, 0.001f)));
    for (int k = 0; k < bins; ++k)
    {
        const auto i = static_cast<size_t> (k);
        const float f = 0.25f * target[static_cast<size_t> (std::max (k - 1, 0))] + 0.5f * target[i] + 0.25f * target[static_cast<size_t> (std::min (k + 1, bins - 1))];
        auto& s = smoothedGain[i];
        // La subida rápida solo hacia ganancias altas (señal presente); hacia las bajas, los picos aleatorios del ruido abrirían el bin.
        const float c = (f > s && f > 0.5f) ? attackCoeff : recover;
        s += (1.0f - c) * (f - s);
        gain[k] = s;
    }
}

// ───────────────────────────────── STFT ─────────────────────────────────

struct MaruEngine::Stft
{
    Stft (int fftSize, double sampleRate, int numChannels)
        : N (fftSize), H (fftSize / 4), bins (fftSize / 2 + 1), channels (numChannels), fs (sampleRate),
          fft (static_cast<int> (std::lround (std::log2 (fftSize))))
    {
        window.resize (static_cast<size_t> (N));
        for (int n = 0; n < N; ++n)
            window[static_cast<size_t> (n)] = std::sqrt (0.5f - 0.5f * std::cos (2.0f * std::numbers::pi_v<float> * static_cast<float> (n) / static_cast<float> (N)));

        // Escalas: solape-suma (suma de w² de las cuatro tramas) y la de la FFT inversa de JUCE, medida con una señal de prueba.
        float overlap = 0.0f;
        for (int m = 0; m < 4; ++m)
            overlap += window[static_cast<size_t> (m * H)] * window[static_cast<size_t> (m * H)];
        std::vector<float> test (static_cast<size_t> (2 * N), 0.0f), original (static_cast<size_t> (N));
        for (int n = 0; n < N; ++n)
            original[static_cast<size_t> (n)] = test[static_cast<size_t> (n)] = std::sin (0.3f * static_cast<float> (n)) + 0.2f * std::cos (1.7f * static_cast<float> (n * n % 97));
        fft.performRealOnlyForwardTransform (test.data(), true);
        fft.performRealOnlyInverseTransform (test.data());
        double xy = 0.0, xx = 0.0;
        for (int n = 0; n < N; ++n)
        {
            xy += double (original[static_cast<size_t> (n)]) * test[static_cast<size_t> (n)];
            xx += double (original[static_cast<size_t> (n)]) * original[static_cast<size_t> (n)];
        }
        outScale = static_cast<float> (xx / xy) / overlap;

        const double hopSeconds = static_cast<double> (H) / fs;
        for (auto& c : ch)
        {
            c.ring.assign (static_cast<size_t> (N), 0.0f);
            c.acc.assign (static_cast<size_t> (N), 0.0f);
            c.final.assign (static_cast<size_t> (H), 0.0f);
            c.spec.assign (static_cast<size_t> (2 * N), 0.0f);
            c.power.assign (static_cast<size_t> (bins), 0.0f);
            c.gain.assign (static_cast<size_t> (bins), 1.0f);
            c.estimator.prepare (bins, hopSeconds);
            c.suppressor.prepare (bins, hopSeconds);
        }
        mid.power.assign (static_cast<size_t> (bins), 0.0f);
        mid.gain.assign (static_cast<size_t> (bins), 1.0f);
        mid.estimator.prepare (bins, hopSeconds);
        mid.suppressor.prepare (bins, hopSeconds);

        // Bandas de la pantalla: 32 bandas logarítmicas de 100 Hz a 16 kHz.
        const double top = std::min (16000.0, 0.48 * fs);
        for (int b = 0; b <= Telemetry::bands; ++b)
        {
            const double f = 100.0 * std::pow (top / 100.0, static_cast<double> (b) / Telemetry::bands);
            edges[static_cast<size_t> (b)] = std::clamp (static_cast<int> (std::lround (f / (fs / N))), 1, bins - 1);
        }
        for (int b = 1; b <= Telemetry::bands; ++b)
            edges[static_cast<size_t> (b)] = std::max (edges[static_cast<size_t> (b)], edges[static_cast<size_t> (b - 1)] + 1);
        bandDb.fill (-120.0f);
    }

    /** Banda (HP a LP del sidechain) en la que se calcula el piso de ruido del Gate. */
    void setBand (float loHz, float hiHz) noexcept
    {
        const double binHz = fs / N;
        bandLo = std::clamp (static_cast<int> (std::floor (loHz / binHz)), 1, bins - 1);
        bandHi = std::clamp (static_cast<int> (std::ceil (hiHz / binHz)), bandLo + 1, bins);
    }

    void reset()
    {
        for (auto& c : ch)
        {
            std::fill (c.ring.begin(), c.ring.end(), 0.0f);
            std::fill (c.acc.begin(), c.acc.end(), 0.0f);
            std::fill (c.final.begin(), c.final.end(), 0.0f);
            c.estimator.reset();
            c.suppressor.reset();
            std::fill (c.gain.begin(), c.gain.end(), 1.0f);
        }
        mid.estimator.reset();
        mid.suppressor.reset();
        std::fill (mid.gain.begin(), mid.gain.end(), 1.0f);
        writePos = sinceHop = outPos = 0;
        bandFloorDb = -120.0f;
    }

    /** Una muestra por canal: devuelve la salida con latencia N y, cada salto, procesa una trama. */
    void tick (const float* in, float* out, bool synthesise, const SuppressionGain::Params& params, float link, Telemetry& tel) noexcept
    {
        for (int c = 0; c < channels; ++c)
        {
            out[c] = ch[static_cast<size_t> (c)].final[static_cast<size_t> (outPos)];
            ch[static_cast<size_t> (c)].ring[static_cast<size_t> (writePos)] = in[c];
        }
        outPos = (outPos + 1) % H;
        writePos = (writePos + 1) % N;
        if (++sinceHop >= H)
        {
            sinceHop = 0;
            frame (synthesise, params, link, tel);
        }
    }

    void frame (bool synthesise, const SuppressionGain::Params& params, float link, Telemetry& tel) noexcept
    {
        for (int c = 0; c < channels; ++c)
        {
            auto& s = ch[static_cast<size_t> (c)];
            for (int i = 0; i < N; ++i)
                s.spec[static_cast<size_t> (i)] = s.ring[static_cast<size_t> ((writePos + i) % N)] * window[static_cast<size_t> (i)];
            std::fill (s.spec.begin() + N, s.spec.end(), 0.0f);
            fft.performRealOnlyForwardTransform (s.spec.data(), true);
            for (int k = 0; k < bins; ++k)
            {
                const float re = s.spec[static_cast<size_t> (2 * k)], im = s.spec[static_cast<size_t> (2 * k + 1)];
                s.power[static_cast<size_t> (k)] = re * re + im * im;
            }
        }

        // Análisis sobre la suma mid (la señal misma si es mono).
        for (int k = 0; k < bins; ++k)
        {
            if (channels == 1)
                mid.power[static_cast<size_t> (k)] = ch[0].power[static_cast<size_t> (k)];
            else
            {
                const float re = 0.5f * (ch[0].spec[static_cast<size_t> (2 * k)] + ch[1].spec[static_cast<size_t> (2 * k)]);
                const float im = 0.5f * (ch[0].spec[static_cast<size_t> (2 * k + 1)] + ch[1].spec[static_cast<size_t> (2 * k + 1)]);
                mid.power[static_cast<size_t> (k)] = re * re + im * im;
            }
        }
        const float* noise = mid.estimator.update (mid.power.data());
        mid.suppressor.compute (mid.power.data(), noise, params, mid.gain.data());
        if (channels > 1)
            for (auto& c : ch)
            {
                const float* own = c.estimator.update (c.power.data());
                c.suppressor.compute (c.power.data(), own, params, c.gain.data());
            }

        // Telemetría: piso de ruido (media cuadrática por Parseval con la ventana √Hann), reducción y bandas.
        double lambdaSum = 0.0, inSum = 0.0, outSum = 0.0;
        for (int k = 0; k < bins; ++k)
        {
            lambdaSum += noise[k];
            inSum += mid.power[static_cast<size_t> (k)];
            outSum += double (mid.power[static_cast<size_t> (k)]) * mid.gain[static_cast<size_t> (k)] * mid.gain[static_cast<size_t> (k)];
        }
        const double normal = 4.0 / (double (N) * N);
        double bandSum = 0.0;
        for (int k = bandLo; k < bandHi; ++k)
            bandSum += noise[k];
        bandFloorDb = mid.estimator.frames() > static_cast<int> (0.5 * fs / H) ? static_cast<float> (10.0 * std::log10 (std::max (bandSum * normal, 1.0e-12))) : -120.0f;
        tel.noiseFloorDb.store (static_cast<float> (10.0 * std::log10 (std::max (lambdaSum * normal, 1.0e-12))), std::memory_order_relaxed);
        const float reduction = gateMode ? -gateGainDb : static_cast<float> (10.0 * std::log10 (std::max (inSum, 1.0e-20) / std::max (outSum, 1.0e-20)));
        tel.reductionDb.store (reduction, std::memory_order_relaxed);
        tel.working.store (reduction > 0.5f, std::memory_order_relaxed);
        for (int b = 0; b < Telemetry::bands; ++b)
        {
            double pin = 0.0, pout = 0.0, pn = 0.0;
            for (int k = edges[static_cast<size_t> (b)]; k < edges[static_cast<size_t> (b + 1)]; ++k)
            {
                pin += mid.power[static_cast<size_t> (k)];
                pout += double (mid.power[static_cast<size_t> (k)]) * mid.gain[static_cast<size_t> (k)] * mid.gain[static_cast<size_t> (k)];
                pn += noise[k];
            }
            const double width = std::max (1, edges[static_cast<size_t> (b + 1)] - edges[static_cast<size_t> (b)]);
            const auto dbOf = [&] (double sum) { return static_cast<float> (10.0 * std::log10 (std::max (sum * normal * width / width, 1.0e-12))); };
            const auto i = static_cast<size_t> (b);
            const float vIn = dbOf (pin), vOut = dbOf (pout), vNoise = dbOf (pn);
            bandDb[i] += (vIn > bandDb[i] ? 0.6f : 0.15f) * (vIn - bandDb[i]);
            tel.input[i].store (bandDb[i], std::memory_order_relaxed);
            tel.output[i].store (bandDb[i] + (gateMode ? gateGainDb : vOut - vIn), std::memory_order_relaxed);
            tel.noise[i].store (vNoise, std::memory_order_relaxed);
        }

        if (! synthesise)
            return;

        // Ganancia final por canal (enlace) → STFT inversa → solape-suma.
        for (int c = 0; c < channels; ++c)
        {
            auto& s = ch[static_cast<size_t> (c)];
            for (int k = 0; k < bins; ++k)
            {
                const float g = channels == 1 ? mid.gain[static_cast<size_t> (k)] : link * mid.gain[static_cast<size_t> (k)] + (1.0f - link) * s.gain[static_cast<size_t> (k)];
                s.spec[static_cast<size_t> (2 * k)] *= g;
                s.spec[static_cast<size_t> (2 * k + 1)] *= g;
            }
            fft.performRealOnlyInverseTransform (s.spec.data());
            for (int i = 0; i < N; ++i)
                s.acc[static_cast<size_t> ((writePos + i) % N)] += s.spec[static_cast<size_t> (i)] * window[static_cast<size_t> (i)] * outScale;
            for (int i = 0; i < H; ++i) // las H muestras más antiguas ya están completas
            {
                const auto idx = static_cast<size_t> ((writePos + i) % N);
                s.final[static_cast<size_t> (i)] = s.acc[idx];
                s.acc[idx] = 0.0f;
            }
        }
        outPos = 0;
    }

    struct Channel
    {
        std::vector<float> ring, acc, final, spec, power, gain;
        NoiseEstimator estimator;
        SuppressionGain suppressor;
    };
    struct Mid
    {
        std::vector<float> power, gain;
        NoiseEstimator estimator;
        SuppressionGain suppressor;
    };

    int N, H, bins, channels;
    double fs;
    juce::dsp::FFT fft;
    std::vector<float> window;
    std::array<Channel, 2> ch;
    Mid mid;
    std::array<int, Telemetry::bands + 1> edges {};
    std::array<float, Telemetry::bands> bandDb {};
    float outScale = 1.0f;
    int writePos = 0, sinceHop = 0, outPos = 0;
    int bandLo = 1, bandHi = 2;
    float bandFloorDb = -120.0f;
    bool gateMode = false;
    float gateGainDb = 0.0f;
};

// ───────────────────────────────── Motor ─────────────────────────────────

float MaruEngine::gateStep (GateLane& lane, float x, const Settings& s, float thresholdDb, float floorGain) noexcept
{
    // Detector: banda del sidechain y envolvente RMS de 3 ms.
    const float band = lane.lp.process (lane.hp.process (x));
    const double c = 1.0 - std::exp (-1.0 / (0.003 * fs));
    lane.envelope += c * (static_cast<double> (band) * band - lane.envelope);
    lane.levelDb = static_cast<float> (10.0 * std::log10 (lane.envelope + 1.0e-12));

    // Abre sobre el umbral; cierra cuando el nivel lleva `hold` por debajo de umbral − histéresis.
    const int holdSamples = static_cast<int> (std::lround (s.holdMs * 0.001 * fs));
    if (! lane.open)
    {
        if (lane.levelDb > thresholdDb)
        {
            lane.open = true;
            lane.holdLeft = holdSamples;
        }
    }
    else if (lane.levelDb >= thresholdDb - s.hystDb)
        lane.holdLeft = holdSamples;
    else if (--lane.holdLeft < 0)
        lane.open = false;

    // Apertura con rampa lineal en el tiempo y ganancia de coseno elevado (sin clics).
    const float attackStep = 1.0f / std::max (1.0f, static_cast<float> (s.attackMs * 0.001 * fs));
    const float releaseStep = 1.0f / std::max (1.0f, static_cast<float> (s.recoveryMs * 0.001 * fs));
    lane.openness = lane.open ? std::min (1.0f, lane.openness + attackStep) : std::max (0.0f, lane.openness - releaseStep);
    lane.gain = floorGain + (1.0f - floorGain) * 0.5f * (1.0f - std::cos (std::numbers::pi_v<float> * lane.openness));
    return lane.gain;
}

MaruEngine::MaruEngine() = default;
MaruEngine::~MaruEngine() = default;

int MaruEngine::fftSizeFor (double sampleRate, bool lowLatency) noexcept
{
    const int factor = sampleRate >= 150000.0 ? 4 : (sampleRate >= 75000.0 ? 2 : 1);
    return (lowLatency ? 512 : 1024) * factor;
}

int MaruEngine::latencyFor (const Settings& s, double sampleRate) noexcept
{
    if (s.gate)
        return static_cast<int> (std::lround (std::clamp (s.lookaheadMs, 0.0f, 10.0f) * 0.001 * sampleRate));
    return fftSizeFor (sampleRate, s.lowLatency);
}

void MaruEngine::prepare (double sampleRate, int)
{
    fs = sampleRate;
    stft[0] = std::make_unique<Stft> (fftSizeFor (fs, false), fs, 2);
    stft[1] = std::make_unique<Stft> (fftSizeFor (fs, true), fs, 2);
    for (auto& d : dryDelay)
        d.prepare (fftSizeFor (fs, false) + 8);
    bypassCoeff = coeff (bypassSeconds, fs);
    listenCoeff = coeff (listenSeconds, fs);
    outputCoeff = coeff (outputSeconds, fs);
    modeSwitch.prepare (fs, structureFadeSeconds, fftSizeFor (fs, false) + 256);
    reset();
}

void MaruEngine::reset()
{
    for (auto& s : stft)
        if (s != nullptr)
            s->reset();
    for (auto& d : dryDelay)
        d.reset();
    needsInit = true;
    activeKey = -1;
    bypassMix = listenMix = 0.0;
    outputGain = 1.0;
}

int MaruEngine::structureLatency (int key) const noexcept
{
    Settings s;
    s.gate = (key >> 1) != 0;
    s.lowLatency = (key & 1) != 0;
    s.lookaheadMs = 1000.0f * static_cast<float> (lookaheadSamples) / static_cast<float> (fs);
    return s.gate ? lookaheadSamples : fftSizeFor (fs, s.lowLatency);
}

void MaruEngine::applyStructure (int key)
{
    activeKey = key;
    const int latency = structureLatency (key);
    for (auto& d : dryDelay)
    {
        d.reset();
        d.setDelay (latency);
    }
    stft[static_cast<size_t> (key & 1)]->reset();
    for (auto& g : gateLanes)
    {
        g.hp.reset();
        g.lp.reset();
        g.envelope = 0.0;
        g.open = false;
        g.holdLeft = 0;
        g.openness = 0.0f;
        g.gain = 1.0f;
    }
    lastHp = lastLp = -1.0f;
}

void MaruEngine::process (juce::AudioBuffer<float>& buffer, const juce::AudioBuffer<float>* sidechain, const Settings& request) noexcept
{
        const int channels = std::min (buffer.getNumChannels(), static_cast<int> (maxChannels));
    const int total = buffer.getNumSamples();
    if (channels == 0 || total == 0 || stft[0] == nullptr)
        return;

    Settings s = request;
    s.amount = std::clamp (s.amount, 0.0f, 1.0f);
    s.rangeDb = std::clamp (s.rangeDb, 6.0f, 60.0f);
    s.sens = std::clamp (s.sens, -10.0f, 10.0f);
    s.recoveryMs = std::clamp (s.recoveryMs, 10.0f, 1000.0f);
    s.outputDb = std::clamp (s.outputDb, -12.0f, 12.0f);
    s.lookaheadMs = std::clamp (s.lookaheadMs, 0.0f, 10.0f);
    const float link = std::clamp (s.linkPercent, 0.0f, 100.0f) * 0.01f;

    // Mapeo del control principal (valores iniciales de la ficha): β = 1 + 2a; reducción máxima por tramos 0, 6, 12, 21 dB y Rango.
    const float a = s.amount;
    const float points[5] = { 0.0f, 6.0f, 12.0f, 21.0f, s.rangeDb };
    const float scaled = a * 4.0f;
    const int seg = std::min (3, static_cast<int> (scaled));
    const float reduction = std::min (s.rangeDb, points[seg] + (points[seg + 1] - points[seg]) * (scaled - static_cast<float> (seg)));
    SuppressionGain::Params params;
    params.overestimate = 1.0f + 2.0f * a;
    params.floorGain = std::pow (10.0f, -reduction / 20.0f);
    params.recoverySeconds = s.recoveryMs * 0.001f;
    params.sensGain = std::pow (10.0f, s.sens / 10.0f);

    lookaheadSamples = static_cast<int> (std::lround (s.lookaheadMs * 0.001 * fs));
    float* data[2] = { buffer.getWritePointer (0), channels > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int pos = 0; pos < total; pos += chunkSize)
    {
        const int n = std::min (chunkSize, total - pos);
        const int key = (s.gate ? 2 : 0) + (s.lowLatency ? 1 : 0);
        if (needsInit)
        {
            modeSwitch.reset (key);
            applyStructure (key);
            needsInit = false;
        }
        const auto step = modeSwitch.update (key, n);
        if (step.switched || step.mode != activeKey)
            applyStructure (step.mode);
        Stft& st = *stft[static_cast<size_t> (step.mode & 1)];
        const bool gate = (step.mode >> 1) != 0;
        if (std::abs (s.scHpHz - lastHp) > 0.01f || std::abs (s.scLpHz - lastLp) > 0.01f)
        {
            lastHp = s.scHpHz;
            lastLp = s.scLpHz;
            const double top = std::min (static_cast<double> (std::max (s.scLpHz, s.scHpHz * 1.2f)), 0.45 * fs);
            for (auto& g : gateLanes)
            {
                g.hp.setCoefficients (ferb::Biquad::highPass (fs, std::clamp (static_cast<double> (s.scHpHz), 10.0, 0.4 * fs)));
                g.lp.setCoefficients (ferb::Biquad::lowPass (fs, top));
            }
            st.setBand (s.scHpHz, static_cast<float> (top));
        }
        st.gateMode = gate;
        // Gate: margen sobre el piso de ruido (6 + 18·Supresión, más la Sensibilidad) y ganancia de cierre.
        const float margin = 6.0f + 18.0f * s.amount + s.sens;
        const float floorGain = std::pow (10.0f, -s.rangeDb / 20.0f);
        const bool hasSidechain = s.scExt && sidechain != nullptr && sidechain->getNumChannels() > 0 && sidechain->getNumSamples() >= total;
        const float* sc[2] = { nullptr, nullptr };
        if (hasSidechain)
        {
            sc[0] = sidechain->getReadPointer (0);
            sc[1] = sidechain->getNumChannels() > 1 ? sidechain->getReadPointer (1) : sc[0];
        }

        const double bypassTarget = s.bypass ? 1.0 : 0.0, listenTarget = s.listen ? 1.0 : 0.0;
        const double outputTarget = std::pow (10.0, static_cast<double> (s.outputDb) / 20.0);
        for (int i = 0; i < n; ++i)
        {
            const int index = pos + i;
            float in[2] = { sane (data[0][index]), data[1] != nullptr ? sane (data[1][index]) : 0.0f };
            float wet[2] = { 0.0f, 0.0f }, dry[2];
            st.channels = channels;
            for (int c = 0; c < channels; ++c)
                dry[c] = dryDelay[static_cast<size_t> (c)].process (in[c]);

            if (! gate)
                st.tick (in, wet, true, params, link, telemetry);
            else
            {
                // Entradas del detector: el sidechain externo, o el propio audio.
                float det[2] = { in[0], in[1] };
                if (hasSidechain)
                {
                    det[0] = sane (sc[0][index]);
                    det[1] = sane (sc[1][index]);
                }
                const float detMid = channels > 1 ? 0.5f * (det[0] + det[1]) : det[0];
                const float thresholdDb = s.gateAuto ? (st.bandFloorDb > -119.0f ? st.bandFloorDb + margin : -200.0f) : s.gateThreshDb + s.sens;
                const float gMid = gateStep (gateLanes[0], detMid, s, thresholdDb, floorGain);
                float g[2] = { gMid, gMid };
                if (channels > 1)
                    for (int c = 0; c < 2; ++c)
                    {
                        const float own = gateStep (gateLanes[static_cast<size_t> (1 + c)], det[c], s, thresholdDb, floorGain);
                        g[c] = link * gMid + (1.0f - link) * own;
                    }
                if (s.amount <= 0.0f) // Supresión 0 %: sin efecto
                    g[0] = g[1] = 1.0f;
                st.gateGainDb = 20.0f * std::log10 (std::max (g[0], 1.0e-6f));
                st.tick (in, wet, false, params, link, telemetry); // solo el análisis (piso de ruido)
                for (int c = 0; c < channels; ++c)
                    wet[c] = dry[c] * g[c];
            }

            bypassMix += bypassCoeff * (bypassTarget - bypassMix);
            listenMix += listenCoeff * (listenTarget - listenMix);
            outputGain += outputCoeff * (outputTarget - outputGain);
            const float fade = step.gainStart + (step.gainEnd - step.gainStart) * static_cast<float> (i + 1) / static_cast<float> (n);
            for (int c = 0; c < channels; ++c)
            {
                const double heard = wet[c] + listenMix * ((dry[c] - wet[c]) - wet[c]); // lo quitado = entrada retrasada − salida
                const double processed = heard * outputGain;
                data[c][index] = static_cast<float> ((processed + bypassMix * (dry[c] - processed)) * fade);
            }
        }
    }
}
} // namespace maru
