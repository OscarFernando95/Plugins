#pragma once

#include <PresetBar.h>
#include <ScaledEditor.h>

#include "PluginProcessor.h"
#include "ui/KeyButton.h"
#include "ui/KnobUnit.h"
#include "ui/ScopeView.h"
#include "ui/TubeView.h"
#include "ui/TufiLookAndFeel.h"
#include "ui/VuMeter.h"

class TufiEditor : public ferb::ScaledEditor
{
public:
    explicit TufiEditor (TufiProcessor&);
    ~TufiEditor() override;

    static constexpr int baseWidth = 1080;
    static constexpr int baseHeight = 600;

private:
    class Chassis;

    tufi::ui::TufiLookAndFeel lookAndFeel;
    std::unique_ptr<Chassis> chassis;
    std::unique_ptr<ferb::PresetBar> presetBar;
    std::array<std::unique_ptr<tufi::ui::KeyButton>, 5> keys;
    std::unique_ptr<tufi::ui::ScopeView> scope;
    std::unique_ptr<tufi::ui::VuMeter> vu;
    std::unique_ptr<tufi::ui::TubeView> tube;
    std::array<std::unique_ptr<tufi::ui::KnobUnit>, 7> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TufiEditor)
};
