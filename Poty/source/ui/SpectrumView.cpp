#include "SpectrumView.h"

#include "EqMath.h"

namespace poty::ui
{
namespace
{
constexpr float nodeRadius = 9.0f;
constexpr float hitRadius = 15.0f;
const char* typeNames[] = { "Tubo", "Cinta", "S\xc3\xb3lido", "Diodo Ge", "Digital" };
} // namespace

/** Rejilla y rótulos: no cambian, así que se guardan como imagen. */
class SpectrumView::Grid : public juce::Component
{
public:
    Grid() { setBufferedToImage (true); setInterceptsMouseClicks (false, false); }

    void paint (juce::Graphics& g) override
    {
        const auto view = static_cast<SpectrumView*> (getParentComponent());
        const juce::Rectangle<float> plot (plotX, plotY, plotW, plotH);

        g.setColour (plotBg);
        g.fillRoundedRectangle (plot, 4.0f);

        // Líneas horizontales cada 20 dB y verticales en las décadas.
        g.setFont (mono (11.0f));
        for (int db = 0; db >= -80; db -= 20)
        {
            const float y = view->dbToY (db);
            g.setColour (border.withAlpha (db == 0 || db == -80 ? 0.0f : 0.8f));
            g.drawHorizontalLine (juce::roundToInt (y), plotX, plotX + plotW);
            g.setColour (textDim);
            g.drawText (juce::String (db), juce::Rectangle<float> (0.0f, y - 8.0f, plotX - 8.0f, 16.0f), juce::Justification::centredRight);
        }

        const std::pair<double, const char*> marks[] = { { 20, "20" }, { 50, "50" }, { 100, "100" }, { 200, "200" },
                                                          { 500, "500" }, { 1000, "1k" }, { 2000, "2k" }, { 5000, "5k" },
                                                          { 10000, "10k" }, { 20000, "20k" } };
        for (auto [f, label] : marks)
        {
            const float x = view->freqToX (f);
            const bool decade = juce::roundToInt (f) == 100 || juce::roundToInt (f) == 1000 || juce::roundToInt (f) == 10000;
            g.setColour (border.withAlpha (decade ? 0.9f : 0.45f));
            g.drawVerticalLine (juce::roundToInt (x), plotY, plotY + plotH);
            g.setColour (textDim);
            g.drawText (label, juce::Rectangle<float> (x - 20.0f, plotY + plotH + 5.0f, 40.0f, 14.0f), juce::Justification::centred);
        }
    }
};

/** Espectros, curva y nodos: por encima de la rejilla y sin recibir el ratón (lo recibe SpectrumView). */
class SpectrumView::Overlay : public juce::Component
{
public:
    explicit Overlay (SpectrumView& v) : view (v) { setInterceptsMouseClicks (false, false); }
    void paint (juce::Graphics& g) override { view.paintOverlay (g); }

private:
    SpectrumView& view;
};

SpectrumView::SpectrumView (PotyProcessor& p, int& selectedBand, std::function<void (int)> select)
    : processor (p), selected (selectedBand), onSelect (std::move (select)),
      samples (static_cast<size_t> (fftSize)), fftData (static_cast<size_t> (fftSize) * 2)
{
    inDb.fill (floorDb);
    outDb.fill (floorDb);

    grid = std::make_unique<Grid>();
    addAndMakeVisible (*grid);
    overlay = std::make_unique<Overlay> (*this);
    addAndMakeVisible (*overlay);

    processor.setAnalyzerActive (true);
    startTimerHz (30);
}

SpectrumView::~SpectrumView()
{
    processor.setAnalyzerActive (false);
}

void SpectrumView::resized()
{
    grid->setBounds (getLocalBounds());
    overlay->setBounds (getLocalBounds());
}

// ---------------------------------------------------------------- geometría

float SpectrumView::freqToX (double f) const noexcept
{
    return plotX + plotW * static_cast<float> (std::log (f / minFreq) / std::log (maxFreq / minFreq));
}

double SpectrumView::xToFreq (float x) const noexcept
{
    return minFreq * std::pow (maxFreq / minFreq, juce::jlimit (0.0, 1.0, static_cast<double> ((x - plotX) / plotW)));
}

float SpectrumView::dbToY (double db) const noexcept
{
    return plotY + plotH * static_cast<float> (-db / 80.0);
}

double SpectrumView::yToDb (float y) const noexcept
{
    return -80.0 * static_cast<double> ((y - plotY) / plotH);
}

juce::RangedAudioParameter* SpectrumView::param (int band, const char* suffix) const
{
    return processor.apvts.getParameter (params::bandId (band, suffix));
}

float SpectrumView::realValue (int band, const char* suffix) const
{
    auto* p = param (band, suffix);
    return p->convertFrom0to1 (p->getValue());
}

void SpectrumView::setReal (int band, const char* suffix, float real)
{
    auto* p = param (band, suffix);
    p->setValueNotifyingHost (p->convertTo0to1 (real));
}

juce::Point<float> SpectrumView::nodePosition (int band) const
{
    return { freqToX (realValue (band, params::bandFreq)),
             dbToY (curveOffset + curveScale * realValue (band, params::bandGain)) };
}

int SpectrumView::bandAt (juce::Point<float> p) const
{
    int best = -1;
    float bestDist = hitRadius;
    for (int b = params::numBands - 1; b >= 0; --b) // las de arriba (números altos) se pintan encima
    {
        const float d = nodePosition (b).getDistanceFrom (p);
        if (d <= bestDist)
        {
            best = b;
            bestDist = d;
        }
    }
    return best;
}

// ---------------------------------------------------------------- análisis

void SpectrumView::analyze (const ferb::SampleRing& ring, std::array<float, numPoints>& dbOut)
{
    ring.copyLatest (samples.data(), fftSize);
    std::copy (samples.begin(), samples.end(), fftData.begin());
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
    window.multiplyWithWindowingTable (fftData.data(), static_cast<size_t> (fftSize));
    fft.performFrequencyOnlyForwardTransform (fftData.data());

    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;
    const double binHz = sr / fftSize;

    // Amplitud de una senoidal a plena escala = N/4 con ventana de Hann.
    constexpr float norm = 4.0f / static_cast<float> (fftSize);
    auto bin = [&] (int k) { return fftData[static_cast<size_t> (juce::jlimit (0, fftSize / 2 - 1, k))] * norm; };

    const float dt = 1.0f / 30.0f;
    for (int i = 0; i < numPoints; ++i)
    {
        const double f0 = minFreq * std::pow (maxFreq / minFreq, static_cast<double> (i) / numPoints);
        const double f1 = minFreq * std::pow (maxFreq / minFreq, static_cast<double> (i + 1) / numPoints);
        const double k0 = f0 / binHz, k1 = f1 / binHz;

        float level;
        if (k1 - k0 < 1.0)
        {
            const double k = 0.5 * (k0 + k1);
            const int lo = static_cast<int> (std::floor (k));
            const float t = static_cast<float> (k - lo);
            level = bin (lo) * (1.0f - t) + bin (lo + 1) * t; // graves: interpolación entre bins
        }
        else
        {
            level = 0.0f;
            for (int k = static_cast<int> (std::ceil (k0)); k <= static_cast<int> (std::floor (k1)); ++k)
                level = std::max (level, bin (k)); // agudos: pico del tramo
        }

        const float db = juce::jlimit (floorDb, 0.0f, juce::Decibels::gainToDecibels (level, floorDb));
        // Sube de inmediato y baja 15 dB por segundo.
        dbOut[static_cast<size_t> (i)] = std::max (db, dbOut[static_cast<size_t> (i)] - 15.0f * dt);
    }
}

void SpectrumView::rebuildCurve()
{
    const double sr = processor.getSampleRate() > 0.0 ? processor.getSampleRate() : 48000.0;

    curve.clear();
    for (int i = 0; i < curvePoints; ++i)
    {
        const double f = minFreq * std::pow (maxFreq / minFreq, static_cast<double> (i) / (curvePoints - 1));
        double sumDb = 0.0;
        for (int b = 0; b < params::numBands; ++b)
            if (realValue (b, params::bandOn) >= 0.5f)
                sumDb += peakingDb (realValue (b, params::bandFreq), realValue (b, params::bandGain),
                                    realValue (b, params::bandQ), f, sr);

        const float x = freqToX (f), y = dbToY (curveOffset + curveScale * sumDb);
        i == 0 ? curve.startNewSubPath (x, y) : curve.lineTo (x, y);
    }
}

void SpectrumView::timerCallback()
{
    analyze (processor.getInputRing(), inDb);
    analyze (processor.getOutputRing(), outDb);

    // La curva solo se recalcula cuando cambia algún parámetro de banda.
    std::array<float, 5 * 4> now {};
    for (int b = 0; b < params::numBands; ++b)
    {
        now[size_t (b * 4 + 0)] = realValue (b, params::bandFreq);
        now[size_t (b * 4 + 1)] = realValue (b, params::bandGain);
        now[size_t (b * 4 + 2)] = realValue (b, params::bandQ);
        now[size_t (b * 4 + 3)] = realValue (b, params::bandOn);
    }
    if (now != lastParams || std::abs (processor.getSampleRate() - lastSampleRate) > 0.5)
    {
        lastParams = now;
        lastSampleRate = processor.getSampleRate();
        rebuildCurve();
    }
    overlay->repaint();
}

// ------------------------------------------------------------------ dibujo

juce::Path SpectrumView::makePath (const std::array<float, numPoints>& db, bool closed) const
{
    juce::Path p;
    for (int i = 0; i < numPoints; ++i)
    {
        const double f = minFreq * std::pow (maxFreq / minFreq, (static_cast<double> (i) + 0.5) / numPoints);
        const float x = freqToX (f), y = dbToY (db[static_cast<size_t> (i)]);
        if (i == 0)
        {
            if (closed)
            {
                p.startNewSubPath (plotX, plotY + plotH);
                p.lineTo (x, y);
            }
            else
                p.startNewSubPath (x, y);
        }
        else
            p.lineTo (x, y);
    }
    if (closed)
    {
        p.lineTo (plotX + plotW, plotY + plotH);
        p.closeSubPath();
    }
    return p;
}

void SpectrumView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (bounds, 14.0f);
    g.setColour (border);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 14.0f, 1.0f);

    // Título y leyenda.
    g.setColour (textDim);
    g.setFont (ui (11.5f, true).withExtraKerningFactor (0.1f));
    g.drawText (juce::CharPointer_UTF8 ("ANALIZADOR · EQ DE REALCE"), juce::Rectangle<float> (17.0f, 17.0f, 400.0f, 18.0f), juce::Justification::centredLeft);

    struct Legend { const char* text; juce::Colour colour; bool fill; };
    const Legend legend[] = { { "Entrada", inputStroke, true }, { "Salida saturada", accent, false }, { "Curva del EQ", text, false } };
    float x = bounds.getRight() - 17.0f;
    g.setFont (ui (12.0f));
    for (int i = 2; i >= 0; --i)
    {
        const float w = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), legend[i].text);
        x -= w;
        g.setColour (textDim);
        g.drawText (legend[i].text, juce::Rectangle<float> (x, 17.0f, w + 2.0f, 18.0f), juce::Justification::centredLeft);
        x -= 8.0f;
        g.setColour (legend[i].colour);
        if (legend[i].fill)
            g.fillRoundedRectangle (x - 12.0f, 22.0f, 12.0f, 8.0f, 2.0f);
        else
            g.fillRoundedRectangle (x - 14.0f, 24.0f, 14.0f, 3.0f, 1.5f);
        x -= 14.0f + 18.0f;
    }

}

void SpectrumView::paintOverlay (juce::Graphics& g)
{
    // Espectros y curva, recortados al plano.
    {
        juce::Graphics::ScopedSaveState state (g);
        g.reduceClipRegion (juce::Rectangle<float> (plotX, plotY, plotW, plotH).getSmallestIntegerContainer());

        const auto in = makePath (inDb, true);
        g.setColour (inputFill);
        g.fillPath (in);
        g.setColour (inputStroke);
        g.strokePath (makePath (inDb, false), juce::PathStrokeType (1.0f));

        g.setColour (accent);
        g.strokePath (makePath (outDb, false), juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        g.setColour (text);
        g.strokePath (curve, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    drawNodes (g);
}

void SpectrumView::drawNodes (juce::Graphics& g)
{
    // La banda seleccionada se pinta la última (encima) con su aro y su guía a la escala.
    for (int pass = 0; pass < 2; ++pass)
        for (int b = 0; b < params::numBands; ++b)
        {
            const bool isSel = b == selected;
            if ((pass == 0) == isSel)
                continue;

            const auto c = nodePosition (b);
            const bool on = realValue (b, params::bandOn) >= 0.5f;
            const auto colour = bandColour[size_t (b)].withAlpha (on ? 1.0f : 0.35f);

            if (isSel)
            {
                const float dash[] = { 3.0f, 3.0f };
                g.setColour (colour.withAlpha (0.55f));
                g.drawDashedLine (juce::Line<float> (c.x, c.y + 16.0f, c.x, plotY + plotH), dash, 2, 1.0f);
                g.setColour (colour.withAlpha (0.18f));
                g.fillEllipse (c.x - 16.0f, c.y - 16.0f, 32.0f, 32.0f);
                g.setColour (colour.withAlpha (0.8f));
                g.drawEllipse (c.x - 16.0f, c.y - 16.0f, 32.0f, 32.0f, 1.5f);
            }

            g.setColour (bg);
            g.fillEllipse (c.x - nodeRadius - 2.0f, c.y - nodeRadius - 2.0f, (nodeRadius + 2.0f) * 2.0f, (nodeRadius + 2.0f) * 2.0f);
            g.setColour (colour);
            g.fillEllipse (c.x - nodeRadius, c.y - nodeRadius, nodeRadius * 2.0f, nodeRadius * 2.0f);
            g.setColour (bg);
            g.setFont (ui (11.0f, true));
            g.drawText (juce::String (b + 1), juce::Rectangle<float> (c.x - nodeRadius, c.y - nodeRadius, nodeRadius * 2.0f, nodeRadius * 2.0f),
                        juce::Justification::centred);
        }

    if (selected >= 0)
        drawLabel (g, selected);
}

void SpectrumView::drawLabel (juce::Graphics& g, int band)
{
    const auto c = nodePosition (band);
    const auto type = juce::jlimit (0, 4, juce::roundToInt (realValue (band, params::bandType)));

    const auto dot = juce::String::fromUTF8 (" · ");
    const auto line1 = hz (realValue (band, params::bandFreq)) + dot + signedDb (realValue (band, params::bandGain));
    const auto line2 = "Q " + comma (realValue (band, params::bandQ), 1) + dot + juce::String::fromUTF8 (typeNames[type]);

    const float w = 136.0f, h = 46.0f;
    float x = juce::jlimit (plotX + 2.0f, plotX + plotW - w - 2.0f, c.x - w * 0.2f);
    float y = c.y - 22.0f - h;
    if (y < plotY + 2.0f)
        y = c.y + 24.0f;

    const juce::Rectangle<float> r (x, y, w, h);
    g.setColour (panel.withAlpha (0.96f));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (bandColour[size_t (band)].withAlpha (0.9f));
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);

    g.setColour (text);
    g.setFont (mono (12.0f, true));
    g.drawText (line1, r.withTrimmedTop (6.0f).withHeight (18.0f).reduced (9.0f, 0.0f), juce::Justification::centredLeft);
    g.setColour (textDim);
    g.setFont (mono (11.5f));
    g.drawText (line2, r.withTrimmedTop (24.0f).withHeight (16.0f).reduced (9.0f, 0.0f), juce::Justification::centredLeft);
}

// ------------------------------------------------------------------ ratón

void SpectrumView::mouseMove (const juce::MouseEvent& e)
{
    hoverBand = bandAt (e.position);
    setMouseCursor (hoverBand >= 0 ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
}

void SpectrumView::mouseDown (const juce::MouseEvent& e)
{
    dragBand = bandAt (e.position);
    if (dragBand < 0)
        return;

    onSelect (dragBand);
    dragOffset = nodePosition (dragBand) - e.position;
    param (dragBand, params::bandFreq)->beginChangeGesture();
    param (dragBand, params::bandGain)->beginChangeGesture();
}

void SpectrumView::mouseDrag (const juce::MouseEvent& e)
{
    if (dragBand < 0)
        return;

    const auto p = e.position + dragOffset;
    const double db = yToDb (p.y);
    setReal (dragBand, params::bandFreq, static_cast<float> (xToFreq (p.x)));
    setReal (dragBand, params::bandGain, static_cast<float> ((db - curveOffset) / curveScale));
}

void SpectrumView::mouseUp (const juce::MouseEvent&)
{
    if (dragBand < 0)
        return;
    param (dragBand, params::bandFreq)->endChangeGesture();
    param (dragBand, params::bandGain)->endChangeGesture();
    dragBand = -1;
}

void SpectrumView::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int b = bandAt (e.position);
    if (b < 0)
        return;

    // Doble clic: la banda vuelve a su valor de fábrica.
    for (auto* suffix : { params::bandFreq, params::bandGain, params::bandQ })
    {
        auto* p = param (b, suffix);
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->getDefaultValue());
        p->endChangeGesture();
    }
    onSelect (b);
}

void SpectrumView::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& d)
{
    // La rueda cambia la Q de la banda bajo el cursor (o de la seleccionada).
    int b = bandAt (e.position);
    if (b < 0)
        b = selected;
    if (b < 0 || std::abs (d.deltaY) < 1.0e-4f)
        return;

    auto* p = param (b, params::bandQ);
    const float q = realValue (b, params::bandQ) * std::pow (1.12f, d.deltaY * (d.isReversed ? -4.0f : 4.0f));
    p->beginChangeGesture();
    p->setValueNotifyingHost (p->convertTo0to1 (q));
    p->endChangeGesture();
}
} // namespace poty::ui
