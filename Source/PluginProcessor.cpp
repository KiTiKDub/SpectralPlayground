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
    crush = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("crush"));
    order = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("order"));
    overlap = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("overlap"));
    bypass = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("bypass"));
    gain = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("gain"));
    mix = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("mix"));

    asyncUpdater.setCallback([this] { resetFFTs(); });
    lastOrder = order->get();
    lastHopSize = overlap->get();
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
    for (auto &fft : fftMapRight)
    {
        fft.second->reset();
    }

    setLatencySamples(fftMapLeft.at(order->get())->getLatencyInSamples());

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

    setLatencySamples(fftMapLeft.at(order->get())->getLatencyInSamples());
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    juce::AudioBuffer<float> copyBuffer;
    copyBuffer.makeCopyOf(buffer);

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    const auto& bitRateValue = crush->get();

    if(lastOrder != order->get())
    {
        const auto& newFFTLeft = fftMapLeft.at(order->get());
        const auto& newFFTRight = fftMapRight.at(order->get());
        if(newFFTLeft->isFFTReady() && newFFTRight->isFFTReady())
        {   
            newFFTLeft->setFFTInUse(true);
            newFFTRight->setFFTInUse(true);
            fftMapLeft.at(lastOrder)->prepFFTForReset();
            fftMapRight.at(lastOrder)->prepFFTForReset();
            newFFTLeft->handleHopSizeChange(overlap->get());
            newFFTRight->handleHopSizeChange(overlap->get());
            asyncUpdater.triggerAsyncUpdate();
            lastOrder = order->get();
        }
    }

    if(lastHopSize != overlap->get()) //Needs to be checked when fft changes as well, fix in update
    {
        fftMapLeft.at(lastOrder)->handleHopSizeChange(overlap->get());
        fftMapRight.at(lastOrder)->handleHopSizeChange(overlap->get());
        lastHopSize = overlap->get();
    }

    auto bitcrush = [this, bitRateValue](std::complex<float> *fft_data) 
    {
        auto numBins = (1 << lastOrder) / 2 + 1;
        for (int bin = 1; bin < numBins; bin++) 
        {
            float magnitude = std::abs(fft_data[bin]);
            float phase = std::arg(fft_data[bin]);
            auto crusher = pow(2, 16);
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
    float *copyLeft = copyBuffer.getWritePointer(0);
    float *copyRight = copyBuffer.getWritePointer(1);

    if(totalNumInputChannels == 1)
    {
        dataRight = buffer.getWritePointer(0);
        copyRight = buffer.getWritePointer(0);
    }

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        copyLeft[i] = fftMapLeft.at(lastOrder)->processSample(dataLeft[i], false, bitcrush); //I need this to be seperate ffts because I need different buffers
        copyRight[i] = fftMapRight.at(lastOrder)->processSample(dataRight[i], false, bitcrush);
    }
    
    if(!bypass->get())
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i) //add mix
        {
            dataLeft[i] = copyLeft[i] * mix->get() + dataLeft[i] * (1 - mix->get());
            dataRight[i] = copyRight[i] * mix->get() + dataRight[i] * (1 - mix->get());
        }

        auto block = juce::dsp::AudioBlock<float>(buffer); //add gain
        for(int channel = 0; channel < totalNumOutputChannels; ++channel)
        {
            osg.process(block, gain->get(), channel);
        }
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid()) {
        apvts.replaceState(tree);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto mixRange = NormalisableRange<float>(0.f, 1.f, .01f);
    auto mixAttributes = AudioParameterFloatAttributes().withStringFromValueFunction([](auto x, auto) { return juce::String(x*100) + "%";});
    auto orderAttributes = AudioParameterIntAttributes().withStringFromValueFunction([](int x, auto)
                                                                                           { return juce::String(1 << x); });

    layout.add(std::make_unique<AudioParameterInt>(juce::ParameterID{"crush",1}, "Krush", 1, 25, 1));
    layout.add(std::make_unique<AudioParameterInt>(juce::ParameterID{"order",1}, "Order", 8, 12, 10, orderAttributes));
    layout.add(std::make_unique<AudioParameterInt>(juce::ParameterID{"overlap",1}, "Overlap", 2, 5, 2, orderAttributes));
    layout.add(std::make_unique<AudioParameterBool>(juce::ParameterID{"bypass",1}, "Bypass", false));
    layout.add(std::make_unique<AudioParameterFloat>(juce::ParameterID{"gain",1}, "Gain", -24.f, 24.f, 0.f));
    layout.add(std::make_unique<AudioParameterFloat>(juce::ParameterID{"mix",1}, "Mix", mixRange, 1.f, mixAttributes));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
