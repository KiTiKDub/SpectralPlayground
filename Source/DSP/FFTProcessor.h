#pragma once
#include "juce_dsp/juce_dsp.h"

/**
  STFT analysis and resynthesis of audio data.

  Each channel should have its own FFTProcessor.
 */
class FFTProcessor
{
public:
    FFTProcessor(int order, int overlap);
    void prepareToPlay(int order, int overlap);

    int getLatencyInSamples() const { return fftSize; }

    juce::dsp::AudioBlock<float> fftBlock();

    void reset();
    float processSample(float sample, bool bypassed, float factor);
    void processBlock(float *data, int numSamples, bool bypassed);

    int numBins;

private:
    void processFrame(bool bypassed, float factor);
    void processSpectrum(float *data, int numBins, float factor);

    // The FFT has 2^order points and fftSize/2 + 1 bins.
    // static constexpr int fftOrder = 11;
    // static constexpr int fftSize = 1 << fftOrder;     // 1024 samples
    // static constexpr int numBins = fftSize / 2 + 1;   // 513 bins
    // static constexpr int overlap = 4;                 // 75% overlap -> need to research how I can get more
    // static constexpr int hopSize = fftSize / overlap; // 256 samples

    int fftOrder, fftSize, overlap, hopSize;

    // Gain correction for using Hann window with 75% overlap.
    static constexpr float windowCorrection = 2.0f / 3.0f; //-> need to double check math for different sizes.

    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;

    // Counts up until the next hop.
    int count = 0;

    // Write position in input FIFO and read position in output FIFO.
    int pos = 0;

    // Circular buffers for incoming and outgoing audio data.
    std::vector<float> inputFifo;
    std::vector<float> outputFifo;

    // The FFT working space. Contains interleaved complex numbers.
    std::vector<float> fftData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTProcessor)
};