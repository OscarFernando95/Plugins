#pragma once

#include <Controls.h>

#include "../PluginProcessor.h"
#include "../dsp/CremitaCurve.h"

namespace cremita::ui
{
/** Ventana de cristal con la curva de transferencia real (con k y b), la diagonal de referencia y un
    punto blanco que marca el nivel de trabajo. */
class CurveView : public ferb::vintage::GlassView
{
public:
    CurveView (CremitaProcessor&, const ferb::vintage::Palette&);

    /** Nivel de trabajo (lineal, 0 a 1) que marca el punto. */
    void setWorkingLevel (float level) noexcept { working = level; }

protected:
    void paintContent (juce::Graphics&, float width, float height) override;

private:
    CremitaProcessor& processor;
    float working = 0.0f;
};
} // namespace cremita::ui
