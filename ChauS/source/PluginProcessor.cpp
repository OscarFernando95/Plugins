#include "PluginProcessor.h"

#include <StateRestore.h>

#include "PluginEditor.h"
#include "Presets.h"

namespace k = chaus::params;

ChauSProcessor::ChauSProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withInput ("Sidechain", juce::AudioChannelSet::stereo(), false)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "CHAUS", k::createLayout())
{
    const std::set<juce::String> excluded { k::bypass };
    presets = std::make_unique<ferb::PresetManager> (apvts, "ChauS", chaus::factoryPresets(), excluded);
    ab = std::make_unique<ferb::AbCompare> (apvts, excluded);
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);

    // Cambian la latencia: el lookahead y su interruptor.
    for (auto* id : { k::look, k::lookOn })
        apvts.addParameterListener (id, this);
}

ChauSProcessor::~ChauSProcessor()
{
    for (auto* id : { k::look, k::lookOn })
        apvts.removeParameterListener (id, this);
    cancelPendingUpdate();
}

void ChauSProcessor::parameterChanged (const juce::String&, float)
{
    triggerAsyncUpdate(); // el anfitrión puede cambiar parámetros desde el hilo de audio
}

void ChauSProcessor::handleAsyncUpdate() { updateLatency(); }

void ChauSProcessor::updateLatency()
{
    const int latency = chaus::ChausEngine::latencyFor (readSettings(), currentSampleRate);
    if (latency != lastLatency)
    {
        lastLatency = latency;
        setLatencySamples (latency);
    }
}

void ChauSProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock);
    lastLatency = -1;
    updateLatency();
}

chaus::Settings ChauSProcessor::readSettings() const noexcept
{
    const auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    const auto on = [&raw] (const char* id) { return raw (id) >= 0.5f; };
    chaus::Settings s;
    s.voiceMode = juce::roundToInt (raw (k::mode)) == k::VoiceOnly;
    s.thresholdDb = raw (k::thr);
    s.adaptive = on (k::adaptive);
    s.rangeDb = raw (k::range);
    s.hpHz = raw (k::hp);
    s.lpHz = raw (k::lp);
    s.track = on (k::track);
    s.qAuto = on (k::qAuto);
    s.q = raw (k::q);
    s.wideBand = juce::roundToInt (raw (k::band)) == k::Wide;
    s.shape = static_cast<chaus::Shape> (juce::jlimit (0, 2, juce::roundToInt (raw (k::shape))));
    s.sensSoftDb = raw (k::sensSoft);
    s.sensHardDb = raw (k::sensHard);
    s.lookaheadMs = on (k::lookOn) ? raw (k::look) : 0.0f;
    s.releaseAuto = on (k::releaseAuto);
    s.releaseMs = raw (k::release);
    s.linkPercent = raw (k::link);
    s.stereo = static_cast<chaus::Stereo> (juce::jlimit (0, 2, juce::roundToInt (raw (k::ms))));
    s.externalSidechain = on (k::scExt);
    s.listenTrigger = on (k::listenTrig);
    s.listenSidechain = on (k::listenSC);
    s.outputDb = raw (k::output);
    s.bypass = on (k::bypass);
    return s;
}

bool ChauSProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void ChauSProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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

int ChauSProcessor::getNumPrograms() { return presets->getFactoryNames().size(); }

int ChauSProcessor::getCurrentProgram()
{
    return juce::jmax (0, presets->getFactoryNames().indexOf (presets->getCurrentName()));
}

void ChauSProcessor::setCurrentProgram (int index)
{
    const auto names = presets->getFactoryNames();
    if (juce::isPositiveAndBelow (index, names.size()))
        presets->load (names[index]);
}

const juce::String ChauSProcessor::getProgramName (int index)
{
    const auto names = presets->getFactoryNames();
    return juce::isPositiveAndBelow (index, names.size()) ? names[index] : juce::String();
}

juce::AudioProcessorEditor* ChauSProcessor::createEditor() { return new ChauSEditor (*this); }

void ChauSProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void ChauSProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            ferb::syncParametersFromState (apvts);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChauSProcessor(); }
