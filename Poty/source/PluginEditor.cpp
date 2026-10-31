#include "PluginEditor.h"

using namespace poty::ui;

/** Cabecera: marca, nombre, selector de preset, modo y bypass. */
class PotyEditor::Header : public juce::Component
{
public:
    explicit Header (PotyProcessor& p)
        : presetBar (p.getPresets(), presetStyle()),
          mode (*dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (poty::params::mode)),
                juce::StringArray { juce::String::fromUTF8 ("Estéreo"), "Mid", "Side" }, segmentStyle()),
          bypassAttachment (p.apvts, poty::params::bypass, bypass)
    {
        addAndMakeVisible (presetBar);
        mode.setSegmentWidths ({ 78.0f, 52.0f, 56.0f });
        addAndMakeVisible (mode);
        bypass.setButtonText ("BYPASS");
        bypass.setClickingTogglesState (true);
        addAndMakeVisible (bypass);
    }

    void resized() override
    {
        presetBar.setBounds (489, 14, 220, 44);
        mode.setBounds (777, 14, 196, 44);
        bypass.setBounds (991, 14, 89, 44);
    }

    void paint (juce::Graphics& g) override
    {
        // Marca de cinco barras de colores.
        for (size_t i = 0; i < bandColour.size(); ++i)
        {
            g.setColour (bandColour[i]);
            g.fillRoundedRectangle (20.0f + float (i) * 11.0f, 22.0f, 7.0f, 28.0f, 3.5f);
        }

        g.setColour (text);
        g.setFont (ui (23.0f, true).withExtraKerningFactor (0.22f));
        g.drawText ("POTY", juce::Rectangle<float> (88.0f, 12.0f, 90.0f, 30.0f), juce::Justification::centredLeft);

        // Etiqueta del creador.
        const juce::Rectangle<float> pill (178.0f, 16.0f, 75.0f, 22.0f);
        g.setColour (border.brighter (0.25f));
        g.drawRoundedRectangle (pill.reduced (0.5f), 11.0f, 1.0f);
        g.setColour (textDim);
        g.setFont (ui (11.0f, true).withExtraKerningFactor (0.1f));
        g.drawText ("By FERB", pill, juce::Justification::centred);

        g.setColour (textDim);
        g.setFont (ui (11.5f).withExtraKerningFactor (0.16f));
        g.drawText ("ENHANCER MULTIBANDA", juce::Rectangle<float> (88.0f, 40.0f, 260.0f, 18.0f), juce::Justification::centredLeft);

        g.setFont (ui (11.5f, true).withExtraKerningFactor (0.14f));
        g.drawText ("MODO", juce::Rectangle<float> (722.0f, 25.0f, 52.0f, 22.0f), juce::Justification::centredLeft);

        g.setColour (border);
        g.fillRect (0.0f, 71.0f, float (getWidth()), 1.0f);
    }

private:
    static ferb::PresetBar::Style presetStyle()
    {
        ferb::PresetBar::Style s;
        s.background = bg.withAlpha (0.55f);
        s.border = border;
        s.text = text;
        s.textDim = textDim;
        s.accent = accent;
        s.font = ui (13.0f);
        s.boldFont = ui (13.0f, true);
        s.corner = 8.0f;
        return s;
    }

    static ferb::SegmentedControl::Style segmentStyle()
    {
        ferb::SegmentedControl::Style s;
        s.border = border;
        s.text = text;
        s.activeFill = accent;
        s.activeText = bg;
        s.font = ui (13.0f, true);
        s.corner = 8.0f;
        s.gap = 5.0f;
        return s;
    }

    ferb::PresetBar presetBar;
    ferb::SegmentedControl mode;
    juce::ToggleButton bypass;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;
};

PotyEditor::PotyEditor (PotyProcessor& p)
    : ferb::ScaledEditor (p, p.apvts.state, baseWidth, baseHeight)
{
    backdrop = bg;
    setLookAndFeel (&lookAndFeel);

    header = std::make_unique<Header> (p);
    header->setBounds (0, 0, baseWidth, 72);
    content.addAndMakeVisible (*header);

    spectrum = std::make_unique<SpectrumView> (p, selectedBand, [this] (int b) { selectBand (b); });
    spectrum->setBounds (20, 92, 1060, 330);
    content.addAndMakeVisible (*spectrum);

    for (int b = 0; b < poty::params::numBands; ++b)
    {
        cards[size_t (b)] = std::make_unique<BandCard> (p, b, selectedBand, [this] (int idx) { selectBand (idx); });
        cards[size_t (b)]->setBounds (20 + b * 145, 438, 132, 252);
        content.addAndMakeVisible (*cards[size_t (b)]);
    }

    globalPanel = std::make_unique<GlobalPanel> (p);
    globalPanel->setBounds (750, 438, 330, 252);
    content.addAndMakeVisible (*globalPanel);
}

PotyEditor::~PotyEditor()
{
    setLookAndFeel (nullptr);
}

void PotyEditor::selectBand (int band)
{
    selectedBand = band;
    for (auto& c : cards)
        c->repaint();
}
