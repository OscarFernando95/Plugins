#include "PluginProcessor.h"

#include <StateRestore.h>

#include "PluginEditor.h"
#include "Presets.h"

namespace k = maru::params;

MaruProcessor::MaruProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "MARU", k::createLayout())
{
    const std::set<juce::String> excluded { k::bypass };
    presets = std::make_unique<ferb::PresetManager> (apvts, "Maru", maru::factoryPresets(), excluded);
    ab = std::make_unique<ferb::AbCompare> (apvts, excluded);
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);
}

void MaruProcessor::prepareToPlay (double, int) {}

bool MaruProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (out != layouts.getMainInputChannelSet())
        return false;

    // Sidechain externo: apagado, mono o estéreo.
    if (layouts.inputBuses.size() > 1)
    {
        const auto& sc = layouts.inputBuses.getReference (1);
        return sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo();
    }
    return true;
}

void MaruProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    // M0: paso directo. Las salidas sin entrada correspondiente se silencian.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
}

int MaruProcessor::getNumPrograms() { return presets->getFactoryNames().size(); }

int MaruProcessor::getCurrentProgram()
{
    return juce::jmax (0, presets->getFactoryNames().indexOf (presets->getCurrentName()));
}

void MaruProcessor::setCurrentProgram (int index)
{
    const auto names = presets->getFactoryNames();
    if (juce::isPositiveAndBelow (index, names.size()))
        presets->load (names[index]);
}

const juce::String MaruProcessor::getProgramName (int index)
{
    const auto names = presets->getFactoryNames();
    return juce::isPositiveAndBelow (index, names.size()) ? names[index] : juce::String();
}

juce::AudioProcessorEditor* MaruProcessor::createEditor() { return new MaruEditor (*this); }

void MaruProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MaruProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            ferb::syncParametersFromState (apvts);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MaruProcessor(); }
