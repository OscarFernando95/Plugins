#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <AbCompare.h>
#include <PresetManager.h>

#include "Parameters.h"
#include "dsp/CremitaEngine.h"

class CremitaProcessor : public juce::AudioProcessor
{
public:
    CremitaProcessor();
    ~CremitaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    /** El bypass del anfitrión (botón del dispositivo) es el mismo parámetro `bypass`. */
    juce::AudioProcessorParameter* getBypassParameter() const override
    {
        return apvts.getParameter (cremita::params::bypass);
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

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    const cremita::CremitaEngine& getEngine() const noexcept { return engine; }
    cremita::CremitaEngine& getEngine() noexcept { return engine; }
    ferb::PresetManager& getPresets() noexcept { return *presets; }
    ferb::AbCompare& getAb() noexcept { return *ab; }

private:
    cremita::Settings readSettings() const noexcept;

    cremita::CremitaEngine engine;
    std::unique_ptr<ferb::PresetManager> presets;
    std::unique_ptr<ferb::AbCompare> ab;

    // Punteros a los valores atómicos de los parámetros, resueltos una vez.
    std::atomic<float>* inputParam = nullptr;
    std::atomic<float>* effectParam = nullptr;
    std::atomic<float>* curveParam = nullptr;
    std::atomic<float>* biasParam = nullptr;
    std::atomic<float>* splitParam = nullptr;
    std::atomic<float>* xo1Param = nullptr;
    std::atomic<float>* xo2Param = nullptr;
    std::array<std::atomic<float>*, 3> trimParams {};
    std::atomic<float>* clipParam = nullptr;
    std::atomic<float>* msParam = nullptr;
    std::atomic<float>* osParam = nullptr;
    std::atomic<float>* matchParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* inOnParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CremitaProcessor)
};
