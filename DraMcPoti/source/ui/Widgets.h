#pragma once

#include <Controls.h>

#include <cmath>
#include <numbers>

/** Piezas propias de Dra. McPoti con el aspecto de la referencia de rack clásico: perilla de baquelita con falda
    estriada y escala numerada alrededor, y palanca (interruptor de gatillo) cromada con tuerca. */
namespace mcpoti::ui
{
using namespace ferb::vintage;

/** Perilla negra con falda estriada, puntero blanco y escala de 0 a 10 grabada alrededor. Hereda de VKnob los gestos
    (arrastre vertical, rueda, doble clic, flechas, Mayús para afinar), el valor y el accesible; solo cambia el dibujo. */
class ScaleKnob : public VKnob
{
public:
    ScaleKnob (juce::RangedAudioParameter& p, const Palette& pal, Spec s, juce::UndoManager* um = nullptr)
        : VKnob (p, pal, s, um), param (p), palette (pal), spec (std::move (s)),
          attachment (p, [this] (float) { repaint(); }, um)
    {
        attachment.sendInitialUpdate();
    }

    void paint (juce::Graphics& g) override
    {
        const float size = static_cast<float> (spec.size), cx = static_cast<float> (getWidth()) * 0.5f, cy = size * 0.5f;
        const float bodyR = size * (spec.big ? 0.30f : 0.29f), scaleR = size * 0.435f;
        constexpr float sweep = 270.0f * std::numbers::pi_v<float> / 180.0f, start = -sweep * 0.5f;
        const auto pos = [&] (float t, float radius) { const float a = start + sweep * t; return juce::Point<float> (cx + radius * std::sin (a), cy - radius * std::cos (a)); };

        // Escala: marcas cada 0,5 y cifras de 0 a 10.
        g.setColour (palette.ink.withAlpha (0.85f));
        for (int i = 0; i <= 20; ++i)
        {
            const float t = static_cast<float> (i) / 20.0f;
            const bool major = (i % 2) == 0;
            g.drawLine (juce::Line<float> (pos (t, bodyR * 1.32f), pos (t, bodyR * (major ? 1.52f : 1.43f))), major ? 1.2f : 0.8f);
        }
        const float fontPx = spec.big ? 9.5f : 7.6f;
        g.setFont (digitFont (fontPx, true));
        for (int n = 0; n <= 10; ++n)
        {
            const auto p = pos (static_cast<float> (n) / 10.0f, scaleR * 1.17f);
            g.drawText (juce::String (n), juce::Rectangle<float> (fontPx * 2.0f, fontPx * 1.3f).withCentre (p), juce::Justification::centred, false);
        }

        // Sombra, falda estriada (28 dientes) y cúpula negra con brillo.
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (cx - bodyR * 1.18f, cy - bodyR * 1.18f + 2.0f, bodyR * 2.36f, bodyR * 2.36f);
        constexpr int teeth = 28;
        for (int i = 0; i < teeth; ++i)
        {
            juce::Path tooth;
            tooth.addRoundedRectangle (-bodyR * 0.07f, -bodyR * 1.16f, bodyR * 0.14f, bodyR * 0.30f, bodyR * 0.04f);
            g.setColour (juce::Colour (0xff1a1a1c));
            g.fillPath (tooth, juce::AffineTransform::rotation (juce::MathConstants<float>::twoPi * static_cast<float> (i) / teeth).translated (cx, cy));
        }
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a4e), cx - bodyR, cy - bodyR, juce::Colour (0xff060607), cx + bodyR, cy + bodyR, false));
        g.fillEllipse (cx - bodyR * 1.1f, cy - bodyR * 1.1f, bodyR * 2.2f, bodyR * 2.2f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3d41), cx - bodyR * 0.45f, cy - bodyR * 0.55f, juce::Colour (0xff050506), cx + bodyR * 0.6f, cy + bodyR * 0.7f, true));
        g.fillEllipse (cx - bodyR * 0.9f, cy - bodyR * 0.9f, bodyR * 1.8f, bodyR * 1.8f);
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.drawEllipse (cx - bodyR * 0.9f, cy - bodyR * 0.9f, bodyR * 1.8f, bodyR * 1.8f, 0.8f);

        // Puntero blanco.
        const float value = juce::jlimit (0.0f, 1.0f, param.getValue());
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.drawLine (juce::Line<float> (pos (value, bodyR * 0.30f), pos (value, bodyR * 1.04f)), spec.big ? 2.8f : 2.2f);

        // Rótulo (el valor lo dibuja la pastilla de VKnob).
        drawSpaced (g, spec.label, juce::Rectangle<float> (0.0f, size + 2.0f, static_cast<float> (getWidth()), spec.big ? 16.0f : 13.0f),
                    labelFont (spec.big ? 13.0f : 10.0f, 1.2f), juce::Justification::centred);
    }

private:
    juce::RangedAudioParameter& param;
    Palette palette;
    Spec spec;
    juce::ParameterAttachment attachment;
};

/** Palanca de gatillo cromada: tuerca redonda, vástago cónico con brillo y punta redondeada. Ligada a un parámetro Bool o
    Choice de dos valores. En modo «apilado» lleva el nombre arriba y un rótulo arriba y otro abajo (el activo se enciende);
    en modo «en línea» lleva el texto a la derecha. Arriba = valor 0 en el apilado, y encendido en el de línea. */
class ChromeToggle : public juce::Button
{
public:
    struct Spec
    {
        juce::String caption, top, bottom; // apilado
        juce::String text;                  // en línea
        bool stacked = false;
    };

    ChromeToggle (juce::RangedAudioParameter& p, const Palette& pal, Spec s, juce::UndoManager* um = nullptr)
        : juce::Button (s.caption.isEmpty() ? s.text : s.caption), palette (pal), spec (std::move (s)), attachment (p, *this, um)
    {
        setClickingTogglesState (true);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void paintButton (juce::Graphics& g, bool highlighted, bool) override
    {
        const bool on = getToggleState();
        const bool up = spec.stacked ? ! on : on;
        const float w = static_cast<float> (getWidth());
        const float cx = spec.stacked ? w * 0.5f : 20.0f, cy = spec.stacked ? 56.0f : static_cast<float> (getHeight()) * 0.5f;
        drawLever (g, cx, cy, up, highlighted);

        if (spec.stacked)
        {
            g.setColour (palette.ink);
            drawSpaced (g, spec.caption, { 0.0f, 0.0f, w, 14.0f }, labelFont (11.0f, 1.8f), juce::Justification::centred);
            const auto lit = juce::Colour (0xffd8372d), dark = palette.btnB;
            const auto label = [&] (const juce::String& text, float y, bool active)
            {
                const auto r = juce::Rectangle<float> (w * 0.5f - 42.0f, y, 84.0f, 18.0f);
                g.setColour (active ? lit : dark);
                g.fillRoundedRectangle (r, 2.0f);
                g.setColour (juce::Colours::black.withAlpha (0.6f));
                g.drawRoundedRectangle (r, 2.0f, 1.0f);
                g.setColour (active ? juce::Colours::white : palette.ink.withAlpha (0.85f));
                drawSpaced (g, text, r, labelFont (10.5f, 1.6f), juce::Justification::centred);
            };
            label (spec.top, 16.0f, up);
            label (spec.bottom, static_cast<float> (getHeight()) - 20.0f, ! up);
        }
        else
        {
            const auto r = juce::Rectangle<float> (42.0f, cy - 12.0f, w - 44.0f, 24.0f);
            g.setColour (on ? juce::Colour (0xffd8372d) : palette.btnB);
            g.fillRoundedRectangle (r, 2.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.drawRoundedRectangle (r, 2.0f, 1.0f);
            g.setColour (on ? juce::Colours::white : palette.ink.withAlpha (0.9f));
            drawSpaced (g, spec.text, r, labelFont (10.5f, 1.6f), juce::Justification::centred);
        }
    }

private:
    void drawLever (juce::Graphics& g, float cx, float cy, bool up, bool highlighted) const
    {
        // Tuerca: aro cromado con degradado y centro hundido.
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (cx - 13.0f, cy - 11.0f, 26.0f, 26.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f5f6), cx - 11.0f, cy - 12.0f, juce::Colour (0xff6d7176), cx + 11.0f, cy + 12.0f, false));
        g.fillEllipse (cx - 12.0f, cy - 12.0f, 24.0f, 24.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5d6166), cx - 6.0f, cy - 6.0f, juce::Colour (0xffd2d5d8), cx + 7.0f, cy + 7.0f, false));
        g.fillEllipse (cx - 8.0f, cy - 8.0f, 16.0f, 16.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (cx - 12.0f, cy - 12.0f, 24.0f, 24.0f, 1.0f);

        // Vástago: cónico, inclinado hacia arriba o hacia abajo, con brillo longitudinal y punta redonda.
        const float dir = up ? -1.0f : 1.0f, length = 28.0f, baseW = 4.8f, tipW = 3.4f;
        juce::Path bat;
        bat.startNewSubPath (cx - baseW, cy);
        bat.lineTo (cx - tipW, cy + dir * length);
        bat.quadraticTo (cx, cy + dir * (length + 4.0f), cx + tipW, cy + dir * length);
        bat.lineTo (cx + baseW, cy);
        bat.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.35f)); // sombra
        g.fillPath (bat, juce::AffineTransform::translation (2.0f, 2.5f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6d7176), cx - baseW, cy, juce::Colour (0xff8f9499), cx + baseW, cy, false));
        g.fillPath (bat);
        juce::ColourGradient shine (juce::Colours::white.withAlpha (0.0f), cx - baseW, cy, juce::Colours::white.withAlpha (0.0f), cx + baseW, cy, false);
        shine.addColour (0.3, juce::Colours::white.withAlpha (0.85f));
        shine.addColour (0.5, juce::Colours::white.withAlpha (0.25f));
        g.setGradientFill (shine);
        g.fillPath (bat);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.strokePath (bat, juce::PathStrokeType (0.8f));
        if (highlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillEllipse (cx - 12.0f, cy - 12.0f, 24.0f, 24.0f);
        }
    }

    Palette palette;
    Spec spec;
    juce::ButtonParameterAttachment attachment;
};
} // namespace mcpoti::ui
