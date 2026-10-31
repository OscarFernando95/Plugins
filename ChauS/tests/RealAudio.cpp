// Herramienta de estudio con grabaciones reales (no es una prueba):
//   CHAUS_REAL_FILE=<wav> CHAUS_REAL_OUT=<prefijo> [CHAUS_THR=-36 CHAUS_RANGE=10 CHAUS_RAMP=15 CHAUS_HP=.. CHAUS_LP=..] ./build/ChauS/ChauSTests "[.real]"
// Lee el WAV (16 o 24 bits; si es estéreo, el canal izquierdo), pasa el audio por el motor y escribe:
//   <prefijo>.csv       un renglón por salto de 128 muestras: tiempo, nivel de banda, umbral, reducción, frecuencia, puntuación, activo
//   <prefijo>_feat.csv  los rasgos de cada cuadro del detector
//   <prefijo>_out.wav   el audio procesado (alineado con la entrada) y <prefijo>_delta.wav, lo que se quita
// No modifica el archivo de entrada.

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "dsp/ChausEngine.h"
#include "dsp/SibilanceDetector.h"

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

TEST_CASE ("Audio real: volcado y procesado de un archivo", "[.real]")
{
    const char* file = std::getenv ("CHAUS_REAL_FILE");
    const char* prefix = std::getenv ("CHAUS_REAL_OUT");
    if (file == nullptr || prefix == nullptr)
        SKIP ("define CHAUS_REAL_FILE y CHAUS_REAL_OUT");

    std::vector<float> x;
    double fs = 48000.0;
    REQUIRE (readWavMono (file, x, fs));

    chaus::Settings s;
    s.thresholdDb = envOr ("CHAUS_THR", -36.0f);
    s.rangeDb = envOr ("CHAUS_RANGE", 10.0f);
    s.hpHz = envOr ("CHAUS_HP", 4000.0f);
    s.lpHz = envOr ("CHAUS_LP", 12000.0f);
    s.lookaheadMs = envOr ("CHAUS_LOOK", 10.0f);
    s.eventRampMs = envOr ("CHAUS_RAMP", 8.0f);
    s.voiceMode = envOr ("CHAUS_GENERAL", 0.0f) < 0.5f;

    const int latency = chaus::ChausEngine::latencyFor (s, fs);
    const size_t total = x.size();
    x.resize (total + size_t (latency), 0.0f); // se vacía el retardo

    chaus::ChausEngine engine;
    engine.prepare (fs, 4096);
    chaus::SibilanceDetector detector;
    detector.prepare (fs);
    detector.setBand (s.hpHz, s.lpHz);

    std::FILE* csv = std::fopen ((std::string (prefix) + ".csv").c_str(), "w");
    std::FILE* feat = std::fopen ((std::string (prefix) + "_feat.csv").c_str(), "w");
    REQUIRE (csv != nullptr);
    REQUIRE (feat != nullptr);
    std::fprintf (csv, "t,level,thr,red,centre,score,in_rms\n");
    std::fprintf (feat, "t,level,ratio,flat,centroid,slope,air,spread\n");

    constexpr int chunk = 4096;
    std::vector<float> out (x.size()), red (chunk), centre (chunk), level (chunk), thr (chunk), score (chunk);
    juce::AudioBuffer<float> buf (1, chunk);
    long seen = 0;
    for (size_t pos = 0; pos < x.size(); pos += chunk)
    {
        const int n = int (std::min<size_t> (chunk, x.size() - pos));
        buf.setSize (1, n, false, false, true);
        std::copy_n (x.begin() + long (pos), n, buf.getWritePointer (0));
        chaus::Probe probe;
        probe.reductionDb[0] = red.data(); probe.centreHz[0] = centre.data(); probe.levelDb[0] = level.data();
        probe.thresholdDb[0] = thr.data(); probe.score[0] = score.data();
        engine.process (buf, nullptr, s, &probe);
        std::copy_n (buf.getReadPointer (0), n, out.begin() + long (pos));

        for (int i = 0; i < n; ++i)
        {
            const auto o = detector.tick (x[pos + size_t (i)]);
            if (detector.frameCount() != seen)
            {
                seen = detector.frameCount();
                const auto& f = detector.features();
                std::fprintf (feat, "%.4f,%.2f,%.2f,%.3f,%.0f,%.2f,%.2f,%.3f\n", double (pos + size_t (i)) / fs, f.levelDb, f.ratioDb, f.flatness, f.centroidHz, f.slopeDbOct, f.airDb, f.spreadOct);
            }
            if (((pos + size_t (i)) % 128) == 0)
                std::fprintf (csv, "%.4f,%.2f,%.2f,%.2f,%.0f,%.3f,%d\n", double (pos + size_t (i)) / fs, level[size_t (i)], thr[size_t (i)], red[size_t (i)], centre[size_t (i)], score[size_t (i)], o.active ? 1 : 0);
        }
    }
    std::fclose (csv);
    std::fclose (feat);

    // Alineado con la entrada: se quita la latencia.
    std::vector<float> aligned (out.begin() + latency, out.end()), delta (total);
    for (size_t i = 0; i < total; ++i)
        delta[i] = x[i] - aligned[i];
    writeWavFloat (std::string (prefix) + "_out.wav", aligned, fs);
    writeWavFloat (std::string (prefix) + "_delta.wav", delta, fs);
    SUCCEED();
}
