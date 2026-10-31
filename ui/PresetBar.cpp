#include "PresetBar.h"

namespace ferb
{
namespace
{
constexpr float arrowZone = 26.0f;

juce::String U (const char* utf8) { return juce::String::fromUTF8 (utf8); }
} // namespace

PresetBar::PresetBar (PresetManager& m, Style s) : manager (m), style (std::move (s))
{
    manager.onChange = [this] { repaint(); };
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

PresetBar::~PresetBar()
{
    manager.onChange = nullptr;
}

void PresetBar::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour (style.background);
    g.fillRoundedRectangle (b, style.corner);
    g.setColour (style.border);
    g.drawRoundedRectangle (b.reduced (0.5f), style.corner, 1.0f);

    // Flechas anterior / siguiente.
    g.setColour (style.textDim);
    g.setFont (style.boldFont.withHeight (16.0f));
    const auto whole = b;
    g.drawText (U ("‹"), b.removeFromLeft (arrowZone).toNearestInt(), juce::Justification::centred);
    g.drawText (U ("›"), b.removeFromRight (arrowZone).toNearestInt(), juce::Justification::centred);

    // Nombre del preset, con asterisco si se ha tocado algo.
    auto name = manager.getCurrentName();
    if (name.isEmpty())
        name = U ("Sin preset");
    else if (manager.isModified())
        name += " *";

    const auto label = U ("Preset · ");
    auto text = b.reduced (4.0f, 0.0f).withTrimmedRight (14.0f);

    const float labelW = juce::GlyphArrangement::getStringWidth (style.font, label);
    g.setColour (style.textDim);
    g.setFont (style.font);
    g.drawText (label, text.removeFromLeft (labelW).toNearestInt(), juce::Justification::centredLeft);
    g.setColour (style.text);
    g.setFont (style.boldFont);
    g.drawText (name, text.toNearestInt(), juce::Justification::centredLeft, true);

    // Chevron.
    const auto c = juce::Point<float> (whole.getRight() - arrowZone - 8.0f, whole.getCentreY());
    juce::Path chevron;
    chevron.startNewSubPath (c.x - 4.0f, c.y - 2.0f);
    chevron.lineTo (c.x, c.y + 2.0f);
    chevron.lineTo (c.x + 4.0f, c.y - 2.0f);
    g.setColour (style.textDim);
    g.strokePath (chevron, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void PresetBar::mouseDown (const juce::MouseEvent& e)
{
    if (e.position.x < arrowZone)
        manager.loadRelative (-1);
    else if (e.position.x > static_cast<float> (getWidth()) - arrowZone)
        manager.loadRelative (1);
    else
        showMenu();
}

void PresetBar::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& d)
{
    if (std::abs (d.deltaY) > 0.0f)
        manager.loadRelative (d.deltaY > 0.0f ? -1 : 1);
}

void PresetBar::showMenu()
{
    juce::PopupMenu menu;
    const auto current = manager.getCurrentName();

    const auto factoryNames = manager.getFactoryNames();
    juce::StringArray categories;
    for (const auto& n : factoryNames)
        categories.addIfNotAlreadyThere (manager.getFactoryCategory (n));
    for (const auto& category : categories)
    {
        menu.addSectionHeader (category.isEmpty() ? U ("De fábrica") : category);
        for (int i = 0; i < factoryNames.size(); ++i)
            if (manager.getFactoryCategory (factoryNames[i]) == category)
                menu.addItem (factoryBase + i, factoryNames[i], true, factoryNames[i] == current);
    }

    const auto userNames = manager.getUserNames();
    menu.addSectionHeader (U ("Mis presets"));
    if (userNames.isEmpty())
        menu.addItem (-1, U ("(vacío)"), false);
    for (int i = 0; i < userNames.size(); ++i)
        menu.addItem (userBase + i, userNames[i], true, userNames[i] == current);

    menu.addSeparator();
    const bool userCurrent = current.isNotEmpty() && ! manager.isFactory (current) && userNames.contains (current);
    menu.addItem (save, U ("Guardar"), userCurrent && manager.isModified());
    menu.addItem (saveAs, U ("Guardar como…"));
    menu.addItem (remove, U ("Eliminar «") + current + U ("»"), userCurrent);
    menu.addSeparator();
    menu.addItem (reveal, U ("Mostrar carpeta de presets"));

    juce::Component::SafePointer<PresetBar> safe (this);
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMinimumWidth (getWidth()),
                        [safe, factoryNames, userNames, current] (int result)
                        {
                            if (safe == nullptr || result <= 0)
                                return;

                            auto& m = safe->manager;
                            if (result >= userBase)
                                m.load (userNames[result - userBase]);
                            else if (result >= factoryBase)
                                m.load (factoryNames[result - factoryBase]);
                            else if (result == save)
                                m.save (current);
                            else if (result == saveAs)
                                safe->askName (current.isEmpty() || m.isFactory (current) ? juce::String() : current);
                            else if (result == remove)
                                m.remove (current);
                            else if (result == reveal)
                            {
                                m.getDirectory().createDirectory();
                                m.getDirectory().revealToUser();
                            }
                        });
}

void PresetBar::askName (const juce::String& initial)
{
    auto* w = new juce::AlertWindow (U ("Guardar preset"), U ("Nombre del preset:"), juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", initial);
    w->addButton (U ("Guardar"), 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton (U ("Cancelar"), 0, juce::KeyPress (juce::KeyPress::escapeKey));

    juce::Component::SafePointer<PresetBar> safe (this);
    w->enterModalState (true,
                        juce::ModalCallbackFunction::create ([safe, w] (int result)
                        {
                            if (result == 1 && safe != nullptr)
                                safe->commit (w->getTextEditorContents ("name"));
                        }),
                        true);
}

void PresetBar::commit (const juce::String& rawName)
{
    const auto name = PresetManager::sanitize (rawName);
    if (name.isEmpty())
        return;

    if (manager.isFactory (name))
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon, U ("Nombre reservado"),
                                                U ("«") + name + U ("» es un preset de fábrica. Elige otro nombre."));
        return;
    }

    if (manager.getUserNames().contains (name) && name != manager.getCurrentName())
    {
        juce::Component::SafePointer<PresetBar> safe (this);
        juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                          .withIconType (juce::MessageBoxIconType::QuestionIcon)
                                          .withTitle (U ("Sobrescribir preset"))
                                          .withMessage (U ("Ya existe «") + name + U ("». ¿Sobrescribirlo?"))
                                          .withButton (U ("Sobrescribir"))
                                          .withButton (U ("Cancelar")),
                                      [safe, name] (int result)
                                      {
                                          if (result == 1 && safe != nullptr)
                                              safe->manager.save (name);
                                      });
        return;
    }

    manager.save (name);
}
} // namespace ferb
