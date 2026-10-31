#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <AbCompare.h>
#include <LoudnessHistory.h>
#include <PresetManager.h>

#include "Parameters.h"
#include "dsp/NelEngine.h"

class NelProcessor : public juce::AudioProcessor,
                     private juce::AudioProcessorValueTreeState::Listener,
                     private juce::AsyncUpdater,
                     private juce::Timer
{
public:
    NelProcessor();
    ~NelProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void setNonRealtime (bool isNonRealtime) noexcept override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    /** El bypass del anfitrión (botón del dispositivo) es el mismo parámetro `bypass`. */
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter (nel::params::bypass); }

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

    /** Elige un modo y escribe sus valores en los parámetros avanzados (lo que hace un clic en la interfaz). */
    void selectMode (int mode);
    /** ¿Algún parámetro avanzado difiere de la tabla del modo? La interfaz marca el modo con un asterisco. */
    bool isModeModified() const;
    void setLoudnessFromInput (bool fromInput) noexcept;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

    nel::NelEngine& getEngine() noexcept { return engine; }
    ferb::PresetManager& getPresets() noexcept { return *presets; }
    ferb::AbCompare& getAb() noexcept { return *ab; }
    nel::Settings readSettings() const noexcept;
    /** Historial de sonoridad de hasta 10 h (se llena a 1 punto por segundo mientras suena el audio). */
    const ferb::LoudnessHistory& getHistory() const noexcept { return history; }

private:
    void parameterChanged (const juce::String& id, float value) override;
    void handleAsyncUpdate() override;
    void timerCallback() override;
    void updateLatency();

    nel::NelEngine engine;
    std::unique_ptr<ferb::PresetManager> presets;
    std::unique_ptr<ferb::AbCompare> ab;
    double currentSampleRate = 48000.0;
    std::atomic<bool> loudnessFromInput { false };
    int lastLatency = -1;
    ferb::LoudnessHistory history;
    float lastElapsed = -1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NelProcessor)
};
