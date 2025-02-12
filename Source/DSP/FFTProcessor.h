#pragma once
#include "juce_dsp/juce_dsp.h"

/**
  STFT analysis and resynthesis of audio data.

  Each channel should have its own FFTProcessor.
 */
class FFTProcessor
{
public:
    FFTProcessor(int order, int overlapOrder);

    int getLatencyInSamples() const { return fftSize; }
    void reset();
    void handleHopSizeChange(int overlapOrder);

    template <typename FProcess>
    float processSample(float sample, bool bypassed, FProcess process_fn)
    {
        // Push the new sample value into the input FIFO.
        inputFifo[pos] = sample;

        // Read the output value from the output FIFO. Since it takes fftSize
        // timesteps before actual samples are read from this FIFO instead of
        // the initial zeros, the sound output is delayed by fftSize samples,
        // which we will report as our latency.
        float outputSample = outputFifo[pos];

        // Once we've read the sample, set this position in the FIFO back to
        // zero so we can add the IFFT results to it later.
        outputFifo[pos] = 0.0f;

        // Advance the FIFO index and wrap around if necessary.
        pos += 1;
        if (pos == fftSize)
        {
            pos = 0;
        }

        // Process the FFT frame once we've collected hopSize samples.
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

            // Apply the window to avoid spectral leakage.
            window.multiplyWithWindowingTable(fftPtr, fftSize);

            if (!bypassed)
            {
                // Perform the forward FFT.
                fft.performRealOnlyForwardTransform(fftPtr, true);
                // Do stuff with the FFT data.
                process_fn(reinterpret_cast<std::complex<float> *>(fftPtr));

                // Perform the inverse FFT.
                fft.performRealOnlyInverseTransform(fftPtr);
            }

            // Apply the window again for resynthesis.
            window.multiplyWithWindowingTable(fftPtr, fftSize);

            // Scale down the output samples because of the overlapping windows.
            for (int i = 0; i < fftSize; ++i)
            {
                fftPtr[i] *= windowCorrection;
            }

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

    bool isFFTReady() { return isReady;}
    void prepFFTForReset() { isReady = false; inUse = false;}
    bool isFFTInUse() { return inUse;}
    void setFFTInUse(bool use) { inUse = use;}

private:

    int fftOrder, fftSize, overlap, hopSize, numBins;
    
    // Gain correction for using Hann window with 75% overlap.
    //a squared hann window has amplitude effect of 1/3.
    //This is then multiplied by your overlap factor.
    //The resulting correction will be the inverse of this 1/3*overlap.
    //static constexpr float windowCorrection = 2.0f / 3.0f; //-> need to double check math for different sizes.
    float windowCorrection{1};
    
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
    
    bool isReady = true;
    bool inUse = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FFTProcessor)
};