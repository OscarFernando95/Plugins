#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <Format.h>

// Kit de interfaz «aparato analógico» de la familia By FERB: lo comparten Cremita, Maru, ChauS y Nel.
// Los valores de colores y medidas salen de «Maru, Cremita y ChauS.html» (las maquetas aprobadas).

namespace ferb::vintage
{
/** Colores de un aparato. Los tres primeros son el degradado vertical del frente. */
struct Palette
{
    juce::Colour panelHi, panel, panelLo;
    juce::Colour ink, ink2;       // serigrafía y serigrafía secundaria
    juce::Colour acc, acc2;       // cifras y acentos
    juce::Colour glass;           // fondo de las ventanas
    juce::Colour btnA, btnB, btnInk, edge; // botones
    juce::Colour rule, lamp, emboss;       // filetes, lámparas, relieve del texto
    juce::Colour earA, earB, earC;         // orejas del rack
    juce::Colour bezel, bezelDark;         // aro de los vúmetros y las ventanas
    bool woodEars = false;
    bool darkKnobs = true;

    static Palette cremita();
    static Palette maru();
    static Palette chaus();
    static Palette nel();
};

// Tipografías incrustadas: Limelight (logotipo), Josefin Sans (rótulos) y Oxanium (cifras).
juce::Font logoFont (float height);
juce::Font labelFont (float height, float letterSpacing = 0.0f); // `letterSpacing` en px, como en CSS
juce::Font digitFont (float height, bool medium = false);

inline const juce::Colour brass { 0xffc9a24d };
inline const juce::Colour brassDark { 0xff8a6a2a };
inline const juce::Colour focusRing { 0xffc7a3ff };
inline const juce::Colour alert { 0xffff5a7a };
inline const juce::Colour violet { 0xff8a5cf0 };
inline const juce::Colour lilac { 0xffc7a3ff };

/** Texto con espaciado entre letras (como `letter-spacing` de CSS), centrado o alineado. */
void drawSpaced (juce::Graphics&, const juce::String&, juce::Rectangle<float>, const juce::Font&, juce::Justification);

/** Ventana de cristal: fondo, aro y, al terminar, las rayas de pantalla y el brillo. */
void paintGlassBackground (juce::Graphics&, juce::Rectangle<float>, const Palette&, float corner = 7.0f);
void paintGlassOverlay (juce::Graphics&, juce::Rectangle<float>, float corner = 7.0f);

/** Aro de bisel (negro, color del aparato y filete oscuro) alrededor de `r`. */
void paintBezel (juce::Graphics&, juce::Rectangle<float> r, const Palette&, float corner, bool thick = true);

/** Degradado radial como el `radialGradient` de SVG (cx, cy, r en fracciones del cuadro `box`). */
juce::ColourGradient svgRadial (juce::Rectangle<float> box, float cx, float cy, float r,
                                std::initializer_list<std::pair<float, juce::Colour>> stops);
} // namespace ferb::vintage
