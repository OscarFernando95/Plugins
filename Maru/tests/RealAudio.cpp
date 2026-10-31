// Herramienta de estudio con grabaciones reales (no es una prueba):
//   MARU_REAL_FILE=<wav> MARU_REAL_OUT=<prefijo> [MARU_AMOUNT=50 MARU_RANGE=30 MARU_GATE=0 MARU_LOW=0] ./build/Maru/MaruTests "[.real]"
// Lee el WAV (16 o 24 bits; si es estéreo, el canal izquierdo), lo pasa por el motor y escribe:
//   <prefijo>.csv       un renglón por 4096 muestras: tiempo, piso estimado, reducción, nivel de entrada (dBFS eficaces en ese tramo)
//   <prefijo>_out.wav   el audio procesado (alineado con la entrada) y <prefijo>_delta.wav, lo que se quita
// No modifica el archivo de entrada.

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "dsp/MaruEngine.h"

namespace
{
bool readWavMono (const std::string& path, std::vector<float>& out, double& fs)
{
    std::FILE* f = std::fopen (path.c_str(), "rb");
    if (f == nullptr)
        return false;
    std::vector<unsigned char> b;
    unsigned char chunk[1 << 16];
    for (size_t n; (n = std::fread (chunk, 1, sizeof chunk, f)) > 0;)
        b.insert (b.end(), chunk, chunk + n);
    std::fclose (f);
    const auto u32 = [&] (size_t p) { return uint32_t (b[p]) | uint32_t (b[p + 1]) << 8 | uint32_t (b[p + 2]) << 16 | uint32_t (b[p + 3]) << 24; };
    const auto u16 = [&] (size_t p) { return int (b[p] | b[p + 1] << 8); };
    int channels = 1, bits = 16;
    size_t pos = 12;
    while (pos + 8 <= b.size())
    {
        const std::string id (reinterpret_cast<const char*> (&b[pos]), 4);
        const size_t size = u32 (pos + 4);
        if (id == "fmt ")
        {
            channels = u16 (pos + 10);
            fs = u32 (pos + 12);
            bits = u16 (pos + 22);
        }
        else if (id == "data")
        {
            const size_t bytes = bits / 8, frames = std::min (size, b.size() - pos - 8) / (bytes * size_t (channels));
            out.resize (frames);
            for (size_t i = 0; i < frames; ++i)
            {
                const size_t o = pos + 8 + i * bytes * size_t (channels);
                if (bits == 16)
                    out[i] = float (int16_t (b[o] | b[o + 1] << 8)) / 32768.0f;
                else
                {
                    int32_t v = b[o] << 8 | b[o + 1] << 16 | b[o + 2] << 24;
                    out[i] = float (v / 256) / 8388608.0f;
                }
            }
            return bits == 16 || bits == 24;
        }
        pos += 8 + size + (size & 1);
    }
    return false;
}

void writeWavFloat (const std::string& path, const std::vector<float>& x, double fs)
{
    std::FILE* f = std::fopen (path.c_str(), "wb");
    if (f == nullptr)
        return;
    const uint32_t dataBytes = uint32_t (x.size() * 4), rate = uint32_t (fs);
    const auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    const auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + dataBytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (3); w16 (1); w32 (rate); w32 (rate * 4); w16 (4); w16 (32);
    std::fwrite ("data", 1, 4, f); w32 (dataBytes);
    std::fwrite (x.data(), 4, x.size(), f);
    std::fclose (f);
}

float envOr (const char* name, float fallback)
{
    const char* v = std::getenv (name);
    return v != nullptr ? float (std::atof (v)) : fallback;
}
} // namespace

TEST_CASE ("Audio real de Maru: volcado y procesado de un archivo", "[.real]")
{
    const char* file = std::getenv ("MARU_REAL_FILE");
    const char* prefix = std::getenv ("MARU_REAL_OUT");
    if (file == nullptr || prefix == nullptr)
        SKIP ("define MARU_REAL_FILE y MARU_REAL_OUT");

    std::vector<float> x;
    double fs = 48000.0;
    REQUIRE (readWavMono (file, x, fs));

    maru::Settings s;
    s.amount = envOr ("MARU_AMOUNT", 50.0f) * 0.01f;
    s.rangeDb = envOr ("MARU_RANGE", 30.0f);
    s.gate = envOr ("MARU_GATE", 0.0f) > 0.5f;
    s.lowLatency = envOr ("MARU_LOW", 0.0f) > 0.5f;
    s.recoveryMs = envOr ("MARU_RECOVERY", 120.0f);
    s.sens = envOr ("MARU_SENS", 0.0f);
    s.gateAuto = true;

    const int latency = maru::MaruEngine::latencyFor (s, fs);
    const size_t total = x.size();
    x.resize (total + size_t (latency) + 4096, 0.0f);

    maru::MaruEngine engine;
    engine.prepare (fs, 4096);
    std::FILE* csv = std::fopen ((std::string (prefix) + ".csv").c_str(), "w");
    REQUIRE (csv != nullptr);
    std::fprintf (csv, "t,floor,reduction,in_rms\n");

    constexpr int chunk = 4096;
    std::vector<float> out (x.size());
    juce::AudioBuffer<float> buf (1, chunk);
    for (size_t pos = 0; pos < x.size(); pos += chunk)
    {
        const int n = int (std::min<size_t> (chunk, x.size() - pos));
        buf.setSize (1, n, false, false, true);
        std::copy_n (x.begin() + long (pos), n, buf.getWritePointer (0));
        double sum = 0.0;
        for (int i = 0; i < n; ++i)
            sum += double (x[pos + size_t (i)]) * x[pos + size_t (i)];
        engine.process (buf, nullptr, s);
        std::copy_n (buf.getReadPointer (0), n, out.begin() + long (pos));
        const auto& t = engine.getTelemetry();
        std::fprintf (csv, "%.3f,%.2f,%.2f,%.2f\n", double (pos) / fs, double (t.noiseFloorDb.load()), double (t.reductionDb.load()), 10.0 * std::log10 (sum / n + 1.0e-12));
    }
    std::fclose (csv);

    std::vector<float> aligned (out.begin() + latency, out.begin() + latency + long (total)), delta (total);
    for (size_t i = 0; i < total; ++i)
        delta[i] = x[i] - aligned[i];
    writeWavFloat (std::string (prefix) + "_out.wav", aligned, fs);
    writeWavFloat (std::string (prefix) + "_delta.wav", delta, fs);
    SUCCEED();
}
