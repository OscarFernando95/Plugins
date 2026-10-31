#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

PotyProcessor::PotyProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "POTY", poty::params::createLayout())
{
    namespace p = poty::params;
    inGainParam = apvts.getRawParameterValue (p::inGain);
    outGainParam = apvts.getRawParameterValue (p::outGain);
    mixParam = apvts.getRawParameterValue (p::mix);
    bypassParam = apvts.getRawParameterValue (p::bypass);
    qualityParam = apvts.getRawParameterValue (p::quality);
    intensityParam = apvts.getRawParameterValue (p::intensity);
    modeParam = apvts.getRawParameterValue (p::mode);

    for (int b = 0; b < p::numBands; ++b)
    {
        auto& bp = bandParams[static_cast<size_t> (b)];
        bp.on = apvts.getRawParameterValue (p::bandId (b, p::bandOn));
        bp.freq = apvts.getRawParameterValue (p::bandId (b, p::bandFreq));
        bp.gain = apvts.getRawParameterValue (p::bandId (b, p::bandGain));
        bp.q = apvts.getRawParameterValue (p::bandId (b, p::bandQ));
        bp.type = apvts.getRawParameterValue (p::bandId (b, p::bandType));
        bp.drive = apvts.getRawParameterValue (p::bandId (b, p::bandDrive));
    }

    inputRing.prepare (32768);
    outputRing.prepare (32768);

    // El bypass no forma parte de un preset. «Brillo vocal» equivale a los valores por defecto.
    presets = std::make_unique<ferb::PresetManager> (apvts, "Poty", poty::factoryPresets(), std::set<juce::String> { p::bypass });
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);
}

void PotyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    monoScratch.assign (static_cast<size_t> (juce::jmax (samplesPerBlock, 1)), 0.0f);

    // Latencia constante en todas las calidades (ver PotyEngine): nunca cambia con la perilla.
    setLatencySamples (poty::PotyEngine::getLatencySamples());
}

poty::Settings PotyProcessor::readSettings() const noexcept
{
    poty::Settings s;
    s.inGainDb = inGainParam->load();
    s.outGainDb = outGainParam->load();
    s.mixPercent = mixParam->load();
    s.bypass = bypassParam->load() >= 0.5f;
    s.quality = juce::roundToInt (qualityParam->load()) + 1; // Buena = 2×, Mejor = 4×, Máxima = 8×
    s.intensity = juce::roundToInt (intensityParam->load());
    s.mode = juce::roundToInt (modeParam->load());

    for (size_t b = 0; b < bandParams.size(); ++b)
    {
        const auto& bp = bandParams[b];
        s.bands[b] = { bp.on->load() >= 0.5f, bp.freq->load(), bp.gain->load(), bp.q->load(),
                       juce::roundToInt (bp.type->load()), bp.drive->load() };
    }
    return s;
}

bool PotyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return out == layouts.getMainInputChannelSet();
}

void PotyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const bool analyze = analyzerActive.load (std::memory_order_relaxed);
    const int n = buffer.getNumSamples();
    auto pushMono = [&] (ferb::SampleRing& ring)
    {
        // Pocos bloques exceden el tamaño declarado; si pasa, el analizador salta ese bloque.
        if (n > static_cast<int> (monoScratch.size()))
            return;
        const int channels = buffer.getNumChannels();
        const float scale = 1.0f / static_cast<float> (juce::jmax (channels, 1));
        for (int i = 0; i < n; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < channels; ++ch)
                sum += buffer.getSample (ch, i);
            monoScratch[static_cast<size_t> (i)] = sum * scale;
        }
        ring.push (monoScratch.data(), n);
    };

    if (analyze)
        pushMono (inputRing);

    engine.process (buffer, readSettings());

    if (analyze)
        pushMono (outputRing);
}

juce::AudioProcessorEditor* PotyProcessor::createEditor()
{
    return new PotyEditor (*this);
}

void PotyProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PotyProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PotyProcessor();
}
