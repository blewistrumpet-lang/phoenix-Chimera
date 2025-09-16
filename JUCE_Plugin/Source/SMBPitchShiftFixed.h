#pragma once
#include "IPitchShiftStrategy.h"
#include <vector>
#include <cmath>
#include <cstring>
#include <memory>

#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
#include <juce_dsp/juce_dsp.h>
#endif

/**
 * Fixed implementation of Stephan M. Bernsee's pitch shifting algorithm
 * Adapted for JUCE's FFT
 */
class SMBPitchShiftFixed : public IPitchShiftStrategy {
public:
    SMBPitchShiftFixed() {
        // Initialize with default sample rate
        prepare(44100.0, 512);
    }
    ~SMBPitchShiftFixed() = default;
    
    void prepare(double sampleRate, int maxBlockSize) override {
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        // Store sample rate
        sampleRate_ = sampleRate;
        
        // FFT parameters
        fftFrameSize = 2048;  // Increased for better quality
        fftFrameSize2 = fftFrameSize / 2;
        osamp = 4;  // 75% overlap
        stepSize = fftFrameSize / osamp;
        freqPerBin = sampleRate / (double)fftFrameSize;
        expct = 2.0 * M_PI * (double)stepSize / (double)fftFrameSize;
        inFifoLatency = fftFrameSize - stepSize;
        
        // Allocate buffers
        gInFIFO.resize(fftFrameSize);
        gOutFIFO.resize(fftFrameSize);
        gFFTworksp.resize(2 * fftFrameSize);  // For interleaved format
        gLastPhase.resize(fftFrameSize2 + 1);
        gSumPhase.resize(fftFrameSize2 + 1);
        gOutputAccum.resize(2 * fftFrameSize);
        gAnaFreq.resize(fftFrameSize);
        gAnaMagn.resize(fftFrameSize);
        gSynFreq.resize(fftFrameSize);
        gSynMagn.resize(fftFrameSize);
        
        // Create window (Hann window)
        window.resize(fftFrameSize);
        for (int k = 0; k < fftFrameSize; k++) {
            window[k] = 0.5f - 0.5f * cos(2.0f * M_PI * k / (float)fftFrameSize);
        }
        
        // Create FFT object (order 11 for 2048 samples)
        fft = std::make_unique<juce::dsp::FFT>(11);
        
        reset();
        gRover = inFifoLatency;
#endif
    }
    
    void reset() override {
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        std::fill(gInFIFO.begin(), gInFIFO.end(), 0.0f);
        std::fill(gOutFIFO.begin(), gOutFIFO.end(), 0.0f);
        gOutFifoWritePos = 0;
        std::fill(gFFTworksp.begin(), gFFTworksp.end(), 0.0f);
        std::fill(gLastPhase.begin(), gLastPhase.end(), 0.0f);
        std::fill(gSumPhase.begin(), gSumPhase.end(), 0.0f);
        std::fill(gOutputAccum.begin(), gOutputAccum.end(), 0.0f);
        std::fill(gAnaFreq.begin(), gAnaFreq.end(), 0.0f);
        std::fill(gAnaMagn.begin(), gAnaMagn.end(), 0.0f);
#endif
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) override {
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        if (!fft) {
            // Not initialized, pass through
            std::copy(input, input + numSamples, output);
            return;
        }
        
        // Complete bypass for unison (no pitch shift)
        if (std::abs(pitchRatio - 1.0f) < 0.001f) {
            std::copy(input, input + numSamples, output);
            return;
        }
        
        // Clamp pitch ratio
        pitchRatio = std::max(0.5f, std::min(2.0f, pitchRatio));
        
        // Main processing loop
        for (int i = 0; i < numSamples; i++) {
            
            // Load data into FIFO
            gInFIFO[gRover] = input[i];
            output[i] = gOutFIFO[gRover - inFifoLatency];
            gRover++;
            
            // Process frame
            if (gRover >= fftFrameSize) {
                gRover = inFifoLatency;
                
                // ANALYSIS
                // Window the data and prepare for JUCE FFT
                for (int k = 0; k < fftFrameSize; k++) {
                    gFFTworksp[k] = gInFIFO[k] * window[k];
                }
                // Zero the second half for real FFT
                for (int k = fftFrameSize; k < fftFrameSize * 2; k++) {
                    gFFTworksp[k] = 0.0f;
                }
                
                // Do forward FFT using JUCE
                fft->performRealOnlyForwardTransform(gFFTworksp.data(), true);
                
                // Analyze phase and frequency
                // JUCE FFT output format: interleaved real/imag pairs
                for (int k = 0; k <= fftFrameSize2; k++) {
                    
                    float real = gFFTworksp[k * 2];
                    float imag = gFFTworksp[k * 2 + 1];
                    
                    // Compute magnitude and phase with higher precision
                    float magn = 2.0f * sqrt(real * real + imag * imag);
                    double phase = atan2((double)imag, (double)real);
                    
                    // Compute phase difference with double precision
                    double tmp = phase - gLastPhase[k];
                    gLastPhase[k] = phase;
                    
                    // Subtract expected phase difference
                    tmp -= (double)k * expct;
                    
                    // Improved phase unwrapping - princarg function
                    // Wrap delta phase into +/- Pi interval more accurately
                    while (tmp > M_PI) tmp -= 2.0 * M_PI;
                    while (tmp < -M_PI) tmp += 2.0 * M_PI;
                    
                    // Get deviation from bin frequency
                    tmp = osamp * tmp / (2.0 * M_PI);
                    
                    // Compute the k-th partials' true frequency
                    tmp = (float)k * freqPerBin + tmp * freqPerBin;
                    
                    // Store magnitude and true frequency
                    gAnaMagn[k] = magn;
                    gAnaFreq[k] = tmp;
                }
                
                // PITCH SHIFTING - CORRECTED WITH UNISON HANDLING
                std::fill(gSynMagn.begin(), gSynMagn.end(), 0.0f);
                std::fill(gSynFreq.begin(), gSynFreq.end(), 0.0f);
                
                // Special case for unison (no pitch shift)
                if (std::abs(pitchRatio - 1.0f) < 0.001f) {
                    // Just copy the analysis data unchanged
                    for (int k = 0; k <= fftFrameSize2; k++) {
                        gSynMagn[k] = gAnaMagn[k];
                        gSynFreq[k] = gAnaFreq[k];
                    }
                } else {
                    // Use INVERSE mapping: for each OUTPUT bin, find the INPUT bin
                    // This avoids gaps in the spectrum
                    
                    // Anti-aliasing: limit output frequency to prevent aliasing
                    float maxOutFreq = sampleRate_ * 0.48f;  // Close to Nyquist but safe
                    int maxOutBin = (int)(maxOutFreq / freqPerBin);
                    if (maxOutBin > fftFrameSize2) maxOutBin = fftFrameSize2;
                    
                    for (int k = 0; k <= fftFrameSize2; k++) {
                        // Find source bin for this target bin
                        float sourceBin = k / pitchRatio;
                        
                        // Anti-aliasing check for upward shifts
                        if (pitchRatio > 1.0f && k > maxOutBin) {
                            // This bin would alias, zero it out
                            gSynMagn[k] = 0.0f;
                            gSynFreq[k] = 0.0f;
                        } else if (sourceBin <= fftFrameSize2) {
                            // Cubic interpolation for smoother results
                            int bin1 = (int)sourceBin;
                            float frac = sourceBin - bin1;
                            
                            // Get 4 points for cubic interpolation (handle boundaries)
                            int bin0 = bin1 > 0 ? bin1 - 1 : 0;
                            int bin2 = bin1 < fftFrameSize2 ? bin1 + 1 : fftFrameSize2;
                            int bin3 = bin2 < fftFrameSize2 ? bin2 + 1 : fftFrameSize2;
                            
                            if (bin2 <= fftFrameSize2) {
                                // Cubic Hermite interpolation for magnitude
                                float mag0 = gAnaMagn[bin0];
                                float mag1 = gAnaMagn[bin1];
                                float mag2 = gAnaMagn[bin2];
                                float mag3 = gAnaMagn[bin3];
                                
                                // Hermite basis functions
                                float frac2 = frac * frac;
                                float frac3 = frac2 * frac;
                                float h00 = 2*frac3 - 3*frac2 + 1;
                                float h10 = frac3 - 2*frac2 + frac;
                                float h01 = -2*frac3 + 3*frac2;
                                float h11 = frac3 - frac2;
                                
                                // Tangents (slopes)
                                float m0 = 0.5f * (mag2 - mag0);
                                float m1 = 0.5f * (mag3 - mag1);
                                
                                // Interpolate magnitude
                                gSynMagn[k] = h00*mag1 + h10*m0 + h01*mag2 + h11*m1;
                                
                                // Cubic interpolation for frequency too
                                float freq0 = gAnaFreq[bin0] * pitchRatio;
                                float freq1 = gAnaFreq[bin1] * pitchRatio;
                                float freq2 = gAnaFreq[bin2] * pitchRatio;
                                float freq3 = gAnaFreq[bin3] * pitchRatio;
                                
                                // Tangents for frequency
                                float fm0 = 0.5f * (freq2 - freq0);
                                float fm1 = 0.5f * (freq3 - freq1);
                                
                                // Interpolate frequency
                                float interpolatedFreq = h00*freq1 + h10*fm0 + h01*freq2 + h11*fm1;
                                
                                // Clamp frequency to prevent aliasing
                                if (interpolatedFreq > maxOutFreq) {
                                    interpolatedFreq = maxOutFreq;
                                }
                                gSynFreq[k] = interpolatedFreq;
                            } else if (bin1 <= fftFrameSize2) {
                                // Edge case: use only bin1
                                gSynMagn[k] = gAnaMagn[bin1];
                                gSynFreq[k] = gAnaFreq[bin1] * pitchRatio;
                            }
                        }
                    }
                }
                
                // SYNTHESIS  
                for (int k = 0; k <= fftFrameSize2; k++) {
                    // Get magnitude and true frequency
                    float magn = gSynMagn[k];
                    float tmp = gSynFreq[k];
                    
                    // Subtract bin mid frequency with double precision
                    double tmpD = (double)tmp - (double)k * freqPerBin;
                    
                    // Get bin deviation from freq deviation
                    tmpD /= freqPerBin;
                    
                    // Take osamp into account
                    tmpD = 2.0 * M_PI * tmpD / (double)osamp;
                    
                    // Add the overlap phase advance back in
                    tmpD += (double)k * expct;
                    
                    // Accumulate delta phase with double precision
                    gSumPhase[k] += tmpD;
                    double phase = gSumPhase[k];
                    
                    // Get real and imag part for JUCE format
                    gFFTworksp[k * 2] = magn * cos(phase);
                    gFFTworksp[k * 2 + 1] = magn * sin(phase);
                }
                
                // Zero negative frequencies (not needed for JUCE real IFFT)
                for (int k = fftFrameSize2 + 1; k < fftFrameSize; k++) {
                    gFFTworksp[k * 2] = 0.0f;
                    gFFTworksp[k * 2 + 1] = 0.0f;
                }
                
                // Do inverse FFT using JUCE
                fft->performRealOnlyInverseTransform(gFFTworksp.data());
                
                // Do windowing and add to output accumulator
                // JUCE IFFT already normalizes, so we only need to account for overlap
                // The factor of 2.0 accounts for the window overlap, osamp is for the overlap factor
                for (int k = 0; k < fftFrameSize; k++) {
                    gOutputAccum[k] += 2.0f * window[k] * gFFTworksp[k] / osamp;
                }
                
                // Copy output accumulator to output FIFO
                for (int k = 0; k < stepSize; k++) {
                    gOutFIFO[k] = gOutputAccum[k];
                }
                
                // Shift accumulator
                memmove(gOutputAccum.data(), gOutputAccum.data() + stepSize, fftFrameSize * sizeof(float));
                
                // Shift input FIFO
                for (int k = 0; k < inFifoLatency; k++) {
                    gInFIFO[k] = gInFIFO[k + stepSize];
                }
            }
        }
#else
        // No JUCE, pass through
        std::copy(input, input + numSamples, output);
#endif
    }
    
    int getLatencySamples() const override { 
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        return inFifoLatency;
#else
        return 0;
#endif
    }
    
    const char* getName() const override { return "SMB Pitch Shift Fixed"; }
    bool isHighQuality() const override { return true; }
    int getQualityRating() const override { return 90; }
    int getCpuUsage() const override { return 40; }
    
private:
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
    // Custom FFT for interleaved data (Bernsee's implementation)
    void smbFft(float *fftBuffer, long fftFrameSize, long sign) {
        float wr, wi, arg, *p1, *p2, temp;
        float tr, ti, ur, ui, *p1r, *p1i, *p2r, *p2i;
        long i, bitm, j, le, le2, k;
        
        for (i = 2; i < 2*fftFrameSize-2; i += 2) {
            for (bitm = 2, j = 0; bitm < 2*fftFrameSize; bitm <<= 1) {
                if (i & bitm) j++;
                j <<= 1;
            }
            if (i < j) {
                p1 = fftBuffer + i; p2 = fftBuffer + j;
                temp = *p1; *(p1++) = *p2;
                *(p2++) = temp; temp = *p1;
                *p1 = *p2; *p2 = temp;
            }
        }
        for (k = 0, le = 2; k < (long)(log(fftFrameSize)/log(2.)+.5); k++) {
            le <<= 1;
            le2 = le>>1;
            ur = 1.0;
            ui = 0.0;
            arg = M_PI / (le2>>1);
            wr = cos(arg);
            wi = sign*sin(arg);
            for (j = 0; j < le2; j += 2) {
                p1r = fftBuffer+j; p1i = p1r+1;
                p2r = p1r+le2; p2i = p2r+1;
                for (i = j; i < 2*fftFrameSize; i += le) {
                    tr = *p2r * ur - *p2i * ui;
                    ti = *p2r * ui + *p2i * ur;
                    *p2r = *p1r - tr; *p2i = *p1i - ti;
                    *p1r += tr; *p1i += ti;
                    p1r += le; p1i += le;
                    p2r += le; p2i += le;
                }
                tr = ur*wr - ui*wi;
                ui = ur*wi + ui*wr;
                ur = tr;
            }
        }
    }
    
    // FFT parameters
    int fftFrameSize = 2048;
    int fftFrameSize2 = 1024;
    int osamp = 4;
    int stepSize = 512;
    double sampleRate_ = 44100.0;
    double freqPerBin = 0.0;
    double expct = 0.0;
    int inFifoLatency = 0;
    int gRover = 0;
    int gOutFifoWritePos = 0;  // Separate write position for output FIFO
    
    // Buffers
    std::vector<float> gInFIFO;
    std::vector<float> gOutFIFO;
    std::vector<float> gFFTworksp;
    std::vector<double> gLastPhase;  // Higher precision for phase tracking
    std::vector<double> gSumPhase;   // Higher precision for phase accumulation
    std::vector<float> gOutputAccum;
    std::vector<float> gAnaFreq;
    std::vector<float> gAnaMagn;
    std::vector<float> gSynFreq;
    std::vector<float> gSynMagn;
    std::vector<float> window;
    
    std::unique_ptr<juce::dsp::FFT> fft;
#endif
};