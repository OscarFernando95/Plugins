#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>

#include "AllocGuard.h"
#include "Signals.h"
#include "TestUtils.h"
#include "dsp/SibilanceDetector.h"

using Catch::Approx;
using namespace chaus;

namespace
{
struct Trace
{
    std::vector<float> level, score;
    std::vector<char> active;
};

Trace run (const std::vector<float>& x, double fs, float hp = 4000.0f, float lp = 12000.0f, bool classifier = true, const Classifier& model = Classifier::initial())
{
    SibilanceDetector d;
    d.prepare (fs);
    d.setClassifier (model);
    d.setBand (hp, lp);
    d.setClassifierEnabled (classifier);
    Trace t;
    t.level.reserve (x.size());
    for (float v : x)
    {
        const auto o = d.tick (v);
        t.level.push_back (o.levelDb);
        t.score.push_back (o.score);
        t.active.push_back (o.active);
    }
    return t;
}

int ms (double fs, double milliseconds) { return int (milliseconds * 0.001 * fs); }

/** Frase sintética: vocales continuas con eses de 6 a 9 kHz entre ellas (modeladas como ruido de banda con envolvente). */
struct Phrase
{
    std::vector<float> audio;
    struct Event { int start, end; double levelDb; };
    std::vector<Event> events;
};

Phrase makePhrase (double fs, int count, unsigned seed, double sibLevelDb = -22.0, double lo = 6000.0, double hi = 9000.0, double vowelDb = -24.0)
{
    sig::Rng rng (seed);
    Phrase p;
    p.audio = sig::vowels (fs, int ((0.5 + 0.55 * count) * fs), rng.uni (100.0f, 220.0f), vowelDb, rng);
    int pos = ms (fs, 250);
    for (int i = 0; i < count; ++i)
    {
        const double dur = rng.uni (0.09f, 0.2f), level = sibLevelDb + rng.uni (-5.0f, 4.0f);
        const auto s = sig::sibilant (fs, lo, hi, dur, level, rng);
        sig::mixAt (p.audio, s, pos);
        p.events.push_back ({ pos, pos + int (s.size()), level });
        pos += int (s.size()) + ms (fs, rng.uni (260.0f, 420.0f));
    }
    return p;
}

/** ¿Estaba activo el detector en el pico de la sibilante (el centro de su meseta)? */
bool activeAtPeak (const Trace& t, const Phrase::Event& e) { return t.active[size_t ((e.start + e.end) / 2)] != 0; }
} // namespace

// ───────────────────────────── Rasgos ─────────────────────────────

TEST_CASE ("Rasgos: planitud, centroide y pendiente sobre espectros analíticos")
{
    const int N = 512;
    const double fs = 48000.0, df = fs / N;
    std::vector<double> p (N / 2 + 1, 1.0);

    // Espectro plano: planitud 1 y centroide en el medio de la banda.
    auto f = extractFeatures (p.data(), N, fs, 4000.0f, 12000.0f);
    REQUIRE (f.flatness == Approx (1.0f).margin (0.01));
    REQUIRE (f.centroidHz == Approx (8000.0f).margin (150.0f));

    // Un tono: planitud casi cero.
    std::vector<double> tone (N / 2 + 1, 1e-12);
    tone[size_t (7000.0 / df)] = 1.0;
    REQUIRE (extractFeatures (tone.data(), N, fs, 4000.0f, 12000.0f).flatness < 0.05f);

    // Potencia proporcional a f^m: la pendiente de 2 a 4 kHz vale 3·m dB por octava.
    for (double m : { -2.0, 0.0, 2.0, 4.0 })
    {
        for (size_t k = 1; k < p.size(); ++k)
            p[k] = std::pow (double (k) * df / 1000.0, m);
        const auto g = extractFeatures (p.data(), N, fs, 4000.0f, 12000.0f);
        INFO ("m = " << m);
        REQUIRE (g.slopeDbOct == Approx (3.0 * m).margin (0.4));
    }

    // Aire: con densidad espectral plana vale 0 dB; con potencia ∝ f^-4 (−12 dB por octava) cae.
    std::fill (p.begin(), p.end(), 1.0);
    REQUIRE (extractFeatures (p.data(), N, fs, 4000.0f, 12000.0f).airDb == Approx (0.0f).margin (0.2));
    for (size_t k = 1; k < p.size(); ++k)
        p[k] = std::pow (double (k) * df / 1000.0, -4.0);
    const float airSlope = extractFeatures (p.data(), N, fs, 4000.0f, 12000.0f).airDb;
    REQUIRE (airSlope < -10.0f);
    REQUIRE (airSlope > -20.0f);

    // Razón banda / 300 Hz a 3 kHz: espectro plano → razón por anchura de banda (8 kHz frente a 2,7 kHz).
    std::fill (p.begin(), p.end(), 1.0);
    REQUIRE (extractFeatures (p.data(), N, fs, 4000.0f, 12000.0f).ratioDb == Approx (10.0 * std::log10 (8000.0 / 2700.0)).margin (0.4));
}

TEST_CASE ("Rasgos: el nivel de la banda de una senoidal coincide con su valor eficaz")
{
    // Una senoidal de 7 kHz y amplitud 0,1 tiene −23,01 dBFS eficaces.
    const double fs = 48000.0;
    const auto x = testutil::sine (4800, 7000.0, fs, 0.1);
    SibilanceDetector d;
    d.prepare (fs);
    for (float v : x)
        d.tick (v);
    REQUIRE (d.features().levelDb == Approx (-23.01f).margin (0.5f));
    // El del dominio del tiempo (filtrado HP+LP) es parecido: la senoidal cae en la meseta de la banda.
    const auto t = run (x, fs);
    REQUIRE (t.level.back() == Approx (-23.01f).margin (1.0f));
}

// ───────────────────────────── Histéresis y suavizado ─────────────────────────────

namespace
{
Classifier constant (float probability)
{
    Classifier c;
    c.weights.fill (0.0f);
    c.bias = std::log (probability / (1.0f - probability));
    return c;
}
} // namespace

TEST_CASE ("Histéresis: enciende en 0,6, apaga en 0,4 y entre medias conserva el estado")
{
    const double fs = 48000.0;
    SibilanceDetector d;
    d.prepare (fs);
    const auto noise = testutil::noise (int (fs * 0.4), 0.2f);
    const auto state = [&] (float prob)
    {
        d.setClassifier (constant (prob));
        bool on = false;
        for (int rep = 0; rep < 2; ++rep) // 0,8 s: la puntuación se asienta
            for (float v : noise)
                on = d.tick (v).active;
        return on;
    };
    REQUIRE_FALSE (state (0.5f)); // desde apagado, 0,5 no enciende
    REQUIRE (state (0.7f));
    REQUIRE (state (0.5f));       // entre 0,4 y 0,6 se queda encendido
    REQUIRE_FALSE (state (0.3f));
    REQUIRE_FALSE (state (0.55f)); // y desde apagado tampoco enciende
    REQUIRE (state (0.65f));
}

TEST_CASE ("Suavizado: la puntuación alcanza el 63 % de un escalón en unos 5 ms")
{
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        SibilanceDetector d;
        d.prepare (fs);
        d.setClassifier (constant (0.0001f));
        const auto noise = testutil::noise (int (fs * 0.5), 0.2f);
        size_t i = 0;
        for (; i < size_t (fs * 0.1); ++i)
            d.tick (noise[i]);
        d.setClassifier (constant (0.9999f));
        const size_t step = i;
        while (d.tick (noise[i++ % noise.size()]).score < 0.63f) {}
        const double timeMs = double (i - step) / fs * 1000.0;
        INFO ("fs " << fs);
        REQUIRE (timeMs > 3.0);
        REQUIRE (timeMs < 9.0); // 5 ms más hasta el siguiente salto del cuadro
    }
}

// ───────────────────────────── Umbral adaptativo ─────────────────────────────

TEST_CASE ("Umbral adaptativo: percentil 90 de los picos de los últimos 5 s")
{
    const double fs = 48000.0;
    AdaptiveReference ref;
    ref.prepare (fs);

    const auto event = [&] (float peakDb)
    {
        for (int i = 0; i < ms (fs, 50); ++i) ref.tick (true, peakDb);
        for (int i = 0; i < ms (fs, 100); ++i) ref.tick (false, -90.0f);
    };

    REQUIRE (ref.targetDb() == Approx (AdaptiveReference::initialReferenceDb)); // sin eventos: valor inicial
    for (int i = 1; i <= 10; ++i)
        event (-40.0f + float (i)); // picos de −39 a −30 dB
    // Rango del percentil 90 de 10 valores (rango más cercano): el noveno, −31 dB.
    REQUIRE (ref.targetDb() == Approx (-31.0f).margin (1e-4));
    REQUIRE (ref.eventCount() == 10);

    // Un cruce de menos de 10 ms no cuenta.
    for (int i = 0; i < ms (fs, 5); ++i) ref.tick (true, 0.0f);
    for (int i = 0; i < ms (fs, 100); ++i) ref.tick (false, -90.0f);
    REQUIRE (ref.eventCount() == 10);

    // El suavizado sigue al objetivo.
    for (int i = 0; i < int (fs * 2.0); ++i) ref.tick (false, -90.0f);
    REQUIRE (ref.referenceDb() == Approx (-31.0f).margin (0.2f));

    // Pasados 5 s sin eventos se conserva el último valor (no vuelve al inicial). Los eventos salen de la ventana
    // de uno en uno, así que el último valor es el pico del último evento (−30 dB).
    for (int i = 0; i < int (fs * 8.0); ++i) ref.tick (false, -90.0f);
    REQUIRE (ref.eventCount() == 0);
    REQUIRE (ref.targetDb() == Approx (-30.0f).margin (1e-4));
    for (int i = 0; i < int (fs * 8.0); ++i) ref.tick (false, -90.0f);
    REQUIRE (ref.targetDb() == Approx (-30.0f).margin (1e-4));

    // Los eventos viejos salen de la ventana: tres eventos tenues nuevos desplazan al percentil.
    for (int i = 0; i < 3; ++i) event (-50.0f);
    REQUIRE (ref.targetDb() == Approx (-50.0f).margin (1e-4));
}

TEST_CASE ("Umbral adaptativo: la misma frase 6 dB más baja da una referencia 6 dB más baja")
{
    for (double fs : { 44100.0, 48000.0 })
    {
        float refAt[2] = {};
        int index = 0;
        for (double gainDb : { -6.0, -12.0 })
        {
            auto phrase = makePhrase (fs, 14, 777, -22.0);
            const float g = float (testutil::fromDb (gainDb));
            for (auto& v : phrase.audio)
                v *= g;
            SibilanceDetector d;
            d.prepare (fs);
            AdaptiveReference ref;
            ref.prepare (fs);
            for (float v : phrase.audio)
            {
                const auto o = d.tick (v);
                ref.tick (o.active, o.levelDb);
            }
            refAt[index++] = ref.referenceDb();
        }
        INFO ("fs " << fs << ", referencias " << refAt[0] << " y " << refAt[1]);
        REQUIRE (refAt[0] - refAt[1] == Approx (6.0f).margin (1.5f));
    }
}

// ───────────────────────────── Detección ─────────────────────────────

namespace
{
struct Tally
{
    int hits = 0, triggered = 0;
    long samples = 0, active = 0;
    double rate() const { return hits > 0 ? double (triggered) / hits : 0.0; }
    double fraction() const { return samples > 0 ? double (active) / double (samples) : 0.0; }
};

struct Rates
{
    int events = 0, detected = 0;
    Tally vowels, cymbal, hatMetal, hatNoisy, tee;
};

/** Mide con el clasificador dado: sibilantes de 6 a 9 kHz (ficha), y falsos positivos en vocales, platillo, hi-hat y «t».
    Todo a 44,1, 48 y 96 kHz, con semillas distintas de las del ajuste de los pesos. */
Rates measure (const Classifier& model)
{
    Rates r;
    for (double fs : { 44100.0, 48000.0, 96000.0 })
    {
        for (unsigned seed = 1; seed <= 6; ++seed)
        {
            const auto phrase = makePhrase (fs, 12, 5000 + seed);
            const auto t = run (phrase.audio, fs, 4000.0f, 12000.0f, true, model);
            for (const auto& e : phrase.events)
            {
                ++r.events;
                r.detected += activeAtPeak (t, e) ? 1 : 0;
            }
        }

        for (unsigned seed = 1; seed <= 10; ++seed)
        {
            sig::Rng rng (9000 + seed);
            const double level = rng.uni (-26.0f, -14.0f);
            const auto account = [&] (Tally& tally, const std::vector<float>& x, int from, int to)
            {
                const auto t = run (x, fs, 4000.0f, 12000.0f, true, model);
                bool any = false;
                for (int i = from; i < std::min<int> (to, int (x.size())); ++i)
                {
                    any |= t.active[size_t (i)] != 0;
                    tally.active += t.active[size_t (i)];
                    ++tally.samples;
                }
                ++tally.hits;
                tally.triggered += any ? 1 : 0;
            };
            const auto bed = [&] (double seconds) { return sig::vowels (fs, int (seconds * fs), rng.uni (100.0f, 220.0f), -26.0, rng); };

            { auto x = bed (3.0); account (r.vowels, x, ms (fs, 100), int (x.size())); }
            { // platillo: 1,2 s sobre la voz
                auto x = bed (1.6);
                sig::mixAt (x, sig::cymbal (fs, 1.2, level, rng, rng.uni (0.5f, 1.3f)), ms (fs, 100));
                account (r.cymbal, x, ms (fs, 100), ms (fs, 1300));
            }
            { // hi-hat metálico: parciales de un 808, sin ruido blanco
                auto x = bed (0.8);
                sig::mixAt (x, sig::hihat (fs, 0.25, level, rng, rng.uni (0.05f, 0.14f), rng.uni (5500.0f, 8000.0f), 0.0), ms (fs, 200));
                account (r.hatMetal, x, ms (fs, 200), ms (fs, 450));
            }
            { // hi-hat con mucho ruido blanco filtrado: espectralmente es una «s» aguda
                auto x = bed (0.8);
                sig::mixAt (x, sig::hihat (fs, 0.25, level, rng, rng.uni (0.05f, 0.14f), rng.uni (5500.0f, 8000.0f), 1.5), ms (fs, 200));
                account (r.hatNoisy, x, ms (fs, 200), ms (fs, 450));
            }
            { // «t»
                auto x = bed (0.8);
                const auto burst = sig::tBurst (fs, level - 4.0, rng, rng.uni (4200.0f, 5400.0f), rng.uni (0.010f, 0.025f));
                sig::mixAt (x, burst, ms (fs, 300));
                account (r.tee, x, ms (fs, 300), ms (fs, 300) + int (burst.size()) + ms (fs, 20));
            }
        }
    }
    return r;
}
} // namespace

TEST_CASE ("Detección: sibilantes sintéticas de 6 a 9 kHz se detectan en el 95 % o más de los eventos")
{
    const auto r = measure (Classifier::initial());
    INFO (r.detected << " de " << r.events);
    REQUIRE (r.detected >= int (std::ceil (0.95 * r.events)));
}

// El hi-hat y la «t» NO están aquí: con estos rasgos espectrales no cumplen la meta de la ficha (menos del 5 %) sin perder
// detección; sus cifras salen en el informe y en «Estado y cambios sobre la ficha».
TEST_CASE ("Falsos positivos: voz sin eses y platillo activan el detector en menos del 5 %")
{
    const auto r = measure (Classifier::initial());
    INFO ("fracción de tiempo activo: vocales " << r.vowels.fraction() << ", platillo " << r.cymbal.fraction());
    INFO ("golpes de platillo que activan: " << r.cymbal.rate());
    REQUIRE (r.vowels.fraction() < 0.01);
    REQUIRE (r.cymbal.fraction() < 0.05);
    REQUIRE (r.cymbal.rate() < 0.05);
}

TEST_CASE ("Informe: detección y falsos positivos según el sesgo del clasificador", "[.report]")
{
    for (float offset : { -0.5f, -0.3f, 0.0f, 0.3f })
    {
        auto model = Classifier::initial();
        model.bias += offset;
        const auto r = measure (model);
        std::printf ("sesgo %+.1f: detección %3d/%3d (%.1f %%) | tiempo activo: vocales %.1f %%, platillo %.1f %%, hi-hat metálico %.1f %%, hi-hat con ruido %.1f %%, t %.1f %% | golpes: platillo %.0f %%, metálico %.0f %%, con ruido %.0f %%, t %.0f %%\n",
                     offset, r.detected, r.events, 100.0 * r.detected / r.events, 100 * r.vowels.fraction(), 100 * r.cymbal.fraction(),
                     100 * r.hatMetal.fraction(), 100 * r.hatNoisy.fraction(), 100 * r.tee.fraction(),
                     100 * r.cymbal.rate(), 100 * r.hatMetal.rate(), 100 * r.hatNoisy.rate(), 100 * r.tee.rate());
    }
}

TEST_CASE ("Modo General: sin clasificador siempre está activo y no calcula la FFT")
{
    const double fs = 48000.0;
    sig::Rng rng (3);
    const auto x = sig::vowels (fs, 4800, 140.0, -20.0, rng);
    SibilanceDetector d;
    d.prepare (fs);
    d.setClassifierEnabled (false);
    for (float v : x)
    {
        const auto o = d.tick (v);
        REQUIRE (o.active);
        REQUIRE (o.score == 1.0f);
    }
    REQUIRE (d.frameCount() == 0);
}

// ───────────────────────────── Robustez ─────────────────────────────

TEST_CASE ("Robustez: silencio, continua, NaN, infinitos y subnormales")
{
    for (double fs : { 44100.0, 48000.0, 96000.0, 192000.0 })
    {
        const auto check = [&] (const std::vector<float>& x, bool mustStayQuiet)
        {
            const auto t = run (x, fs);
            for (size_t i = 0; i < x.size(); ++i)
            {
                REQUIRE (std::isfinite (t.level[i]));
                REQUIRE (t.score[i] >= 0.0f);
                REQUIRE (t.score[i] <= 1.0f);
            }
            if (mustStayQuiet)
                REQUIRE (std::count (t.active.begin(), t.active.end(), 1) == 0);
        };
        const int n = int (fs * 0.5);
        check (std::vector<float> (size_t (n), 0.0f), true);
        check (std::vector<float> (size_t (n), 0.5f), true); // continua
        check (std::vector<float> (size_t (n), 1.0e-40f), true); // subnormales
        std::vector<float> bad (size_t (n), 0.0f);
        for (size_t i = 0; i < bad.size(); i += 37)
            bad[i] = std::numeric_limits<float>::quiet_NaN();
        for (size_t i = 5; i < bad.size(); i += 101)
            bad[i] = std::numeric_limits<float>::infinity();
        check (bad, false);
    }
}

TEST_CASE ("tick no reserva memoria")
{
    const double fs = 48000.0;
    SibilanceDetector d;
    d.prepare (fs);
    AdaptiveReference ref;
    ref.prepare (fs);
    const auto phrase = makePhrase (fs, 6, 42);
    long total = 0;
    {
        allocguard::Scope scope;
        for (float v : phrase.audio)
        {
            const auto o = d.tick (v);
            ref.tick (o.active, o.levelDb);
        }
        total = scope.allocations();
    }
    REQUIRE (total == 0);
}

TEST_CASE ("El contador de reservas detecta una reserva deliberada")
{
    static int* volatile sink;
    allocguard::Scope scope;
    sink = new int[static_cast<size_t> (std::rand() % 100 + 50)];
    delete[] sink;
    REQUIRE (scope.allocations() > 0);
}

// ───────────────────────────── Voces del sistema (informe) ─────────────────────────────

namespace
{
/** WAV PCM de 16 bits, un canal. Devuelve las muestras y la frecuencia. */
bool readWav (const std::string& path, std::vector<float>& out, double& fs)
{
    std::FILE* f = std::fopen (path.c_str(), "rb");
    if (f == nullptr)
        return false;
    std::vector<unsigned char> b;
    unsigned char chunk[4096];
    for (size_t n; (n = std::fread (chunk, 1, sizeof chunk, f)) > 0;)
        b.insert (b.end(), chunk, chunk + n);
    std::fclose (f);
    size_t pos = 12;
    fs = 48000.0;
    const auto u32 = [&] (size_t p) { return uint32_t (b[p]) | uint32_t (b[p + 1]) << 8 | uint32_t (b[p + 2]) << 16 | uint32_t (b[p + 3]) << 24; };
    while (pos + 8 <= b.size())
    {
        const std::string id (reinterpret_cast<const char*> (&b[pos]), 4);
        const size_t size = u32 (pos + 4);
        if (id == "fmt ")
            fs = u32 (pos + 12);
        else if (id == "data")
        {
            const size_t n = std::min (size, b.size() - pos - 8) / 2;
            out.resize (n);
            for (size_t i = 0; i < n; ++i)
                out[i] = float (int16_t (b[pos + 8 + 2 * i] | b[pos + 9 + 2 * i] << 8)) / 32768.0f;
            return true;
        }
        pos += 8 + size + (size & 1);
    }
    return false;
}
} // namespace

TEST_CASE ("Informe: palabras de voces del sistema (macOS «say»), con y sin «s»", "[.report]")
{
    const char* dir = std::getenv ("CHAUS_TTS_DIR");
    if (dir == nullptr)
        SKIP ("define CHAUS_TTS_DIR con los WAV s_*, n_* y f_*");

    for (float offset : { 0.0f, 0.3f, 0.7f })
    {
        auto model = Classifier::initial();
        model.bias += offset;
        struct Group { int words = 0, any = 0; long samples = 0, active = 0; };
        Group s, n, f;
        for (const auto& entry : std::filesystem::directory_iterator (dir))
        {
            const auto name = entry.path().filename().string();
            std::vector<float> x;
            double fs = 48000.0;
            if (name.size() < 3 || ! readWav (entry.path().string(), x, fs))
                continue;
            x.resize (x.size() + size_t (0.1 * fs), 0.0f);
            const auto t = run (x, fs, 4000.0f, 12000.0f, true, model);
            Group& g = name[0] == 's' ? s : (name[0] == 'n' ? n : f);
            bool any = false;
            for (size_t i = size_t (0.03 * fs); i < t.active.size(); ++i)
            {
                any |= t.active[i] != 0;
                g.active += t.active[i];
                ++g.samples;
            }
            ++g.words;
            g.any += any ? 1 : 0;
            if (any && name[0] != 's' && offset == 0.0f && std::getenv ("CHAUS_TTS_LIST") != nullptr)
                std::printf ("    activa sin s: %s\n", name.c_str());
        }
        std::printf ("sesgo %+.1f | palabras con s activadas: %d/%d (%.0f %%) | sin s: %d/%d (%.0f %%) | f y j: %d/%d (%.0f %%) | tiempo activo: con s %.1f %%, sin s %.1f %%, f y j %.1f %%\n",
                     offset, s.any, s.words, 100.0 * s.any / std::max (1, s.words), n.any, n.words, 100.0 * n.any / std::max (1, n.words),
                     f.any, f.words, 100.0 * f.any / std::max (1, f.words),
                     100.0 * s.active / std::max<long> (1, s.samples), 100.0 * n.active / std::max<long> (1, n.samples), 100.0 * f.active / std::max<long> (1, f.samples));
    }
}
