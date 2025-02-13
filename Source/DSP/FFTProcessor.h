#pragma once
#include "juce_dsp/juce_dsp.h"

/*
  Each channel should have its own FFTProcessor.
 */
class FFTProcessor
{
public:
    FFTProcessor::FFTProcessor(int order, int overlapOrder) 
        : fft(order), fftSize(1 << order), overlap(1 << overlapOrder),
            hopSize(fftSize / overlap),
            window(fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false),
            inputFifo(fftSize), outputFifo(fftSize), fftData(fftSize * 2)
    {
        fftOrder = order;
        numBins = fftSize / 2 + 1;
        windowCorrection = (1.f / (.375f * overlap));
    }

    void FFTProcessor::reset()
    {
        count = 0;
        pos = 0;

        std::fill(inputFifo.begin(), inputFifo.end(), 0.0f);
        std::fill(outputFifo.begin(), outputFifo.end(), 0.0f);
        isReady = true;
        inUse = false;
    }

    void FFTProcessor::handleHopSizeChange(int overlapOrder)
    {
        overlap = 1 << overlapOrder;
        windowCorrection = /* (1.f / (.5*overlap)) */ 2.f / 3.f;
    }

    template <typename FProcess>
    float processSample(float sample, bool bypassed, FProcess process_fn)
    {
        inputFifo[pos] = sample;
        float outputSample = outputFifo[pos];
        outputFifo[pos] = 0.0f;

        pos += 1;
        if (pos == fftSize)
            pos = 0;

        count += 1;
        if (count == hopSize)
        {
            count = 0;
            const float *inputPtr = inputFifo.data();
            float *fftPtr = fftData.data();

            // Copy the input FIFO into the FFT working space in two parts.
            std::memcpy(fftPtr, inputPtr + pos, (fftSize - pos) * sizeof(float));
            if (pos > 0)
            {
                std::memcpy(fftPtr + fftSize - pos, inputPtr, pos * sizeof(float));
            }

            window.multiplyWithWindowingTable(fftPtr, fftSize);

            if (!bypassed)
            {
                fft.performRealOnlyForwardTransform(fftPtr, true);
                process_fn(reinterpret_cast<std::complex<float> *>(fftPtr));
                fft.performRealOnlyInverseTransform(fftPtr);
            }

            window.multiplyWithWindowingTable(fftPtr, fftSize);

            // Scale down the output samples because of the overlapping windows.
            // for (int i = 0; i < fftSize; ++i)
            // {
            //     fftPtr[i] *= windowCorrection;
            // }

            juce::FloatVectorOperations::multiply(fftPtr, windowCorrection, fftSize);

            // Add the IFFT results to the output FIFO.
            for (int i = 0; i < pos; ++i)
            {
                outputFifo[i] += fftData[i + fftSize - pos];
            }
            for (int i = 0; i < fftSize - pos; ++i)
            {
                outputFifo[i + pos] += fftData[i];
            }
        }

        return outputSample;
    }
    
    int getLatencyInSamples() const { return fftSize; }
    bool isFFTReady() { return isReady;}
    void prepFFTForReset() { isReady = false; inUse = false;}
    bool isFFTInUse() { return inUse;}
    void setFFTInUse(bool use) { inUse = use;}

private:

    bool isReady = true;
    bool inUse = false;
    int fftOrder, fftSize, overlap, hopSize, numBins;
    int count = 0;
    int pos = 0;
    float windowCorrection{1};
    
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    
    std::vector<float> inputFifo;
    std::vector<float> outputFifo;
    std::vector<float> fftData; 

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTProcessor)
};