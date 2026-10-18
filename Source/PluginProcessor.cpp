#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace groq;
using APVTS = juce::AudioProcessorValueTreeState;

namespace
{
juce::String fmtFreq (float v, int)
{
    return v >= 1000.f ? juce::String (v / 1000.f, v >= 10000.f ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
}
juce::String fmtDb (float v, int) { return (v > 0.f ? "+" : "") + juce::String (v, 1) + " dB"; }
juce::String fmtQ (float v, int) { return juce::String (v, 2); }
juce::String fmtMs (float v, int) { return v >= 1000.f ? juce::String (v / 1000.f, 2) + " s" : juce::String (v, v < 10.f ? 1 : 0) + " ms"; }
juce::String fmtPct (float v, int) { return juce::String (juce::roundToInt (v)) + " %"; }

inline float satPade (float x)
{
    x = juce::jlimit (-3.f, 3.f, x);
    return x * (27.f + x * x) / (27.f + 9.f * x * x);
}
}

APVTS::ParameterLayout GroqAudioProcessor::createLayout()
{
    using namespace juce;
    APVTS::ParameterLayout layout;

    for (int b = 0; b < kNumBands; ++b)
    {
        const String n = "Band " + String (b + 1) + " ";
        const float defF = 40.f * std::pow (400.f, (float) b / (float) (kNumBands - 1));

        NormalisableRange<float> fr (10.f, 30000.f, 0.01f); fr.setSkewForCentre (1000.f);
        NormalisableRange<float> qr (0.1f, 40.f, 0.001f);   qr.setSkewForCentre (1.f);
        NormalisableRange<float> ar (0.1f, 500.f, 0.01f);   ar.setSkewForCentre (20.f);
        NormalisableRange<float> rr (5.f, 3000.f, 0.01f);   rr.setSkewForCentre (150.f);

        layout.add (std::make_unique<AudioParameterBool> (ParameterID { bandId (b, "on"), 1 }, n + "On", false));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { bandId (b, "type"), 1 }, n + "Type",
                    StringArray (kTypeNames, NumTypes), 0));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "freq"), 1 }, n + "Freq", fr, defF,
                    AudioParameterFloatAttributes().withStringFromValueFunction (fmtFreq)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "gain"), 1 }, n + "Gain",
                    NormalisableRange<float> (-30.f, 30.f, 0.01f), 0.f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (fmtDb)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "q"), 1 }, n + "Q", qr, 1.f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (fmtQ)));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { bandId (b, "slope"), 1 }, n + "Slope",
                    StringArray (kSlopeNames, 8), 3));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { bandId (b, "ch"), 1 }, n + "Channel",
                    StringArray { "Stereo", "Left", "Right", "Mid", "Side" }, 0));
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { bandId (b, "dyn"), 1 }, n + "Dynamic", false));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "thr"), 1 }, n + "Threshold",
                    NormalisableRange<float> (-80.f, 0.f, 0.1f), -30.f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (fmtDb)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "att"), 1 }, n + "Attack", ar, 10.f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (fmtMs)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "rel"), 1 }, n + "Release", rr, 120.f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (fmtMs)));
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { bandId (b, "range"), 1 }, n + "Range",
                    NormalisableRange<float> (-30.f, 30.f, 0.1f), -6.f,
                    AudioParameterFloatAttributes().withStringFromValueFunction (fmtDb)));
    }

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "outGain", 1 }, "Output Gain",
                NormalisableRange<float> (-24.f, 24.f, 0.01f), 0.f,
                AudioParameterFloatAttributes().withStringFromValueFunction (fmtDb)));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "gainScale", 1 }, "Gain Scale",
                NormalisableRange<float> (0.f, 200.f, 1.f), 100.f,
                AudioParameterFloatAttributes().withStringFromValueFunction (fmtPct)));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "autoGain", 1 }, "Auto Gain", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "character", 1 }, "Character",
                StringArray { "Clean", "Subtle", "Warm" }, 0));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "extSC", 1 }, "External Sidechain", false));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "viewRange", 1 }, "Display Range",
                StringArray { "3 dB", "6 dB", "12 dB", "30 dB" }, 2));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "analyzer", 1 }, "Analyzer", true));
    return layout;
}

GroqAudioProcessor::GroqAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                          .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "GROQ4", createLayout())
{
    for (int b = 0; b < kNumBands; ++b)
    {
        auto g = [&] (const char* id) { return apvts.getRawParameterValue (bandId (b, id)); };
        ptrs[b] = { g ("on"), g ("type"), g ("freq"), g ("gain"), g ("q"), g ("slope"), g ("ch"),
                    g ("dyn"), g ("thr"), g ("att"), g ("rel"), g ("range") };
        dynAmt[b].store (0.f);
    }
    pOut = apvts.getRawParameterValue ("outGain");
    pScale = apvts.getRawParameterValue ("gainScale");
    pAuto = apvts.getRawParameterValue ("autoGain");
    pChar = apvts.getRawParameterValue ("character");
    pExtSC = apvts.getRawParameterValue ("extSC");
    outPeak[0].store (0.f); outPeak[1].store (0.f);
    for (auto& a : preFifo) a.store (0.f);
    for (auto& a : postFifo) a.store (0.f);
}

BandParams GroqAudioProcessor::readBand (int i) const
{
    const auto& p = ptrs[i];
    BandParams r;
    r.on = p.on->load() > 0.5f;
    r.dyn = p.dyn->load() > 0.5f;
    r.type = juce::jlimit (0, NumTypes - 1, (int) std::lround (p.type->load()));
    r.slope = juce::jlimit (0, 7, (int) std::lround (p.slope->load()));
    r.ch = juce::jlimit (0, 4, (int) std::lround (p.ch->load()));
    r.freq = p.freq->load(); r.gain = p.gain->load(); r.q = p.q->load();
    r.thr = p.thr->load(); r.att = p.att->load(); r.rel = p.rel->load(); r.range = p.range->load();
    return r;
}

bool GroqAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (l.getMainInputChannelSet() != out) return false;
    if (l.inputBuses.size() > 1)
    {
        const auto sc = l.getChannelSet (true, 1);
        if (! (sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo()))
            return false;
    }
    return true;
}

void GroqAudioProcessor::resetBand (BandState& B)
{
    for (int s = 0; s < kMaxStages; ++s) { B.cur[s] = Coef(); B.tgt[s] = Coef(); B.st[0][s] = {}; B.st[1][s] = {}; }
    B.det = {}; B.procStages = 0; B.tgtStages = 0; B.amt = 0.f; B.busy = 0; B.wasActive = false;
}

void GroqAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr.store (sampleRate);
    for (auto& b : bands) { resetBand (b); b.dirty = true; b.last = BandParams(); }
    for (int i = 0; i < kAGPoints; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, (double) i / (kAGPoints - 1));
        const double w = 2.0 * juce::MathConstants<double>::pi * f / sampleRate;
        agCos[(size_t) i] = std::cos (w); agCos2[(size_t) i] = std::cos (2.0 * w);
    }
    lastScale = -1.f; autoGainDb = 0.f;
    outGainSm.reset (sampleRate, 0.02);
    outGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (pOut->load()));
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0;
}

void GroqAudioProcessor::updateAutoGain (float scale)
{
    const double fs = sr.load();
    double sum[kAGPoints] = {};
    Coef c[kMaxStages];
    for (int b = 0; b < kNumBands; ++b)
    {
        const auto& p = bands[(size_t) b].last;
        if (! p.on) continue;
        const int ns = designBand (p, fs, hasGain (p.type) ? p.gain * scale : 0.f, c);
        for (int i = 0; i < kAGPoints; ++i)
            for (int s = 0; s < ns; ++s)
                sum[i] += 10.0 * std::log10 (std::max (magSq (c[s], agCos[(size_t) i], agCos2[(size_t) i]), 1e-12));
    }
    double mean = 0.0;
    for (double v : sum) mean += v;
    mean /= kAGPoints;
    autoGainDb = juce::jlimit (-24.f, 24.f, (float) -mean);
}

void GroqAudioProcessor::runStages (float* x, int len, const Coef* c, Biq* s, int n)
{
    for (int k = 0; k < n; ++k)
    {
        const double b0 = c[k].b0, b1 = c[k].b1, b2 = c[k].b2, a1 = c[k].a1, a2 = c[k].a2;
        double z1 = s[k].z1, z2 = s[k].z2;
        for (int i = 0; i < len; ++i)
        {
            const double in = x[i];
            const double y = b0 * in + z1;
            z1 = b1 * in - a1 * y + z2;
            z2 = b2 * in - a2 * y;
            x[i] = (float) y;
        }
        s[k].z1 = z1; s[k].z2 = z2;
    }
}

void GroqAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const double fs = sr.load();
    auto mainBus = getBusBuffer (buffer, true, 0);
    const int nCh = mainBus.getNumChannels();
    const int n = buffer.getNumSamples();
    if (nCh < 1 || n < 1) return;

    float* L = mainBus.getWritePointer (0);
    float* R = nCh > 1 ? mainBus.getWritePointer (1) : nullptr;

    const float* scL = nullptr; const float* scR = nullptr;
    if (pExtSC->load() > 0.5f && getBusCount (true) > 1)
    {
        auto sc = getBusBuffer (buffer, true, 1);
        if (sc.getNumChannels() > 0)
        {
            scL = sc.getReadPointer (0);
            scR = sc.getNumChannels() > 1 ? sc.getReadPointer (1) : scL;
        }
    }

    // Analizador (entrada)
    int pos = fifoPos.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
        preFifo[(size_t) ((pos + i) & (kFifo - 1))].store (R ? 0.5f * (L[i] + R[i]) : L[i], std::memory_order_relaxed);

    // Lectura de parámetros y detección de cambios
    const float scale = pScale->load() * 0.01f;
    bool anyDirty = (scale != lastScale);
    for (int b = 0; b < kNumBands; ++b)
    {
        auto& B = bands[(size_t) b];
        const auto p = readBand (b);
        if (p != B.last || B.dirty || scale != lastScale)
        {
            B.last = p; B.dirty = true; B.busy = 64; anyDirty = true;
        }
    }
    if (anyDirty) { lastScale = scale; updateAutoGain (scale); }
    const bool autoOn = pAuto->load() > 0.5f;
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (pOut->load() + (autoOn ? autoGainDb : 0.f)));

    float tmpA[kSub], tmpB[kSub];

    for (int off = 0; off < n; off += kSub)
    {
        const int len = juce::jmin (kSub, n - off);
        float* Lp = L + off;
        float* Rp = R ? R + off : nullptr;

        for (int b = 0; b < kNumBands; ++b)
        {
            auto& B = bands[(size_t) b];
            const auto& p = B.last;
            const bool active = p.on || B.busy > 0;
            if (! active)
            {
                if (B.wasActive) resetBand (B);
                dynAmt[b].store (0.f, std::memory_order_relaxed);
                continue;
            }
            B.wasActive = true;
            const bool dynamic = p.on && p.dyn && hasGain (p.type);

            // --- detector ---
            if (dynamic)
            {
                if (B.dirty)
                    B.detC = rbj (4, fs, juce::jlimit (5.0, fs * 0.49, (double) p.freq), juce::jlimit (0.5, 10.0, (double) p.q), 0.0);
                double sum = 0.0;
                for (int i = 0; i < len; ++i)
                {
                    float x;
                    if (scL) x = 0.5f * (scL[off + i] + scR[off + i]);
                    else switch (p.ch)
                    {
                        case Left:  x = Lp[i]; break;
                        case Right: x = Rp ? Rp[i] : Lp[i]; break;
                        case Mid:   x = Rp ? 0.5f * (Lp[i] + Rp[i]) : Lp[i]; break;
                        case Side:  x = Rp ? 0.5f * (Lp[i] - Rp[i]) : 0.f; break;
                        default:    x = Rp ? 0.5f * (Lp[i] + Rp[i]) : Lp[i]; break;
                    }
                    const double y = B.detC.b0 * x + B.det.z1;
                    B.det.z1 = B.detC.b1 * x - B.detC.a1 * y + B.det.z2;
                    B.det.z2 = B.detC.b2 * x - B.detC.a2 * y;
                    sum += y * y;
                }
                const float lvl = 10.f * std::log10 ((float) (sum / len) + 1e-12f);
                const float target = juce::jlimit (0.f, 1.f, (lvl - p.thr) / 10.f);
                const float ms = target > B.amt ? p.att : p.rel;
                const float k = 1.f - std::exp (-(float) len / (float) (fs * 0.001 * juce::jmax (0.05f, ms)));
                B.amt += (target - B.amt) * k;
                dynAmt[b].store (B.amt, std::memory_order_relaxed);
            }
            else
            {
                B.amt = 0.f;
                dynAmt[b].store (0.f, std::memory_order_relaxed);
            }

            // --- coeficientes objetivo ---
            if (B.dirty || dynamic)
            {
                if (p.on)
                {
                    float g = p.gain;
                    if (dynamic) g += p.range * B.amt;
                    g *= scale;
                    B.tgtStages = designBand (p, fs, juce::jlimit (-48.f, 48.f, g), B.tgt);
                }
                else
                {
                    for (int s = 0; s < kMaxStages; ++s) B.tgt[s] = Coef();
                    B.tgtStages = 0;
                }
                if (B.tgtStages > B.procStages) B.procStages = B.tgtStages;
                B.dirty = false;
            }

            // --- suavizado de coeficientes ---
            const float kc = 0.25f;
            for (int s = 0; s < B.procStages; ++s)
            {
                auto& c = B.cur[s]; const auto& t = B.tgt[s];
                c.b0 += (t.b0 - c.b0) * kc; c.b1 += (t.b1 - c.b1) * kc; c.b2 += (t.b2 - c.b2) * kc;
                c.a1 += (t.a1 - c.a1) * kc; c.a2 += (t.a2 - c.a2) * kc;
            }
            if (B.busy > 0) { if (--B.busy == 0) { B.procStages = B.tgtStages; for (int s = B.tgtStages; s < kMaxStages; ++s) { B.cur[s] = Coef(); B.st[0][s] = {}; B.st[1][s] = {}; } } }

            const int ns = B.procStages;
            if (ns == 0) continue;

            // --- filtrado según canal ---
            switch (p.ch)
            {
                case Stereo:
                    runStages (Lp, len, B.cur, B.st[0], ns);
                    if (Rp) runStages (Rp, len, B.cur, B.st[1], ns);
                    break;
                case Left:  runStages (Lp, len, B.cur, B.st[0], ns); break;
                case Right: if (Rp) runStages (Rp, len, B.cur, B.st[1], ns); break;
                case Mid:
                    if (Rp)
                    {
                        for (int i = 0; i < len; ++i) { tmpA[i] = 0.5f * (Lp[i] + Rp[i]); tmpB[i] = 0.5f * (Lp[i] - Rp[i]); }
                        runStages (tmpA, len, B.cur, B.st[0], ns);
                        for (int i = 0; i < len; ++i) { Lp[i] = tmpA[i] + tmpB[i]; Rp[i] = tmpA[i] - tmpB[i]; }
                    }
                    else runStages (Lp, len, B.cur, B.st[0], ns);
                    break;
                case Side:
                    if (Rp)
                    {
                        for (int i = 0; i < len; ++i) { tmpA[i] = 0.5f * (Lp[i] + Rp[i]); tmpB[i] = 0.5f * (Lp[i] - Rp[i]); }
                        runStages (tmpB, len, B.cur, B.st[1], ns);
                        for (int i = 0; i < len; ++i) { Lp[i] = tmpA[i] + tmpB[i]; Rp[i] = tmpA[i] - tmpB[i]; }
                    }
                    break;
                default: break;
            }
        }
    }

    // --- carácter + ganancia de salida ---
    const int chr = juce::jlimit (0, 2, (int) std::lround (pChar->load()));
    float pk[2] = { 0.f, 0.f };
    for (int i = 0; i < n; ++i)
    {
        const float g = outGainSm.getNextValue();
        for (int c = 0; c < nCh; ++c)
        {
            float* d = c == 0 ? L : R;
            float x = d[i];
            if (chr == 1)
                x = x + 0.3f * (satPade (2.f * x) * 0.5f - x);
            else if (chr == 2)
            {
                const float z = satPade (2.f * x + 0.5f * x * x) * 0.5f;
                x = x + 0.6f * (z - x);
                const double y = (double) x - dcX[c] + 0.9995 * dcY[c];   // bloqueo de continua
                dcX[c] = x; dcY[c] = y; x = (float) y;
            }
            x *= g;
            d[i] = x;
            pk[c] = juce::jmax (pk[c], std::abs (x));
        }
    }
    for (int c = 0; c < nCh; ++c)
        if (pk[c] > outPeak[c].load (std::memory_order_relaxed)) outPeak[c].store (pk[c], std::memory_order_relaxed);

    // Analizador (salida)
    for (int i = 0; i < n; ++i)
        postFifo[(size_t) ((pos + i) & (kFifo - 1))].store (R ? 0.5f * (L[i] + R[i]) : L[i], std::memory_order_relaxed);
    fifoPos.store ((pos + n) & (kFifo - 1), std::memory_order_release);
}

// ---------------------------------------------------------------- presets
namespace
{
const char* const kPresetNames[] = { "Init", "Voz clara", "Limpia barro (dinamico)", "Aire y subgrave", "Brillo de master" };
}

int GroqAudioProcessor::getNumPrograms() { return (int) (sizeof (kPresetNames) / sizeof (kPresetNames[0])); }
const juce::String GroqAudioProcessor::getProgramName (int i) { return kPresetNames[juce::jlimit (0, getNumPrograms() - 1, i)]; }
void GroqAudioProcessor::setCurrentProgram (int i) { applyPreset (i); }

void GroqAudioProcessor::applyPreset (int index)
{
    currentPreset = juce::jlimit (0, getNumPrograms() - 1, index);
    auto setP = [this] (const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    };
    for (int b = 0; b < kNumBands; ++b)
        for (auto* id : { "on", "type", "freq", "gain", "q", "slope", "ch", "dyn", "thr", "att", "rel", "range" })
            if (auto* p = apvts.getParameter (bandId (b, id))) p->setValueNotifyingHost (p->getDefaultValue());

    auto band = [&] (int b, int type, float f, float g, float q, int slope = 3)
    {
        setP (bandId (b, "on"), 1.f); setP (bandId (b, "type"), (float) type); setP (bandId (b, "freq"), f);
        setP (bandId (b, "gain"), g); setP (bandId (b, "q"), q); setP (bandId (b, "slope"), (float) slope);
    };
    switch (currentPreset)
    {
        case 1: band (0, LowCut, 90.f, 0.f, 0.71f); band (1, Bell, 250.f, -2.5f, 1.2f);
                band (2, Bell, 3200.f, 2.5f, 1.1f); band (3, HighShelf, 10000.f, 2.f, 0.7f); break;
        case 2: band (0, LowCut, 35.f, 0.f, 0.71f);
                band (1, Bell, 300.f, 0.f, 1.5f);
                setP (bandId (1, "dyn"), 1.f); setP (bandId (1, "range"), -5.f); setP (bandId (1, "thr"), -32.f);
                setP (bandId (1, "att"), 10.f); setP (bandId (1, "rel"), 120.f); break;
        case 3: band (0, LowCut, 25.f, 0.f, 0.71f, 5); band (1, LowShelf, 60.f, 2.5f, 0.7f);
                band (2, Bell, 400.f, -1.5f, 0.8f); band (3, HighShelf, 12000.f, 3.f, 0.6f); break;
        case 4: band (0, LowCut, 20.f, 0.f, 0.71f, 1); band (1, Bell, 40.f, 1.f, 0.9f);
                band (2, Bell, 3500.f, 1.f, 0.5f); band (3, HighShelf, 9000.f, 1.5f, 0.5f); break;
        default: break;
    }
}


// ---------------------------------------------------------------- importar .ffp (Pro-Q 3/4, formato de texto)
juce::String GroqAudioProcessor::importFfp (const juce::String& text)
{
    juce::StringPairArray kv;
    bool header = false;
    for (auto line : juce::StringArray::fromLines (text))
    {
        line = line.trim();
        if (line.startsWithIgnoreCase ("Signature=FQ")) header = true;
        const int eq = line.indexOfChar ('=');
        if (eq > 0 && ! line.startsWithChar ('[')) kv.set (line.substring (0, eq), line.substring (eq + 1));
    }
    if (! header)
        return "ERROR: este archivo no tiene el formato de texto de Pro-Q (FQ4p). Si es de Pro-Q 3 o muy antiguo puede ser binario y no esta soportado.";

    auto num = [&] (const juce::String& k, double def) { return kv.getAllKeys().contains (k) ? kv[k].getDoubleValue() : def; };
    auto setP = [this] (const juce::String& id, float v)
    {
        if (auto* p = apvts.getParameter (id)) p->setValueNotifyingHost (p->convertTo0to1 (v));
    };

    for (int b = 0; b < kNumBands; ++b)
        for (auto* id : { "on", "type", "freq", "gain", "q", "slope", "ch", "dyn", "thr", "att", "rel", "range" })
            if (auto* p = apvts.getParameter (bandId (b, id))) p->setValueNotifyingHost (p->getDefaultValue());

    static const int shapeMap[] = { Bell, LowShelf, LowCut, HighShelf, HighCut, Notch, BandPass, TiltShelf, TiltShelf, AllPass };
    static const int chMap[]    = { Left, Right, Stereo, Mid, Side };
    static const int slopeDb[]  = { 6, 6, 12, 18, 24, 30, 36, 48, 72, 96 };   // supuesto: indice 2 = 12 dB/oct
    static const int slopeOurs[] = { 6, 12, 18, 24, 36, 48, 72, 96 };

    int loaded = 0, dynamicBands = 0, spectralBands = 0;
    for (int b = 0; b < kNumBands; ++b)
    {
        const juce::String pre = "Band " + juce::String (b + 1) + " ";
        if (num (pre + "Used", 0) < 0.5) continue;
        const bool enabled = num (pre + "Enabled", 1) > 0.5;
        const int shape = juce::jlimit (0, 9, (int) num (pre + "Shape", 0));
        const int type = shapeMap[shape];
        const double f = std::pow (2.0, num (pre + "Frequency", 9.97));
        const double qPq = 0.025 * std::pow (1600.0, num (pre + "Q", 0.5));
        const double q = (type == Bell || type == Notch || type == BandPass || type == AllPass) ? qPq : qPq * 0.7071;

        setP (bandId (b, "on"), enabled ? 1.f : 0.f);
        setP (bandId (b, "type"), (float) type);
        setP (bandId (b, "freq"), (float) juce::jlimit (10.0, 30000.0, f));
        setP (bandId (b, "gain"), (float) juce::jlimit (-30.0, 30.0, num (pre + "Gain", 0)));
        setP (bandId (b, "q"), (float) juce::jlimit (0.1, 40.0, q));
        setP (bandId (b, "ch"), (float) chMap[juce::jlimit (0, 4, (int) num (pre + "Stereo Placement", 2))]);

        const int dbOct = slopeDb[juce::jlimit (0, 9, (int) num (pre + "Slope", 2))];
        int si = 0;
        for (int i = 0; i < 8; ++i) if (slopeOurs[i] <= dbOct) si = i;
        if (dbOct == 30) si = 4;
        setP (bandId (b, "slope"), (float) si);

        const double range = num (pre + "Dynamic Range", 0);
        if (num (pre + "Dynamics Enabled", 0) > 0.5 && std::abs (range) > 0.01 && hasGain (type))
        {
            setP (bandId (b, "dyn"), 1.f);
            setP (bandId (b, "range"), (float) juce::jlimit (-30.0, 30.0, range));
            if (num (pre + "Dynamics Auto", 1) < 0.5)
                setP (bandId (b, "thr"), (float) (-80.0 + 80.0 * juce::jlimit (0.0, 1.0, num (pre + "Threshold", 1))));
            setP (bandId (b, "att"), (float) (0.1 * std::pow (10.0, 4.0 * num (pre + "Attack", 50) / 100.0)));
            setP (bandId (b, "rel"), (float) (5.0 * std::pow (600.0, num (pre + "Release", 50) / 100.0)));
            ++dynamicBands;
        }
        if (num (pre + "Spectral Enabled", 0) > 0.5) ++spectralBands;
        ++loaded;
    }

    setP ("character", (float) juce::jlimit (0, 2, (int) num ("Character", 0)));
    setP ("outGain", (float) juce::jlimit (-24.0, 24.0, num ("Output Level", 0)));
    setP ("gainScale", (float) juce::jlimit (0.0, 200.0, num ("Gain Scale", 1) * 100.0));
    setP ("autoGain", num ("Auto Gain", 0) > 0.5 ? 1.f : 0.f);
    setP ("viewRange", (float) juce::jlimit (0, 3, (int) num ("Display Range", 2)));

    juce::String r = "Importadas " + juce::String (loaded) + " bandas";
    if (dynamicBands > 0) r << ", " << dynamicBands << " dinamicas";
    r << ".";
    const int mode = (int) num ("Processing Mode", 0);
    if (mode != 0) r << "\nAviso: el preset usa Natural/Linear Phase; GRO Q 4 trabaja en Zero Latency, la fase sera distinta.";
    if (spectralBands > 0) r << "\nAviso: " << spectralBands << " banda(s) usan Spectral Dynamics, que aun no existe; se cargan como EQ estatico.";
    return r;
}

// ---------------------------------------------------------------- estado
void GroqAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void GroqAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* GroqAudioProcessor::createEditor() { return new GroqAudioProcessorEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new GroqAudioProcessor(); }
