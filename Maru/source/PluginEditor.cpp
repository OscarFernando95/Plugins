#include "PluginEditor.h"

using namespace ferb::vintage;
namespace k = maru::params;

MaruEditor::MaruEditor (MaruProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight, 0.75)
{
    backdrop = juce::Colour (0xff0b0d0c);
    auto& bypass = *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (k::bypass));

    frame = std::make_unique<Frame> (palette, true);
    frame->setBounds (0, 0, baseWidth, baseHeight);
    content.addAndMakeVisible (*frame);

    Header::Spec headerSpec;
    headerSpec.name = "Maru";
    headerSpec.subtitle = juce::String::fromUTF8 ("SUPRESOR DE RUIDO · GATE");
    headerSpec.presetBarX = 330;
    headerSpec.width = baseWidth;
    header = std::make_unique<Header> (palette, headerSpec, p.getPresets(), p.getAb(), p.undoManager, bypass);
    header->setBounds (0, 0, baseWidth, 56);
    content.addAndMakeVisible (*header);
}
