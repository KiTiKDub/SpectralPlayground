#pragma once

#include "PluginProcessor.h"
#include "BinaryData.h"
#include "juce_core/juce_core.h"
#include "GUI/KitikLookAndFeel.h"
#include "GUI/RotarySliderWithLabels.h"
#include "GUI/Animator.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    
    void updateRSWL(juce::AudioProcessorValueTreeState& apvts);

    AudioPluginAudioProcessor& processorRef;

    AnimationView animator{ juce::Easings::createEaseIn(), processorRef.apvts };

    Laf lnf;
    juce::ToggleButton bypass, showAnimator;
    std::unique_ptr<RotarySliderWithLabels> bitDepth;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
