#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <AbCompare.h>
#include <PresetManager.h>

#include "Parameters.h"
#include "dsp/ChausEngine.h"

class ChauSProcessor : public juce::AudioProcessor,
                       private juce::AudioProcessorValueTreeState::Listener,
                       private juce::AsyncUpdater
{
public:
    ChauSProcessor();
    ~ChauSProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    /** El bypass del anfitrión (botón del dispositivo) es el mismo parámetro `bypass`. */
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter (chaus::params::bypass); }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Los presets de fábrica aparecen en el menú de presets del anfitrión.
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    chaus::ChausEngine& getEngine() noexcept { return engine; }
    chaus::Settings readSettings() const noexcept;
    ferb::PresetManager& getPresets() noexcept { return *presets; }
    ferb::AbCompare& getAb() noexcept { return *ab; }

private:
    void parameterChanged (const juce::String& id, float value) override;
    void handleAsyncUpdate() override;
    void updateLatency();

    chaus::ChausEngine engine;
    double currentSampleRate = 48000.0;
    int lastLatency = -1;
    std::unique_ptr<ferb::PresetManager> presets;
    std::unique_ptr<ferb::AbCompare> ab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChauSProcessor)
};
