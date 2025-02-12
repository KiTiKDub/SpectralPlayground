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

    AnimationView animator{ juce::Easings::createEaseOut(), processorRef.apvts };

    juce::URL url{"https://kwhaley5.gumroad.com/"};

    juce::HyperlinkButton gumroad{"More Plugins", url};

    Laf lnf;
    juce::ToggleButton bypass, showAnimator;
    std::unique_ptr<RotarySliderWithLabels> crush;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitRateAT;
    juce::AudioProcessorValueTreeState::ButtonAttachment bypassAttachment;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
