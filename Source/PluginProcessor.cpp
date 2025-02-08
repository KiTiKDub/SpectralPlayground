#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
{
    bitDepth = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("bitDepth"));
    bitRate = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("bitRate"));
    order = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("order"));
    overlap = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("overlap"));

    asyncUpdater.setCallback([this] { resetFFTs(); });
    lastOrder = order->get();
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    for(auto& fft: fftMapLeft)
    {
        fft.second->reset();
    }

    juce::ignoreUnused (sampleRate, samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::resetFFTs()
{
    for(auto& fft: fftMapLeft)
    {
        if(!fft.second->isFFTInUse())
        fft.second->reset();
    }
    for (auto &fft : fftMapRight)
    {
        if (!fft.second->isFFTInUse())
            fft.second->reset();
    }
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    const auto& bitRateValue = bitRate->get();
    const auto& bitDepthValue = bitDepth->get();

    if(lastOrder != order->get())
    {
        const auto& newFFTLeft = fftMapLeft.at(order->get());
        const auto& newFFTRight = fftMapRight.at(order->get());
        if(newFFTLeft->isFFTReady() && newFFTRight->isFFTReady())
        {   
            newFFTLeft->setFFTInUse(true);
            newFFTRight->setFFTInUse(true);
            fftMapLeft.at(lastOrder)->setFFTReady(false);
            fftMapRight.at(lastOrder)->setFFTReady(false);
            asyncUpdater.triggerAsyncUpdate();
            lastOrder = order->get();
        }
    }

    auto bitcrush = [this, bitRateValue, bitDepthValue](std::complex<float> *fft_data) 
    {
        for (int bin = 1; bin < 513; bin++) //get bins so they are not hardcoded
        {
            float magnitude = std::abs(fft_data[bin]);
            float phase = std::arg(fft_data[bin]);
            auto crusher = pow(2, bitDepthValue);
            double crushedData = floor(crusher * magnitude) / crusher;

            magnitude = static_cast<float>(crushedData);

            if (bitRateValue > 1)
            {
                if (bin % bitRateValue != 0)
                {
                    auto redux = std::abs(fft_data[bin - bin % bitRateValue]);
                    magnitude = redux;
                }
            }
            fft_data[bin] = std::polar(magnitude, phase);
        }
    };

    float *dataLeft = buffer.getWritePointer(0);
    float *dataRight = buffer.getWritePointer(1);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        dataLeft[i] = fftMapLeft.at(lastOrder)->processSample(dataLeft[i], false, bitcrush); //I need this to be seperate ffts because I need different buffers
        dataRight[i] = fftMapRight.at(lastOrder)->processSample(dataRight[i], false, bitcrush);
    }

}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<AudioParameterInt>("bitDepth", "Bit Depth", 1,16,16));
    layout.add(std::make_unique<AudioParameterInt>("bitRate", "Bit Rate", 1, 25, 1));
    layout.add(std::make_unique<AudioParameterInt>("order", "Order", 7, 16, 11));
    layout.add(std::make_unique<AudioParameterInt>("overlap", "Overlap", 3, 5, 3));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
