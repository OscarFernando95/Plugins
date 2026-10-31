#pragma once

#include <Controls.h>
#include <Frame.h>
#include <Meters.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"
#include "ui/MaruUi.h"

class MaruEditor : public ferb::ScaledEditor, private juce::Timer
{
public:
    explicit MaruEditor (MaruProcessor&);
    ~MaruEditor() override;

    static constexpr int baseWidth = 1260;
    static constexpr int baseHeight = 300;

private:
    class Captions;

    void timerCallback() override;
    void layoutForEngine();

    MaruProcessor& owner;
    ferb::vintage::Palette palette = ferb::vintage::Palette::maru();

    std::unique_ptr<ferb::vintage::Frame> frame;
    std::unique_ptr<Captions> captions;
    std::unique_ptr<ferb::vintage::Header> header;
    std::unique_ptr<maru::ui::FloorView> floorView;
    std::unique_ptr<ferb::vintage::VuNeedle> vu;
    std::unique_ptr<maru::ui::JewelLamp> lamp;
    std::unique_ptr<ferb::vintage::ReadoutWindow> floorWindow, reductionWindow;
    std::unique_ptr<ferb::vintage::VSeg> latencySeg;
    std::unique_ptr<ferb::vintage::VKnob> amountKnob, linkKnob;
    std::vector<std::unique_ptr<ferb::vintage::VLever>> levers;
    std::vector<std::unique_ptr<ferb::vintage::VButton>> toggles;
    std::vector<std::unique_ptr<juce::ButtonParameterAttachment>> toggleAttachments;
    std::vector<std::unique_ptr<ferb::vintage::VKnob>> knobs;
    std::unique_ptr<juce::ParameterAttachment> engineWatch, autoWatch;

    // Controles que cambian con el motor.
    std::vector<ferb::vintage::VKnob*> suppressionOnly, gateOnly;
    std::vector<ferb::vintage::VButton*> gateButtons;
    ferb::vintage::VKnob* thresholdKnob = nullptr;

    bool gateEngine = false;
    float vuValue = -20.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaruEditor)
};
