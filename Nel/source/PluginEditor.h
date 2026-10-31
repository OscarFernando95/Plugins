#pragma once

#include <Controls.h>
#include <Frame.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"
#include "ui/NelUi.h"

class NelEditor : public ferb::ScaledEditor, private juce::Timer
{
public:
    explicit NelEditor (NelProcessor&);
    ~NelEditor() override;

    static constexpr int baseWidth = 1040;
    static constexpr int baseHeight = 640;

private:
    class Captions;
    class SpanButton;

    void timerCallback() override;
    void cycleScale();
    void cycleSpan();
    void refreshScope();

    NelProcessor& owner;
    ferb::vintage::Palette palette = ferb::vintage::Palette::nel();

    std::unique_ptr<ferb::vintage::Frame> frame;
    std::unique_ptr<Captions> captions;
    std::unique_ptr<ferb::vintage::Header> header;
    std::unique_ptr<nel::ui::LadderMeter> inLadder, outLadder;
    std::unique_ptr<nel::ui::ScopeView> scope;
    std::unique_ptr<SpanButton> spanButton;
    std::unique_ptr<nel::ui::ReadoutStrip> strip;
    std::unique_ptr<nel::ui::GrMeter> grMeter;
    std::unique_ptr<nel::ui::LoudnessPanel> loudness;
    std::unique_ptr<nel::ui::ModeSelector> modes;
    std::vector<std::unique_ptr<ferb::vintage::VKnob>> knobs;
    ferb::vintage::VKnob* releaseKnob = nullptr;
    std::vector<std::unique_ptr<nel::ui::LampToggle>> toggles;
    std::unique_ptr<nel::ui::LampToggle> renderToggle;
    std::unique_ptr<ferb::vintage::VSeg> oversample, phase, ditherBits, ditherShape;

    // Estado de los medidores (se actualiza a 30 Hz).
    nel::ui::MeterState meters;
    std::array<std::array<float, 2>, 2> holdAge {};
    float grMax = 0.0f, vuGr = 0.0f, tpMax = -100.0f;
    bool lastReleaseAuto = true;
    int lastMode = -1;
    bool lastModified = false;
    int historyTicks = 0, lastSpan = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NelEditor)
};
