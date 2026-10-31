#pragma once

#include <Controls.h>
#include <Frame.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"

class MaruEditor : public ferb::ScaledEditor
{
public:
    explicit MaruEditor (MaruProcessor&);

    static constexpr int baseWidth = 1260;
    static constexpr int baseHeight = 300;

private:
    ferb::vintage::Palette palette = ferb::vintage::Palette::maru();
    std::unique_ptr<ferb::vintage::Frame> frame;
    std::unique_ptr<ferb::vintage::Header> header;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaruEditor)
};
