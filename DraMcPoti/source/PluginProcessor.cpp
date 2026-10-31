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
    const auto ptr = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };
    raw.input = ptr (k::input);   raw.output = ptr (k::output); raw.mix = ptr (k::mix);   raw.mode = ptr (k::mode);
    raw.era = ptr (k::era);       raw.voice = ptr (k::voice);   raw.tune = ptr (k::tune); raw.link = ptr (k::link);
    raw.xo1 = ptr (k::xo1);       raw.xo2 = ptr (k::xo2);       raw.xo3 = ptr (k::xo3);   raw.delta = ptr (k::delta);
    raw.os = ptr (k::os);
    raw.air = ptr (k::airCorner);
    for (int b = 0; b < k::numBands; ++b)
    {
        const auto i = static_cast<size_t> (b);
        raw.drive[i] = ptr (k::bandId ("drive", b));
        raw.dyn[i] = ptr (k::bandId ("dyn", b));
        raw.level[i] = ptr (k::bandId ("mix", b));
        raw.on[i] = ptr (k::bandId ("on", b));
        raw.solo[i] = ptr (k::bandId ("solo", b));
    }

    const std::set<juce::String> excluded { k::bypass };
    presets = std::make_unique<ferb::PresetManager> (apvts, "DraMcPoti", mcpoti::factoryPresets(), excluded);
    ab = std::make_unique<ferb::AbCompare> (apvts, excluded);
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);
}

void DraMcPotiProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    setLatencySamples (mcpoti::McPotiEngine::latencySamples()); // constante: la de 8× (la guarda y la fase lineal sumarán la suya después)
}

mcpoti::Settings DraMcPotiProcessor::readSettings() const noexcept
{
    mcpoti::Settings s;
    s.inputDb = raw.input->load();
    s.outputDb = raw.output->load();
    s.mixPercent = raw.mix->load();
    s.master = juce::roundToInt (raw.mode->load()) == k::Master;
    s.era = juce::roundToInt (raw.era->load());
    s.voice = juce::roundToInt (raw.voice->load());
    s.tuneHz = raw.tune->load();
    s.linkPercent = raw.link->load();
    s.xo1 = raw.xo1->load();
    s.xo2 = raw.xo2->load();
    s.xo3 = raw.xo3->load();
    s.delta = raw.delta->load() >= 0.5f;
    s.osIndex = juce::roundToInt (raw.os->load());
    s.airCornerHz = raw.air->load();
    for (size_t b = 0; b < k::numBands; ++b)
    {
        auto& band = s.bands[b];
        band.drive = raw.drive[b]->load();
        band.dyn = raw.dyn[b]->load();
        band.level = raw.level[b]->load();
        band.on = raw.on[b]->load() >= 0.5f;
        band.solo = raw.solo[b]->load() >= 0.5f;
    }
    return s;
}

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
    engine.process (buffer, readSettings());
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
