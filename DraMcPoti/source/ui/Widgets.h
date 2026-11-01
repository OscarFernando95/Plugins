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
        const float bodyR = size * (spec.big ? 0.285f : 0.275f);
        constexpr float sweep = 270.0f * std::numbers::pi_v<float> / 180.0f, start = -sweep * 0.5f;
        const auto pos = [&] (float t, float radius) { const float a = start + sweep * t; return juce::Point<float> (cx + radius * std::sin (a), cy - radius * std::cos (a)); };

        // Escala impresa en el panel: marcas finas cada 0,5 y las cifras de 0 a 10 en grande, como en un aparato de rack clásico.
        g.setColour (palette.ink.withAlpha (0.9f));
        for (int i = 0; i <= 20; ++i)
        {
            const float t = static_cast<float> (i) / 20.0f;
            const bool major = (i % 2) == 0;
            g.drawLine (juce::Line<float> (pos (t, bodyR * 1.40f), pos (t, bodyR * (major ? 1.62f : 1.50f))), major ? 1.3f : 0.8f);
        }
        const float fontPx = spec.big ? 12.5f : 9.8f;
        g.setFont (labelFont (fontPx, 0.0f).boldened());
        for (int n = 0; n <= 10; ++n)
        {
            const auto p = pos (static_cast<float> (n) / 10.0f, bodyR * 2.05f);
            g.drawText (juce::String (n), juce::Rectangle<float> (fontPx * 2.0f, fontPx * 1.3f).withCentre (p), juce::Justification::centred, false);
        }

        // Sombra proyectada.
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (cx - bodyR * 1.28f, cy - bodyR * 1.28f + 2.5f, bodyR * 2.56f, bodyR * 2.56f);

        // Falda cónica con estrías verticales (knurling): aro oscuro con 60 muescas claras que se ven de canto.
        const float skirt = bodyR * 1.28f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3a3d), cx - skirt, cy - skirt, juce::Colour (0xff0a0a0b), cx + skirt, cy + skirt, false));
        g.fillEllipse (cx - skirt, cy - skirt, skirt * 2.0f, skirt * 2.0f);
        constexpr int flutes = 60;
        for (int i = 0; i < flutes; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * static_cast<float> (i) / flutes;
            const juce::Point<float> p0 (cx + std::sin (a) * skirt * 0.80f, cy - std::cos (a) * skirt * 0.80f), p1 (cx + std::sin (a) * skirt * 0.995f, cy - std::cos (a) * skirt * 0.995f);
            const float lit = 0.5f + 0.5f * std::cos (a + 0.8f); // más claras hacia la luz (arriba a la izquierda)
            g.setColour (juce::Colour (0xff8a8a90).withAlpha (0.10f + 0.40f * lit));
            g.drawLine (juce::Line<float> (p0, p1), 0.9f);
        }
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawEllipse (cx - skirt, cy - skirt, skirt * 2.0f, skirt * 2.0f, 1.0f);

        // Cúpula lisa de baquelita negra con brillo.
        const float dome = bodyR * 0.98f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a50), cx - dome * 0.55f, cy - dome * 0.65f, juce::Colour (0xff050506), cx + dome * 0.7f, cy + dome * 0.8f, true));
        g.fillEllipse (cx - dome, cy - dome, dome * 2.0f, dome * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.fillEllipse (cx - dome * 0.62f, cy - dome * 0.82f, dome * 0.9f, dome * 0.42f); // reflejo
        g.setColour (juce::Colours::black.withAlpha (0.8f));
        g.drawEllipse (cx - dome, cy - dome, dome * 2.0f, dome * 2.0f, 0.9f);

        // Puntero: línea blanca fina del centro al borde de la cúpula.
        const float value = juce::jlimit (0.0f, 1.0f, param.getValue());
        g.setColour (juce::Colours::white.withAlpha (0.97f));
        g.drawLine (juce::Line<float> (pos (value, dome * 0.12f), pos (value, dome * 0.98f)), spec.big ? 2.4f : 1.9f);

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

/** Palanca de la maqueta: tuerca hexagonal plateada, agujero negro y vástago cromado cilíndrico con bola en la punta,
    inclinado hacia arriba o hacia abajo. Ligada a un parámetro Bool o Choice de dos valores. En modo «apilado» lleva el
    nombre arriba y un rótulo arriba y otro abajo (el activo se enciende en rojo con brillo); en modo «en línea» lleva el
    texto a la derecha. Apilado: arriba = valor 0. En línea: arriba = encendido. */
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
        const float w = static_cast<float> (getWidth()), h = static_cast<float> (getHeight());

        if (spec.stacked)
        {
            g.setColour (palette.ink);
            drawSpaced (g, spec.caption, { 0.0f, 0.0f, w, 14.0f }, labelFont (11.0f, 2.2f), juce::Justification::centred);
            drawLever (g, w * 0.5f, h * 0.5f + 4.0f, 20.0f, up, highlighted);
            plate (g, { w * 0.5f - 42.0f, 16.0f, 84.0f, 22.0f }, spec.top, up);
            plate (g, { w * 0.5f - 42.0f, h - 24.0f, 84.0f, 22.0f }, spec.bottom, ! up);
        }
        else
        {
            drawLever (g, 24.0f, h * 0.5f, 15.0f, up, highlighted);
            plate (g, { 46.0f, h * 0.5f - 11.0f, w - 48.0f, 22.0f }, spec.text, on);
        }
    }

private:
    /** Rótulo negro; encendido, rojo con brillo (como «VOCAL» y «VINTAGE» de la maqueta). */
    void plate (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& text, bool lit) const
    {
        if (lit)
        {
            for (int i = 4; i >= 1; --i)
            {
                g.setColour (juce::Colour (0xffff3b30).withAlpha (0.07f));
                g.fillRoundedRectangle (r.expanded (static_cast<float> (i) * 1.6f), 4.0f);
            }
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8473d), r.getX(), r.getY(), juce::Colour (0xffa81f17), r.getX(), r.getBottom(), false));
        }
        else
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff202124), r.getX(), r.getY(), juce::Colour (0xff09090a), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        g.setColour (lit ? juce::Colours::white : palette.ink.withAlpha (0.9f));
        drawSpaced (g, text, r, labelFont (10.5f, 1.8f), juce::Justification::centred);
    }

    void drawLever (juce::Graphics& g, float cx, float cy, float R, bool up, bool highlighted) const
    {
        // Tuerca hexagonal plateada con bisel.
        const auto hex = [&] (float radius)
        {
            juce::Path p;
            for (int i = 0; i < 6; ++i)
            {
                const float a = juce::MathConstants<float>::pi / 3.0f * static_cast<float> (i) - juce::MathConstants<float>::halfPi;
                const juce::Point<float> v (cx + radius * std::cos (a), cy + radius * std::sin (a));
                i == 0 ? p.startNewSubPath (v) : p.lineTo (v);
            }
            p.closeSubPath();
            return p.createPathWithRoundedCorners (radius * 0.12f);
        };
        g.setColour (juce::Colours::black.withAlpha (0.5f)); // sombra
        g.fillPath (hex (R), juce::AffineTransform::translation (1.5f, 2.5f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f4f6), cx - R, cy - R, juce::Colour (0xff6a6e73), cx + R, cy + R, false));
        g.fillPath (hex (R));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8d9297), cx - R * 0.8f, cy - R * 0.8f, juce::Colour (0xffd9dcdf), cx + R * 0.8f, cy + R * 0.8f, false));
        g.fillPath (hex (R * 0.84f));
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.strokePath (hex (R), juce::PathStrokeType (1.0f));

        // Agujero negro con aro.
        const float hole = R * 0.62f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9ebed), cx - hole, cy - hole, juce::Colour (0xff55595e), cx + hole, cy + hole, false));
        g.fillEllipse (cx - hole - 1.5f, cy - hole - 1.5f, (hole + 1.5f) * 2.0f, (hole + 1.5f) * 2.0f);
        g.setColour (juce::Colour (0xff050505));
        g.fillEllipse (cx - hole, cy - hole, hole * 2.0f, hole * 2.0f);

        // Vástago cilíndrico cromado, grueso y muy brillante, inclinado, con collar en la base y bola pulida en la punta.
        const float angle = (up ? -13.0f : 167.0f) * juce::MathConstants<float>::pi / 180.0f, length = R * 1.55f, w0 = R * 0.40f, w1 = R * 0.28f;
        const juce::Point<float> dir (std::sin (angle), -std::cos (angle)), perp (-dir.y, dir.x), base (cx, cy), tip = base + dir * length;
        juce::Path bat;
        bat.startNewSubPath (base - perp * w0);
        bat.lineTo (tip - perp * w1);
        bat.lineTo (tip + perp * w1);
        bat.lineTo (base + perp * w0);
        bat.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillPath (bat, juce::AffineTransform::translation (2.5f, 2.5f)); // sombra sobre el panel
        juce::ColourGradient chrome (juce::Colour (0xff2c2f33), (base - perp * w0).x, (base - perp * w0).y, juce::Colour (0xff2c2f33), (base + perp * w0).x, (base + perp * w0).y, false);
        chrome.addColour (0.12, juce::Colour (0xffa9aeb3));
        chrome.addColour (0.26, juce::Colour (0xffffffff));
        chrome.addColour (0.42, juce::Colour (0xffc4c8cc));
        chrome.addColour (0.62, juce::Colour (0xff6c7176));
        chrome.addColour (0.80, juce::Colour (0xffb0b5ba));
        chrome.addColour (0.92, juce::Colour (0xff4a4e52));
        g.setGradientFill (chrome);
        g.fillPath (bat);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.strokePath (bat, juce::PathStrokeType (0.8f));

        // Collar cromado donde el vástago sale de la tuerca.
        const float collar = R * 0.50f;
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, cx - collar, cy - collar, juce::Colour (0xff5b5f64), cx + collar, cy + collar, false));
        g.fillEllipse (cx - collar, cy - collar, collar * 2.0f, collar * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (cx - collar, cy - collar, collar * 2.0f, collar * 2.0f, 0.9f);

        const float ball = R * 0.40f;
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillEllipse (tip.x - ball + 2.0f, tip.y - ball + 2.5f, ball * 2.0f, ball * 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, tip.x - ball * 0.45f, tip.y - ball * 0.55f, juce::Colour (0xff50555a), tip.x + ball * 0.7f, tip.y + ball * 0.8f, true));
        g.fillEllipse (tip.x - ball, tip.y - ball, ball * 2.0f, ball * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.fillEllipse (tip.x - ball * 0.55f, tip.y - ball * 0.65f, ball * 0.5f, ball * 0.32f); // punto de luz
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawEllipse (tip.x - ball, tip.y - ball, ball * 2.0f, ball * 2.0f, 0.8f);
        if (highlighted)
        {
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.fillPath (hex (R));
        }
    }

    Palette palette;
    Spec spec;
    juce::ButtonParameterAttachment attachment;
};

/** Válvula de vidrio: bulbo oscuro con placas y una ventana ámbar que brilla con lo que añade la banda, zócalo plateado
    con patillas y un halo cálido. */
class GlassTube : public juce::Component
{
public:
    void setGlow (float g)
    {
        if (std::abs (g - glow) > 0.01f)
        {
            glow = g;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const float w = static_cast<float> (getWidth()), h = static_cast<float> (getHeight());
        const auto amber = juce::Colour (0xffffa030);
        if (glow > 0.02f)
        {
            g.setGradientFill (juce::ColourGradient (amber.withAlpha (0.35f * glow), w * 0.5f, h * 0.4f, amber.withAlpha (0.0f), w * 0.5f, h * 0.4f + w * 0.9f, true));
            g.fillEllipse (-w * 0.4f, h * 0.4f - w * 0.9f, w * 1.8f, w * 1.8f);
        }
        const auto bulb = juce::Rectangle<float> (3.0f, 1.0f, w - 6.0f, h - 12.0f);
        juce::Path glass;
        glass.addRoundedRectangle (bulb.getX(), bulb.getY(), bulb.getWidth(), bulb.getHeight(), bulb.getWidth() * 0.5f, bulb.getWidth() * 0.5f, true, true, false, false);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a2d2e), bulb.getX(), 0.0f, juce::Colour (0xff120c0d), bulb.getRight(), 0.0f, false));
        g.fillPath (glass);
        // Placas internas y ventana ámbar.
        g.setColour (juce::Colour (0xff1a1718));
        g.fillRect (w * 0.5f - 7.0f, h * 0.2f, 14.0f, h * 0.55f);
        g.setColour (juce::Colour (0xff6b6a6a));
        g.fillRect (w * 0.5f - 6.0f, h * 0.17f, 12.0f, 2.0f); // getter
        g.setColour (amber.withAlpha (0.12f + 0.85f * glow));
        g.fillRect (w * 0.5f - 4.0f, h * 0.27f, 8.0f, h * 0.38f);
        // Reflejo del vidrio.
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.fillRoundedRectangle (bulb.getX() + 3.0f, bulb.getY() + 6.0f, 3.0f, bulb.getHeight() * 0.62f, 1.5f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.strokePath (glass, juce::PathStrokeType (1.0f));
        // Zócalo plateado y patillas.
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f1f2), 0.0f, h - 12.0f, juce::Colour (0xff6a6e73), 0.0f, h - 5.0f, false));
        g.fillRoundedRectangle (2.0f, h - 12.0f, w - 4.0f, 7.0f, 1.5f);
        g.setColour (juce::Colour (0xffb9bdc2));
        for (int i = 0; i < 3; ++i)
            g.fillRect (w * 0.5f - 9.0f + 8.0f * static_cast<float> (i), h - 5.0f, 2.5f, 5.0f);
    }

private:
    float glow = 0.0f;
};

/** Placa negra con el nombre de la banda y, debajo, su rango de frecuencias en cifras pequeñas. */
class BandPlate : public juce::Component
{
public:
    explicit BandPlate (juce::String name) : title (std::move (name)) {}

    void setRange (float lowHz, float highHz)
    {
        const auto text = hz (lowHz) + juce::String::fromUTF8 (" – ") + hz (highHz);
        if (text != range)
        {
            range = text;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const float w = static_cast<float> (getWidth());
        const juce::Rectangle<float> r (0.0f, 0.0f, w, 22.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff222326), 0.0f, 0.0f, juce::Colour (0xff070708), 0.0f, 22.0f, false));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (r, 4.0f, 1.2f);
        g.setColour (juce::Colour (0xffece9e2));
        drawSpaced (g, title, r, labelFont (13.0f, 2.4f), juce::Justification::centred);
        g.setColour (juce::Colour (0xffb9b6ae));
        g.setFont (digitFont (9.0f));
        g.drawText (range, juce::Rectangle<float> (0.0f, 24.0f, w, 12.0f), juce::Justification::centred, false);
    }

private:
    static juce::String hz (float f) { return f >= 1000.0f ? ferb::fmt::comma (f / 1000.0f, f >= 10000.0f ? 0 : 1) + " kHz" : ferb::fmt::comma (f, 0) + " Hz"; }
    juce::String title, range;
};

/** Selector rotativo de cuatro posiciones (Voicing): cúpula negra con barra cromada y bola, los cuatro nombres alrededor
    con su número, y el activo en ámbar. Se maneja haciendo clic en un nombre o girando con el ratón. */
class VoicingKnob : public juce::Component
{
public:
    VoicingKnob (juce::RangedAudioParameter& p, const Palette& pal, std::function<int()> eraOf, juce::UndoManager* um = nullptr)
        : palette (pal), era (std::move (eraOf)), attachment (p, [this] (float v) { index = juce::jlimit (0, 3, juce::roundToInt (v)); repaint(); }, um)
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void refresh() { repaint(); }

    void paint (juce::Graphics& g) override
    {
        const float cx = static_cast<float> (getWidth()) * 0.5f, cy = static_cast<float> (getHeight()) * 0.5f + 8.0f, R = 27.0f;
        static const char* names[2][4] = { { "CLÁSICO", "CINTA", "VÁLVULA", "TRANSISTOR" }, { "DINÁMICO", "ADAPTATIVO", "SEDA", "AIRE" } };
        const int e = juce::jlimit (0, 1, era());
        for (int i = 0; i < 4; ++i)
        {
            const auto a = angleOf (i);
            const juce::Point<float> tick (cx + std::sin (a) * (R + 12.0f), cy - std::cos (a) * (R + 12.0f));
            const juce::Point<float> text (cx + std::sin (a) * (R + 34.0f), cy - std::cos (a) * (R + 30.0f));
            const bool active = i == index;
            g.setColour (active ? juce::Colour (0xffffb13b) : palette.ink);
            g.setFont (digitFont (10.0f, true));
            g.drawText (juce::String (i + 1), juce::Rectangle<float> (14.0f, 12.0f).withCentre (tick), juce::Justification::centred, false);
            drawSpaced (g, juce::String::fromUTF8 (names[e][i]), juce::Rectangle<float> (90.0f, 14.0f).withCentre (text), labelFont (10.5f, 1.4f), juce::Justification::centred);
        }
        g.setColour (palette.ink2);
        drawSpaced (g, "VOICING", { 0.0f, 2.0f, static_cast<float> (getWidth()), 14.0f }, labelFont (11.0f, 2.2f), juce::Justification::centred);

        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (cx - R, cy - R + 3.0f, R * 2.0f, R * 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3a3b), cx - R, cy - R, juce::Colour (0xff070606), cx + R, cy + R, false));
        g.fillEllipse (cx - R, cy - R, R * 2.0f, R * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawEllipse (cx - R, cy - R, R * 2.0f, R * 2.0f, 1.0f);
        const auto a = angleOf (index);
        const juce::Point<float> dir (std::sin (a), -std::cos (a)), tip (cx + dir.x * (R * 0.78f), cy + dir.y * (R * 0.78f));
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawLine (cx - dir.x * 10.0f + 1.5f, cy - dir.y * 10.0f + 2.0f, tip.x + 1.5f, tip.y + 2.0f, 7.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7f8f9), cx - 4.0f, cy - 4.0f, juce::Colour (0xff868b90), cx + 4.0f, cy + 4.0f, false));
        g.drawLine (cx - dir.x * 10.0f, cy - dir.y * 10.0f, tip.x, tip.y, 6.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, tip.x - 2.0f, tip.y - 2.0f, juce::Colour (0xff6a6e73), tip.x + 3.0f, tip.y + 3.0f, true));
        g.fillEllipse (tip.x - 4.5f, tip.y - 4.5f, 9.0f, 9.0f);
    }

    void mouseDown (const juce::MouseEvent& ev) override { select (ev.position); }
    void mouseDrag (const juce::MouseEvent& ev) override { select (ev.position); }

private:
    static float angleOf (int i) { return juce::degreesToRadians (-135.0f + 90.0f * static_cast<float> (i)); }

    void select (juce::Point<float> p)
    {
        const float cx = static_cast<float> (getWidth()) * 0.5f, cy = static_cast<float> (getHeight()) * 0.5f + 8.0f;
        const float a = std::atan2 (p.x - cx, -(p.y - cy));
        int best = 0;
        float bestDiff = 1.0e9f;
        for (int i = 0; i < 4; ++i)
        {
            float d = std::abs (a - angleOf (i));
            d = std::min (d, juce::MathConstants<float>::twoPi - d);
            if (d < bestDiff)
            {
                bestDiff = d;
                best = i;
            }
        }
        if (best != index)
            attachment.setValueAsCompleteGesture (static_cast<float> (best));
    }

    Palette palette;
    std::function<int()> era;
    int index = 0;
    juce::ParameterAttachment attachment;
};

/** Joya roja de «Encendido»: brilla mientras el plugin no está en bypass. */
class Jewel : public juce::Component
{
public:
    void setLit (bool on)
    {
        if (on != lit)
        {
            lit = on;
            repaint();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const float cx = static_cast<float> (getWidth()) * 0.5f, cy = 17.0f, r = 13.0f;
        if (lit)
            for (int i = 4; i >= 1; --i)
            {
                g.setColour (juce::Colour (0xffff3b30).withAlpha (0.08f));
                g.fillEllipse (cx - r - static_cast<float> (i) * 2.5f, cy - r - static_cast<float> (i) * 2.5f, (r + static_cast<float> (i) * 2.5f) * 2.0f, (r + static_cast<float> (i) * 2.5f) * 2.0f);
            }
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9ebed), cx - r, cy - r, juce::Colour (0xff5a5e63), cx + r, cy + r, false));
        g.fillEllipse (cx - r - 3.0f, cy - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f);
        g.setGradientFill (juce::ColourGradient (lit ? juce::Colour (0xffff8a80) : juce::Colour (0xff6a1c18), cx - r * 0.3f, cy - r * 0.4f,
                                                 lit ? juce::Colour (0xff9a0f0a) : juce::Colour (0xff230807), cx + r * 0.8f, cy + r * 0.9f, true));
        g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (lit ? 0.55f : 0.2f));
        g.fillEllipse (cx - r * 0.55f, cy - r * 0.7f, r * 0.7f, r * 0.45f);
        g.setColour (juce::Colour (0xffece9e2));
        drawSpaced (g, "ENCENDIDO", { 0.0f, cy + r + 6.0f, static_cast<float> (getWidth()), 12.0f }, labelFont (8.5f, 1.6f), juce::Justification::centred);
    }

private:
    bool lit = true;
};

} // namespace mcpoti::ui
