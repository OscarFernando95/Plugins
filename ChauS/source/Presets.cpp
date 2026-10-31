#include "Presets.h"

namespace chaus
{
std::vector<ferb::PresetManager::Factory> factoryPresets()
{
    ferb::PresetManager::Factory init;
    init.name = "Init";
    return { init };
}
} // namespace chaus
