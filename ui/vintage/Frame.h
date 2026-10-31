#pragma once

#include <AbCompare.h>
#include <PresetBar.h>

#include "Controls.h"

namespace ferb::vintage
{
/** Frente del aparato: degradado con grano y rayas finas. Con `rack` lleva orejas (de nogal o metálicas)
    con tornillos; sin él, es un panel con esquinas redondeadas. Se dibuja una vez en una imagen. */
class Frame : public juce::Component
{
public:
    Frame (const Palette&, bool rack, int earWidth = 44);
    void paint (juce::Graphics&) override;

private:
    void paintEar (juce::Graphics&, juce::Rectangle<float>, bool left) const;
    static void paintScrew (juce::Graphics&, juce::Point<float> centre, float slotDegrees);

    Palette palette;
    bool rack;
    int earWidth;
};

/** Cabecera común: logotipo con la placa «BY FERB», subtítulo, selector de presets (anterior, nombre con
    menú, siguiente), A/B, deshacer, rehacer y el bypass del anfitrión. */
class Header : public juce::Component
{
public:
    struct Spec
    {
        juce::String name, subtitle;
        bool rack = true;
        int presetBarX = 330;
        int width = 1260;
        int height = 56;
        float nameSize = 34.0f;
        int displayWidth = 200;
        int buttonHeight = 36;
        bool saveButton = false;
        std::optional<juce::Colour> lineColour; // línea inferior de la cabecera (sin rack)
    };

    Header (const Palette&, Spec, PresetManager&, AbCompare&, juce::UndoManager&, juce::RangedAudioParameter& bypass);
    ~Header() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class PresetDisplay;
    class AbSwitch;
    friend class PresetDisplay;

    Palette palette;
    Spec spec;
    AbCompare& ab;
    juce::UndoManager& undo;

    VButton previous, next, undoButton, redoButton, bypassButton, saveButton;
    std::unique_ptr<PresetDisplay> display;
    std::unique_ptr<AbSwitch> abSwitch;
    juce::ButtonParameterAttachment bypassAttachment;
};
} // namespace ferb::vintage
