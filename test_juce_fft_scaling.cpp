// Test JUCE FFT scaling to understand the normalization
#include <juce_dsp/juce_dsp.h>
#include <iostream>
#include <vector>
#include <cmath>

int main() {
    std::cout << "=== JUCE FFT Scaling Test ===" << std::endl;
    
    const int fftSize = 2048;
    const int order = 11; // 2^11 = 2048
    
    juce::dsp::FFT fft(order);
    
    // Create test signal - simple sine wave with known amplitude
    std::vector<float> testSignal(fftSize * 2); // Need 2x size for real FFT
    float testAmplitude = 0.5f;
    
    for (int i = 0; i < fftSize; ++i) {
        testSignal[i] = testAmplitude * std::sin(2.0f * M_PI * 10.0f * i / fftSize);
    }
    
    // Calculate input RMS
    float inputRMS = 0.0f;
    for (int i = 0; i < fftSize; ++i) {
        inputRMS += testSignal[i] * testSignal[i];
    }
    inputRMS = std::sqrt(inputRMS / fftSize);
    
    std::cout << "Input amplitude: " << testAmplitude << std::endl;
    std::cout << "Input RMS: " << inputRMS << std::endl;
    
    // Forward FFT
    fft.performRealOnlyForwardTransform(testSignal.data());
    
    // Check FFT magnitude (just for reference)
    float maxMag = 0.0f;
    for (int i = 0; i < fftSize; ++i) {
        float real = testSignal[i * 2];
        float imag = testSignal[i * 2 + 1];
        float mag = std::sqrt(real * real + imag * imag);
        maxMag = std::max(maxMag, mag);
    }
    std::cout << "Max FFT magnitude: " << maxMag << std::endl;
    
    // Inverse FFT
    fft.performRealOnlyInverseTransform(testSignal.data());
    
    // Calculate output RMS
    float outputRMS = 0.0f;
    float maxOutput = 0.0f;
    for (int i = 0; i < fftSize; ++i) {
        outputRMS += testSignal[i] * testSignal[i];
        maxOutput = std::max(maxOutput, std::abs(testSignal[i]));
    }
    outputRMS = std::sqrt(outputRMS / fftSize);
    
    std::cout << "\nAfter forward + inverse FFT:" << std::endl;
    std::cout << "Output max amplitude: " << maxOutput << std::endl;
    std::cout << "Output RMS: " << outputRMS << std::endl;
    std::cout << "Scaling factor needed: " << (inputRMS / outputRMS) << std::endl;
    
    // Test with Hann window
    std::cout << "\n=== Testing with Hann Window ===" << std::endl;
    
    std::vector<float> windowed(fftSize * 2);
    std::vector<float> window(fftSize);
    
    // Create Hann window
    for (int i = 0; i < fftSize; ++i) {
        window[i] = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / fftSize);
        windowed[i] = testAmplitude * std::sin(2.0f * M_PI * 10.0f * i / fftSize) * window[i];
    }
    
    // Calculate windowed input RMS
    float windowedInputRMS = 0.0f;
    for (int i = 0; i < fftSize; ++i) {
        windowedInputRMS += windowed[i] * windowed[i];
    }
    windowedInputRMS = std::sqrt(windowedInputRMS / fftSize);
    
    std::cout << "Windowed input RMS: " << windowedInputRMS << std::endl;
    
    // Forward + inverse
    fft.performRealOnlyForwardTransform(windowed.data());
    fft.performRealOnlyInverseTransform(windowed.data());
    
    // Apply window again (as in overlap-add)
    for (int i = 0; i < fftSize; ++i) {
        windowed[i] *= window[i];
    }
    
    // Calculate final output RMS
    float windowedOutputRMS = 0.0f;
    for (int i = 0; i < fftSize; ++i) {
        windowedOutputRMS += windowed[i] * windowed[i];
    }
    windowedOutputRMS = std::sqrt(windowedOutputRMS / fftSize);
    
    std::cout << "Windowed output RMS: " << windowedOutputRMS << std::endl;
    std::cout << "Window scaling factor needed: " << (windowedInputRMS / windowedOutputRMS) << std::endl;
    
    return 0;
}