#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <map>
#include <set>

namespace ferb
{
/** Comparación A/B: dos ranuras con el valor de todos los parámetros (salvo los excluidos, p. ej. el
    bypass). Cambiar de ranura guarda la actual y carga la otra; la primera vez la ranura nueva es
    una copia de la actual, para empezar a comparar desde donde se está. Vive en el procesador, así
    que sobrevive a abrir y cerrar el editor; no se guarda con la sesión. */
class AbCompare
{
public:
    explicit AbCompare (juce::AudioProcessorValueTreeState& state, std::set<juce::String> excludedIds = {})
        : apvts (state), excluded (std::move (excludedIds))
    {
    }

    int getActive() const noexcept { return active; }

    void select (int slot)
    {
        slot = slot == 0 ? 0 : 1;
        if (slot == active)
            return;

        slots[idx (active)] = capture();
        if (slots[idx (slot)].empty())
            slots[idx (slot)] = slots[idx (active)];

        active = slot;
        apply (slots[idx (slot)]);
        notify();
    }

    /** Copia la ranura activa en la otra (el botón «A→B»). */
    void copyActiveToOther()
    {
        slots[idx (1 - active)] = capture();
        notify();
    }

    std::function<void()> onChange;

private:
    using Values = std::map<juce::String, float>; // id → valor normalizado

    Values capture() const
    {
        Values v;
        for (auto* p : apvts.processor.getParameters())
            if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (p))
                if (! excluded.count (r->paramID))
                    v[r->paramID] = r->getValue();
        return v;
    }

    void apply (const Values& v)
    {
        for (auto& [id, value] : v)
            if (auto* p = apvts.getParameter (id))
                p->setValueNotifyingHost (value);
    }

    static size_t idx (int i) noexcept { return static_cast<size_t> (i); }
    void notify() { if (onChange) onChange(); }

    juce::AudioProcessorValueTreeState& apvts;
    std::set<juce::String> excluded;
    std::array<Values, 2> slots;
    int active = 0;
};
} // namespace ferb
