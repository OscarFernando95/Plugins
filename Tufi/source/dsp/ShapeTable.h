#pragma once

#include <juce_core/juce_core.h>

#include <TripleBuffer.h>

#include <algorithm>
#include <atomic>

#include "ShapeCurve.h"

namespace tufi
{
/** Curva lineal a tramos con su antiderivada exacta, para ADAA de primer orden sin funciones
    trascendentes en el hilo de audio. Con 4096 puntos el error frente a la curva continua es ~1e-7. */
class ShapeLut
{
public:
    void setSize() { y.assign (tableSize, 0.0); F.assign (tableSize, 0.0); }

    /** Recalcula la antiderivada (suma acumulada de áreas de trapecio) a partir de `y`. */
    void rebuildIntegral() noexcept
    {
        F[0] = 0.0;
        for (size_t i = 0; i + 1 < y.size(); ++i)
            F[i + 1] = F[i] + 0.5 * (y[i] + y[i + 1]) * tableStep;
    }

    double f (double v) const noexcept
    {
        const auto [i, u] = locate (v);
        return y[i] + u * (y[i + 1] - y[i]) / tableStep;
    }

    double antiderivative (double v) const noexcept
    {
        const auto [i, u] = locate (v);
        const double m = (y[i + 1] - y[i]) / tableStep;
        return F[i] + y[i] * u + 0.5 * m * u * u;
    }

    std::vector<double> y, F;

private:
    struct Loc { size_t i; double u; };

    Loc locate (double v) const noexcept
    {
        const double pos = (v + tableHalfRange) / tableStep;
        const auto i = static_cast<size_t> (std::clamp (static_cast<int> (std::floor (pos)), 0, tableSize - 2));
        return { i, (v + tableHalfRange) - static_cast<double> (i) * tableStep };
    }
};

/** ADAA de primer orden sobre un ShapeLut. Un canal por instancia.

    La tabla se mueve poco a poco cuando cambia una perilla, y con ella su antiderivada: la de la
    muestra anterior se recalcula con la tabla nueva (`version` cambia con cada tabla). Sin eso,
    (F nueva − F vieja) / dv con un dv diminuto produce picos enormes. */
class LutAdaa
{
public:
    void reset() noexcept { primed = false; }

    float process (const ShapeLut& lut, double v, unsigned version) noexcept
    {
        const double Fv = lut.antiderivative (v);
        if (! primed)
        {
            vPrev = v;
            FPrev = Fv;
            primed = true;
            lastVersion = version;
        }
        else if (version != lastVersion)
        {
            FPrev = lut.antiderivative (vPrev);
            lastVersion = version;
        }

        const double dv = v - vPrev;
        const double out = std::abs (dv) > 1.0e-4 ? (Fv - FPrev) / dv : lut.f (0.5 * (v + vPrev));
        vPrev = v;
        FPrev = Fv;
        return static_cast<float> (out);
    }

private:
    double vPrev = 0.0, FPrev = 0.0;
    unsigned lastVersion = 0;
    bool primed = false;
};

/** Una tabla de forma calculada, con los números que enseñan los contadores de la interfaz. */
struct ShapeResult
{
    Table y;
    float slope = 1.0f;       // pendiente en cero
    float ceilingDb = -3.0f;  // techo de picos
    float asymmetry = 0.0f;   // % de componente par frente a impar en x = ±1
    ShapeResult() : y (tableSize, 0.0) {}
};

/** Calcula tablas de forma fuera del hilo de audio.

    El hilo de audio publica la forma que necesita (`request`) y recoge la última tabla lista
    (`takeLatest`), sin bloqueos en ningún sentido. Para pruebas y renderizado sin conexión, `pump`
    hace el trabajo en el propio hilo que lo llama. */
class ShapeTableBuilder : private juce::Thread
{
public:
    ShapeTableBuilder() : juce::Thread ("Tufi shape builder") {}
    ~ShapeTableBuilder() override { stop(); }

    void start() { startThread (juce::Thread::Priority::low); }
    void stop() { stopThread (500); }
    bool isRunning() const { return isThreadRunning(); }

    /** Hilo de audio. */
    void request (const ShapeParams& p) noexcept
    {
        requests.writeSlot() = p;
        requests.publish();
    }

    /** Hilo de audio: true si hay una tabla nueva en `latest()`. */
    bool takeLatest() noexcept { return results.update(); }
    const ShapeResult& latest() const noexcept { return results.read(); }

    /** Atiende la última petición pendiente. Lo llama el hilo secundario, o el de audio en modo síncrono. */
    void pump()
    {
        if (! requests.update())
            return;
        build (requests.read(), results.writeSlot());
        results.publish();
    }

    /** Calcula la tabla de unos parámetros (también la usa la interfaz para dibujar la curva). */
    static void build (const ShapeParams& p, ShapeResult& out);

private:
    void run() override
    {
        while (! threadShouldExit())
        {
            pump();
            wait (4);
        }
    }

    ferb::TripleBuffer<ShapeParams> requests;
    ferb::TripleBuffer<ShapeResult> results;
};
} // namespace tufi
