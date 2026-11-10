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
    layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID ("oversampling", 1), "Oversampling 2x", false));
    return layout;
}

bool SteamVoxAudioProcessor::isBusesLayoutSupported (const BusesLayout& b) const
{
    const auto in = b.getMainInputChannelSet(), out = b.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && in == out;
}

void SteamVoxAudioProcessor::prepareToPlay (double sampleRate, int blockSize)
{
    dsp.prepare (sampleRate);
    oversampledDsp.prepare (sampleRate * 2.0);
    const auto count = (size_t) juce::jmax (1, getTotalNumInputChannels());
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        count, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true, true);
    oversampler->initProcessing ((size_t) juce::jmax (1,blockSize));
    oversampler->reset();
    delaySize = juce::jmax (0, juce::roundToInt (oversampler->getLatencyInSamples()));
    setLatencySamples (delaySize);
    delayIndex = 0;
    for (auto& v : bypassDelay) v.assign ((size_t) delaySize, 0.f);
    meterInput.store (0.f);
    meterOutput.store (0.f);
}

void SteamVoxAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples(), ch = buffer.getNumChannels();
    if (n == 0 || ch < 1) return;
    steamvox::Controls c;
    c.boiler = pBoiler->load (std::memory_order_relaxed) / 100.0;
    c.steam = pSteam->load (std::memory_order_relaxed) / 100.0;
    c.blastpipe = pBlastpipe->load (std::memory_order_relaxed) / 100.0;
    c.mix = pMix->load (std::memory_order_relaxed) / 100.0;
    c.inputDB = pIn->load (std::memory_order_relaxed);
    c.outputDB = pOut->load (std::memory_order_relaxed);
    c.oversampling = pOS->load (std::memory_order_relaxed) >= .5f;
    dsp.set (c);
    oversampledDsp.set (c);

    float inputPeak = 0.f;
    for (int i = 0; i < n; ++i)
        for (int channel = 0; channel < juce::jmin (ch,2); ++channel)
            inputPeak = juce::jmax (inputPeak, std::abs (buffer.getSample (channel,i)));

    if (c.oversampling && oversampler != nullptr)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto up = oversampler->processSamplesUp (block);
        const auto upSamples = (int) up.getNumSamples();
        float* l = up.getChannelPointer (0);
        float* r = ch > 1 ? up.getChannelPointer (1) : nullptr;
        for (int i = 0; i < upSamples; ++i)
        {
            const float a = l[i], b = r != nullptr ? r[i] : a;
            const auto result = oversampledDsp.process (a,b);
            l[i] = result[0];
            if (r != nullptr) r[i] = result[1];
        }
        oversampler->processSamplesDown (block);
    }
    else
    {
        float* l = buffer.getWritePointer (0);
        float* r = ch > 1 ? buffer.getWritePointer (1) : nullptr;
        for (int i = 0; i < n; ++i)
        {
            const float a = l[i], b = r != nullptr ? r[i] : a;
            const auto result = dsp.process (a,b);
            float left = result[0], right = result[1];
            if (delaySize > 0)
            {
                left = bypassDelay[0][(size_t) delayIndex];
                bypassDelay[0][(size_t) delayIndex] = result[0];
                if (r != nullptr)
                {
                    right = bypassDelay[1][(size_t) delayIndex];
                    bypassDelay[1][(size_t) delayIndex] = result[1];
                }
                if (++delayIndex == delaySize) delayIndex = 0;
            }
            l[i] = left;
            if (r != nullptr) r[i] = right;
        }
    }
    double outputPower = 0.0;
    const auto active = juce::jmin (ch,2);
    for (int channel=0; channel<active; ++channel)
        for (int i=0; i<n; ++i)
        {
            const auto x = (double) buffer.getSample (channel,i);
            outputPower += x*x;
        }
    const auto rms = (float) std::sqrt (outputPower / juce::jmax (1,active*n));
    const auto coeff = (float) std::exp (-n / (getSampleRate() * .300));
    meterInput.store (coeff * meterInput.load (std::memory_order_relaxed)
                         + (1.f - coeff) * inputPeak, std::memory_order_relaxed);
    meterOutput.store (coeff * meterOutput.load (std::memory_order_relaxed)
                          + (1.f - coeff) * rms, std::memory_order_relaxed);
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
