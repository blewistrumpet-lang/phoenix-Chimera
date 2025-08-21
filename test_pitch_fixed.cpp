#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <map>

// Test if PitchShifter parameters finally work after the fix

// Forward declarations
class PitchShifter {
public:
    PitchShifter();
    ~PitchShifter();
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);
    void updateParameters(const std::map<int, float>& params);
    juce::String getParameterName(int index) const;
};

#include <juce_audio_basics/juce_audio_basics.h>
#include "JUCE_Plugin/Source/PitchShifter.cpp"

float calculateRMS(const float* data, int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / numSamples);
}

float calculateSpectralCentroid(const float* data, int numSamples, float sampleRate) {
    // Simple spectral centroid calculation
    const int fftSize = 512;
    if (numSamples < fftSize) return 0.0f;
    
    std::vector<float> magnitude(fftSize/2 + 1);
    
    // Simple DFT for testing (not efficient but accurate)
    for (int k = 0; k <= fftSize/2; ++k) {
        float real = 0.0f, imag = 0.0f;
        for (int n = 0; n < fftSize; ++n) {
            float angle = -2.0f * M_PI * k * n / fftSize;
            real += data[n] * std::cos(angle);
            imag += data[n] * std::sin(angle);
        }
        magnitude[k] = std::sqrt(real * real + imag * imag);
    }
    
    // Calculate centroid
    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;
    
    for (int k = 1; k <= fftSize/2; ++k) {
        float freq = k * sampleRate / fftSize;
        weightedSum += freq * magnitude[k];
        magnitudeSum += magnitude[k];
    }
    
    return (magnitudeSum > 0.0f) ? (weightedSum / magnitudeSum) : 0.0f;
}

void testPitchParameter() {
    std::cout << "\n=== Testing PitchShifter After Fix ===" << std::endl;
    
    const float sampleRate = 44100.0f;
    const int blockSize = 512;
    const int testDuration = 8192;  // Samples
    
    // Create PitchShifter
    auto pitchShifter = std::make_unique<PitchShifter>();
    pitchShifter->prepareToPlay(sampleRate, blockSize);
    
    // Generate test signal (440 Hz sine wave)
    juce::AudioBuffer<float> buffer(1, testDuration);
    float* samples = buffer.getWritePointer(0);
    
    for (int i = 0; i < testDuration; ++i) {
        samples[i] = std::sin(2.0f * M_PI * 440.0f * i / sampleRate) * 0.5f;
    }
    
    // Test different pitch values
    float pitchValues[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    
    std::cout << "\nPitch parameter test results:" << std::endl;
    std::cout << "Input signal: 440 Hz sine wave" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    for (float pitchParam : pitchValues) {
        // Reset and copy buffer
        pitchShifter->reset();
        juce::AudioBuffer<float> testBuffer(1, testDuration);
        testBuffer.copyFrom(0, 0, buffer, 0, 0, testDuration);
        
        // Set parameters
        std::map<int, float> params;
        params[0] = pitchParam;  // Pitch
        params[1] = 0.5f;        // Formant (default, no shift)
        params[2] = 1.0f;        // Mix (full wet)
        pitchShifter->updateParameters(params);
        
        // Process several blocks to let phase vocoder stabilize
        for (int block = 0; block < testDuration / blockSize; ++block) {
            int offset = block * blockSize;
            int samplesThisBlock = std::min(blockSize, testDuration - offset);
            
            juce::AudioBuffer<float> blockBuffer(1, samplesThisBlock);
            blockBuffer.copyFrom(0, 0, testBuffer, 0, offset, samplesThisBlock);
            
            pitchShifter->process(blockBuffer);
            
            testBuffer.copyFrom(0, offset, blockBuffer, 0, 0, samplesThisBlock);
        }
        
        // Analyze output
        float rms = calculateRMS(testBuffer.getReadPointer(0), testDuration);
        float centroid = calculateSpectralCentroid(
            testBuffer.getReadPointer(0) + testDuration/2,  // Use second half for stability
            testDuration/2, 
            sampleRate
        );
        
        // Calculate expected frequency
        float semitones = (pitchParam - 0.5f) * 48.0f;
        float expectedFreq = 440.0f * std::pow(2.0f, semitones / 12.0f);
        
        std::cout << "Param: " << pitchParam;
        std::cout << " (" << semitones << " st)";
        std::cout << " -> Expected: " << expectedFreq << " Hz";
        std::cout << ", Centroid: " << centroid << " Hz";
        std::cout << ", RMS: " << rms;
        
        // Check if pitch is actually changing
        if (pitchParam == 0.5f && std::abs(centroid - 440.0f) > 50.0f) {
            std::cout << " [ERROR: Should be unchanged!]";
        } else if (pitchParam != 0.5f && std::abs(centroid - 440.0f) < 10.0f) {
            std::cout << " [ERROR: No pitch change detected!]";
        } else if (std::abs(centroid - expectedFreq) < expectedFreq * 0.2f) {
            std::cout << " [GOOD: Pitch shift working!]";
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "\n----------------------------------------" << std::endl;
    std::cout << "ANALYSIS:" << std::endl;
    
    // Check if we have frequency variation
    bool hasVariation = false;
    float lastCentroid = 0.0f;
    
    for (float pitchParam : pitchValues) {
        // Process again to get centroids
        pitchShifter->reset();
        juce::AudioBuffer<float> testBuffer(1, testDuration);
        testBuffer.copyFrom(0, 0, buffer, 0, 0, testDuration);
        
        std::map<int, float> params;
        params[0] = pitchParam;
        params[1] = 0.5f;
        params[2] = 1.0f;
        pitchShifter->updateParameters(params);
        
        for (int block = 0; block < testDuration / blockSize; ++block) {
            int offset = block * blockSize;
            int samplesThisBlock = std::min(blockSize, testDuration - offset);
            
            juce::AudioBuffer<float> blockBuffer(1, samplesThisBlock);
            blockBuffer.copyFrom(0, 0, testBuffer, 0, offset, samplesThisBlock);
            pitchShifter->process(blockBuffer);
            testBuffer.copyFrom(0, offset, blockBuffer, 0, 0, samplesThisBlock);
        }
        
        float centroid = calculateSpectralCentroid(
            testBuffer.getReadPointer(0) + testDuration/2,
            testDuration/2, 
            sampleRate
        );
        
        if (lastCentroid > 0.0f && std::abs(centroid - lastCentroid) > 20.0f) {
            hasVariation = true;
        }
        lastCentroid = centroid;
    }
    
    if (hasVariation) {
        std::cout << "✓ PITCH SHIFTING IS NOW WORKING!" << std::endl;
        std::cout << "  The phase vocoder fix was successful." << std::endl;
    } else {
        std::cout << "✗ PITCH SHIFTING STILL NOT WORKING" << std::endl;
        std::cout << "  Phase vocoder may need additional debugging." << std::endl;
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "PITCH SHIFTER FIX VERIFICATION TEST" << std::endl;
    std::cout << "========================================" << std::endl;
    
    testPitchParameter();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "TEST COMPLETE" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}