// Test JUCE FFT scaling factor
#include <iostream>
#include <array>
#include <cmath>
#include <memory>

// Minimal JUCE FFT simulation - we'll assume JUCE FFT doesn't scale
// In real JUCE, forward FFT typically doesn't scale, inverse FFT scales by 1/N

constexpr int FFT_SIZE = 2048;

void simulateJuceFFTScaling() {
    std::cout << "Simulating JUCE FFT scaling behavior\n";
    std::cout << "====================================\n";
    
    // For most FFT implementations:
    // - Forward FFT: no scaling
    // - Inverse FFT: scales by 1/N
    
    // So for a round-trip (forward + inverse), we get scaling by 1/N
    float fftScaling = 1.0f / FFT_SIZE;
    std::cout << "Expected FFT round-trip scaling: 1/" << FFT_SIZE << " = " << fftScaling << "\n";
    
    // This means our window normalization needs to account for this scaling
    // If our current normalization gives 0.667, but FFT scales by 1/2048,
    // the final result would be: 0.667 / 2048 = very small number
    
    // To get unity gain, we need: normalization * fftScaling = 1.0
    // So: normalization = 1.0 / fftScaling = FFT_SIZE
    
    float currentNormalization = 0.667008f; // From our test
    float withFFTScaling = currentNormalization * fftScaling;
    std::cout << "Current result with FFT scaling: " << withFFTScaling << "\n";
    
    float neededNormalization = 1.0f / fftScaling;
    std::cout << "Normalization needed for unity gain: " << neededNormalization << "\n";
    std::cout << "Ratio: " << neededNormalization / currentNormalization << "\n";
    
    // This suggests we need to multiply our current normalization by FFT_SIZE
    // Which means the original code with "/ (overlapCompensation[i] * FFT_SIZE)" was actually correct!
}

int main() {
    simulateJuceFFTScaling();
    return 0;
}