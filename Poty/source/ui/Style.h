#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <array>

#include "PotyAssets.h"

namespace poty::ui
{
// Paleta del bosquejo aprobado.
inline const juce::Colour bg { 0xff0e1116 };
inline const juce::Colour panel { 0xff151a22 };
inline const juce::Colour plotBg { 0xff10151c };
inline const juce::Colour border { 0xff232b36 };
inline const juce::Colour text { 0xffe8edf4 };
inline const juce::Colour textDim { 0xff9aa7b8 };
inline const juce::Colour accent { 0xff4cc9f0 };
inline const juce::Colour inputFill { 0xff2a3646 };
inline const juce::Colour inputStroke { 0xff3d5068 };
inline const std::array<juce::Colour, 5> bandColour {
    juce::Colour (0xffff6b8a), juce::Colour (0xffffb454), juce::Colour (0xff7ee08a),
    juce::Colour (0xff4cc9f0), juce::Colour (0xffb18cff)
};

namespace detail
{
inline juce::Typeface::Ptr load (const char* data, int size) { return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size)); }
}

/** Manrope para la interfaz y JetBrains Mono para los valores, incrustadas en el binario. */
inline juce::Font ui (float height, bool bold = false)
{
    static const auto medium = detail::load (PotyData::ManropeMedium_ttf, PotyData::ManropeMedium_ttfSize);
    static const auto strong = detail::load (PotyData::ManropeBold_ttf, PotyData::ManropeBold_ttfSize);
    return juce::Font (juce::FontOptions (bold ? strong : medium).withHeight (height));
}

inline juce::Font mono (float height, bool bold = false)
{
    static const auto medium = detail::load (PotyData::JetBrainsMonoMedium_ttf, PotyData::JetBrainsMonoMedium_ttfSize);
    static const auto strong = detail::load (PotyData::JetBrainsMonoBold_ttf, PotyData::JetBrainsMonoBold_ttfSize);
    return juce::Font (juce::FontOptions (bold ? strong : medium).withHeight (height));
}

// ---- Formatos con coma decimal, como en el bosquejo.

inline juce::String comma (double value, int decimals)
{
    return juce::String (value, decimals).replaceCharacter ('.', ',');
}

inline juce::String hz (double f)
{
    if (f >= 10000.0)
        return comma (f / 1000.0, 0) + " kHz";
    if (f >= 1000.0)
        return comma (f / 1000.0, 1) + " kHz";
    return juce::String (juce::roundToInt (f)) + " Hz";
}

inline juce::String signedDb (double db, int decimals = 1)
{
    return juce::String (db >= 0.0 ? "+" : juce::CharPointer_UTF8 ("−")) + comma (std::abs (db), decimals) + " dB";
}

/** Acepta «1,8k», «1800», «5 kHz» o «90 hz». Devuelve el valor en Hz. */
inline float parseHz (const juce::String& s, float fallback)
{
    auto t = s.trim().toLowerCase().replaceCharacter (',', '.');
    const bool kilo = t.contains ("k");
    t = t.retainCharacters ("0123456789.-");
    if (t.isEmpty())
        return fallback;
    return t.getFloatValue() * (kilo ? 1000.0f : 1.0f);
}

/** Acepta «3,5», «+3.5 dB» o «-2». */
inline float parseNumber (const juce::String& s, float fallback)
{
    auto t = s.trim().replaceCharacter (',', '.').replace (juce::CharPointer_UTF8 ("−"), "-").retainCharacters ("0123456789.-");
    return t.isEmpty() ? fallback : t.getFloatValue();
}
} // namespace poty::ui
