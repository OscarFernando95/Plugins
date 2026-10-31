#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Parameters.h"
#include <PresetManager.h>

#include "dsp/TufiEngine.h"

class TufiProcessor : public juce::AudioProcessor
{
public:
    TufiProcessor();
    ~TufiProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    /** El bypass del anfitrión (botón del dispositivo) es el mismo parámetro `bypass`. */
    juce::AudioProcessorParameter* getBypassParameter() const override
    {
        return apvts.getParameter (tufi::params::bypass);
    }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    ferb::LevelMeter& getOutputMeter() noexcept { return engine.getMeter(); }
    ferb::PresetManager& getPresets() noexcept { return *presets; }

private:
    tufi::Settings readSettings() const noexcept;

    tufi::TufiEngine engine;
    std::unique_ptr<ferb::PresetManager> presets;

    // Punteros a los valores atómicos de los parámetros, resueltos una vez.
    std::atomic<float>* stageParam = nullptr;
    std::atomic<float>* oversampleParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* trimParam = nullptr;
    std::atomic<float>* lowParam = nullptr;
    std::atomic<float>* peakParam = nullptr;
    std::atomic<float>* hardnessParam = nullptr;
    std::atomic<float>* evenTopParam = nullptr;
    std::atomic<float>* evenBottomParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TufiProcessor)
};
