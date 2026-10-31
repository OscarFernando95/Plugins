#include "BandCard.h"

namespace poty::ui
{
namespace
{
juce::RangedAudioParameter& get (PotyProcessor& p, int band, const char* suffix)
{
    return *dynamic_cast<juce::RangedAudioParameter*> (p.apvts.getParameter (params::bandId (band, suffix)));
}
} // namespace

BandCard::BandCard (PotyProcessor& p, int bandIndex, const int& selectedBand, std::function<void (int)> onSelect)
    : processor (p), band (bandIndex), selected (selectedBand), select (std::move (onSelect)),
      onAttachment (get (p, bandIndex, params::bandOn), [this] (float v) { isOn = v >= 0.5f; repaint(); }),
      freq (get (p, bandIndex, params::bandFreq), [] (float v) { return hz (v); },
            [] (const juce::String& s, float old) { return parseHz (s, old); }),
      gain (get (p, bandIndex, params::bandGain), [] (float v) { return signedDb (v); },
            [] (const juce::String& s, float old) { return parseNumber (s, old); }),
      q (get (p, bandIndex, params::bandQ), [] (float v) { return comma (v, 1); },
         [] (const juce::String& s, float old) { return parseNumber (s, old); }),
      drive (get (p, bandIndex, params::bandDrive), bandColour[size_t (bandIndex)])
{
    onAttachment.sendInitialUpdate();

    addAndMakeVisible (freq);
    addAndMakeVisible (gain);
    addAndMakeVisible (q);
    addAndMakeVisible (drive);

    for (auto name : { "Tubo", "Cinta", "S\xc3\xb3lido", "Diodo Ge", "Digital" })
        type.addItem (juce::String::fromUTF8 (name), type.getNumItems() + 1);
    type.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (type);
    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processor.apvts, params::bandId (band, params::bandType), type);
}

void BandCard::resized()
{
    freq.setBounds (48, 36, 71, 20);
    gain.setBounds (48, 61, 71, 20);
    q.setBounds (48, 86, 71, 20);
    drive.setBounds (13, 130, 106, 14);
    type.setBounds (13, 195, 106, 44);
}

void BandCard::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const bool isSelected = selected == band;
    const auto colour = bandColour[size_t (band)];

    g.setColour (panel);
    g.fillRoundedRectangle (b, 12.0f);
    g.setColour (isSelected ? colour : border);
    g.drawRoundedRectangle (b.reduced (isSelected ? 1.0f : 0.5f), 12.0f, isSelected ? 2.0f : 1.0f);

    // Punto de color: apaga y enciende la banda.
    const juce::Point<float> dot (19.0f, 21.0f);
    g.setColour (colour.withAlpha (isOn ? 1.0f : 0.3f));
    g.fillEllipse (dot.x - 5.0f, dot.y - 5.0f, 10.0f, 10.0f);

    g.setColour (isOn ? text : textDim);
    g.setFont (ui (12.0f, true).withExtraKerningFactor (0.08f));
    g.drawText ("BANDA " + juce::String (band + 1), juce::Rectangle<float> (33.0f, 11.0f, 90.0f, 20.0f), juce::Justification::centredLeft);

    g.setColour (textDim);
    g.setFont (ui (11.0f).withExtraKerningFactor (0.06f));
    const char* labels[] = { "FREQ", "GAN", "Q", "DRIVE" };
    const float ys[] = { 46.0f, 71.0f, 96.0f, 121.0f };
    for (int i = 0; i < 4; ++i)
        g.drawText (labels[i], juce::Rectangle<float> (13.0f, ys[i] - 9.0f, 50.0f, 18.0f), juce::Justification::centredLeft);

    if (! isOn)
    {
        g.setColour (panel.withAlpha (0.45f));
        g.fillRoundedRectangle (b.reduced (1.0f), 12.0f);
    }
}

void BandCard::mouseDown (const juce::MouseEvent& e)
{
    select (band);

    if (e.position.getDistanceFrom ({ 19.0f, 21.0f }) < 12.0f)
    {
        auto& p = get (processor, band, params::bandOn);
        p.beginChangeGesture();
        p.setValueNotifyingHost (isOn ? 0.0f : 1.0f);
        p.endChangeGesture();
    }
}
} // namespace poty::ui
