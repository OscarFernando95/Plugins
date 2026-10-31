#pragma once

#include <Format.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "TufiAssets.h"

namespace tufi::ui
{
// Paleta del bosquejo aprobado.
inline const juce::Colour ivory { 0xffe4d6b4 };
inline const juce::Colour ink { 0xff2b2118 };
inline const juce::Colour inkSoft { 0xff5b4a38 };
inline const juce::Colour brass { 0xffb08d4f };
inline const juce::Colour bakelite { 0xff17110d };
inline const juce::Colour walnut { 0xff4a2d1a };
inline const juce::Colour cream { 0xffe6d9bb };
inline const juce::Colour phosphor { 0xff8dffa8 };
inline const juce::Colour screenGlass { 0xff07140b };
inline const juce::Colour amber { 0xffffb347 };
inline const juce::Colour vuFace { 0xfff2dca0 };
inline const juce::Colour vuRed { 0xffa8321d };

namespace detail
{
inline juce::Typeface::Ptr load (const char* data, int size) { return juce::Typeface::createSystemTypefaceFor (data, static_cast<size_t> (size)); }
}

/** DM Serif Display para el nombre, Josefin Sans para los rótulos y Courier Prime para los valores. */
inline juce::Font serif (float height)
{
    static const auto face = detail::load (TufiData::DMSerifDisplayRegular_ttf, TufiData::DMSerifDisplayRegular_ttfSize);
    return juce::Font (juce::FontOptions (face).withHeight (height));
}

inline juce::Font label (float height, float kerning = 0.0f)
{
    static const auto face = detail::load (TufiData::JosefinSansSemiBold_ttf, TufiData::JosefinSansSemiBold_ttfSize);
    return juce::Font (juce::FontOptions (face).withHeight (height)).withExtraKerningFactor (kerning);
}

inline juce::Font mono (float height, bool bold = false)
{
    static const auto regular = detail::load (TufiData::CourierPrimeRegular_ttf, TufiData::CourierPrimeRegular_ttfSize);
    static const auto strong = detail::load (TufiData::CourierPrimeBold_ttf, TufiData::CourierPrimeBold_ttfSize);
    return juce::Font (juce::FontOptions (bold ? strong : regular).withHeight (height));
}

using ferb::fmt::comma;
using ferb::fmt::parseNumber;
using ferb::fmt::signedDb;
} // namespace tufi::ui
