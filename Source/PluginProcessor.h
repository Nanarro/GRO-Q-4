#pragma once
#include "BandDSP.h"
#include <array>
#include <atomic>

class GroqAudioProcessor : public juce::AudioProcessor
{
public:
    GroqAudioProcessor();
    ~GroqAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "GRO Q 4"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int i) override;
    const juce::String getProgramName (int i) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    groq::BandParams readBand (int i) const;
    void applyPreset (int index);
    // Importa un preset de FabFilter Pro-Q (texto .ffp). Devuelve un informe; cadena vacia con prefijo "ERROR" si falla.
    juce::String importFfp (const juce::String& text);
    static juce::String bandId (int band, const char* name) { return "b" + juce::String (band + 1) + "_" + name; }

    juce::AudioProcessorValueTreeState apvts;

    // audio -> interfaz (solo atomics)
    static constexpr int kFifo = 4096;
    std::array<std::atomic<float>, kFifo> preFifo {}, postFifo {};
    std::atomic<int> fifoPos { 0 };
    std::atomic<float> dynAmt[groq::kNumBands];
    std::atomic<float> outPeak[2];
    std::atomic<double> sr { 44100.0 };

private:
    struct BandPtrs { std::atomic<float> *on, *type, *freq, *gain, *q, *slope, *ch, *dyn, *thr, *att, *rel, *range; };
    BandPtrs ptrs[groq::kNumBands];
    std::atomic<float> *pOut, *pScale, *pAuto, *pChar, *pExtSC;

    struct Biq { double z1 = 0, z2 = 0; };
    struct BandState
    {
        groq::Coef cur[groq::kMaxStages], tgt[groq::kMaxStages];
        Biq st[2][groq::kMaxStages];
        Biq det; groq::Coef detC;
        float amt = 0.f;
        groq::BandParams last;
        int busy = 0, procStages = 0, tgtStages = 0;
        bool dirty = true, wasActive = false;
    };
    std::array<BandState, groq::kNumBands> bands;

    static constexpr int kAGPoints = 40;
    std::array<double, kAGPoints> agCos {}, agCos2 {};
    float autoGainDb = 0.f, lastScale = -1.f;
    juce::SmoothedValue<float> outGainSm;
    double dcX[2] { 0, 0 }, dcY[2] { 0, 0 };
    int currentPreset = 0;

    void resetBand (BandState&);
    void updateAutoGain (float scale);
    static void runStages (float* x, int len, const groq::Coef* c, Biq* s, int n);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GroqAudioProcessor)
};
