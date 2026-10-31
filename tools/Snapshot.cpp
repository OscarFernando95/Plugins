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
#elif FERB_PLUGIN_MARU
using Processor = MaruProcessor;
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

#if FERB_PLUGIN_CHAUS || FERB_PLUGIN_MARU
/** Voz sintética con eses: vocales (diente de sierra con formantes) y una «s» de ruido de 6 a 9 kHz cada 0,6 s. */
struct VoiceSignal
{
    juce::Random rng { 7 };
    double phase = 0.0, t = 0.0;
    float f1a = 0, f1b = 0, f2a = 0, f2b = 0, hp1 = 0, hp2 = 0, lp1 = 0, lp2 = 0;

    float next (double fs)
    {
        t += 1.0 / fs;
        phase += 150.0 / fs;
        phase -= std::floor (phase);
        const float saw = static_cast<float> (2.0 * phase - 1.0);
        // dos resonancias de un polo para dar color de vocal
        f1a += 0.08f * (saw - f1a);
        f1b += 0.08f * (f1a - f1b);
        f2a += 0.30f * (saw - f2a);
        f2b += 0.30f * (f2a - f2b);
        const float vowel = 0.35f * (f1b * 2.0f + 0.4f * (f2a - f2b));

        const double cycle = std::fmod (t, 0.6);
        const double env = cycle < 0.16 ? 0.5 - 0.5 * std::cos (2.0 * juce::MathConstants<double>::pi * cycle / 0.16) : 0.0;
        // «s»: ruido filtrado paso alto (≈ 5 kHz) y paso bajo (≈ 10 kHz)
        const float white = rng.nextFloat() * 2.0f - 1.0f;
        hp1 += 0.45f * (white - hp1);
        const float high = white - hp1;
        hp2 += 0.45f * (high - hp2);
        const float band = high - hp2;
        lp1 += 0.55f * (band - lp1);
        return vowel * 0.7f + static_cast<float> (env) * 0.45f * lp1 * 2.0f;
    }
};
#endif

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
#if FERB_PLUGIN_CHAUS || FERB_PLUGIN_MARU
    VoiceSignal signal;
#else
    TestSignal signal;
#endif
    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    for (int block = 0; block < numBlocks; ++block)
    {
        for (int i = 0; i < 512; ++i)
        {
            float v = signal.next (48000.0);
#if FERB_PLUGIN_MARU
            {   // palabras de 0,8 s con pausas de 0,5 s, y ruido de fondo de ≈ −50 dBFS
                const double cyc = std::fmod (double (block * 512 + i) / 48000.0, 1.3);
                const float word = cyc < 0.8 ? 1.0f : (cyc < 0.84 ? 1.0f - float ((cyc - 0.8) / 0.04) : 0.0f);
                v = v * word + 0.003f * (signal.rng.nextFloat() * 3.46f - 1.73f);
            }
#endif
            buffer.setSample (0, i, v);
            buffer.setSample (1, i, v);
        }
        processor.processBlock (buffer, midi);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (10);
    }

#if FERB_PLUGIN_MARU
    std::printf ("piso %.1f dB, reducción %.1f dB\n", double (processor.getTelemetry().noiseFloorDb.load()), double (processor.getTelemetry().reductionDb.load()));
#endif
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
