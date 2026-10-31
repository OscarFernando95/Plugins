#pragma once

#include <Controls.h>
#include <Meters.h>

#include <array>
#include <functional>
#include <vector>

#include "../PluginProcessor.h"

namespace nel::ui
{
/** Valores que el editor lee de los medidores del motor y reparte a las piezas, a 30 Hz. */
struct MeterState
{
    std::array<float, 2> inPeakDb { -100.0f, -100.0f }, outPeakDb { -100.0f, -100.0f };
    std::array<float, 2> inHoldDb { -100.0f, -100.0f }, outHoldDb { -100.0f, -100.0f };
    float inRmsDb = -100.0f, outRmsDb = -100.0f;
    float ceilingDb = -1.0f;
    float meterOffset = 0.0f; // escala K: se suma a las marcas de la escalera
};

/** Medidor de escalera de 48 segmentos (−48 a 0 dB) con RMS más brillante dentro del pico, retención
    de pico y, en el de salida, la marca del techo en latón. */
class LadderMeter : public juce::Component
{
public:
    LadderMeter (const ferb::vintage::Palette&, juce::String title, bool isOutput);
    void setState (const MeterState& s);
    void paint (juce::Graphics&) override;

    static constexpr int width = 78, height = 316;

private:
    ferb::vintage::Palette palette;
    juce::String title;
    bool output;
    MeterState state;
};

/** Pantalla de 6 s: entrada con ganancia, salida y reducción de ganancia en ámbar desde arriba. */
class ScopeView : public ferb::vintage::GlassView
{
public:
    explicit ScopeView (const ferb::vintage::Palette&);

    /** Un punto de historia (valores en dB; la reducción en dB positivos). */
    void push (float inputDb, float outputDb, float reductionDb);
    void setCeiling (float db) noexcept { ceilingDb = db; }
    void setAudition (bool on) noexcept { audition = on; }
    void setLegend (juce::String output) { outputLegend = std::move (output); }

    static constexpr int historyLength = 180; // 6 s a 30 Hz

protected:
    void paintContent (juce::Graphics&, float width, float height) override;

private:
    std::vector<float> in, out, gr;
    float ceilingDb = -1.0f;
    bool audition = false;
    juce::String outputLegend = "salida";
};

/** Vúmetro de reducción de ganancia (0 a 12 dB, la aguja va de derecha a izquierda). */
class GrMeter : public juce::Component
{
public:
    explicit GrMeter (const ferb::vintage::Palette&);
    void setValue (float reductionDb);
    void paint (juce::Graphics&) override;

    static constexpr int margin = 9;

private:
    ferb::vintage::Palette palette;
    float value = 0.0f;
    juce::Image cache;
    float cacheScale = 0.0f;
};

/** Panel de sonoridad: columnas M y S respecto del objetivo, ventanas INT, Δ OBJ, LRA, PLR y TP MÁX,
    teclas de objetivo y reinicio. */
class LoudnessPanel : public juce::Component
{
public:
    struct Values
    {
        float momentary = -200.0f, shortTerm = -200.0f, integrated = -200.0f, range = 0.0f, elapsed = 0.0f;
        float truePeakMax = -100.0f;
    };

    LoudnessPanel (const ferb::vintage::Palette&, int initialTarget);
    void setValues (const Values& v);
    int getTarget() const noexcept { return target; }

    std::function<void (int)> onTarget;
    std::function<void()> onReset;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    static constexpr int width = 228, height = 190;

private:
    juce::Rectangle<float> targetButton (int i) const;
    juce::Rectangle<float> resetButton() const;

    ferb::vintage::Palette palette;
    Values values;
    int target;
};

/** Ocho botones de modo; el elegido va hundido, con su lámpara encendida. */
class ModeSelector : public juce::Component
{
public:
    ModeSelector (const ferb::vintage::Palette&, std::function<void (int)> onSelect);
    void setMode (int mode);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    static constexpr int buttonW = 53, buttonH = 40, gap = 4;

private:
    ferb::vintage::Palette palette;
    std::function<void (int)> select;
    int mode = 0;
};

/** Interruptor con lámpara (parámetro Bool): `[●  TEXTO]`; encendido, el botón queda hundido. */
class LampToggle : public juce::Button
{
public:
    LampToggle (juce::RangedAudioParameter&, const ferb::vintage::Palette&, juce::String text, juce::UndoManager* = nullptr);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    void setCream (bool shouldBeCream) noexcept { cream = shouldBeCream; repaint(); }

private:
    ferb::vintage::Palette palette;
    juce::String caption;
    bool cream = false; // estilo de segmento activo (crema), para «+1 escalón»
    juce::ButtonParameterAttachment attachment;
};

/** Cuatro celdas bajo la pantalla: GR actual, GR máx, overs y el botón de escala. */
class ReadoutStrip : public juce::Component
{
public:
    explicit ReadoutStrip (const ferb::vintage::Palette&);
    void setValues (float grNow, float grMax, int overs);
    void setScaleName (juce::String name);

    std::function<void()> onScale;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> cell (int i) const;
    ferb::vintage::Palette palette;
    float grNow = 0.0f, grMax = 0.0f;
    int overs = 0;
    juce::String scaleName = "ESTÁNDAR";
};
} // namespace nel::ui
