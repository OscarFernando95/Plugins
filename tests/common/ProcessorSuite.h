#pragma once

// Pruebas a nivel de procesador completo, comunes a Tufi y Poty. Cubren lo que un validador de
// plugins comprueba desde fuera: parámetros aleatorios, tasas de muestreo y tamaños de bloque
// extremos, estados y vida del editor.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

namespace suite
{
template <typename Processor>
void randomise (Processor& p, juce::Random& rng)
{
    for (auto* param : p.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (ranged != p.getBypassParameter()) // el bypass se prueba aparte
                ranged->setValueNotifyingHost (rng.nextFloat());
}

/** Parámetros aleatorios con muchas tasas de muestreo, bloques y canales: nada de NaN ni de picos absurdos. */
template <typename Processor>
void fuzz (int seconds = 1)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::Random rng (20261028);

    for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
        for (int channels : { 1, 2 })
            for (int block : { 1, 7, 64, 255, 256, 257, 1024, 4096 })
            {
                Processor p;
                p.setPlayConfigDetails (channels, channels, sr, block);
                p.prepareToPlay (sr, block);

                juce::AudioBuffer<float> buffer (channels, block);
                juce::MidiBuffer midi;

                const int blocks = std::max (8, int (sr * seconds) / block);
                const int step = std::max (1, blocks / 12);
                for (int b = 0; b < blocks; ++b)
                {
                    if (b % step == 0)
                        randomise (p, rng); // cambia todo con el audio sonando

                    for (int ch = 0; ch < channels; ++ch)
                        for (int i = 0; i < block; ++i)
                            buffer.setSample (ch, i, (rng.nextFloat() * 2.0f - 1.0f) * 0.7f);

                    p.processBlock (buffer, midi);

                    for (int ch = 0; ch < channels; ++ch)
                        for (int i = 0; i < block; ++i)
                        {
                            const float v = buffer.getSample (ch, i);
                            INFO ("sr " << sr << ", canales " << channels << ", bloque " << block << ", bloque nº " << b);
                            REQUIRE (std::isfinite (v));
                            REQUIRE (std::abs (v) < 60.0f);
                        }
                }

                p.releaseResources();
            }
}

/** Guardar y restaurar el estado reproduce todos los parámetros y el nombre del preset. */
template <typename Processor>
void stateRoundTrip()
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::Random rng (7);

    Processor a, b;
    randomise (a, rng);
    a.getPresets().save ("prueba-de-estado-ferb");

    juce::MemoryBlock blob;
    a.getStateInformation (blob);
    b.setStateInformation (blob.getData(), int (blob.getSize()));

    for (auto* pa : a.getParameters())
    {
        auto* ra = dynamic_cast<juce::RangedAudioParameter*> (pa);
        auto* rb = b.apvts.getParameter (ra->paramID);
        INFO ("parámetro " << ra->paramID);
        REQUIRE (rb != nullptr);
        // Se comparan los valores reales: un Bool o un Choice aleatorizado se guarda ya redondeado.
        REQUIRE (rb->convertFrom0to1 (rb->getValue()) == Catch::Approx (ra->convertFrom0to1 (ra->getValue())).margin (1e-4));
    }
    REQUIRE (b.getPresets().getCurrentName() == "prueba-de-estado-ferb");
    a.getPresets().remove ("prueba-de-estado-ferb");
}

/** Abrir, redimensionar y cerrar el editor muchas veces, con audio sonando. */
template <typename Processor>
void editorLifecycle()
{
    juce::ScopedJuceInitialiser_GUI gui;
    Processor p;
    p.setPlayConfigDetails (2, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    juce::Random rng (3);

    for (int round = 0; round < 12; ++round)
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
        REQUIRE (editor != nullptr);

        for (double scale : { 1.0, 0.6, 2.0, 1.25 })
        {
            editor->setSize (juce::roundToInt (editor->getWidth() / (editor->getWidth() / double (editor->getWidth()))), editor->getHeight());
            const int w = juce::jlimit (editor->getConstrainer()->getMinimumWidth(), editor->getConstrainer()->getMaximumWidth(),
                                        int (scale * editor->getWidth()));
            editor->setSize (w, int (w / editor->getConstrainer()->getFixedAspectRatio()));

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                    buffer.setSample (ch, i, rng.nextFloat() * 0.5f - 0.25f);
            p.processBlock (buffer, midi);
            juce::MessageManager::getInstance()->runDispatchLoopUntil (15);
        }
        randomise (p, rng); // los controles del editor se actualizan con cambios externos
        juce::MessageManager::getInstance()->runDispatchLoopUntil (40);
    }
}

/** La latencia declarada es la misma con cualquier parámetro, y el bypass es el del anfitrión. */
template <typename Processor>
void constantLatency()
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::Random rng (11);

    for (double sr : { 44100.0, 48000.0, 96000.0 })
    {
        Processor p;
        p.setPlayConfigDetails (2, 2, sr, 256);
        p.prepareToPlay (sr, 256);
        const int declared = p.getLatencySamples();
        REQUIRE (declared > 0);

        for (int i = 0; i < 25; ++i)
        {
            randomise (p, rng);
            REQUIRE (p.getLatencySamples() == declared);
        }
    }

    Processor p;
    REQUIRE (p.getBypassParameter() != nullptr);
}

/** Mono y estéreo se aceptan; otras configuraciones, no. */
template <typename Processor>
void busLayouts()
{
    juce::ScopedJuceInitialiser_GUI gui;
    Processor p;

    auto layout = [] (const juce::AudioChannelSet& in, const juce::AudioChannelSet& out)
    {
        juce::AudioProcessor::BusesLayout l;
        l.inputBuses.add (in);
        l.outputBuses.add (out);
        return l;
    };

    REQUIRE (p.isBusesLayoutSupported (layout (juce::AudioChannelSet::stereo(), juce::AudioChannelSet::stereo())));
    REQUIRE (p.isBusesLayoutSupported (layout (juce::AudioChannelSet::mono(), juce::AudioChannelSet::mono())));
    REQUIRE_FALSE (p.isBusesLayoutSupported (layout (juce::AudioChannelSet::mono(), juce::AudioChannelSet::stereo())));
    REQUIRE_FALSE (p.isBusesLayoutSupported (layout (juce::AudioChannelSet::create5point1(), juce::AudioChannelSet::create5point1())));
}

/** `processBlock` no reserva memoria, aunque cambien calidad, modo, etapa o sobremuestreo con audio sonando.
    Requiere AllocGuard.h incluido en el archivo de pruebas. */
template <typename Processor, typename Count>
void noAllocationsInProcessBlock (Count countScope)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::Random rng (99);

    for (int channels : { 1, 2 })
        for (int block : { 64, 512, 1000 })
        {
            Processor p;
            p.setPlayConfigDetails (channels, channels, 48000.0, block);
            p.prepareToPlay (48000.0, block);

            juce::AudioBuffer<float> buffer (channels, block);
            juce::MidiBuffer midi;

            // Calentamiento: el primer bloque calcula la tabla inicial, etc.
            for (int b = 0; b < 4; ++b)
            {
                for (int ch = 0; ch < channels; ++ch)
                    for (int i = 0; i < block; ++i)
                        buffer.setSample (ch, i, rng.nextFloat() * 0.5f - 0.25f);
                p.processBlock (buffer, midi);
            }

            long total = 0;
            for (int b = 0; b < 200; ++b)
            {
                if (b % 5 == 0)
                    randomise (p, rng); // fuera de la medición: los parámetros del anfitrión no son del hilo de audio

                for (int ch = 0; ch < channels; ++ch)
                    for (int i = 0; i < block; ++i)
                        buffer.setSample (ch, i, rng.nextFloat() * 1.0f - 0.5f);

                auto scope = countScope();
                p.processBlock (buffer, midi);
                total += scope.allocations();
            }

            INFO ("canales " << channels << ", bloque " << block);
            REQUIRE (total == 0);
        }
}
} // namespace suite
