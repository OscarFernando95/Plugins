#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

TufiProcessor::TufiProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "TUFI", tufi::params::createLayout())
{
    namespace p = tufi::params;
    stageParam = apvts.getRawParameterValue (p::stage);
    oversampleParam = apvts.getRawParameterValue (p::oversample);
    driveParam = apvts.getRawParameterValue (p::drive);
    trimParam = apvts.getRawParameterValue (p::trim);
    lowParam = apvts.getRawParameterValue (p::lowAmount);
    peakParam = apvts.getRawParameterValue (p::peakAmount);
    hardnessParam = apvts.getRawParameterValue (p::peakHardness);
    evenTopParam = apvts.getRawParameterValue (p::evenTop);
    evenBottomParam = apvts.getRawParameterValue (p::evenBottom);
    bypassParam = apvts.getRawParameterValue (p::bypass);

    // El bypass no forma parte de un preset. «Triodo cálido» equivale a los valores por defecto.
    presets = std::make_unique<ferb::PresetManager> (apvts, "Tufi", tufi::factoryPresets(), std::set<juce::String> { p::bypass });
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);
}

void TufiProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);

    // Latencia constante en todos los modos de sobremuestreo (ver TufiEngine).
    setLatencySamples (tufi::TufiEngine::getLatencySamples());
}

tufi::Settings TufiProcessor::readSettings() const noexcept
{
    tufi::Settings s;
    s.stage = juce::roundToInt (stageParam->load());
    s.oversample = juce::roundToInt (oversampleParam->load());
    s.driveDb = driveParam->load();
    s.trimDb = trimParam->load();
    s.lowAmount = lowParam->load();
    s.peakAmount = peakParam->load();
    s.peakHardness = hardnessParam->load();
    s.evenTop = evenTopParam->load();
    s.evenBottom = evenBottomParam->load();
    s.bypass = bypassParam->load() >= 0.5f;
    return s;
}

bool TufiProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void TufiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    engine.process (buffer, readSettings());
}

juce::AudioProcessorEditor* TufiProcessor::createEditor()
{
    return new TufiEditor (*this);
}

void TufiProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void TufiProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TufiProcessor();
}
