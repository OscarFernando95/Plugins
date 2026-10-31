#include "PluginProcessor.h"
#include <StateRestore.h>
#include "PluginEditor.h"
#include "Presets.h"

namespace k = mcpoti::params;

DraMcPotiProcessor::DraMcPotiProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "MCPOTI", k::createLayout())
{
    const std::set<juce::String> excluded { k::bypass };
    presets = std::make_unique<ferb::PresetManager> (apvts, "DraMcPoti", mcpoti::factoryPresets(), excluded);
    ab = std::make_unique<ferb::AbCompare> (apvts, excluded);
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);
}

void DraMcPotiProcessor::prepareToPlay (double, int) { setLatencySamples (0); }

bool DraMcPotiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void DraMcPotiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    // Fase 1: sin DSP, el audio pasa tal cual.
}

int DraMcPotiProcessor::getNumPrograms() { return presets->getFactoryNames().size(); }

int DraMcPotiProcessor::getCurrentProgram()
{
    return juce::jmax (0, presets->getFactoryNames().indexOf (presets->getCurrentName()));
}

void DraMcPotiProcessor::setCurrentProgram (int index)
{
    const auto names = presets->getFactoryNames();
    if (juce::isPositiveAndBelow (index, names.size()))
        presets->load (names[index]);
}

const juce::String DraMcPotiProcessor::getProgramName (int index)
{
    const auto names = presets->getFactoryNames();
    return juce::isPositiveAndBelow (index, names.size()) ? names[index] : juce::String();
}

juce::AudioProcessorEditor* DraMcPotiProcessor::createEditor() { return new DraMcPotiEditor (*this); }

void DraMcPotiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DraMcPotiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            ferb::syncParametersFromState (apvts);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DraMcPotiProcessor(); }
