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
    raw.phase = ptr (k::phase);   raw.color = ptr (k::color); raw.soften = ptr (k::soften);
    raw.width = ptr (k::width);   raw.monoSafe = ptr (k::monoSafe);
    raw.linPhase = ptr (k::linPhase);
    raw.guard = ptr (k::guard);   raw.match = ptr (k::match);
    for (int b = 0; b < k::numBands; ++b)
    {
        const auto i = static_cast<size_t> (b);
        raw.drive[i] = ptr (k::bandId ("drive", b));
        raw.dyn[i] = ptr (k::bandId ("dyn", b));
        raw.level[i] = ptr (k::bandId ("mix", b));
        raw.on[i] = ptr (k::bandId ("on", b));
        raw.solo[i] = ptr (k::bandId ("solo", b));
    }

    inputRing.prepare (32768); // un editor puede abrirse antes de prepareToPlay
    outputRing.prepare (32768);
    apvts.addParameterListener (k::linPhase, this); // la fase lineal cambia la latencia
    apvts.addParameterListener (k::mode, this);     // el modo aplica su plantilla

    const std::set<juce::String> excluded { k::bypass };
    presets = std::make_unique<ferb::PresetManager> (apvts, "DraMcPoti", mcpoti::factoryPresets(), excluded);
    ab = std::make_unique<ferb::AbCompare> (apvts, excluded);
    if (apvts.state.getProperty ("presetName").toString().isEmpty())
        apvts.state.setProperty ("presetName", presets->getFactoryNames()[0], nullptr);
}

DraMcPotiProcessor::~DraMcPotiProcessor()
{
    apvts.removeParameterListener (k::linPhase, this);
    apvts.removeParameterListener (k::mode, this);
    cancelPendingUpdate();
}

void DraMcPotiProcessor::updateLatency()
{
    // Constante (la de 8×) con cualquier sobremuestreo o carácter; solo la fase lineal la cambia.
    const int latency = mcpoti::McPotiEngine::latencySamples (raw.linPhase->load() >= 0.5f);
    if (latency != lastLatency)
    {
        lastLatency = latency;
        setLatencySamples (latency);
        updateHostDisplay (juce::AudioProcessorListener::ChangeDetails().withLatencyChanged (true));
    }
}

void DraMcPotiProcessor::handleAsyncUpdate()
{
    updateLatency();
    syncModeTemplate();
}

namespace
{
/** Plantilla de cada modo (sección 5 de la ficha). Valores iniciales: se afinan de oído. */
struct ModeTemplate
{
    float tune, guard, width, monoSafe, xo1, xo2, xo3, os;
};
constexpr ModeTemplate vocalTemplate { 4000.0f, 60.0f, 100.0f, 0.0f, 150.0f, 1800.0f, 7000.0f, 2.0f };
constexpr ModeTemplate masterTemplate { 5000.0f, 0.0f, 100.0f, 1.0f, 120.0f, 1600.0f, 6500.0f, 2.0f };
} // namespace

void DraMcPotiProcessor::syncModeTemplate()
{
    const int mode = juce::roundToInt (raw.mode->load());
    if (mode == lastMode)
        return;
    const auto& from = lastMode == k::Master ? masterTemplate : vocalTemplate;
    const auto& to = mode == k::Master ? masterTemplate : vocalTemplate;
    lastMode = mode;

    const auto move = [this] (const char* id, float oldDefault, float newValue)
    {
        auto* param = apvts.getParameter (id);
        const float current = param->convertFrom0to1 (param->getValue());
        const auto range = apvts.getParameterRange (id);
        const float tolerance = std::max (0.5f * range.interval, 1.0e-3f * std::abs (oldDefault)); // sin tocar = sigue en la plantilla anterior (medio paso: un paso entero ya es un valor distinto)
        if (std::abs (current - oldDefault) <= tolerance)
            param->setValueNotifyingHost (param->convertTo0to1 (newValue));
    };
    move (k::tune, from.tune, to.tune);
    move (k::guard, from.guard, to.guard);
    move (k::width, from.width, to.width);
    move (k::monoSafe, from.monoSafe, to.monoSafe);
    move (k::xo1, from.xo1, to.xo1);
    move (k::xo2, from.xo2, to.xo2);
    move (k::xo3, from.xo3, to.xo3);
    move (k::os, from.os, to.os);
}

void DraMcPotiProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    lastLatency = -1;
    updateLatency();
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
    s.phasePercent = raw.phase->load();
    s.colorPercent = raw.color->load();
    s.softenHz = raw.soften->load();
    s.widthPercent = raw.width->load();
    s.monoSafe = raw.monoSafe->load() >= 0.5f;
    s.guardPercent = raw.guard->load();
    s.match = raw.match->load() >= 0.5f;
    s.linearPhase = raw.linPhase->load() >= 0.5f;
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

void DraMcPotiProcessor::pushMono (const juce::AudioBuffer<float>& buffer, ferb::SampleRing& ring) noexcept
{
    const int channels = buffer.getNumChannels(), total = buffer.getNumSamples();
    if (channels == 0)
        return;
    for (int start = 0; start < total; start += static_cast<int> (monoScratch.size()))
    {
        const int n = std::min (static_cast<int> (monoScratch.size()), total - start);
        for (int i = 0; i < n; ++i)
        {
            float sum = 0.0f;
            for (int ch = 0; ch < channels; ++ch)
                sum += buffer.getReadPointer (ch)[start + i];
            monoScratch[static_cast<size_t> (i)] = sum / static_cast<float> (channels);
        }
        ring.push (monoScratch.data(), n);
    }
}

void DraMcPotiProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());
    pushMono (buffer, inputRing);
    engine.process (buffer, readSettings());
    pushMono (buffer, outputRing);
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
            lastMode = juce::roundToInt (raw.mode->load()); // una sesión restaurada trae sus valores: no se les aplica plantilla
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DraMcPotiProcessor(); }
