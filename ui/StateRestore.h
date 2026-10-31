#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace ferb
{
/** Fuerza cada parámetro al valor que guarda el árbol de estado.

    Tras `replaceState`, el APVTS no actualiza un parámetro si el valor guardado coincide con el que
    él tiene en caché; con un Bool o un Choice que alguien dejó en un valor normalizado fraccionario
    (0,84 se redondea a 1 en la caché) el parámetro se queda sin restaurar. Los validadores
    (pluginval) lo prueban con valores aleatorios. */
inline void syncParametersFromState (juce::AudioProcessorValueTreeState& apvts)
{
    for (const auto& child : apvts.state)
    {
        if (! child.hasType ("PARAM"))
            continue;
        auto* param = apvts.getParameter (child.getProperty ("id").toString());
        if (param == nullptr)
            continue;

        const float real = child.hasProperty ("value") ? static_cast<float> (child.getProperty ("value"))
                                                         : param->convertFrom0to1 (param->getDefaultValue());
        const float target = param->convertTo0to1 (real);
        if (std::abs (param->getValue() - target) > 1.0e-6f)
            param->setValueNotifyingHost (target);
    }
}
} // namespace ferb
