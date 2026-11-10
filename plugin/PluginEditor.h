#pragma once
#include "PluginProcessor.h"
#include <array>
class SteamVoxEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SteamVoxEditor (SteamVoxAudioProcessor&);
    ~SteamVoxEditor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct BrassLook final : juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    } look;
    SteamVoxAudioProcessor& processor;
    std::array<juce::Slider, 6> controls;
    std::array<juce::Label, 6> legends;
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<Attachment>, 6> attachments;
    juce::TextButton osButton {"OS"};
    float meter = 0.f;
    void timerCallback() override;
    void drawMeter (juce::Graphics&, juce::Rectangle<float>);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SteamVoxEditor)
};
