#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout PandaAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "rage", 1 }, "Rage",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    return layout;
}

PandaAudioProcessor::PandaAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

void PandaAudioProcessor::prepareToPlay (double sampleRate, int)
{
    rageSmoothed.reset (sampleRate, 0.03);
    rageSmoothed.setCurrentAndTargetValue (apvts.getRawParameterValue ("rage")->load());
}

bool PandaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void PandaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), getTotalNumOutputChannels());

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    rageSmoothed.setTargetValue (apvts.getRawParameterValue ("rage")->load());

    for (int i = 0; i < numSamples; ++i)
    {
        const float r = rageSmoothed.getNextValue();

        // Drive ramps from 1x (clean-ish) up to 60x (total destruction)
        const float drive    = 1.0f + 59.0f * r * r;
        const float makeup   = 1.0f / std::pow (drive, 0.15f);
        const float noiseAmt = 0.12f * r * r * r;   // noise bed only really shows up near the top

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer (ch);

            float y = std::tanh (drive * data[i]) * makeup;
            y += (noiseRng.nextFloat() * 2.0f - 1.0f) * noiseAmt;

            // Safety clip so a bug or hot input can't blast your ears
            data[i] = juce::jlimit (-0.98f, 0.98f, y);
        }
    }
}

juce::AudioProcessorEditor* PandaAudioProcessor::createEditor()
{
    return new PandaAudioProcessorEditor (*this);
}

void PandaAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PandaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PandaAudioProcessor();
}
