#pragma once

#include <Controls.h>
#include <Frame.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"
#include "ui/ChausUi.h"

class ChauSEditor : public ferb::ScaledEditor, private juce::Timer
{
public:
    explicit ChauSEditor (ChauSProcessor&);
    ~ChauSEditor() override;

    static constexpr int baseWidth = 960;
    static constexpr int baseHeight = 580;

private:
    class Captions;

    void timerCallback() override;
    void showAdvancedMenu();
    void setParameter (const char* id, float realValue);

    ChauSProcessor& owner;
    ferb::vintage::Palette palette = ferb::vintage::Palette::chaus();

    std::unique_ptr<ferb::vintage::Frame> frame;
    std::unique_ptr<Captions> captions;
    std::unique_ptr<ferb::vintage::Header> header;
    std::unique_ptr<chaus::ui::ScreenView> screen;
    std::unique_ptr<chaus::ui::TimelineView> timeline;
    std::vector<std::unique_ptr<ferb::vintage::VKnob>> knobs;
    std::vector<std::unique_ptr<ferb::vintage::VSeg>> segments;
    std::vector<std::unique_ptr<ferb::vintage::VButton>> toggles;
    std::vector<std::unique_ptr<juce::ButtonParameterAttachment>> toggleAttachments;
    std::unique_ptr<ferb::vintage::VButton> advancedButton;

    chaus::ui::SpectrumAnalyzer inputAnalyzer, outputAnalyzer;
    chaus::ui::ScreenView::Data screenData;
    std::array<float, chaus::ui::SpectrumAnalyzer::numPoints> rawIn {}, rawOut {};
    std::vector<chaus::Telemetry::Point> points;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChauSEditor)
};
