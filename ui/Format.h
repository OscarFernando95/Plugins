#pragma once

#include <juce_core/juce_core.h>

namespace ferb::fmt
{
/** Número con coma decimal, como en el bosquejo. */
inline juce::String comma (double value, int decimals)
{
    return juce::String (value, decimals).replaceCharacter ('.', ',');
}

inline juce::String signedDb (double db, int decimals = 1)
{
    return juce::String (db >= 0.0 ? "+" : juce::CharPointer_UTF8 ("−")) + comma (std::abs (db), decimals) + " dB";
}

/** Acepta «3,5», «+3.5 dB» o «-2». */
inline float parseNumber (const juce::String& s, float fallback)
{
    auto t = s.trim().replaceCharacter (',', '.').replace (juce::CharPointer_UTF8 ("−"), "-").retainCharacters ("0123456789.-");
    return t.isEmpty() ? fallback : t.getFloatValue();
}
} // namespace ferb::fmt
