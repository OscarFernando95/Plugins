#include "PluginEditor.h"

using namespace ferb::vintage;
namespace k = chaus::params;

ChauSEditor::ChauSEditor (ChauSProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75)
{
    backdrop = juce::Colour (0xff0b090d);
    auto& bypass = *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (k::bypass));

    frame = std::make_unique<Frame> (palette, false);
    frame->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*frame);

    Header::Spec headerSpec;
    headerSpec.name = "ChauS";
    headerSpec.subtitle = juce::String::fromUTF8 ("DE-ESSER · EQ DINÁMICO");
    headerSpec.rack = false;
    headerSpec.presetBarX = 236;
    headerSpec.width = baseWidth;
    headerSpec.height = 56;
    header = std::make_unique<Header> (palette, headerSpec, p.getPresets(), p.getAb(), p.undoManager, bypass);
    header->setBounds (0, 0, baseWidth, 56);
    content.addAndMakeVisible (*header);
}
