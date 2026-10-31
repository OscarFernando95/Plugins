#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace k = nel::params;

NelProcessor::NelProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "NEL", k::createLayout())
{
    const std::set<juce::String> excluded { k::bypass };
    presets = std::make_unique<ferb::PresetManager> (apvts, "Nel", nel::factoryPresets(), excluded);
    ab = std::make_unique<ferb::AbCompare> (apvts, excluded);
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);

    // Cambian la latencia: sobremuestreo, fase, lookahead y la mejora de render.
    for (auto* id : { k::oversample, k::osPhase, k::lookahead, k::osOffline })
        apvts.addParameterListener (id, this);
}

NelProcessor::~NelProcessor()
{
    for (auto* id : { k::oversample, k::osPhase, k::lookahead, k::osOffline })
        apvts.removeParameterListener (id, this);
    cancelPendingUpdate();
}

void NelProcessor::parameterChanged (const juce::String&, float)
{
    triggerAsyncUpdate(); // el anfitrión puede cambiar parámetros desde el hilo de audio
}

void NelProcessor::handleAsyncUpdate() { updateLatency(); }

void NelProcessor::updateLatency()
{
    const int latency = nel::NelEngine::latencyFor (readSettings(), currentSampleRate);
    if (latency != lastLatency)
    {
        lastLatency = latency;
        setLatencySamples (latency);
    }
}

void NelProcessor::setNonRealtime (bool isNonRealtime) noexcept
{
    AudioProcessor::setNonRealtime (isNonRealtime);
    triggerAsyncUpdate();
}

void NelProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock);
    lastLatency = -1;
    updateLatency();
}

nel::Settings NelProcessor::readSettings() const noexcept
{
    const auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    nel::Settings s;
    s.gainDb = raw (k::gain);
    s.ceilingDb = raw (k::ceiling);
    s.mode = juce::roundToInt (raw (k::mode));
    s.characterPercent = raw (k::character);
    s.lookaheadMs = raw (k::lookahead);
    s.attackMs = raw (k::attack);
    s.releaseMs = raw (k::release);
    s.releaseAuto = raw (k::releaseAuto) >= 0.5f;
    s.kneeDb = raw (k::knee);
    s.linkTransient = raw (k::linkTransient);
    s.linkRelease = raw (k::linkRelease);
    s.truePeak = raw (k::truePeak) >= 0.5f;
    s.oversample = juce::roundToInt (raw (k::oversample));
    s.minimumPhase = juce::roundToInt (raw (k::osPhase)) == k::Minimum;
    s.offlineBoost = raw (k::osOffline) >= 0.5f;
    s.ditherBits = k::ditherBitsFor[juce::jlimit (0, 5, juce::roundToInt (raw (k::ditherBits)))];
    s.ditherShape = juce::roundToInt (raw (k::ditherShape));
    s.dcFilter = raw (k::dcFilter) >= 0.5f;
    s.unityGain = raw (k::unityGain) >= 0.5f;
    s.audition = raw (k::audition) >= 0.5f;
    s.bypass = raw (k::bypass) >= 0.5f;
    s.nonRealtime = isNonRealtime();
    s.loudnessFromInput = loudnessFromInput.load (std::memory_order_relaxed);
    return s;
}

bool NelProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void NelProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    engine.process (buffer, readSettings());
}

int NelProcessor::getNumPrograms() { return presets->getFactoryNames().size(); }

int NelProcessor::getCurrentProgram()
{
    return juce::jmax (0, presets->getFactoryNames().indexOf (presets->getCurrentName()));
}

void NelProcessor::setCurrentProgram (int index)
{
    const auto names = presets->getFactoryNames();
    if (juce::isPositiveAndBelow (index, names.size()))
        presets->load (names[index]);
}

const juce::String NelProcessor::getProgramName (int index)
{
    const auto names = presets->getFactoryNames();
    return juce::isPositiveAndBelow (index, names.size()) ? names[index] : juce::String();
}

void NelProcessor::selectMode (int mode)
{
    mode = juce::jlimit (0, nel::numModes - 1, mode);
    const auto& m = nel::modes[static_cast<size_t> (mode)];
    const auto set = [this] (const char* id, float real)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (real));
    };
    set (k::mode, static_cast<float> (mode));
    set (k::lookahead, m.lookaheadMs);
    set (k::attack, m.attackMs);
    set (k::release, m.releaseMs);
    set (k::knee, m.kneeDb);
}

bool NelProcessor::isModeModified() const
{
    const auto& m = nel::modes[static_cast<size_t> (juce::jlimit (0, nel::numModes - 1, juce::roundToInt (apvts.getRawParameterValue (k::mode)->load())))];
    const auto differs = [this] (const char* id, float expected, float tolerance)
    {
        return std::abs (apvts.getRawParameterValue (id)->load() - expected) > tolerance;
    };
    return differs (k::lookahead, m.lookaheadMs, 0.06f) || differs (k::attack, m.attackMs, 0.06f * std::max (1.0f, m.attackMs * 0.05f))
           || differs (k::release, m.releaseMs, 0.6f) || differs (k::knee, m.kneeDb, 0.06f);
}

void NelProcessor::setLoudnessFromInput (bool fromInput) noexcept
{
    loudnessFromInput.store (fromInput, std::memory_order_relaxed);
    apvts.state.setProperty (k::uiLoudnessSource, fromInput ? 1 : 0, nullptr);
}

juce::AudioProcessorEditor* NelProcessor::createEditor() { return new NelEditor (*this); }

void NelProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void NelProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            loudnessFromInput.store (static_cast<int> (apvts.state.getProperty (k::uiLoudnessSource, 0)) != 0);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NelProcessor(); }
