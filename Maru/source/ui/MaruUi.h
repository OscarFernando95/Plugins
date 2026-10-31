#pragma once

#include <array>

#include <Controls.h>

#include "../dsp/MaruEngine.h"

namespace maru::ui
{
/** Ventana de cristal con el piso de ruido que aprende el estimador: barras de la entrada (crema) y de la salida
    (verde pálido), o, con «Escuchar lo quitado», la diferencia en ámbar, y una línea ámbar discontinua con el piso por banda. */
class FloorView : public ferb::vintage::GlassView
{
public:
    static constexpr int size = 150;
    static constexpr int bands = Telemetry::bands;

    struct Data
    {
        std::array<float, bands> input, output, noise;
        bool listening = false;
    };

    explicit FloorView (const ferb::vintage::Palette& p) : GlassView (p) { data.input.fill (-120.0f); data.output.fill (-120.0f); data.noise.fill (-120.0f); }
    void setData (const Data& d) { data = d; repaint(); }

protected:
    void paintContent (juce::Graphics&, float w, float h) override;

private:
    Data data;
};

/** Lámpara de joya con su rótulo («TRABAJANDO», «PUERTA ABIERTA»…). */
class JewelLamp : public juce::Component
{
public:
    explicit JewelLamp (const ferb::vintage::Palette& p) : palette (p) { setInterceptsMouseClicks (false, false); }
    void set (bool lit, const juce::String& text);
    void paint (juce::Graphics&) override;

private:
    ferb::vintage::Palette palette;
    bool on = false;
    juce::String caption;
};
} // namespace maru::ui
