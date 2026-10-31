#pragma once

#include "Vintage.h"

namespace ferb::vintage
{
/** Vúmetro de aguja: esfera crema con escala de −20 a +3 VU, zona roja desde 0, aguja con sombra y
    bisel. `setValue` recibe el nivel en VU (0 VU = la referencia del aparato). La esfera se dibuja una
    vez en una imagen y solo la aguja se repinta. */
class VuNeedle : public juce::Component
{
public:
    static constexpr int margin = 9; // espacio para el bisel, fuera de la esfera

    VuNeedle (const Palette&, juce::String title, bool warm = true);

    void setValue (float vu);
    void paint (juce::Graphics&) override;

    /** Coloca el vúmetro de modo que la ESFERA ocupe (x, y, w, h); el bisel queda por fuera. */
    void setFaceBounds (juce::Rectangle<int> face) { setBounds (face.expanded (margin)); }

private:
    juce::Rectangle<float> face() const noexcept { return getLocalBounds().toFloat().reduced (static_cast<float> (margin)); }
    void renderFace (juce::Graphics&, juce::Rectangle<float>) const;

    Palette palette;
    juce::String title;
    bool warm;
    float value = -20.0f;
    juce::Image cache;
    float cacheScale = 0.0f;
};
} // namespace ferb::vintage
