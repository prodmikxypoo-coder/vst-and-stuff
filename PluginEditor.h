#pragma once
#include "PluginProcessor.h"

class PandaAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    explicit PandaAudioProcessorEditor (PandaAudioProcessor&);
    ~PandaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PandaAudioProcessor& proc;
    juce::Slider rageSlider;
    juce::AudioProcessorValueTreeState::SliderAttachment rageAttach;
    std::unique_ptr<juce::LookAndFeel_V4> look;

    float vis = 0.0f;   // smoothed 0..1 "evilness" used for drawing
    int tick = 0;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PandaAudioProcessorEditor)
};
