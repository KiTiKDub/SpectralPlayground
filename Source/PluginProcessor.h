#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/FFTProcessor.h"
#include "Utility/overSampleGain.h"

struct KiTiKAsyncUpdater : public juce::AsyncUpdater
{
    KiTiKAsyncUpdater() {};

    void setCallback(std::function<void(void)> callback)
    {
        callback_ = std::move(callback);
    }

    void handleAsyncUpdate() override
    {
        callback_();
    }

private:
    std::function<void(void)> callback_;
};

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "parameters", createParameterLayout()};
    void resetFFTs();

private:

    overSampleGain osg;

    FFTProcessor fft8Left{8, 2};
    FFTProcessor fft9Left{9, 2};
    FFTProcessor fft10Left{10, 2};
    FFTProcessor fft11Left{11, 2};
    FFTProcessor fft12Left{12, 2};

    FFTProcessor fft8Right{8, 2};
    FFTProcessor fft9Right{9, 2};
    FFTProcessor fft10Right{10, 2};
    FFTProcessor fft11Right{11, 2};
    FFTProcessor fft12Right{12, 2};

    std::map<int, FFTProcessor*> fftMapLeft{{8, &fft8Left}, {9, &fft9Left}, {10, &fft10Left}, {11, &fft11Left}, {12, &fft12Left}};
    std::map<int, FFTProcessor *> fftMapRight{{8, &fft8Right}, {9, &fft9Right}, {10, &fft10Right}, {11, &fft11Right}, {12, &fft12Right}};

    KiTiKAsyncUpdater asyncUpdater; 

    int lastOrder{1};
    int lastHopSize{1};

    juce::AudioParameterInt* bitRate{nullptr};
    juce::AudioParameterInt* bitDepth{nullptr};
    juce::AudioParameterInt* order{nullptr};
    juce::AudioParameterInt* overlap{nullptr};
    juce::AudioParameterBool* bypass{nullptr};
    juce::AudioParameterFloat* gain{nullptr};
    juce::AudioParameterFloat* mix{nullptr};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
//To Do:
    //Crate Async call to reset ffts
    //Figure out channel processing in fft -> going to add left and right ffts
    //understand overlap math and add it tofft