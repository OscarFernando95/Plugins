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

    // Cambian la latencia: el motor, el lookahead y la calidad.
    for (auto* id : { k::engine, k::lookahead, k::latency })
        apvts.addParameterListener (id, this);
}

MaruProcessor::~MaruProcessor()
{
    for (auto* id : { k::engine, k::lookahead, k::latency })
        apvts.removeParameterListener (id, this);
    cancelPendingUpdate();
}

void MaruProcessor::parameterChanged (const juce::String&, float)
{
    triggerAsyncUpdate(); // el anfitrión puede cambiar parámetros desde el hilo de audio
}

void MaruProcessor::handleAsyncUpdate() { updateLatency(); }

void MaruProcessor::updateLatency()
{
    const int latency = maru::MaruEngine::latencyFor (readSettings(), currentSampleRate);
    if (latency != lastLatency)
    {
        lastLatency = latency;
        setLatencySamples (latency);
    }
}

void MaruProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock);
    lastLatency = -1;
    updateLatency();
}

maru::Settings MaruProcessor::readSettings() const noexcept
{
    const auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const auto on = [&raw] (const char* id) { return raw (id) >= 0.5f; };
    maru::Settings s;
    s.amount = raw (k::amount) * 0.01f;
    s.gate = juce::roundToInt (raw (k::engine)) == k::Gate;
    s.rangeDb = -raw (k::range);
    s.sens = raw (k::sens);
    s.recoveryMs = raw (k::recovery);
    s.gateAuto = on (k::gateAuto);
    s.gateThreshDb = raw (k::gateThresh);
    s.attackMs = raw (k::attack);
    s.holdMs = raw (k::hold);
    s.hystDb = raw (k::hyst);
    s.lookaheadMs = raw (k::lookahead);
    s.scHpHz = raw (k::scHP);
    s.scLpHz = raw (k::scLP);
    s.scExt = on (k::scExt);
    s.linkPercent = raw (k::link);
    s.lowLatency = juce::roundToInt (raw (k::latency)) == k::Low;
    s.listen = on (k::listen);
    s.outputDb = raw (k::output);
    s.bypass = on (k::bypass);
    return s;
}

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
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    auto main = getBusBuffer (buffer, false, 0);
    const bool sidechainOn = getBusCount (true) > 1 && getBus (true, 1)->isEnabled();
    if (sidechainOn)
    {
        auto sidechain = getBusBuffer (buffer, true, 1);
        engine.process (main, &sidechain, readSettings());
    }
    else
        engine.process (main, nullptr, readSettings());
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
