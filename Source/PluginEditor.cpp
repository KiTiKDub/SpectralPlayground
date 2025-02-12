#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), bypassAttachment(p.apvts, "bypass", bypass)
{
    updateRSWL(p.apvts);
    setLookAndFeel(&lnf);

    bypass.setComponentID("Power");
    showAnimator.setButtonText("Settings");
    showAnimator.onClick = [this]()
    {
        if(showAnimator.getToggleState())
            animator.animateIn();
        else
            animator.animateOut();    
    };

    addAndMakeVisible(bypass);
    addAndMakeVisible(gumroad);
    addAndMakeVisible(showAnimator);
    addAndMakeVisible(*crush);
    addAndMakeVisible(animator);
    
    setSize (500, 500);

    juce::ignoreUnused (processorRef);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    auto newFont = juce::Font(juce::Typeface::createSystemTypefaceFor(BinaryData::offshore_ttf, BinaryData::offshore_ttfSize));
    g.setFont(newFont);

    g.fillAll(juce::Colours::black);

    auto logo = juce::ImageCache::getFromMemory(BinaryData::KITIK_LOGO_NO_BKGD_png, BinaryData::KITIK_LOGO_NO_BKGD_pngSize);
    auto logoSpace = getLocalBounds().removeFromTop(getLocalBounds().getHeight() * .1);
    g.drawImage(logo, logoSpace.toFloat(), juce::RectanglePlacement::centred);

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    //g.drawFittedText ("KiTiK Plugin Template", getLocalBounds(), juce::Justification::centredTop, 1);

}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto bypassBounds = bounds;
    bypassBounds.removeFromBottom(bounds.getHeight() * .925);
    bypassBounds.removeFromRight(bounds.getWidth() * .925);
    auto animatorButtonBounds = bounds.removeFromTop(bounds.getHeight() *.075);
    animatorButtonBounds.removeFromLeft(animatorButtonBounds.getWidth() * .85);
    bounds.removeFromBottom(bounds.getHeight() * .1);
    auto bitDepthBounds = bounds;
    crush->setBounds(bitDepthBounds);
    showAnimator.setBounds(animatorButtonBounds);
    bypass.setBounds(bypassBounds);
    animator.setTopLeftPosition(0, getLocalBounds().getHeight());
    animator.setSize(bounds.getWidth(), 100);

    auto morePlugins = getLocalBounds();
    morePlugins.removeFromTop(morePlugins.getHeight() * .95);
    morePlugins.removeFromRight(morePlugins.getWidth() * .8);
    gumroad.setBounds(morePlugins);
}   

void AudioPluginAudioProcessorEditor::updateRSWL(juce::AudioProcessorValueTreeState& apvts)
{
    auto& bitRateParam = getParam(apvts, "crush");

    crush = std::make_unique<RotarySliderWithLabels>(&bitRateParam, "", "Crush");

    makeAttachment(bitRateAT, apvts, "crush", *crush);

    addLabelPairs(crush->labels, 1, 3, bitRateParam, "", 20);

    crush->onValueChange = [this, &bitRateParam]()
    {
        addLabelPairs(crush->labels, 1, 3, bitRateParam, "", 20);
    };
}