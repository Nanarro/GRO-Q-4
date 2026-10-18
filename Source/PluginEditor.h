#pragma once
#include "PluginProcessor.h"

namespace groqui
{
inline juce::Colour bandColour (int b)
{
    return juce::Colour::fromHSV (std::fmod (0.02f + (float) b * 0.61803f, 1.f), 0.62f, 0.96f, 1.f);
}

class GroqLookAndFeel : public juce::LookAndFeel_V4
{
public:
    GroqLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
};

class GraphComponent : public juce::Component, private juce::Timer
{
public:
    GraphComponent (GroqAudioProcessor&, std::function<void (int)> onSelect);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    int selected = -1;

private:
    static constexpr int kN = 2048, kBins = kN / 2 + 1, kCurve = 300;
    GroqAudioProcessor& proc;
    std::function<void (int)> onSelect;
    juce::dsp::FFT fft { 11 };
    juce::dsp::WindowingFunction<float> window { (size_t) kN, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, kN * 2> fftData {};
    std::array<float, kBins> specPre {}, specPost {};
    std::array<double, kCurve> cosw {}, cos2w {};
    double curveSr = 0.0;
    float meter[2] { 0.f, 0.f };
    int hoverBand = -1, dragBand = -1;
    bool dragging = false;
    std::atomic<float> *pRange, *pScale, *pAnalyzer;

    float plotW() const { return (float) getWidth() - 20.f; }
    float freqToX (float f) const { return plotW() * std::log (f / 10.f) / std::log (3000.f); }
    float xToFreq (float x) const { return 10.f * std::exp (std::log (3000.f) * x / plotW()); }
    float rangeDb() const;
    float dbToY (float db) const { return (float) getHeight() * 0.5f * (1.f - db / rangeDb()); }
    float yToDb (float y) const { return (1.f - 2.f * y / (float) getHeight()) * rangeDb(); }
    float specToY (float db) const { return (float) getHeight() * (1.f - (db + 100.f) / 95.f); }

    void timerCallback() override;
    void updateSpectrum();
    void rebuildTables();
    float effectiveGain (int b, const groq::BandParams& p) const;
    juce::Point<float> nodePos (int b, const groq::BandParams& p) const;
    int hitTest (juce::Point<float>) const;
    void setParam (const juce::String& id, float plain, bool gesture);
    void showContextMenu (int band);
    void addBandAt (juce::Point<float>);
    void deleteBand (int band);
};

class BandPanel : public juce::Component
{
public:
    BandPanel (juce::AudioProcessorValueTreeState&);
    void bind (int band);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::AudioProcessorValueTreeState& apvts;
    int band = -1;
    juce::TextButton power, dynBtn { "Dynamic" };
    juce::ComboBox typeBox, slopeBox, chBox;
    juce::Slider freq, gain, q, thr, att, rel, range;
    juce::Rectangle<int> labelFreq, labelGain, labelQ, labelThr, labelAtt, labelRel, labelRange;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SA> aFreq, aGain, aQ, aThr, aAtt, aRel, aRange;
    std::unique_ptr<BA> aPower, aDyn;
    std::unique_ptr<CA> aType, aSlope, aCh;
    void updateVisibility();
    void setupKnob (juce::Slider&);
};
} // namespace groqui

class GroqAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit GroqAudioProcessorEditor (GroqAudioProcessor&);
    ~GroqAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    GroqAudioProcessor& proc;
    groqui::GroqLookAndFeel laf;   // declarado antes que los componentes
    groqui::GraphComponent graph;
    groqui::BandPanel panel;

    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, presetBtn, importBtn { "Import .ffp" };
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ComboBox rangeBox, charBox;
    juce::TextButton scBtn { "Ext SC" }, anaBtn { "Analyzer" }, autoBtn { "Auto Gain" };
    juce::Slider scaleSlider, outSlider;
    juce::Rectangle<int> scaleLabel, outLabel;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<CA> aRange, aChar;
    std::unique_ptr<BA> aSC, aAna, aAuto;
    std::unique_ptr<SA> aScale, aOut;

    int lastPreset = -1;
    void selectBand (int b);
    void selectFirstEnabled();
    void loadPreset (int i);
    void timerCallback() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GroqAudioProcessorEditor)
};
