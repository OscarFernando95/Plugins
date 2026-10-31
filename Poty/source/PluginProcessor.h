#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Parameters.h"
#include <PresetManager.h>
#include <SampleRing.h>

#include "dsp/PotyEngine.h"

class PotyProcessor : public juce::AudioProcessor
{
public:
    PotyProcessor();
    ~PotyProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    /** El bypass del anfitrión (botón del dispositivo) es el mismo parámetro `bypass`. */
    juce::AudioProcessorParameter* getBypassParameter() const override
    {
        return apvts.getParameter (poty::params::bypass);
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

    const ferb::LevelMeter& getOutputMeter() const noexcept { return engine.getOutputMeter(); }
    ferb::PresetManager& getPresets() noexcept { return *presets; }

    /** El analizador de la interfaz copia la señal solo mientras el editor está abierto. */
    void setAnalyzerActive (bool active) noexcept { analyzerActive.store (active, std::memory_order_relaxed); }
    const ferb::SampleRing& getInputRing() const noexcept { return inputRing; }
    const ferb::SampleRing& getOutputRing() const noexcept { return outputRing; }

private:
    poty::Settings readSettings() const noexcept;

    poty::PotyEngine engine;
    std::unique_ptr<ferb::PresetManager> presets;

    ferb::SampleRing inputRing, outputRing;
    std::atomic<bool> analyzerActive { false };
    std::vector<float> monoScratch;

    // Punteros a los valores atómicos de los parámetros, resueltos una vez.
    struct BandParams
    {
        std::atomic<float>* on = nullptr;
        std::atomic<float>* freq = nullptr;
        std::atomic<float>* gain = nullptr;
        std::atomic<float>* q = nullptr;
        std::atomic<float>* type = nullptr;
        std::atomic<float>* drive = nullptr;
    };

    std::atomic<float>* inGainParam = nullptr;
    std::atomic<float>* outGainParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* qualityParam = nullptr;
    std::atomic<float>* intensityParam = nullptr;
    std::atomic<float>* modeParam = nullptr;
    std::array<BandParams, poty::params::numBands> bandParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PotyProcessor)
};
