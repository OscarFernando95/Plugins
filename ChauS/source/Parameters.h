#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace chaus::params
{
inline constexpr const char* bypass = "bypass"; // el del anfitrión; no forma parte de un preset

juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
} // namespace chaus::params
