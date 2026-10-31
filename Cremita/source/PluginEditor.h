#pragma once

#include <Controls.h>
#include <Frame.h>
#include <Meters.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"
#include "ui/CurveView.h"

class CremitaEditor : public ferb::ScaledEditor, private juce::Timer
{
public:
    explicit CremitaEditor (CremitaProcessor&);
    ~CremitaEditor() override;

    static constexpr int baseWidth = 1260;
    static constexpr int baseHeight = 412;

private:
    class Captions;

    void timerCallback() override;

    CremitaProcessor& owner;
    ferb::vintage::Palette palette = ferb::vintage::Palette::cremita();

    std::unique_ptr<ferb::vintage::Frame> frame;
    std::unique_ptr<Captions> captions;
    std::unique_ptr<ferb::vintage::Header> header;
    std::unique_ptr<ferb::vintage::VuNeedle> vuIn, vuOut;
    std::unique_ptr<cremita::ui::CurveView> curveView;
    std::vector<std::unique_ptr<ferb::vintage::VKnob>> knobs;
    std::vector<std::unique_ptr<ferb::vintage::VLever>> levers;
    std::unique_ptr<ferb::vintage::VSeg> oversample;
    std::unique_ptr<ferb::vintage::ReadoutWindow> effectWindow, overloadWindow;

    double overloadUntil = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CremitaEditor)
};
