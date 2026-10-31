#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PresetManager.h"

namespace ferb
{
/** Selector de presets: `‹  Preset · Nombre ▾  ›`. Clic en el centro abre el menú (fábrica, usuario,
    guardar, guardar como, eliminar, abrir carpeta); las flechas pasan al anterior y al siguiente. */
class PresetBar : public juce::Component
{
public:
    struct Style
    {
        juce::Colour background, border, text, textDim, accent;
        juce::Font font { juce::FontOptions (13.0f) };
        juce::Font boldFont { juce::FontOptions (13.0f, juce::Font::bold) };
        float corner = 8.0f;
    };

    PresetBar (PresetManager& manager, Style s);
    ~PresetBar() override;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

protected:
    enum MenuId { save = 1, saveAs, remove, reveal, factoryBase = 1000, userBase = 2000 };

    void showMenu();
    void askName (const juce::String& initial);
    void commit (const juce::String& name);

    PresetManager& manager;
    Style style;
};
} // namespace ferb
