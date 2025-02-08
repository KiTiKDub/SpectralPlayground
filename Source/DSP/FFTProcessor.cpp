#include "FFTProcessor.h"

FFTProcessor::FFTProcessor(int order, int overlapOrder) : fft(order), fftSize(1 << order), hopSize(fftSize / overlap),
                               window(fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false),
                               inputFifo(fftSize), outputFifo(fftSize), fftData(fftSize * 2)
{
    fftOrder = order;
    overlap = 1 << overlapOrder;
    numBins = fftSize / 2 + 1;
    windowCorrection = (1.f / (.33f*overlap));
}

void FFTProcessor::reset()
{
    count = 0;
    pos = 0;

    std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
    std::fill(outputFifo.begin(), outputFifo.end(), 0.0f);
    isReady = true;
}