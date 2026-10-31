#pragma once

#include <Controls.h>
#include <Frame.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"

/** Fase 1: panel provisional con los 44 controles sobre el kit «aparato analógico» compartido.
    Las válvulas, los vúmetros y la ventana de armónicos de la maqueta llegan en la fase 5. */
class DraMcPotiEditor : public ferb::ScaledEditor
{
public:
    explicit DraMcPotiEditor (DraMcPotiProcessor&);

    static constexpr int baseWidth = 1400;
    static constexpr int baseHeight = 540;

private:
    ferb::vintage::VKnob& addKnob (const juce::String& id, const char* label, int size, bool big, int x, int y, int w,
                                   std::function<juce::String (float)> format);
    void addToggle (const juce::String& id, const char* text, int x, int y, int w);

    DraMcPotiProcessor& owner;
    ferb::vintage::Palette palette = ferb::vintage::Palette::cremita();

    std::unique_ptr<ferb::vintage::Frame> frame;
    std::unique_ptr<ferb::vintage::Header> header;
    std::vector<std::unique_ptr<ferb::vintage::VKnob>> knobs;
    std::vector<std::unique_ptr<ferb::vintage::VSeg>> segments;
    std::vector<std::unique_ptr<ferb::vintage::VButton>> buttons;
    std::vector<std::unique_ptr<juce::ButtonParameterAttachment>> buttonAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraMcPotiEditor)
};
