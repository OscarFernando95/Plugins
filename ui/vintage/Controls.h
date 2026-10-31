#pragma once

#include <ParamField.h>

#include <functional>
#include <optional>

#include "Vintage.h"

namespace ferb::vintage
{
/** Valor de una perilla: cifras ámbar sobre una pastilla de cristal. Arrastrar en vertical lo cambia,
    doble clic permite escribirlo y Alt + clic lo devuelve a su valor por defecto. */
class ValuePill : public ParamField
{
public:
    ValuePill (juce::RangedAudioParameter&, Format, Parse, const Palette&, float fontHeight, float padding, float corner = 3.0f);
    void paint (juce::Graphics&) override;

private:
    juce::Colour glass;
    float pad, cornerSize;
};

/** Perilla de baquelita con marcas grabadas, puntero crema y, si es grande, bisel de latón.
    Recorrido de 270°. Gestos: arrastre vertical (190 px = recorrido, Mayús para afinar), rueda,
    doble clic al valor por defecto, flechas, Re Pág/Av Pág e Inicio/Fin. Incluye su rótulo y su valor. */
class VKnob : public juce::Component
{
public:
    struct Spec
    {
        juce::String label;
        int size = 56;
        bool big = false;
        juce::String endLow, endHigh; // marcas de los extremos (solo si size ≥ 70)
        std::function<juce::String (float)> format;
        std::function<float (const juce::String&, float)> parse = [] (const juce::String& t, float old) { return fmt::parseNumber (t, old); };
    };

    VKnob (juce::RangedAudioParameter&, const Palette&, Spec, juce::UndoManager* = nullptr);

    /** Alto total de la unidad (perilla, rótulo y valor). */
    static int heightFor (const Spec&) noexcept;

    /** Vuelve a formatear el valor (cuando el formato depende de otro control). */
    void refreshValue() { pill->refresh(); }

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override;
    void focusLost (FocusChangeType) override;
    std::unique_ptr<juce::AccessibilityHandler> createAccessibilityHandler() override;

private:
    class AccessibleValue;

    juce::Rectangle<float> dialArea() const noexcept;
    void nudge (float delta);
    void setNormalised (float n);

    juce::RangedAudioParameter& param;
    Palette palette;
    Spec spec;
    juce::ParameterAttachment attachment;
    std::unique_ptr<ValuePill> pill;
    float normalised = 0.0f, dragStartNorm = 0.0f;
    bool showFocus = false;
};

/** Palanca cromada de dos posiciones con sus rótulos «arriba» y «abajo» y el nombre del control.
    Ligada a un parámetro Bool (o Choice de dos). Con `invert`, arriba es el valor 0. */
class VLever : public juce::Button
{
public:
    struct Spec
    {
        juce::String name, top = "ON", bottom = "OFF";
        bool invert = false;
    };

    static constexpr int width = 70, height = 104;

    VLever (juce::RangedAudioParameter&, const Palette&, Spec, juce::UndoManager* = nullptr);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Palette palette;
    Spec spec;
    juce::ButtonParameterAttachment attachment;
};

/** Fila de botones oscuros con el elegido en crema, ligada a un parámetro Choice. */
class VSeg : public juce::Component
{
public:
    VSeg (juce::RangedAudioParameter&, const Palette&, juce::StringArray labels, int buttonWidth, juce::UndoManager* = nullptr);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;
    void focusGained (FocusChangeType) override { repaint(); }
    void focusLost (FocusChangeType) override { repaint(); }

private:
    Palette palette;
    juce::StringArray items;
    int buttonW;
    juce::ParameterAttachment attachment;
    int selected = 0;
};

/** Botón oscuro con texto, icono o lámpara. `Toggle` mantiene su estado (bypass); los demás son pulsadores. */
class VButton : public juce::Button
{
public:
    enum class Icon { none, previous, next, undo, redo };

    VButton (const Palette&, juce::String text, Icon = Icon::none, bool lamp = false);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    Palette palette;
    juce::String caption;
    Icon icon;
    bool hasLamp;
};

/** Ventana de lectura de cristal: un rótulo pequeño arriba y la cifra grande a la derecha. */
class ReadoutWindow : public juce::Component
{
public:
    ReadoutWindow (const Palette&, juce::String caption);
    void setText (const juce::String& text, std::optional<juce::Colour> colour = std::nullopt);
    void paint (juce::Graphics&) override;

private:
    Palette palette;
    juce::String caption, text;
    std::optional<juce::Colour> colourOverride;
};

/** Ventana de cristal con bisel: las piezas que dibujan dentro (espectros, curvas) heredan de ella y
    solo implementan `paintContent`, con el origen en la esquina de la ventana y el tamaño de la maqueta. */
class GlassView : public juce::Component
{
public:
    static constexpr int margin = 9; // espacio para el bisel, fuera del cristal

    explicit GlassView (const Palette& p, bool plainBezel = false) : palette (p), plain (plainBezel) { setInterceptsMouseClicks (false, false); }

    /** El cristal ocupa `glass`; el bisel queda por fuera. */
    void setGlassBounds (juce::Rectangle<int> glass) { setBounds (glass.expanded (margin)); }

    void paint (juce::Graphics&) final;

protected:
    virtual void paintContent (juce::Graphics&, float width, float height) = 0;
    Palette palette;

private:
    bool plain;
};

/** Texto en relieve: una copia desplazada un píxel hacia abajo y el texto encima. */
void drawEmbossed (juce::Graphics&, const juce::String&, juce::Rectangle<float>, const juce::Font&, juce::Justification,
                   const Palette&, float alpha = 1.0f);
} // namespace ferb::vintage
