#pragma once

#include <ScaledEditor.h>

#include "PluginProcessor.h"

class NelEditor : public ferb::ScaledEditor
{
public:
    explicit NelEditor (NelProcessor&);

    static constexpr int baseWidth = 1040;
    static constexpr int baseHeight = 640;
};
