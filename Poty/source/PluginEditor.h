#pragma once

#include <PresetBar.h>
#include <ScaledEditor.h>
#include <SegmentedControl.h>

#include "PluginProcessor.h"
#include "ui/BandCard.h"
#include "ui/GlobalPanel.h"
#include "ui/PotyLookAndFeel.h"
#include "ui/SpectrumView.h"

class PotyEditor : public ferb::ScaledEditor
{
public:
    explicit PotyEditor (PotyProcessor&);
    ~PotyEditor() override;

    static constexpr int baseWidth = 1100;
    static constexpr int baseHeight = 710;

private:
    class Header;

    void selectBand (int band);

    poty::ui::PotyLookAndFeel lookAndFeel;
    int selectedBand = 3; // la banda 4, como en el bosquejo

    std::unique_ptr<Header> header;
    std::unique_ptr<poty::ui::SpectrumView> spectrum;
    std::array<std::unique_ptr<poty::ui::BandCard>, poty::params::numBands> cards;
    std::unique_ptr<poty::ui::GlobalPanel> globalPanel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PotyEditor)
};
