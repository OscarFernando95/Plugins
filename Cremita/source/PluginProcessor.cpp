#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

CremitaProcessor::CremitaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undoManager, "CREMITA", cremita::params::createLayout())
{
    namespace p = cremita::params;
    inputParam = apvts.getRawParameterValue (p::input);
    effectParam = apvts.getRawParameterValue (p::effect);
    curveParam = apvts.getRawParameterValue (p::curve);
    biasParam = apvts.getRawParameterValue (p::bias);
    splitParam = apvts.getRawParameterValue (p::split);
    xo1Param = apvts.getRawParameterValue (p::xo1);
    xo2Param = apvts.getRawParameterValue (p::xo2);
    trimParams = { apvts.getRawParameterValue (p::trimLF), apvts.getRawParameterValue (p::trimMF),
                   apvts.getRawParameterValue (p::trimHF) };
    clipParam = apvts.getRawParameterValue (p::clip);
    msParam = apvts.getRawParameterValue (p::ms);
    osParam = apvts.getRawParameterValue (p::os);
    matchParam = apvts.getRawParameterValue (p::match);
    outputParam = apvts.getRawParameterValue (p::output);
    inOnParam = apvts.getRawParameterValue (p::inOn);
    bypassParam = apvts.getRawParameterValue (p::bypass);

    // El bypass no forma parte de un preset. «Init» equivale a los valores por defecto.
    const std::set<juce::String> excluded { p::bypass };
    presets = std::make_unique<ferb::PresetManager> (apvts, "Cremita", cremita::factoryPresets(), excluded);
    ab = std::make_unique<ferb::AbCompare> (apvts, excluded);
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);
}

void CremitaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);

    // Latencia constante en todos los sobremuestreos (ver CremitaEngine): nunca cambia con la perilla.
    setLatencySamples (cremita::CremitaEngine::getLatencySamples());
}

cremita::Settings CremitaProcessor::readSettings() const noexcept
{
    cremita::Settings s;
    s.inputDb = inputParam->load();
    s.effectPercent = effectParam->load();
    s.curve = curveParam->load();
    s.biasPercent = biasParam->load();
    s.split = splitParam->load() >= 0.5f;
    s.crossover1 = xo1Param->load();
    s.crossover2 = xo2Param->load();
    for (size_t b = 0; b < trimParams.size(); ++b)
        s.trimDb[b] = trimParams[b]->load();
    s.clip = clipParam->load() >= 0.5f;
    s.midSide = msParam->load() >= 0.5f;
    s.oversample = juce::roundToInt (osParam->load());
    s.match = matchParam->load() >= 0.5f;
    s.outputDb = outputParam->load();
    s.inOn = inOnParam->load() >= 0.5f;
    s.bypass = bypassParam->load() >= 0.5f;
    return s;
}

bool CremitaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void CremitaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    engine.process (buffer, readSettings());
}

juce::AudioProcessorEditor* CremitaProcessor::createEditor()
{
    return new CremitaEditor (*this);
}

void CremitaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void CremitaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CremitaProcessor();
}
