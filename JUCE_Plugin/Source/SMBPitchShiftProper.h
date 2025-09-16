#pragma once
#include "IPitchShiftStrategy.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <memory>
#include <algorithm>

#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
#include <juce_dsp/juce_dsp.h>
#endif

/**
 * PROPER implementation of SMB Pitch Shift algorithm
 * Based on Stephan M. Bernsee's smbPitchShift.cpp
 */
class SMBPitchShiftProper : public IPitchShiftStrategy {
public:
    SMBPitchShiftProper() {
        // Initialize with standard parameters
        init(44100.0);
    }
    
    ~SMBPitchShiftProper() = default;
    
    void prepare(double sampleRate, int maxBlockSize) override {
        init(sampleRate);
    }
    
    void reset() override {
        std::fill(gInFIFO.begin(), gInFIFO.end(), 0.0f);
        std::fill(gOutFIFO.begin(), gOutFIFO.end(), 0.0f);
        std::fill(gLastPhase.begin(), gLastPhase.end(), 0.0f);
        std::fill(gSumPhase.begin(), gSumPhase.end(), 0.0f);
        std::fill(gOutputAccum.begin(), gOutputAccum.end(), 0.0f);
        std::fill(gAnaFreq.begin(), gAnaFreq.end(), 0.0f);
        std::fill(gAnaMagn.begin(), gAnaMagn.end(), 0.0f);
        gRover = inFifoLatency;
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) override {
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        // Clamp pitch ratio to reasonable range
        pitchRatio = std::max(0.25f, std::min(4.0f, pitchRatio));
        
        // Process each sample
        for (int i = 0; i < numSamples; i++) {
            // Load data into input FIFO
            gInFIFO[gRover] = input[i];
            output[i] = gOutFIFO[gRover - inFifoLatency];
            gRover++;
            
            // If we have enough data, process a frame
            if (gRover >= fftFrameSize) {
                gRover = inFifoLatency;
                
                // Do windowing
                for (int k = 0; k < fftFrameSize; k++) {
                    window[k] = -0.5f * cos(2.0f * M_PI * k / (float)fftFrameSize) + 0.5f;
                    gFFTworksp[k] = gInFIFO[k] * window[k];
                }
                
                // ********** ANALYSIS **********
                // Do forward FFT
                fft->performRealOnlyForwardTransform(gFFTworksp.data(), true);
                
                // This is the analysis step
                for (int k = 0; k <= fftFrameSize2; k++) {
                    // Get real and imag parts
                    float real = gFFTworksp[k * 2];
                    float imag = gFFTworksp[k * 2 + 1];
                    
                    // Compute magnitude and phase
                    float magn = 2.0f * sqrt(real * real + imag * imag);
                    float phase = atan2(imag, real);
                    
                    // Compute phase difference
                    float tmp = phase - gLastPhase[k];
                    gLastPhase[k] = phase;
                    
                    // Subtract expected phase difference
                    tmp -= (float)k * expct;
                    
                    // Map delta phase into +/- Pi interval
                    int qpd = tmp / M_PI;
                    if (qpd >= 0) qpd += qpd & 1;
                    else qpd -= qpd & 1;
                    tmp -= M_PI * (float)qpd;
                    
                    // Get deviation from bin frequency from phase deviation
                    tmp = osamp * tmp / (2.0f * M_PI);
                    
                    // Compute the k-th partials' true frequency
                    tmp = (float)k * freqPerBin + tmp * freqPerBin;
                    
                    // Store magnitude and true frequency in analysis arrays
                    gAnaMagn[k] = magn;
                    gAnaFreq[k] = tmp;
                }
                
                // ********** PROCESSING (PITCH SHIFTING) **********
                std::fill(gSynMagn.begin(), gSynMagn.end(), 0.0f);
                std::fill(gSynFreq.begin(), gSynFreq.end(), 0.0f);
                
                for (int k = 0; k <= fftFrameSize2; k++) {
                    int index = k * pitchRatio;
                    if (index <= fftFrameSize2) {
                        gSynMagn[index] += gAnaMagn[k];
                        gSynFreq[index] = gAnaFreq[k] * pitchRatio;
                    }
                }
                
                // ********** SYNTHESIS **********
                // This is the synthesis step
                for (int k = 0; k <= fftFrameSize2; k++) {
                    // Get magnitude and true frequency from synthesis arrays
                    float magn = gSynMagn[k];
                    float tmp = gSynFreq[k];
                    
                    // Subtract bin mid frequency
                    tmp -= (float)k * freqPerBin;
                    
                    // Get bin deviation from freq deviation
                    tmp /= freqPerBin;
                    
                    // Take osamp into account
                    tmp = 2.0f * M_PI * tmp / osamp;
                    
                    // Add the overlap phase advance back in
                    tmp += (float)k * expct;
                    
                    // Accumulate delta phase to get bin phase
                    gSumPhase[k] += tmp;
                    float phase = gSumPhase[k];
                    
                    // Get real and imag part
                    gFFTworksp[k * 2] = magn * cos(phase);
                    gFFTworksp[k * 2 + 1] = magn * sin(phase);
                }
                
                // Zero negative frequencies (for real output)
                for (int k = fftFrameSize2 + 1; k < fftFrameSize; k++) {
                    gFFTworksp[k * 2] = 0.0f;
                    gFFTworksp[k * 2 + 1] = 0.0f;
                }
                
                // Do inverse FFT
                fft->performRealOnlyInverseTransform(gFFTworksp.data());
                
                // Do windowing and add to output accumulator
                for (int k = 0; k < fftFrameSize; k++) {
                    window[k] = -0.5f * cos(2.0f * M_PI * k / (float)fftFrameSize) + 0.5f;
                    gOutputAccum[k] += 2.0f * window[k] * gFFTworksp[k] / (fftFrameSize2 * osamp);
                }
                
                // Copy to output buffer
                for (int k = 0; k < stepSize; k++) {
                    gOutFIFO[k] = gOutputAccum[k];
                }
                
                // Shift accumulator
                memmove(gOutputAccum.data(), gOutputAccum.data() + stepSize, 
                       fftFrameSize * sizeof(float));
                
                // Move input FIFO
                memmove(gInFIFO.data(), gInFIFO.data() + stepSize, 
                       inFifoLatency * sizeof(float));
            }
        }
#else
        // No JUCE, pass through
        std::copy(input, input + numSamples, output);
#endif
    }
    
    int getLatencySamples() const override { 
        return inFifoLatency;
    }
    
    const char* getName() const override { return "SMB Pitch Shift Proper"; }
    bool isHighQuality() const override { return true; }
    int getQualityRating() const override { return 95; }
    int getCpuUsage() const override { return 50; }
    
private:
    void init(double sampleRate) {
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        // Set up FFT parameters
        fftFrameSize = 2048;
        osamp = 4;  // 75% overlap
        
        fftFrameSize2 = fftFrameSize / 2;
        stepSize = fftFrameSize / osamp;
        freqPerBin = sampleRate / (double)fftFrameSize;
        expct = 2.0 * M_PI * (double)stepSize / (double)fftFrameSize;
        inFifoLatency = fftFrameSize - stepSize;
        
        // Allocate arrays
        gInFIFO.resize(fftFrameSize);
        gOutFIFO.resize(fftFrameSize);
        gFFTworksp.resize(fftFrameSize * 2);  // For real FFT format
        gLastPhase.resize(fftFrameSize2 + 1);
        gSumPhase.resize(fftFrameSize2 + 1);
        gOutputAccum.resize(2 * fftFrameSize);
        gAnaFreq.resize(fftFrameSize);
        gAnaMagn.resize(fftFrameSize);
        gSynFreq.resize(fftFrameSize);
        gSynMagn.resize(fftFrameSize);
        window.resize(fftFrameSize);
        
        // Create FFT processor
        fft = std::make_unique<juce::dsp::FFT>(11); // 2^11 = 2048
        
        reset();
#endif
    }
    
    // FFT parameters
    static constexpr long MAX_FRAME_LENGTH = 8192;
    int fftFrameSize = 2048;
    int fftFrameSize2 = 1024;
    int osamp = 4;
    int stepSize = 512;
    double freqPerBin = 0.0;
    double expct = 0.0;
    int inFifoLatency = 0;
    int gRover = 0;
    
    // Working arrays
    std::vector<float> gInFIFO;
    std::vector<float> gOutFIFO;
    std::vector<float> gFFTworksp;
    std::vector<float> gLastPhase;
    std::vector<float> gSumPhase;
    std::vector<float> gOutputAccum;
    std::vector<float> gAnaFreq;
    std::vector<float> gAnaMagn;
    std::vector<float> gSynFreq;
    std::vector<float> gSynMagn;
    std::vector<float> window;
    
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
    std::unique_ptr<juce::dsp::FFT> fft;
#endif
};