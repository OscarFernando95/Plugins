#include "PluginEditor.h"

CremitaEditor::CremitaEditor (CremitaProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight)
{
}
