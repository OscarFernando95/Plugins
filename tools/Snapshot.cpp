// Renderiza el editor de un plugin a un PNG, sin anfitrión. Herramienta de desarrollo:
//   PotySnapshot salida.png [escala] [preset-por-nombre] [clave=valor del estado de la interfaz]
// La variable de entorno FERB_SNAPSHOT_BLOCKS (por defecto 180) fija cuántos bloques de 512 muestras se procesan,
// con 10 ms de bucle de eventos cada uno (para que los temporizadores del editor y del procesador avancen).
// Pasa unos segundos de audio de prueba por el procesador para que el analizador y el VU tengan datos.

#include <juce_gui_basics/juce_gui_basics.h>

#include <cstdlib>
#include <cstring>

#include "PluginEditor.h"
#include "PluginProcessor.h"

#if FERB_PLUGIN_POTY
using Processor = PotyProcessor;
#elif FERB_PLUGIN_CREMITA
using Processor = CremitaProcessor;
#elif FERB_PLUGIN_NEL
using Processor = NelProcessor;
#elif FERB_PLUGIN_CHAUS
using Processor = ChauSProcessor;
#else
using Processor = TufiProcessor;
#endif

namespace
{
/** Ruido rosa (aproximación de Paul Kellet) más unos tonos, para que haya espectro que dibujar. */
struct TestSignal
{
    juce::Random rng { 42 };
    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
    double phase = 0.0;

    float next (double sampleRate)
    {
        const float white = rng.nextFloat() * 2.0f - 1.0f;
        b0 = 0.99886f * b0 + white * 0.0555179f;
        b1 = 0.99332f * b1 + white * 0.0750759f;
        b2 = 0.96900f * b2 + white * 0.1538520f;
        b3 = 0.86650f * b3 + white * 0.3104856f;
        b4 = 0.55000f * b4 + white * 0.5329522f;
        b5 = -0.7616f * b5 - white * 0.0168980f;
        const float pink = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f) * 0.11f;
        b6 = white * 0.115926f;
        phase += 2.0 * juce::MathConstants<double>::pi * 220.0 / sampleRate;
        return pink * 0.9f + 0.12f * static_cast<float> (std::sin (phase));
    }
};
} // namespace

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf (stderr, "uso: %s salida.png [escala] [preset]\n", argv[0]);
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI gui;

    Processor processor;
    processor.setPlayConfigDetails (2, 2, 48000.0, 512);
    processor.prepareToPlay (48000.0, 512);

    if (argc > 3 && std::strlen (argv[3]) > 0)
        processor.getPresets().load (juce::String::fromUTF8 (argv[3]));
    if (argc > 4)
    {
        const juce::String kv (argv[4]);
        processor.apvts.state.setProperty (kv.upToFirstOccurrenceOf ("=", false, false), kv.fromFirstOccurrenceOf ("=", false, false).getIntValue(), nullptr);
    }
    const char* blocksEnv = std::getenv ("FERB_SNAPSHOT_BLOCKS");
    const int numBlocks = blocksEnv != nullptr ? std::atoi (blocksEnv) : 180;

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    const double scale = argc > 2 ? std::atof (argv[2]) : 1.0;
    editor->setSize (juce::roundToInt (editor->getWidth() * scale / (static_cast<double> (editor->getWidth()) / static_cast<double> (editor->getWidth()))),
                     juce::roundToInt (editor->getHeight() * scale));
    editor->setVisible (true);

    // Un par de segundos de audio, con la cola de eventos en marcha (los temporizadores del editor).
    TestSignal signal;
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    for (int block = 0; block < numBlocks; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            const float v = signal.next (48000.0);
            buffer.setSample (0, i, v);
            buffer.setSample (1, i, v);
        }
        processor.processBlock (buffer, midi);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    }

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    juce::File out (argv[1]);
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (! stream.openedOk() || ! png.writeImageToStream (image, stream))
    {
        std::fprintf (stderr, "no se pudo escribir %s\n", argv[1]);
        return 1;
    }
    std::printf ("%s  %dx%d\n", argv[1], image.getWidth(), image.getHeight());

    editor.reset();
    return 0;
}
