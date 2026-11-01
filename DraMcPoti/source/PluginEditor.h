#pragma once

#include <Controls.h>
#include <Frame.h>
#include <Meters.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"
#include "ui/Widgets.h"

/** Fase 1: panel provisional con los 44 controles sobre el kit «aparato analógico» compartido.
    Las válvulas, los vúmetros y la ventana de armónicos de la maqueta llegan en la fase 5. */
class DraMcPotiEditor : public ferb::ScaledEditor, private juce::Timer
{
public:
    explicit DraMcPotiEditor (DraMcPotiProcessor&);
    ~DraMcPotiEditor() override;

    static constexpr int baseWidth = 1400;
    static constexpr int baseHeight = 600;

private:
    class Harmonics;

    void timerCallback() override;
    ferb::vintage::VKnob& addKnob (const juce::String& id, const char* label, int size, bool big, int x, int y, int w,
                                   std::function<juce::String (float)> format);
    void addToggle (const juce::String& id, const char* text, int x, int y, int w);
    void addLever (const juce::String& id, mcpoti::ui::ChromeToggle::Spec spec, int x, int y, int w, int h);

    DraMcPotiProcessor& owner;
    ferb::vintage::Palette palette = ferb::vintage::Palette::mcpoti();

    std::unique_ptr<ferb::vintage::Frame> frame;
    std::unique_ptr<ferb::vintage::Header> header;
    std::vector<std::unique_ptr<ferb::vintage::VKnob>> knobs;
    std::vector<std::unique_ptr<ferb::vintage::VSeg>> segments;
    std::vector<std::unique_ptr<ferb::vintage::VButton>> buttons;
    std::vector<std::unique_ptr<mcpoti::ui::ChromeToggle>> levers;
    std::vector<std::unique_ptr<juce::ButtonParameterAttachment>> buttonAttachments;
    std::vector<std::unique_ptr<mcpoti::ui::GlassTube>> tubes;
    std::vector<std::unique_ptr<mcpoti::ui::BandPlate>> plates;
    std::unique_ptr<mcpoti::ui::VoicingKnob> voicing;
    std::unique_ptr<mcpoti::ui::Jewel> jewel;
    std::unique_ptr<Harmonics> harmonics;
    std::unique_ptr<mcpoti::ui::AmberVu> vuIn, vuOut;
    juce::Component *guardKnob = nullptr, *widthKnob = nullptr, *airKnob = nullptr, *monoButton = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraMcPotiEditor)
};
