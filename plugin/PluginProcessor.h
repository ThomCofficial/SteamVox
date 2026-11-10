#pragma once
#include <JuceHeader.h>
#include "../src/SteamVoxDSP.h"
#include <atomic>

class SteamVoxAudioProcessor final : public juce::AudioProcessor
{
public:
    SteamVoxAudioProcessor();
    ~SteamVoxAudioProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "SteamVox"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> meterInput {0.f}, meterOutput {0.f};
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
private:
    steamvox::SteamVoxDSP dsp;
    std::atomic<float>* pBoiler = nullptr;
    std::atomic<float>* pSteam = nullptr;
    std::atomic<float>* pBlastpipe = nullptr;
    std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pIn = nullptr;
    std::atomic<float>* pOut = nullptr;
    std::atomic<float>* pOS = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteamVoxAudioProcessor)
};
