#pragma once

#include <Controls.h>
#include <Frame.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"

class ChauSEditor : public ferb::ScaledEditor
{
public:
    explicit ChauSEditor (ChauSProcessor&);

    static constexpr int baseWidth = 960;
    static constexpr int baseHeight = 580;

private:
    ferb::vintage::Palette palette = ferb::vintage::Palette::chaus();
    std::unique_ptr<ferb::vintage::Frame> frame;
    std::unique_ptr<ferb::vintage::Header> header;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChauSEditor)
};
