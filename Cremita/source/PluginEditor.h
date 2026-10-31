#pragma once

#include <ScaledEditor.h>

#include "PluginProcessor.h"

class CremitaEditor : public ferb::ScaledEditor
{
public:
    explicit CremitaEditor (CremitaProcessor&);

    static constexpr int baseWidth = 1260;
    static constexpr int baseHeight = 412;
};
