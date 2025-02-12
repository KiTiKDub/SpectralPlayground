#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    updateRSWL(p.apvts);
    setLookAndFeel(&lnf);

    bypass.setComponentID("Power");
    showAnimator.setName("Settings");
    showAnimator.onClick = [this]()
    {
        if(showAnimator.getToggleState())
            animator.animateIn();
        else
            animator.animateOut();    
    };

    addAndMakeVisible(bypass);
    addAndMakeVisible(showAnimator);
    addAndMakeVisible(animator);
    addAndMakeVisible(*bitDepth, 0);
    
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
    //g.drawImage(logo, getLocalBounds().toFloat(), juce::RectanglePlacement::centred);

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("KiTiK Plugin Template", getLocalBounds(), juce::Justification::centredTop, 1);

}

void AudioPluginAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto bypassBounds = bounds;
    bypassBounds.removeFromBottom(bounds.getHeight() * .9);
    bypassBounds.removeFromRight(bounds.getWidth() * .9);
    auto animatorButtonBounds = bounds.removeFromTop(bounds.getHeight() *.1);
    animatorButtonBounds.translate(0, 50);
    bounds.removeFromBottom(bounds.getHeight() * .1);
    auto bitDepthBounds = bounds;
    bitDepth->setBounds(bitDepthBounds);
    showAnimator.setBounds(animatorButtonBounds);
    bypass.setBounds(bypassBounds);
    animator.setTopLeftPosition(0, getLocalBounds().getHeight());
    animator.setSize(bounds.getWidth(), 100);
}   

void AudioPluginAudioProcessorEditor::updateRSWL(juce::AudioProcessorValueTreeState& apvts)
{
    auto& bitDepthParam = getParam(apvts, "bitDepth");

    bitDepth = std::make_unique<RotarySliderWithLabels>(&bitDepthParam, "", "Bit Depth");

    makeAttachment(bitDepthAttachment, apvts, "bitDepth", *bitDepth);

    addLabelPairs(bitDepth->labels, 1, 3, bitDepthParam, "", 20);

    bitDepth->onValueChange = [this, &bitDepthParam]()
    {
        addLabelPairs(bitDepth->labels, 1, 3, bitDepthParam, "", 20);
    };
}