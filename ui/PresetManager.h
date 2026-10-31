#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>
#include <map>
#include <set>
#include <vector>

namespace ferb
{
/** Presets de un plugin: los de fábrica van en código; los del usuario, como archivos XML en
    la carpeta `~/Library/Application Support/FERB/<plugin>/Presets` (extensión .ferbpreset).

    Un preset guarda el valor real de cada parámetro (en unidades de la interfaz). Al cargar, los
    parámetros que el preset no menciona vuelven a su valor por defecto, así que los presets
    antiguos siguen funcionando cuando el plugin gana parámetros. El nombre del preset actual viaja
    en el estado del plugin (propiedad `presetName`), no es un parámetro. */
class PresetManager
{
public:
    struct Factory
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values; // id del parámetro → valor real
    };

    /** `excluded`: ids que no forman parte de un preset (p. ej. el bypass). `directory` vacío = la de usuario. */
    PresetManager (juce::AudioProcessorValueTreeState& apvts, const juce::String& pluginName,
                   std::vector<Factory> factory, std::set<juce::String> excluded = {},
                   juce::File directory = {});

    juce::File getDirectory() const { return directory; }

    juce::StringArray getFactoryNames() const;
    juce::StringArray getUserNames() const;
    bool isFactory (const juce::String& name) const;

    /** Nombre del preset actual, o vacío si el estado no viene de ninguno. */
    juce::String getCurrentName() const;

    /** true si algún parámetro difiere del preset actual (o si el nombre ya no existe). */
    bool isModified() const;

    bool load (const juce::String& name);

    /** Guarda el estado actual como preset de usuario. Falla con un nombre de fábrica o vacío. */
    bool save (const juce::String& name);
    bool remove (const juce::String& name);

    /** Pasa al preset siguiente (+1) o anterior (−1), entre los de fábrica y luego los de usuario. */
    void loadRelative (int delta);

    /** Quita caracteres que no valen en un nombre de archivo. */
    static juce::String sanitize (const juce::String& name);

    std::function<void()> onChange; // la interfaz se actualiza

private:
    using Values = std::map<juce::String, float>; // id → valor real

    juce::File fileFor (const juce::String& name) const;
    bool valuesFor (const juce::String& name, Values& out) const;
    Values currentValues() const;
    void notify() { if (onChange) onChange(); }

    juce::AudioProcessorValueTreeState& apvts;
    juce::String pluginName;
    std::vector<Factory> factory;
    std::set<juce::String> excluded;
    juce::File directory;
};
} // namespace ferb
