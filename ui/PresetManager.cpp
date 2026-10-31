#include "PresetManager.h"

namespace ferb
{
namespace
{
constexpr const char* extension = ".ferbpreset";
constexpr const char* nameProperty = "presetName";

juce::File defaultDirectory (const juce::String& pluginName)
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile ("Application Support/FERB").getChildFile (pluginName).getChildFile ("Presets");
}
} // namespace

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state, const juce::String& name,
                              std::vector<Factory> factoryPresets, std::set<juce::String> excludedIds,
                              juce::File dir)
    : apvts (state), pluginName (name), factory (std::move (factoryPresets)), excluded (std::move (excludedIds)),
      directory (dir == juce::File() ? defaultDirectory (name) : dir)
{
}

juce::String PresetManager::getFactoryCategory (const juce::String& name) const
{
    for (const auto& f : factory)
        if (f.name == name)
            return f.category;
    return {};
}

juce::StringArray PresetManager::getFactoryNames() const
{
    juce::StringArray names;
    for (const auto& f : factory)
        names.add (f.name);
    return names;
}

juce::StringArray PresetManager::getUserNames() const
{
    juce::StringArray names;
    for (const auto& file : directory.findChildFiles (juce::File::findFiles, false, juce::String ("*") + extension))
        names.add (file.getFileNameWithoutExtension());
    names.sort (true);
    return names;
}

bool PresetManager::isFactory (const juce::String& name) const
{
    return getFactoryNames().contains (name);
}

juce::String PresetManager::getCurrentName() const
{
    return apvts.state.getProperty (nameProperty).toString();
}

juce::File PresetManager::fileFor (const juce::String& name) const
{
    return directory.getChildFile (sanitize (name) + extension);
}

juce::String PresetManager::sanitize (const juce::String& name)
{
    return name.removeCharacters ("/\\:*?\"<>|\n\r\t").trim().substring (0, 64);
}

PresetManager::Values PresetManager::currentValues() const
{
    Values values;
    for (auto* p : apvts.processor.getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! excluded.count (ranged->paramID))
                values[ranged->paramID] = ranged->convertFrom0to1 (ranged->getValue());
    return values;
}

bool PresetManager::valuesFor (const juce::String& name, Values& out) const
{
    out.clear();

    for (const auto& f : factory)
        if (f.name == name)
        {
            for (const auto& [id, value] : f.values)
                out[id] = value;
            return true;
        }

    const auto file = fileFor (name);
    if (! file.existsAsFile())
        return false;

    const auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("FerbPreset"))
        return false;

    for (auto* e : xml->getChildWithTagNameIterator ("Param"))
        out[e->getStringAttribute ("id")] = static_cast<float> (e->getDoubleAttribute ("value"));
    return true;
}

bool PresetManager::load (const juce::String& name)
{
    Values values;
    if (! valuesFor (name, values))
        return false;

    for (auto* p : apvts.processor.getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p);
        if (ranged == nullptr || excluded.count (ranged->paramID))
            continue;

        const auto it = values.find (ranged->paramID);
        const float real = it != values.end() ? it->second
                                              : ranged->convertFrom0to1 (ranged->getDefaultValue());

        ranged->beginChangeGesture();
        ranged->setValueNotifyingHost (ranged->convertTo0to1 (real));
        ranged->endChangeGesture();
    }

    apvts.state.setProperty (nameProperty, name, nullptr);
    notify();
    return true;
}

bool PresetManager::save (const juce::String& rawName)
{
    const auto name = sanitize (rawName);
    if (name.isEmpty() || isFactory (name))
        return false;

    juce::XmlElement xml ("FerbPreset");
    xml.setAttribute ("plugin", pluginName);
    xml.setAttribute ("version", 1);
    xml.setAttribute ("name", name);

    for (const auto& [id, value] : currentValues())
    {
        auto* e = xml.createNewChildElement ("Param");
        e->setAttribute ("id", id);
        e->setAttribute ("value", static_cast<double> (value));
    }

    if (! directory.createDirectory().wasOk() || ! xml.writeTo (fileFor (name)))
        return false;

    apvts.state.setProperty (nameProperty, name, nullptr);
    notify();
    return true;
}

bool PresetManager::remove (const juce::String& name)
{
    if (isFactory (name) || ! fileFor (name).deleteFile())
        return false;

    if (getCurrentName() == name)
        apvts.state.setProperty (nameProperty, juce::String(), nullptr);
    notify();
    return true;
}

bool PresetManager::isModified() const
{
    const auto name = getCurrentName();
    if (name.isEmpty())
        return false;

    Values reference;
    if (! valuesFor (name, reference))
        return true;

    for (auto* p : apvts.processor.getParameters())
    {
        auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p);
        if (ranged == nullptr || excluded.count (ranged->paramID))
            continue;

        const auto it = reference.find (ranged->paramID);
        const float real = it != reference.end() ? it->second : ranged->convertFrom0to1 (ranged->getDefaultValue());
        if (std::abs (ranged->getValue() - ranged->convertTo0to1 (real)) > 1.0e-3f)
            return true;
    }
    return false;
}

void PresetManager::loadRelative (int delta)
{
    auto all = getFactoryNames();
    all.addArray (getUserNames());
    if (all.isEmpty())
        return;

    const int current = all.indexOf (getCurrentName());
    const int next = current < 0 ? (delta > 0 ? 0 : all.size() - 1)
                                 : (current + delta + all.size()) % all.size();
    load (all[next]);
}
} // namespace ferb
