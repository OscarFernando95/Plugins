#include "PluginEditor.h"

NelEditor::NelEditor (NelProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75)
{
}
