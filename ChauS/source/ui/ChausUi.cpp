#include "ChausUi.h"

#include <Format.h>

#include "../dsp/EqMath.h"

using namespace ferb::vintage;

namespace chaus::ui
{
namespace
{
const juce::Colour cream { 0xffefe5cf };
const juce::Colour coral { 0xffff6b5e };
const juce::Colour amber { 0xffffb347 };
const juce::Colour violetFill { 0xff8f78be };
const juce::Colour violetLine { 0xffb3a0dc };

juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
} // namespace

// ───────────────────────────────── Analizador ─────────────────────────────────

SpectrumAnalyzer::SpectrumAnalyzer()
    : samples (static_cast<size_t> (fftSize + 3 * (fftSize / 4)), 0.0f),
      fftData (static_cast<size_t> (fftSize * 2), 0.0f),
      power (static_cast<size_t> (fftSize / 2 + 1), 0.0),
      cumulative (static_cast<size_t> (fftSize / 2 + 2), 0.0)
{
}

float SpectrumAnalyzer::pointFrequency (int index) noexcept
{
    return minFreq * std::pow (maxFreq / minFreq, static_cast<float> (index) / static_cast<float> (numPoints - 1));
}

bool SpectrumAnalyzer::analyse (const ferb::SampleRing& ring, double sampleRate, std::array<float, numPoints>& dbOut)
{
    ring.copyLatest (samples.data(), static_cast<int> (samples.size()));
    std::fill (power.begin(), power.end(), 0.0);

    constexpr int hop = fftSize / 4; // 75 % de solape
    for (int w = 0; w < 4; ++w)
    {
        std::copy_n (samples.begin() + w * hop, fftSize, fftData.begin());
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.0f);
        window.multiplyWithWindowingTable (fftData.data(), static_cast<size_t> (fftSize));
        fft.performFrequencyOnlyForwardTransform (fftData.data());
        for (int k = 0; k <= fftSize / 2; ++k)
            power[static_cast<size_t> (k)] += static_cast<double> (fftData[static_cast<size_t> (k)]) * fftData[static_cast<size_t> (k)] * 0.25;
    }

    // Potencia acumulada por bin: la banda de 1/6 de octava se integra con bordes fraccionarios.
    const double scale = 2.0 / (static_cast<double> (fftSize) * fftSize * 0.375); // media cuadrática (Hann)
    cumulative[0] = 0.0;
    for (int k = 0; k <= fftSize / 2; ++k)
        cumulative[static_cast<size_t> (k + 1)] = cumulative[static_cast<size_t> (k)] + power[static_cast<size_t> (k)] * scale;

    const double binHz = sampleRate / fftSize;
    const auto cumulativeAt = [&] (double bin)
    {
        bin = std::clamp (bin, 0.0, static_cast<double> (fftSize / 2 + 1));
        const int i = std::min (static_cast<int> (bin), fftSize / 2);
        return cumulative[static_cast<size_t> (i)] + (bin - i) * (cumulative[static_cast<size_t> (i + 1)] - cumulative[static_cast<size_t> (i)]);
    };

    constexpr double sixth = 0.5 / 6.0; // ±1/12 de octava
    for (int i = 0; i < numPoints; ++i)
    {
        const double f = pointFrequency (i);
        const double lo = f * std::pow (2.0, -sixth) / binHz + 0.5, hi = f * std::pow (2.0, sixth) / binHz + 0.5;
        double band = cumulativeAt (hi) - cumulativeAt (lo);
        if (hi - lo < 1.0) // bandas más estrechas que un bin (graves): se reparte la potencia de un bin por su ancho
            band = (cumulativeAt (std::floor (f / binHz) + 1.0) - cumulativeAt (std::floor (f / binHz))) * (hi - lo);
        dbOut[static_cast<size_t> (i)] = static_cast<float> (10.0 * std::log10 (std::max (band, 1.0e-12)));
    }
    return true;
}

// ───────────────────────────────── Pantalla ─────────────────────────────────

void ScreenView::paintContent (juce::Graphics& g, float W, float H)
{
    constexpr float fMin = 200.0f, fMax = 20000.0f;
    const auto fx = [&] (float f) { return std::log (f / fMin) / std::log (fMax / fMin) * W; };
    const auto dbY = [&] (float d) { return H - std::clamp ((d + 85.0f) / 65.0f, 0.0f, 1.0f) * (H - 22.0f) - 6.0f; };

    // Zona de sibilante entre HP y LP.
    const float hx = fx (data.hpHz), lx = fx (data.lpHz);
    g.setColour (coral.withAlpha (0.07f));
    g.fillRect (hx, 0.0f, lx - hx, H);
    g.setColour (coral.withAlpha (0.55f));
    g.fillRect (hx, 0.0f, 1.0f, H);
    g.fillRect (lx, 0.0f, 1.0f, H);

    // Rejilla y rótulos.
    g.setFont (digitFont (10.0f));
    for (int k : { 1, 2, 5, 10, 20 })
    {
        const float x = fx (static_cast<float> (k) * 1000.0f);
        g.setColour (cream.withAlpha (0.09f));
        g.fillRect (x, 0.0f, 1.0f, H);
        g.setColour (cream.withAlpha (0.6f));
        const float cx = std::clamp (x, 14.0f, W - 14.0f);
        g.drawText (juce::String (k) + " k", juce::Rectangle<float> (cx - 20.0f, H - 17.0f, 40.0f, 12.0f), juce::Justification::centred, false);
    }
    for (int d : { -20, -40, -60 })
    {
        const float y = dbY (static_cast<float> (d));
        g.setColour (cream.withAlpha (0.09f));
        g.fillRect (0.0f, y, W, 1.0f);
        g.setColour (cream.withAlpha (0.5f));
        g.drawText (U ("−") + juce::String (-d), juce::Rectangle<float> (6.0f, y - 14.0f, 30.0f, 12.0f), juce::Justification::centredLeft, false);
    }

    // Espectros.
    constexpr int N = SpectrumAnalyzer::numPoints;
    const auto px = [&] (int i) { return static_cast<float> (i) * W / static_cast<float> (N - 1); };
    juce::Path inLine, outLine, inFill, removed;
    for (int i = 0; i < N; ++i)
    {
        const float yi = dbY (data.input[static_cast<size_t> (i)]), yo = dbY (data.output[static_cast<size_t> (i)]);
        i == 0 ? inLine.startNewSubPath (px (i), yi) : inLine.lineTo (px (i), yi);
        i == 0 ? outLine.startNewSubPath (px (i), yo) : outLine.lineTo (px (i), yo);
    }
    inFill = inLine;
    inFill.lineTo (W, H);
    inFill.lineTo (0.0f, H);
    inFill.closeSubPath();
    g.setColour (violetFill.withAlpha (0.30f));
    g.fillPath (inFill);
    g.setColour (violetLine.withAlpha (0.8f));
    g.strokePath (inLine, juce::PathStrokeType (1.2f));

    // Lo que se quita: el área entre la entrada y la salida.
    for (int i = 0; i < N; ++i)
        i == 0 ? removed.startNewSubPath (px (i), dbY (data.input[static_cast<size_t> (i)])) : removed.lineTo (px (i), dbY (data.input[static_cast<size_t> (i)]));
    for (int i = N - 1; i >= 0; --i)
        removed.lineTo (px (i), dbY (data.output[static_cast<size_t> (i)]));
    removed.closeSubPath();
    g.setColour (coral.withAlpha (data.listening ? 0.95f : 0.75f));
    g.fillPath (removed);
    g.setColour (cream);
    g.strokePath (outLine, juce::PathStrokeType (1.6f));

    // Campana dinámica: la respuesta real del filtro (campana, estante o la mezcla), colgando de la línea superior.
    const double fs = data.sampleRate;
    const double fc = std::clamp (static_cast<double> (data.centreHz), 200.0, 0.45 * fs);
    const auto bell = eq::peaking (fs, fc, data.q, -data.reductionDb);
    const auto shelf = ferb::Biquad::highShelf (fs, fc, -data.reductionDb);
    juce::Path curve;
    for (int i = 0; i <= 120; ++i)
    {
        const double f = fMin * std::pow (static_cast<double> (fMax / fMin), i / 120.0);
        const double db = (1.0 - data.shapeWeight) * eq::magnitudeDb (bell, f, fs) + data.shapeWeight * eq::magnitudeDb (shelf, f, fs);
        const float y = 22.0f - static_cast<float> (db) * 3.2f;
        const float x = static_cast<float> (i) * W / 120.0f;
        i == 0 ? curve.startNewSubPath (x, y) : curve.lineTo (x, y);
    }
    g.setColour (coral.withAlpha (0.95f));
    g.strokePath (curve, juce::PathStrokeType (2.0f));

    const float bx = fx (static_cast<float> (fc)), by = 22.0f + data.reductionDb * 3.2f;
    g.setColour (coral);
    g.fillEllipse (bx - 5.0f, by - 5.0f, 10.0f, 10.0f);
    g.setColour (juce::Colours::white);
    g.drawEllipse (bx - 5.0f, by - 5.0f, 10.0f, 10.0f, 1.2f);

    g.setFont (digitFont (11.0f));
    g.setColour (juce::Colour (0xffffb9b2));
    const bool none = data.reductionDb < 0.05f;
    const auto label = ferb::fmt::comma (fc / 1000.0, 1) + " kHz  " + (none ? juce::String() : U ("−")) + ferb::fmt::comma (none ? 0.0f : data.reductionDb, 1) + " dB";
    g.drawText (label, juce::Rectangle<float> (std::clamp (bx + 10.0f, 6.0f, W - 110.0f), std::max (36.0f, by + 4.0f) - 10.0f, 110.0f, 14.0f), juce::Justification::centredLeft, false);

    // Leyenda.
    g.setColour (cream.withAlpha (0.7f));
    g.drawText ("entrada", juce::Rectangle<float> (10.0f, 5.0f, 60.0f, 14.0f), juce::Justification::centredLeft, false);
    g.setColour (cream);
    g.drawText ("salida", juce::Rectangle<float> (66.0f, 5.0f, 50.0f, 14.0f), juce::Justification::centredLeft, false);
    g.setColour (coral);
    g.drawText ("lo que se quita", juce::Rectangle<float> (112.0f, 5.0f, 110.0f, 14.0f), juce::Justification::centredLeft, false);
}

// ───────────────────────────────── Línea de tiempo ─────────────────────────────────

void TimelineView::paintContent (juce::Graphics& g, float W, float TH)
{
    constexpr float lowDb = -66.0f, highDb = -18.0f;
    const auto levelY = [&] (float db) { return TH - 6.0f - std::clamp ((db - lowDb) / (highDb - lowDb), 0.0f, 1.0f) * (TH - 14.0f); };

    g.setColour (cream.withAlpha (0.08f));
    for (float db : { -54.0f, -42.0f, -30.0f })
        g.fillRect (0.0f, levelY (db), W, 1.0f);

    const int N = static_cast<int> (points.size());
    const float step = W / static_cast<float> (numPoints - 1);
    const auto xAt = [&] (int i) { return W - static_cast<float> (N - 1 - i) * step; };

    // Eventos detectados.
    g.setColour (coral.withAlpha (0.22f));
    for (int i = 0; i < N; ++i)
        if (points[static_cast<size_t> (i)].detected)
            g.fillRect (xAt (i) - 1.0f, 0.0f, 3.0f, TH);

    // Nivel de la banda: relleno y línea.
    juce::Path level, fill;
    for (int i = 0; i < N; ++i)
    {
        const float x = xAt (i), y = levelY (points[static_cast<size_t> (i)].levelDb);
        i == 0 ? level.startNewSubPath (x, y) : level.lineTo (x, y);
    }
    fill = level;
    fill.lineTo (W, TH);
    fill.lineTo (xAt (0), TH);
    fill.closeSubPath();
    g.setColour (violetFill.withAlpha (0.35f));
    g.fillPath (fill);
    g.setColour (violetLine);
    g.strokePath (level, juce::PathStrokeType (1.2f));

    // Umbral efectivo.
    juce::Path threshold;
    for (int i = 0; i < N; ++i)
    {
        const float x = xAt (i), y = levelY (points[static_cast<size_t> (i)].thresholdDb);
        i == 0 ? threshold.startNewSubPath (x, y) : threshold.lineTo (x, y);
    }
    g.setColour (coral);
    g.strokePath (threshold, juce::PathStrokeType (1.6f));

    // Reducción: barras ámbar desde arriba (26 dB ocupan la altura útil).
    g.setColour (amber.withAlpha (0.85f));
    for (int i = 0; i < N; ++i)
    {
        const float h = points[static_cast<size_t> (i)].reductionDb / 26.0f * (TH - 14.0f);
        if (h > 0.5f)
            g.fillRect (xAt (i), 0.0f, 2.0f, h);
    }

    g.setFont (digitFont (10.0f));
    g.setColour (cream.withAlpha (0.75f));
    g.drawText (U ("nivel de la banda S"), juce::Rectangle<float> (8.0f, 2.0f, 110.0f, 12.0f), juce::Justification::centredLeft, false);
    g.setColour (coral);
    g.drawText ("umbral efectivo", juce::Rectangle<float> (120.0f, 2.0f, 100.0f, 12.0f), juce::Justification::centredLeft, false);
    g.setColour (amber);
    g.drawText (U ("reducción"), juce::Rectangle<float> (220.0f, 2.0f, 80.0f, 12.0f), juce::Justification::centredLeft, false);
}
} // namespace chaus::ui
