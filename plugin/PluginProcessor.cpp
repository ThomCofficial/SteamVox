#include "PluginProcessor.h"
#include "PluginEditor.h"

SteamVoxAudioProcessor::SteamVoxAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "SteamVoxState", makeLayout())
{
    pBoiler = parameters.getRawParameterValue ("boiler");
    pSteam = parameters.getRawParameterValue ("steam");
    pBlastpipe = parameters.getRawParameterValue ("blastpipe");
    pMix = parameters.getRawParameterValue ("mix");
    pIn = parameters.getRawParameterValue ("input");
    pOut = parameters.getRawParameterValue ("output");
    pOS = parameters.getRawParameterValue ("oversampling");
}

juce::AudioProcessorValueTreeState::ParameterLayout SteamVoxAudioProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    auto percent = juce::NormalisableRange<float> (0.f, 100.f, .01f);
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("blastpipe", 1), "Blastpipe", percent, 0.f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("boiler", 1), "Boiler", percent, 0.f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("steam", 1), "Steam", percent, 0.f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("mix", 1), "Mix", percent, 100.f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("input", 1), "Input", -24.f, 24.f, 0.f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID ("output", 1), "Output", -24.f, 24.f, 0.f));
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID ("oversampling", 1), "Oversampling (reserved)", false));
    return layout;
}

bool SteamVoxAudioProcessor::isBusesLayoutSupported (const BusesLayout& b) const
{
    const auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && in == out;
}

void SteamVoxAudioProcessor::prepareToPlay (double sampleRate, int)
{
    dsp.prepare (sampleRate);
    meterInput.store (0.f);
    meterOutput.store (0.f);
}

void SteamVoxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto n = buffer.getNumSamples(), channels = buffer.getNumChannels();
    steamvox::Controls c;
    c.boiler = pBoiler->load() / 100.0;
    c.steam = pSteam->load() / 100.0;
    c.blastpipe = pBlastpipe->load() / 100.0;
    c.mix = pMix->load() / 100.0;
    c.inputDB = pIn->load(); c.outputDB = pOut->load();
    // Reserved parameter: remains disabled until a properly aligned and tested
    // oversampling path is implemented. Never falsely advertise actual OS.
    c.oversampling = false;
    dsp.set (c);

    if (channels < 1) return;
    auto* l = buffer.getWritePointer (0);
    auto* r = channels > 1 ? buffer.getWritePointer (1) : nullptr;
    float peakIn = 0, peakOut = 0;
    for (int i = 0; i < n; ++i)
    {
        const auto a = l[i], b = r != nullptr ? r[i] : a;
        peakIn = std::max (peakIn, std::max (std::abs (a), std::abs (b)));
        const auto out = dsp.process (a, b);
        l[i] = out[0];
        if (r != nullptr) r[i] = out[1];
        peakOut = std::max (peakOut, std::max (std::abs (out[0]), std::abs (out[1])));
    }
    meterInput.store (0.8f * meterInput.load() + 0.2f * peakIn, std::memory_order_relaxed);
    meterOutput.store (0.8f * meterOutput.load() + 0.2f * peakOut, std::memory_order_relaxed);
}

void SteamVoxAudioProcessor::getStateInformation (juce::MemoryBlock& out)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary (*xml, out);
}
void SteamVoxAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SteamVoxAudioProcessor();
}
