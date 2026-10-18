#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace groq
{
constexpr int kNumBands  = 24;
constexpr int kMaxStages = 8;
constexpr int kSub       = 32;   // tamaño de sub-bloque para actualizar coeficientes

enum FilterType { Bell = 0, LowShelf, HighShelf, LowCut, HighCut, Notch, BandPass, TiltShelf, AllPass, NumTypes };
enum ChannelMode { Stereo = 0, Left, Right, Mid, Side };

inline const char* const kTypeNames[] = { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut",
                                          "Notch", "Band Pass", "Tilt Shelf", "All Pass" };
inline const int kSlopeOrder[8] = { 1, 2, 3, 4, 6, 8, 12, 16 };
inline const char* const kSlopeNames[] = { "6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct",
                                           "36 dB/oct", "48 dB/oct", "72 dB/oct", "96 dB/oct" };

inline bool hasGain (int t) { return t == Bell || t == LowShelf || t == HighShelf || t == TiltShelf; }
inline bool isCut   (int t) { return t == LowCut || t == HighCut; }

struct Coef { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };

struct BandParams
{
    bool on = false, dyn = false;
    int type = 0, slope = 3, ch = 0;
    float freq = 1000.f, gain = 0.f, q = 1.f, thr = -30.f, att = 10.f, rel = 120.f, range = -6.f;

    bool operator== (const BandParams& o) const
    {
        return on == o.on && dyn == o.dyn && type == o.type && slope == o.slope && ch == o.ch
            && freq == o.freq && gain == o.gain && q == o.q && thr == o.thr
            && att == o.att && rel == o.rel && range == o.range;
    }
    bool operator!= (const BandParams& o) const { return ! (*this == o); }
};

// kind: 0 peak, 1 low shelf, 2 high shelf, 3 notch, 4 band pass, 5 all pass, 6 low pass, 7 high pass
inline Coef rbj (int kind, double fs, double f, double Q, double gdB)
{
    const double w0 = 2.0 * juce::MathConstants<double>::pi * f / fs;
    const double cs = std::cos (w0), sn = std::sin (w0);
    const double alpha = sn / (2.0 * Q);
    const double A = std::pow (10.0, gdB / 40.0);
    double b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;

    switch (kind)
    {
        case 0:
            b0 = 1 + alpha * A; b1 = -2 * cs; b2 = 1 - alpha * A;
            a0 = 1 + alpha / A; a1 = -2 * cs; a2 = 1 - alpha / A; break;
        case 1: {
            const double s = 2.0 * std::sqrt (A) * alpha;
            b0 = A * ((A + 1) - (A - 1) * cs + s);
            b1 = 2 * A * ((A - 1) - (A + 1) * cs);
            b2 = A * ((A + 1) - (A - 1) * cs - s);
            a0 = (A + 1) + (A - 1) * cs + s;
            a1 = -2 * ((A - 1) + (A + 1) * cs);
            a2 = (A + 1) + (A - 1) * cs - s; break; }
        case 2: {
            const double s = 2.0 * std::sqrt (A) * alpha;
            b0 = A * ((A + 1) + (A - 1) * cs + s);
            b1 = -2 * A * ((A - 1) + (A + 1) * cs);
            b2 = A * ((A + 1) + (A - 1) * cs - s);
            a0 = (A + 1) - (A - 1) * cs + s;
            a1 = 2 * ((A - 1) - (A + 1) * cs);
            a2 = (A + 1) - (A - 1) * cs - s; break; }
        case 3: b0 = 1; b1 = -2 * cs; b2 = 1; a0 = 1 + alpha; a1 = -2 * cs; a2 = 1 - alpha; break;
        case 4: b0 = alpha; b1 = 0; b2 = -alpha; a0 = 1 + alpha; a1 = -2 * cs; a2 = 1 - alpha; break;
        case 5: b0 = 1 - alpha; b1 = -2 * cs; b2 = 1 + alpha; a0 = 1 + alpha; a1 = -2 * cs; a2 = 1 - alpha; break;
        case 6: b0 = (1 - cs) / 2; b1 = 1 - cs; b2 = (1 - cs) / 2; a0 = 1 + alpha; a1 = -2 * cs; a2 = 1 - alpha; break;
        case 7: b0 = (1 + cs) / 2; b1 = -(1 + cs); b2 = (1 + cs) / 2; a0 = 1 + alpha; a1 = -2 * cs; a2 = 1 - alpha; break;
        default: break;
    }
    Coef c;
    c.b0 = b0 / a0; c.b1 = b1 / a0; c.b2 = b2 / a0; c.a1 = a1 / a0; c.a2 = a2 / a0;
    return c;
}

inline Coef firstOrder (bool highpass, double fs, double f)
{
    const double K = std::tan (juce::MathConstants<double>::pi * f / fs);
    Coef c;
    const double n = 1.0 / (K + 1.0);
    c.a1 = (K - 1.0) * n; c.a2 = 0; c.b2 = 0;
    if (highpass) { c.b0 = n; c.b1 = -n; }
    else          { c.b0 = K * n; c.b1 = K * n; }
    return c;
}

// Rellena out[kMaxStages] y devuelve el nº de etapas usadas (el resto queda en identidad)
inline int designBand (const BandParams& p, double fs, float gainDb, Coef* out)
{
    for (int i = 0; i < kMaxStages; ++i) out[i] = Coef();
    const double f = juce::jlimit (5.0, fs * 0.49, (double) p.freq);
    const double q = juce::jlimit (0.1, 40.0, (double) p.q);

    switch (p.type)
    {
        case Bell:      out[0] = rbj (0, fs, f, q, gainDb); return 1;
        case LowShelf:  out[0] = rbj (1, fs, f, q, gainDb); return 1;
        case HighShelf: out[0] = rbj (2, fs, f, q, gainDb); return 1;
        case TiltShelf: out[0] = rbj (1, fs, f, 0.6, -gainDb * 0.5);
                        out[1] = rbj (2, fs, f, 0.6,  gainDb * 0.5); return 2;
        case Notch:     out[0] = rbj (3, fs, f, q, 0); return 1;
        case BandPass:  out[0] = rbj (4, fs, f, q, 0); return 1;
        case AllPass:   out[0] = rbj (5, fs, f, q, 0); return 1;
        case LowCut:
        case HighCut:
        {
            const bool hp = (p.type == LowCut);
            const int n = kSlopeOrder[juce::jlimit (0, 7, p.slope)];
            int idx = 0;
            if (n & 1) out[idx++] = firstOrder (hp, fs, f);
            const int nb = n / 2;
            const double qScale = q / 0.7071;
            for (int k = 0; k < nb; ++k)
            {
                const double theta = (n & 1) ? (k + 1) * juce::MathConstants<double>::pi / n
                                             : (2 * k + 1) * juce::MathConstants<double>::pi / (2.0 * n);
                double Qk = 1.0 / (2.0 * std::cos (theta));
                if (k == nb - 1) Qk *= qScale;
                out[idx++] = rbj (hp ? 7 : 6, fs, f, Qk, 0);
            }
            return idx;
        }
        default: break;
    }
    return 0;
}

inline double magSq (const Coef& c, double cosw, double cos2w)
{
    const double num = c.b0 * c.b0 + c.b1 * c.b1 + c.b2 * c.b2
                     + 2.0 * (c.b0 * c.b1 + c.b1 * c.b2) * cosw + 2.0 * c.b0 * c.b2 * cos2w;
    const double den = 1.0 + c.a1 * c.a1 + c.a2 * c.a2
                     + 2.0 * (c.a1 + c.a1 * c.a2) * cosw + 2.0 * c.a2 * cos2w;
    return num / std::max (den, 1e-30);
}
} // namespace groq
