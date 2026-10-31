#include "Presets.h"

namespace maru
{
std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    ferb::PresetManager::Factory init;
    init.name = "Init";
    return { init };
}
} // namespace maru
