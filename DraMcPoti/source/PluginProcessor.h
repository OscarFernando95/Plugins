#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <AbCompare.h>
#include <PresetManager.h>

#include "Parameters.h"
#include "dsp/McPotiEngine.h"

/** Fase 2 en curso: el motor tiene cruce, caracteres Vintage y dinámica; el resto de la ficha va llegando. */
class DraMcPotiProcessor : public juce::AudioProcessor
{
public:
    DraMcPotiProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    /** El bypass del anfitrión (botón del dispositivo) es el mismo parámetro `bypass`. */
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter (mcpoti::params::bypass); }

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

    ferb::PresetManager& getPresets() noexcept { return *presets; }
    ferb::AbCompare& getAb() noexcept { return *ab; }

    mcpoti::McPotiEngine& getEngine() noexcept { return engine; }
    mcpoti::Settings readSettings() const noexcept;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState apvts;

private:
    /** Punteros a los valores de los parámetros, resueltos una vez: buscar por texto en el hilo de audio reservaría memoria. */
    struct Raw
    {
        std::atomic<float> *input, *output, *mix, *mode, *era, *voice, *tune, *link, *xo1, *xo2, *xo3, *delta, *os, *air, *phase, *color, *soften, *width, *monoSafe, *guard, *match;
        std::array<std::atomic<float>*, mcpoti::numBands> drive, dyn, level, on, solo;
    } raw {};

    mcpoti::McPotiEngine engine;
    std::unique_ptr<ferb::PresetManager> presets;
    std::unique_ptr<ferb::AbCompare> ab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DraMcPotiProcessor)
};
