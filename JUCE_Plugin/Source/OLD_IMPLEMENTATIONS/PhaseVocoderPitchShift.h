#pragma once
#include "IPitchShiftStrategy.h"
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
#include <JuceHeader.h>
#endif
#include <vector>
#include <complex>
#include <cmath>

/**
 * PhaseVocoderPitchShift - Properly implemented phase vocoder pitch shifter
 * 
 * This implementation uses the correct phase vocoder approach:
 * 1. Overlap-add framework with windowing
 * 2. FFT analysis
 * 3. Frequency domain pitch shifting with phase correction
 * 4. IFFT synthesis
 * 
 * Based on the standard STFT pitch shifting algorithm used in commercial plugins.
 */
class PhaseVocoderPitchShift : public IPitchShiftStrategy {
public:
    PhaseVocoderPitchShift() {
        reset();
    }
    
    ~PhaseVocoderPitchShift() = default;
    
    void reset() override {
        // Clear all buffers
        inputBuffer.clear();
        outputBuffer.clear();
        
        inputWritePos = 0;
        outputReadPos = 0;
        hopCounter = 0;
        
        // Clear FFT data
        if (fftData) {
            std::fill(fftData.get(), fftData.get() + fftSize, std::complex<float>(0, 0));
        }
        
        // Clear phase data
        std::fill(lastPhase.begin(), lastPhase.end(), 0.0f);
        std::fill(sumPhase.begin(), sumPhase.end(), 0.0f);
    }
    
    void prepare(double sampleRate, int maxBlockSize) override {
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        this->sampleRate = sampleRate;
        
        // FFT configuration
        // Use order 10 for 1024 samples (good balance of quality vs latency)
        fftOrder = 10;
        fftSize = 1 << fftOrder;  // 1024
        hopSize = fftSize / 4;     // 256 (75% overlap)
        
        // Create FFT object
        fft = std::make_unique<juce::dsp::FFT>(fftOrder);
        
        // Allocate buffers
        inputBuffer.resize(fftSize * 2);
        outputBuffer.resize(fftSize * 2);
        
        // Allocate FFT data
        fftData.reset(new std::complex<float>[fftSize]);
        
        // Create window (Hann window for good frequency resolution)
        window.resize(fftSize);
        for (int i = 0; i < fftSize; ++i) {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (fftSize - 1)));
        }
        
        // Phase vocoder arrays
        magnitude.resize(fftSize);
        frequency.resize(fftSize);
        lastPhase.resize(fftSize / 2 + 1);
        sumPhase.resize(fftSize / 2 + 1);
        
        reset();
#endif
    }
    
    void process(const float* input, float* output, int numSamples, float pitchRatio) override {
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        if (!fft) {
            // Not initialized, pass through
            for (int i = 0; i < numSamples; ++i) {
                output[i] = input[i];
            }
            return;
        }
        
        // Clamp pitch ratio
        pitchRatio = std::max(0.25f, std::min(4.0f, pitchRatio));
        
        for (int i = 0; i < numSamples; ++i) {
            // Write to input buffer
            inputBuffer[inputWritePos] = input[i];
            inputWritePos = (inputWritePos + 1) % inputBuffer.size();
            
            // Read from output buffer
            output[i] = outputBuffer[outputReadPos];
            outputBuffer[outputReadPos] = 0.0f;  // Clear after reading
            outputReadPos = (outputReadPos + 1) % outputBuffer.size();
            
            // Process a frame every hop size
            if (++hopCounter >= hopSize) {
                hopCounter = 0;
                processFrame(pitchRatio);
            }
        }
#else
        // No JUCE, pass through
        for (int i = 0; i < numSamples; ++i) {
            output[i] = input[i];
        }
#endif
    }
    
    int getLatencySamples() const override { 
        return fftSize - hopSize;  // Typical phase vocoder latency
    }
    
    const char* getName() const override { return "Phase Vocoder"; }
    bool isHighQuality() const override { return true; }
    int getQualityRating() const override { return 80; }
    int getCpuUsage() const override { return 40; }
    
private:
    void processFrame(float pitchRatio) {
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
        // ANALYSIS
        // Copy input frame with windowing
        int readPos = (inputWritePos - fftSize + inputBuffer.size()) % inputBuffer.size();
        for (int i = 0; i < fftSize; ++i) {
            int idx = (readPos + i) % inputBuffer.size();
            float windowed = inputBuffer[idx] * window[i];
            fftData[i] = std::complex<float>(windowed, 0.0f);
        }
        
        // Forward FFT
        fft->perform(fftData.get(), fftData.get(), false);
        
        // PITCH SHIFTING
        // Convert to magnitude and phase
        for (int i = 0; i <= fftSize / 2; ++i) {
            magnitude[i] = std::abs(fftData[i]);
            float phase = std::arg(fftData[i]);
            
            // Calculate instantaneous frequency
            float phaseDiff = phase - lastPhase[i];
            lastPhase[i] = phase;
            
            // Wrap phase difference to [-PI, PI]
            while (phaseDiff > M_PI) phaseDiff -= 2.0f * M_PI;
            while (phaseDiff < -M_PI) phaseDiff += 2.0f * M_PI;
            
            // Calculate deviation from expected phase advance
            float expectedPhaseAdvance = 2.0f * M_PI * hopSize * i / fftSize;
            float deviation = phaseDiff - expectedPhaseAdvance;
            
            // Calculate instantaneous frequency
            float instFreq = (expectedPhaseAdvance + deviation) * sampleRate / (2.0f * M_PI * hopSize);
            frequency[i] = instFreq;
        }
        
        // Clear spectrum for pitch shifting
        std::fill(fftData.get(), fftData.get() + fftSize, std::complex<float>(0, 0));
        
        // Shift frequencies
        for (int i = 0; i <= fftSize / 2; ++i) {
            // Calculate target bin
            int targetBin = static_cast<int>(i * pitchRatio + 0.5f);
            
            if (targetBin <= fftSize / 2) {
                // Calculate new phase
                float targetFreq = frequency[i] * pitchRatio;
                float phaseAdvance = 2.0f * M_PI * hopSize * targetFreq / sampleRate;
                sumPhase[targetBin] += phaseAdvance;
                
                // Wrap phase
                while (sumPhase[targetBin] > M_PI) sumPhase[targetBin] -= 2.0f * M_PI;
                while (sumPhase[targetBin] < -M_PI) sumPhase[targetBin] += 2.0f * M_PI;
                
                // Set bin with magnitude and corrected phase
                fftData[targetBin] = std::polar(magnitude[i], sumPhase[targetBin]);
            }
        }
        
        // Mirror spectrum for real signal
        for (int i = 1; i < fftSize / 2; ++i) {
            fftData[fftSize - i] = std::conj(fftData[i]);
        }
        
        // SYNTHESIS
        // Inverse FFT
        fft->perform(fftData.get(), fftData.get(), true);
        
        // Overlap-add to output buffer
        int writePos = (outputReadPos + hopSize) % outputBuffer.size();
        for (int i = 0; i < fftSize; ++i) {
            int idx = (writePos + i) % outputBuffer.size();
            outputBuffer[idx] += fftData[i].real() * window[i] / fftSize;
        }
#endif
    }
    
private:
    // FFT configuration
    int fftOrder = 10;
    int fftSize = 1024;
    int hopSize = 256;
    
#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
    std::unique_ptr<juce::dsp::FFT> fft;
#endif
    
    // Buffers
    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    std::unique_ptr<std::complex<float>[]> fftData;
    std::vector<float> window;
    
    // Phase vocoder data
    std::vector<float> magnitude;
    std::vector<float> frequency;
    std::vector<float> lastPhase;
    std::vector<float> sumPhase;
    
    // Buffer positions
    int inputWritePos = 0;
    int outputReadPos = 0;
    int hopCounter = 0;
    
    double sampleRate = 48000.0;
};