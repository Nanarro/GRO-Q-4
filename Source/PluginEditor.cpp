#include "PluginEditor.h"

using namespace groq;

namespace groqui
{
static juce::Font mkFont (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

// ======================================================================= LookAndFeel
GroqLookAndFeel::GroqLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff2b2535));
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff4a4258));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xffc8c0d8));
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1726));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff5b4a8a));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::Slider::trackColourId, juce::Colour (0xff4a3d78));
    setColour (juce::Slider::backgroundColourId, juce::Colour (0xff241f30));
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff4a4258));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

void GroqLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float start, float end, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float thick = juce::jmax (3.f, r * 0.14f);
    const float arcR = r - thick * 0.5f;

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, start, end, true);
    g.setColour (juce::Colour (0xff3a3347));
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    float zeroAng = start;
    if (s.getMinimum() < 0.0 && s.getMaximum() > 0.0)
        zeroAng = start + (float) s.valueToProportionOfLength (0.0) * (end - start);
    const float ang = start + pos * (end - start);

    juce::Path val;
    val.addCentredArc (c.x, c.y, arcR, arcR, 0.f, juce::jmin (zeroAng, ang), juce::jmax (zeroAng, ang), true);
    g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
    g.strokePath (val, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    const float br = r - thick * 1.6f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4458), c.x, c.y - br,
                                             juce::Colour (0xff1c1824), c.x, c.y + br, false));
    g.fillEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f);
    g.setColour (juce::Colour (0x40ffffff));
    g.drawEllipse (c.x - br, c.y - br, br * 2.f, br * 2.f, 1.f);

    const juce::Point<float> tip (c.x + br * 0.78f * std::sin (ang), c.y - br * 0.78f * std::cos (ang));
    g.setColour (s.findColour (juce::Slider::thumbColourId));
    g.fillEllipse (juce::Rectangle<float> (5.f, 5.f).withCentre (tip));

    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.setFont (mkFont (juce::jmax (8.f, r * 0.27f)));
    g.drawText (s.getTextFromValue (s.getValue()),
                juce::Rectangle<float> (c.x - br, c.y - br * 0.35f, br * 2.f, br * 0.7f),
                juce::Justification::centred, true);
}

void GroqLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& bt, const juce::Colour&, bool over, bool)
{
    if (bt.getComponentID() == "power") return;
    auto r = bt.getLocalBounds().toFloat().reduced (0.5f);
    juce::Colour fill = bt.getToggleState() ? juce::Colour (0xffd4a82c) : juce::Colour (0xff2b2535);
    if (over) fill = fill.brighter (0.15f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (juce::Colour (0xff4a4258));
    g.drawRoundedRectangle (r, 4.f, 1.f);
}

void GroqLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& bt, bool, bool)
{
    auto r = bt.getLocalBounds().toFloat();
    if (bt.getComponentID() == "power")
    {
        const auto c = r.getCentre();
        const float rad = juce::jmin (r.getWidth(), r.getHeight()) * 0.28f;
        juce::Path p;
        p.addCentredArc (c.x, c.y + 0.5f, rad, rad, 0.f, juce::MathConstants<float>::pi * 0.25f,
                         juce::MathConstants<float>::pi * 1.75f, true);
        p.startNewSubPath (c.x, c.y - rad - 1.f);
        p.lineTo (c.x, c.y - 1.f);
        g.setColour (bt.getToggleState() ? juce::Colour (0xffd4a82c) : juce::Colour (0xff7a7388));
        g.strokePath (p, juce::PathStrokeType (2.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        return;
    }
    g.setColour (bt.getToggleState() ? juce::Colour (0xff1a1522) : juce::Colours::white.withAlpha (0.9f));
    g.setFont (mkFont (12.f));
    g.drawText (bt.getButtonText(), bt.getLocalBounds(), juce::Justification::centred, true);
}

// ======================================================================= Graph
GraphComponent::GraphComponent (GroqAudioProcessor& p, std::function<void (int)> cb)
    : proc (p), onSelect (std::move (cb))
{
    pRange = proc.apvts.getRawParameterValue ("viewRange");
    pScale = proc.apvts.getRawParameterValue ("gainScale");
    pAnalyzer = proc.apvts.getRawParameterValue ("analyzer");
    specPre.fill (-120.f); specPost.fill (-120.f);
    startTimerHz (30);
}

float GraphComponent::rangeDb() const
{
    static const float r[] = { 3.f, 6.f, 12.f, 30.f };
    return r[juce::jlimit (0, 3, (int) std::lround (pRange->load()))];
}

void GraphComponent::rebuildTables()
{
    curveSr = proc.sr.load();
    if (getWidth() <= 20) return;
    for (int k = 0; k < kCurve; ++k)
    {
        const float x = plotW() * (float) k / (float) (kCurve - 1);
        const double f = xToFreq (x);
        const double w = 2.0 * juce::MathConstants<double>::pi * f / curveSr;
        cosw[(size_t) k] = std::cos (w); cos2w[(size_t) k] = std::cos (2.0 * w);
    }
}

void GraphComponent::resized() { rebuildTables(); }

float GraphComponent::effectiveGain (int b, const BandParams& p) const
{
    float g = p.gain;
    if (p.dyn && hasGain (p.type)) g += p.range * proc.dynAmt[b].load();
    return g * pScale->load() * 0.01f;
}

juce::Point<float> GraphComponent::nodePos (int b, const BandParams& p) const
{
    const float y = hasGain (p.type) ? dbToY (effectiveGain (b, p)) : dbToY (0.f);
    return { freqToX (juce::jlimit (10.f, 30000.f, p.freq)),
             juce::jlimit (8.f, (float) getHeight() - 8.f, y) };
}

int GraphComponent::hitTest (juce::Point<float> pt) const
{
    int best = -1; float bd = 11.f;
    for (int b = 0; b < kNumBands; ++b)
    {
        const auto p = proc.readBand (b);
        if (! p.on) continue;
        const float d = nodePos (b, p).getDistanceFrom (pt);
        if (d < bd) { bd = d; best = b; }
    }
    return best;
}

void GraphComponent::timerCallback()
{
    if (std::abs (proc.sr.load() - curveSr) > 1.0) rebuildTables();
    if (pAnalyzer->load() > 0.5f) updateSpectrum();
    for (int c = 0; c < 2; ++c)
        meter[c] = juce::jmax (proc.outPeak[c].exchange (0.f), meter[c] * 0.88f);
    repaint();
}

void GraphComponent::updateSpectrum()
{
    const int pos = proc.fifoPos.load (std::memory_order_acquire);
    const float sr = (float) proc.sr.load();
    auto process = [&] (const std::array<std::atomic<float>, GroqAudioProcessor::kFifo>& ring, std::array<float, kBins>& spec)
    {
        for (int i = 0; i < kN; ++i)
            fftData[(size_t) i] = ring[(size_t) ((pos - kN + i) & (GroqAudioProcessor::kFifo - 1))].load (std::memory_order_relaxed);
        for (int i = kN; i < kN * 2; ++i) fftData[(size_t) i] = 0.f;
        window.multiplyWithWindowingTable (fftData.data(), (size_t) kN);
        fft.performFrequencyOnlyForwardTransform (fftData.data());
        for (int i = 1; i < kBins; ++i)
        {
            const float amp = fftData[(size_t) i] * 4.f / (float) kN;
            const float f = (float) i * sr / (float) kN;
            float db = 20.f * std::log10 (amp + 1e-9f) + 4.5f * std::log2 (f / 1000.f);
            float& s = spec[(size_t) i];
            s += (db - s) * (db > s ? 0.6f : 0.12f);
        }
    };
    process (proc.preFifo, specPre);
    process (proc.postFifo, specPost);
}

void GraphComponent::paint (juce::Graphics& g)
{
    const float W = (float) getWidth(), H = (float) getHeight(), pw = plotW();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff231c31), 0, 0, juce::Colour (0xff13101a), 0, H, false));
    g.fillAll();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0x22806cc8), pw * 0.5f, H * 0.5f,
                                             juce::Colour (0x00000000), pw * 0.5f, 0.f, true));
    g.fillRect (0.f, 0.f, pw, H);

    // rejilla de frecuencias
    for (float dec : { 10.f, 100.f, 1000.f, 10000.f })
        for (int m = 1; m <= 9; ++m)
        {
            const float f = dec * (float) m;
            if (f < 10.f || f > 30000.f) continue;
            const float x = freqToX (f);
            const bool major = (m == 1 || m == 2 || m == 5);
            g.setColour (juce::Colours::white.withAlpha (major ? 0.10f : 0.04f));
            g.drawVerticalLine ((int) x, 0.f, H);
            if (major && f >= 20.f && f <= 20000.f)
            {
                g.setColour (juce::Colours::white.withAlpha (0.45f));
                g.setFont (mkFont (10.f, false));
                const juce::String t = f >= 1000.f ? juce::String ((int) (f / 1000.f)) + "k" : juce::String ((int) f);
                g.drawText (t, juce::Rectangle<float> (x - 20.f, H - 15.f, 40.f, 12.f), juce::Justification::centred);
            }
        }

    // rejilla de dB
    static const float steps[] = { 1.f, 2.f, 3.f, 10.f };
    const float R = rangeDb();
    const float step = steps[juce::jlimit (0, 3, (int) std::lround (pRange->load()))];
    for (float db = -R; db <= R + 0.01f; db += step)
    {
        const float y = dbToY (db);
        const bool zero = std::abs (db) < 0.01f;
        g.setColour (zero ? juce::Colour (0x908fd8e8) : juce::Colours::white.withAlpha (0.07f));
        g.drawHorizontalLine ((int) y, 0.f, pw);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.setFont (mkFont (10.f, false));
        const juce::String t = (db > 0.01f ? "+" : "") + juce::String (juce::roundToInt (db));
        g.drawText (t, juce::Rectangle<float> (pw - 36.f, y - 6.f, 30.f, 12.f), juce::Justification::centredRight);
    }

    // espectro
    if (pAnalyzer->load() > 0.5f && curveSr > 0.0)
    {
        auto buildPath = [&] (const std::array<float, kBins>& s, juce::Path& path)
        {
            const float binHz = (float) curveSr / (float) kN;
            bool first = true;
            for (float x = 0.f; x <= pw; x += 2.f)
            {
                const float b0 = xToFreq (x - 1.f) / binHz, b1 = xToFreq (x + 1.f) / binHz;
                float v;
                if (b1 - b0 < 1.f)
                {
                    const float bc = juce::jlimit (1.f, (float) kBins - 2.f, 0.5f * (b0 + b1));
                    const int i = (int) bc; const float fr = bc - (float) i;
                    v = s[(size_t) i] * (1.f - fr) + s[(size_t) i + 1] * fr;
                }
                else
                {
                    v = -200.f;
                    const int lo = juce::jmax (1, (int) b0), hi = juce::jmin (kBins - 1, (int) b1 + 1);
                    for (int i = lo; i <= hi; ++i) v = juce::jmax (v, s[(size_t) i]);
                }
                const float y = juce::jlimit (0.f, H, specToY (v));
                if (first) { path.startNewSubPath (x, y); first = false; } else path.lineTo (x, y);
            }
        };
        juce::Path pre, post;
        buildPath (specPre, pre);  buildPath (specPost, post);
        auto fillPath = [&] (juce::Path src)
        {
            src.lineTo (pw, H); src.lineTo (0.f, H); src.closeSubPath(); return src;
        };
        g.setColour (juce::Colour (0x1ac8c8e0));
        g.fillPath (fillPath (pre));
        g.setColour (juce::Colour (0x33b8b8d0));
        g.fillPath (fillPath (post));
        g.setColour (juce::Colour (0x70d0d0e4));
        g.strokePath (post, juce::PathStrokeType (1.f));
    }

    // curvas
    if (curveSr > 0.0)
    {
        std::array<float, kCurve> total {};
        std::array<float, kCurve> bdb {};
        const float y0 = dbToY (0.f);
        const float lim = R * 3.f;
        for (int b = 0; b < kNumBands; ++b)
        {
            const auto p = proc.readBand (b);
            if (! p.on) continue;
            Coef c[kMaxStages];
            const int ns = designBand (p, curveSr, juce::jlimit (-48.f, 48.f, effectiveGain (b, p)), c);
            for (int k = 0; k < kCurve; ++k)
            {
                double db = 0.0;
                for (int s = 0; s < ns; ++s)
                    db += 10.0 * std::log10 (std::max (magSq (c[s], cosw[(size_t) k], cos2w[(size_t) k]), 1e-12));
                bdb[(size_t) k] = juce::jlimit (-lim, lim, (float) db);
                total[(size_t) k] += (float) db;
            }
            juce::Path fill, line;
            for (int k = 0; k < kCurve; ++k)
            {
                const float x = pw * (float) k / (float) (kCurve - 1), y = dbToY (bdb[(size_t) k]);
                if (k == 0) { fill.startNewSubPath (x, y0); fill.lineTo (x, y); line.startNewSubPath (x, y); }
                else { fill.lineTo (x, y); line.lineTo (x, y); }
            }
            fill.lineTo (pw, y0); fill.closeSubPath();
            const auto col = bandColour (b);
            g.setColour (col.withAlpha (b == selected ? 0.42f : 0.24f));
            g.fillPath (fill);
            if (b == selected)
            {
                g.setColour (col.withAlpha (0.9f));
                g.strokePath (line, juce::PathStrokeType (1.3f));
            }
        }
        juce::Path tot;
        for (int k = 0; k < kCurve; ++k)
        {
            const float x = pw * (float) k / (float) (kCurve - 1);
            const float y = dbToY (juce::jlimit (-lim, lim, total[(size_t) k]));
            if (k == 0) tot.startNewSubPath (x, y); else tot.lineTo (x, y);
        }
        g.setColour (juce::Colour (0x28f2c230));
        g.strokePath (tot, juce::PathStrokeType (6.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xfff2c230));
        g.strokePath (tot, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // nodos
    for (int b = 0; b < kNumBands; ++b)
    {
        const auto p = proc.readBand (b);
        if (! p.on) continue;
        const auto pt = nodePos (b, p);
        const float r = (b == selected) ? 7.5f : (b == hoverBand ? 7.f : 6.f);
        if (b == selected)
        {
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.fillEllipse (pt.x - r - 4.f, pt.y - r - 4.f, (r + 4.f) * 2.f, (r + 4.f) * 2.f);
        }
        g.setColour (bandColour (b));
        g.fillEllipse (pt.x - r, pt.y - r, r * 2.f, r * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawEllipse (pt.x - r, pt.y - r, r * 2.f, r * 2.f, 1.5f);
        if (p.dyn && hasGain (p.type))
        {
            g.setColour (juce::Colour (0xff1a1522));
            g.fillEllipse (pt.x - 2.f, pt.y - 2.f, 4.f, 4.f);
        }
    }

    // medidor de salida
    g.setColour (juce::Colour (0xff0e0b14));
    g.fillRect (pw, 0.f, W - pw, H);
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawVerticalLine ((int) pw, 0.f, H);
    juce::ColourGradient grad (juce::Colour (0xffe04040), 0.f, 0.f, juce::Colour (0xff3fd07a), 0.f, H, false);
    grad.addColour (0.18, juce::Colour (0xfff0c040));
    for (int c = 0; c < 2; ++c)
    {
        const float db = 20.f * std::log10 (juce::jmax (meter[c], 1e-5f));
        const float h = juce::jlimit (0.f, 1.f, (db + 60.f) / 66.f) * H;
        g.setGradientFill (grad);
        g.fillRect (pw + 3.f + (float) c * 7.f, H - h, 6.f, h);
    }
}

void GraphComponent::setParam (const juce::String& id, float plain, bool gesture)
{
    if (auto* p = proc.apvts.getParameter (id))
    {
        if (gesture) p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (plain));
        if (gesture) p->endChangeGesture();
    }
}

void GraphComponent::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitTest (e.position);
    if (h != hoverBand) { hoverBand = h; repaint(); }
}

void GraphComponent::mouseDown (const juce::MouseEvent& e)
{
    const int hit = hitTest (e.position);
    if (e.mods.isPopupMenu())
    {
        if (hit >= 0) { selected = hit; onSelect (hit); showContextMenu (hit); }
        return;
    }
    if (hit < 0) return;
    selected = hit; onSelect (hit);
    dragBand = hit; dragging = true;
    for (auto* id : { "freq", "gain" })
        if (auto* p = proc.apvts.getParameter (GroqAudioProcessor::bandId (hit, id))) p->beginChangeGesture();
}

void GraphComponent::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging || dragBand < 0) return;
    const auto p = proc.readBand (dragBand);
    setParam (GroqAudioProcessor::bandId (dragBand, "freq"), juce::jlimit (10.f, 30000.f, xToFreq (e.position.x)), false);
    if (hasGain (p.type))
    {
        const float sc = juce::jmax (0.05f, pScale->load() * 0.01f);
        setParam (GroqAudioProcessor::bandId (dragBand, "gain"), juce::jlimit (-30.f, 30.f, yToDb (e.position.y) / sc), false);
    }
}

void GraphComponent::mouseUp (const juce::MouseEvent&)
{
    if (dragging && dragBand >= 0)
        for (auto* id : { "freq", "gain" })
            if (auto* p = proc.apvts.getParameter (GroqAudioProcessor::bandId (dragBand, id))) p->endChangeGesture();
    dragging = false; dragBand = -1;
}

void GraphComponent::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int hit = hitTest (e.position);
    if (hit >= 0) deleteBand (hit); else addBandAt (e.position);
}

void GraphComponent::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& w)
{
    const int b = hoverBand >= 0 ? hoverBand : selected;
    if (b < 0) return;
    const auto p = proc.readBand (b);
    if (! p.on) return;
    setParam (GroqAudioProcessor::bandId (b, "q"), juce::jlimit (0.1f, 40.f, p.q * std::exp (w.deltaY * 0.8f)), true);
}

void GraphComponent::deleteBand (int band)
{
    setParam (GroqAudioProcessor::bandId (band, "on"), 0.f, true);
    if (selected == band) { selected = -1; onSelect (-1); }
}

void GraphComponent::addBandAt (juce::Point<float> pt)
{
    for (int b = 0; b < kNumBands; ++b)
    {
        if (proc.readBand (b).on) continue;
        const auto id = [b] (const char* n) { return GroqAudioProcessor::bandId (b, n); };
        setParam (id ("type"), 0.f, true);
        setParam (id ("freq"), juce::jlimit (10.f, 30000.f, xToFreq (pt.x)), true);
        const float sc = juce::jmax (0.05f, pScale->load() * 0.01f);
        setParam (id ("gain"), juce::jlimit (-30.f, 30.f, yToDb (pt.y) / sc), true);
        setParam (id ("q"), 1.f, true);
        setParam (id ("ch"), 0.f, true);
        setParam (id ("dyn"), 0.f, true);
        setParam (id ("on"), 1.f, true);
        selected = b; onSelect (b);
        return;
    }
}

void GraphComponent::showContextMenu (int band)
{
    const auto p = proc.readBand (band);
    juce::PopupMenu m, types, chans;
    for (int i = 0; i < NumTypes; ++i) types.addItem (100 + i, kTypeNames[i], true, p.type == i);
    const char* chNames[] = { "Stereo", "Left", "Right", "Mid", "Side" };
    for (int i = 0; i < 5; ++i) chans.addItem (200 + i, chNames[i], true, p.ch == i);
    m.addItem (1, "Delete band");
    m.addSubMenu ("Shape", types);
    m.addSubMenu ("Channel", chans);
    m.addItem (3, "Dynamic", hasGain (p.type), p.dyn);

    juce::Component::SafePointer<GraphComponent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options(), [safe, band, p] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto id = [band] (const char* n) { return GroqAudioProcessor::bandId (band, n); };
        if (r == 1) safe->deleteBand (band);
        else if (r == 3) safe->setParam (id ("dyn"), p.dyn ? 0.f : 1.f, true);
        else if (r >= 100 && r < 200) safe->setParam (id ("type"), (float) (r - 100), true);
        else if (r >= 200) safe->setParam (id ("ch"), (float) (r - 200), true);
    });
}

// ======================================================================= Panel
BandPanel::BandPanel (juce::AudioProcessorValueTreeState& a) : apvts (a)
{
    power.setComponentID ("power");
    power.setClickingTogglesState (true);
    dynBtn.setClickingTogglesState (true);
    dynBtn.onStateChange = [this] { updateVisibility(); };
    addAndMakeVisible (power); addAndMakeVisible (dynBtn);

    typeBox.addItemList (juce::StringArray (kTypeNames, NumTypes), 1);
    slopeBox.addItemList (juce::StringArray (kSlopeNames, 8), 1);
    chBox.addItemList (juce::StringArray { "Stereo", "Left", "Right", "Mid", "Side" }, 1);
    typeBox.onChange = [this] { updateVisibility(); };
    addAndMakeVisible (typeBox); addAndMakeVisible (slopeBox); addAndMakeVisible (chBox);

    for (auto* s : { &freq, &gain, &q, &thr, &att, &rel, &range }) { setupKnob (*s); addAndMakeVisible (*s); }
    freq.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffe8b830));
    freq.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe8b830));
    gain.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xfff0eef8));
    gain.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe0463c));
    q.setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffe8b830));
    q.setColour (juce::Slider::thumbColourId, juce::Colour (0xffe8b830));
    for (auto* s : { &thr, &att, &rel, &range })
    {
        s->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff5fd0e0));
        s->setColour (juce::Slider::thumbColourId, juce::Colour (0xff5fd0e0));
    }
    setVisible (false);
}

void BandPanel::setupKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    s.setDoubleClickReturnValue (true, s.getValue());
}

void BandPanel::bind (int b)
{
    aFreq.reset(); aGain.reset(); aQ.reset(); aThr.reset(); aAtt.reset(); aRel.reset(); aRange.reset();
    aPower.reset(); aDyn.reset(); aType.reset(); aSlope.reset(); aCh.reset();
    band = b;
    if (b < 0) { setVisible (false); return; }

    auto id = [b] (const char* n) { return GroqAudioProcessor::bandId (b, n); };
    aPower = std::make_unique<BA> (apvts, id ("on"), power);
    aDyn   = std::make_unique<BA> (apvts, id ("dyn"), dynBtn);
    aType  = std::make_unique<CA> (apvts, id ("type"), typeBox);
    aSlope = std::make_unique<CA> (apvts, id ("slope"), slopeBox);
    aCh    = std::make_unique<CA> (apvts, id ("ch"), chBox);
    aFreq  = std::make_unique<SA> (apvts, id ("freq"), freq);
    aGain  = std::make_unique<SA> (apvts, id ("gain"), gain);
    aQ     = std::make_unique<SA> (apvts, id ("q"), q);
    aThr   = std::make_unique<SA> (apvts, id ("thr"), thr);
    aAtt   = std::make_unique<SA> (apvts, id ("att"), att);
    aRel   = std::make_unique<SA> (apvts, id ("rel"), rel);
    aRange = std::make_unique<SA> (apvts, id ("range"), range);
    const auto col = bandColour (b);
    freq.setColour (juce::Slider::rotarySliderFillColourId, col);
    q.setColour (juce::Slider::rotarySliderFillColourId, col);
    setVisible (true);
    updateVisibility();
    repaint();
}

void BandPanel::updateVisibility()
{
    const int t = juce::jmax (0, typeBox.getSelectedItemIndex());
    slopeBox.setVisible (isCut (t));
    gain.setEnabled (hasGain (t));
    gain.setAlpha (hasGain (t) ? 1.f : 0.35f);
    dynBtn.setEnabled (hasGain (t));
    const float a = (dynBtn.getToggleState() && hasGain (t)) ? 1.f : 0.35f;
    for (auto* s : { &thr, &att, &rel, &range }) s->setAlpha (a);
    repaint();
}

void BandPanel::resized()
{
    power.setBounds (10, 10, 24, 24);
    typeBox.setBounds (40, 10, 104, 24);
    slopeBox.setBounds (10, 42, 134, 24);
    chBox.setBounds (10, 76, 84, 24);
    dynBtn.setBounds (98, 76, 46, 24);
    freq.setBounds (160, 10, 68, 68);  labelFreq  = { 160, 82, 68, 14 };
    gain.setBounds (240, 4, 84, 84);   labelGain  = { 240, 90, 84, 14 };
    q.setBounds (336, 10, 68, 68);     labelQ     = { 336, 82, 68, 14 };
    int x = 430;
    thr.setBounds (x, 16, 54, 54);     labelThr   = { x, 74, 54, 14 }; x += 60;
    att.setBounds (x, 16, 54, 54);     labelAtt   = { x, 74, 54, 14 }; x += 60;
    rel.setBounds (x, 16, 54, 54);     labelRel   = { x, 74, 54, 14 }; x += 60;
    range.setBounds (x, 16, 54, 54);   labelRange = { x, 74, 54, 14 };
}

void BandPanel::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.f);
    g.setColour (juce::Colour (0xe8141018));
    g.fillRoundedRectangle (r, 8.f);
    g.setColour (juce::Colour (0x38ffffff));
    g.drawRoundedRectangle (r, 8.f, 1.f);
    if (band >= 0)
    {
        g.setColour (bandColour (band));
        g.fillRoundedRectangle (juce::Rectangle<float> (1.f, 14.f, 3.f, r.getHeight() - 28.f), 1.5f);
    }
    g.setColour (juce::Colours::white.withAlpha (0.12f));
    g.drawVerticalLine (418, 14.f, r.getHeight() - 14.f);
    g.setFont (mkFont (10.f));
    auto lab = [&] (juce::Rectangle<int> rc, const char* t, float alpha)
    {
        g.setColour (juce::Colours::white.withAlpha (alpha));
        g.drawText (t, rc, juce::Justification::centred);
    };
    lab (labelFreq, "FREQ", 0.6f); lab (labelGain, "GAIN", gain.isEnabled() ? 0.6f : 0.25f); lab (labelQ, "Q", 0.6f);
    const float a = (dynBtn.getToggleState() && dynBtn.isEnabled()) ? 0.6f : 0.25f;
    lab (labelThr, "THRESH", a); lab (labelAtt, "ATTACK", a); lab (labelRel, "RELEASE", a); lab (labelRange, "RANGE", a);
}
} // namespace groqui

// ============================================================================ Editor
namespace { constexpr int kW = 900, kH = 540, kTop = 42, kBottom = 30; }

GroqAudioProcessorEditor::GroqAudioProcessorEditor (GroqAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p),
      graph (p, [this] (int b) { selectBand (b); }),
      panel (p.apvts)
{
    setLookAndFeel (&laf);
    addAndMakeVisible (graph);
    addAndMakeVisible (panel);

    presetBtn.onClick = [this]
    {
        juce::PopupMenu m;
        for (int i = 0; i < proc.getNumPrograms(); ++i)
            m.addItem (i + 1, proc.getProgramName (i), true, i == proc.getCurrentProgram());
        juce::Component::SafePointer<GroqAudioProcessorEditor> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [safe] (int r)
        {
            if (safe != nullptr && r > 0) safe->loadPreset (r - 1);
        });
    };
    prevBtn.onClick = [this] { const int n = proc.getNumPrograms(); loadPreset ((proc.getCurrentProgram() + n - 1) % n); };
    nextBtn.onClick = [this] { loadPreset ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    importBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Importar preset de FabFilter Pro-Q", juce::File(), "*.ffp");
        juce::Component::SafePointer<GroqAudioProcessorEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safe] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (safe == nullptr || ! f.existsAsFile()) return;
            const auto report = safe->proc.importFfp (f.loadFileAsString());
            if (! report.startsWith ("ERROR"))
            {
                safe->presetBtn.setButtonText (f.getFileNameWithoutExtension());
                safe->selectFirstEnabled();
            }
            juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "Importar .ffp", report);
        });
    };
    for (auto* b : { &prevBtn, &nextBtn, &presetBtn, &importBtn }) addAndMakeVisible (*b);

    rangeBox.addItemList (juce::StringArray { "3 dB", "6 dB", "12 dB", "30 dB" }, 1);
    charBox.addItemList (juce::StringArray { "Clean", "Subtle", "Warm" }, 1);
    addAndMakeVisible (rangeBox); addAndMakeVisible (charBox);
    for (auto* b : { &scBtn, &anaBtn, &autoBtn }) { b->setClickingTogglesState (true); addAndMakeVisible (*b); }
    for (auto* s : { &scaleSlider, &outSlider })
    {
        s->setSliderStyle (juce::Slider::LinearBar);
        s->setTextBoxStyle (juce::Slider::TextBoxLeft, false, 80, 20);
        addAndMakeVisible (*s);
    }
    aRange = std::make_unique<CA> (proc.apvts, "viewRange", rangeBox);
    aChar  = std::make_unique<CA> (proc.apvts, "character", charBox);
    aSC    = std::make_unique<BA> (proc.apvts, "extSC", scBtn);
    aAna   = std::make_unique<BA> (proc.apvts, "analyzer", anaBtn);
    aAuto  = std::make_unique<BA> (proc.apvts, "autoGain", autoBtn);
    aScale = std::make_unique<SA> (proc.apvts, "gainScale", scaleSlider);
    aOut   = std::make_unique<SA> (proc.apvts, "outGain", outSlider);

    presetBtn.setButtonText (proc.getProgramName (proc.getCurrentProgram()));
    lastPreset = proc.getCurrentProgram();
    setSize (kW, kH);
    selectFirstEnabled();
    startTimerHz (4);
}

GroqAudioProcessorEditor::~GroqAudioProcessorEditor() { setLookAndFeel (nullptr); }

void GroqAudioProcessorEditor::selectBand (int b)
{
    graph.selected = b;
    panel.bind (b);
    graph.repaint();
}

void GroqAudioProcessorEditor::selectFirstEnabled()
{
    for (int b = 0; b < groq::kNumBands; ++b)
        if (proc.readBand (b).on) { selectBand (b); return; }
    selectBand (-1);
}

void GroqAudioProcessorEditor::loadPreset (int i)
{
    proc.setCurrentProgram (i);
    presetBtn.setButtonText (proc.getProgramName (proc.getCurrentProgram()));
    lastPreset = proc.getCurrentProgram();
    selectFirstEnabled();
}

void GroqAudioProcessorEditor::timerCallback()
{
    if (proc.getCurrentProgram() != lastPreset)
    {
        lastPreset = proc.getCurrentProgram();
        presetBtn.setButtonText (proc.getProgramName (lastPreset));
    }
}

void GroqAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff13101a));
    // barra superior
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2e2839), 0, 0, juce::Colour (0xff1b1724), 0, (float) kTop, false));
    g.fillRect (0, 0, kW, kTop);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (kTop - 1, 0.f, (float) kW);

    // logo propio
    auto plate = juce::Rectangle<float> (12.f, 6.f, 104.f, 30.f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0cc58), plate.getX(), plate.getY(),
                                             juce::Colour (0xffc08a1c), plate.getX(), plate.getBottom(), false));
    g.fillRoundedRectangle (plate, 6.f);
    g.setColour (juce::Colour (0xff1a1522));
    g.setFont (groqui::mkFont (21.f));
    g.drawText ("GRO Q", plate.withTrimmedRight (22.f).toNearestInt(), juce::Justification::centred);
    g.setFont (groqui::mkFont (14.f));
    g.drawText ("4", juce::Rectangle<int> ((int) plate.getRight() - 24, 8, 16, 16), juce::Justification::centred);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.setFont (groqui::mkFont (10.f, false));
    g.drawText ("parametric equalizer", 124, 14, 130, 14, juce::Justification::centredLeft);

    // barra inferior
    const int by = kH - kBottom;
    g.setColour (juce::Colour (0xff191520));
    g.fillRect (0, by, kW, kBottom);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawHorizontalLine (by, 0.f, (float) kW);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.setFont (groqui::mkFont (10.f));
    g.drawText ("SCALE", scaleLabel, juce::Justification::centredRight);
    g.drawText ("OUTPUT", outLabel, juce::Justification::centredRight);
}

void GroqAudioProcessorEditor::resized()
{
    const int gh = kH - kTop - kBottom;
    graph.setBounds (0, kTop, kW, gh);
    panel.setBounds ((kW - 20 - 680) / 2, kTop + gh - 116 - 22, 680, 116);

    prevBtn.setBounds (kW / 2 - 130, 9, 24, 24);
    presetBtn.setBounds (kW / 2 - 100, 9, 200, 24);
    nextBtn.setBounds (kW / 2 + 106, 9, 24, 24);
    rangeBox.setBounds (kW - 96, 9, 84, 24);
    importBtn.setBounds (kW - 96 - 98, 9, 90, 24);

    const int y = kH - kBottom + 3;
    charBox.setBounds (8, y, 90, 24);
    scBtn.setBounds (106, y, 64, 24);
    anaBtn.setBounds (176, y, 76, 24);
    autoBtn.setBounds (258, y, 84, 24);
    scaleLabel = { 346, y, 48, 24 };
    scaleSlider.setBounds (400, y, 100, 24);
    outLabel = { 506, y, 58, 24 };
    outSlider.setBounds (570, y, 110, 24);
}
