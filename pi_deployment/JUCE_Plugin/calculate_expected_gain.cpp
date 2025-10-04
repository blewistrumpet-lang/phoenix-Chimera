// Calculate the expected gain for Hann window with 75% overlap
#include <iostream>
#include <array>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

constexpr int FFT_SIZE = 2048;
constexpr int HOP_SIZE = 512;

int main() {
    std::cout << "Calculating expected gain for Hann window with 75% overlap\n";
    std::cout << "=========================================================\n";
    
    // Generate Hann window
    std::array<float, FFT_SIZE> window;
    for (int i = 0; i < FFT_SIZE; ++i) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
    }
    
    // Calculate overlap sum without normalization
    std::array<float, FFT_SIZE> overlapSum;
    overlapSum.fill(0.0f);
    
    for (int hop = 0; hop < FFT_SIZE; hop += HOP_SIZE) {
        for (int i = 0; i < FFT_SIZE; ++i) {
            int idx = (hop + i) % FFT_SIZE;
            overlapSum[idx] += window[i] * window[i];
        }
    }
    
    // Find the typical overlap value (should be constant in steady state)
    float expectedGain = overlapSum[0];
    std::cout << "Expected overlap gain: " << expectedGain << "\n";
    
    // Theoretical calculation for Hann window with 75% overlap
    // For 4 overlapping frames, each sample gets contributions from up to 4 windows
    float theoretical = 0.0f;
    int numContributions = 0;
    
    // Sample at position 0 gets contributions from:
    // - Current frame at window[0]
    // - Previous frame at window[HOP_SIZE] 
    // - Frame before that at window[2*HOP_SIZE]
    // - Frame before that at window[3*HOP_SIZE]
    
    for (int hop = 0; hop < 4; ++hop) {
        int windowPos = hop * HOP_SIZE;
        if (windowPos < FFT_SIZE) {
            float contribution = window[windowPos] * window[windowPos];
            theoretical += contribution;
            numContributions++;
            std::cout << "Hop " << hop << " (window[" << windowPos << "]) contributes: " 
                      << contribution << "\n";
        }
    }
    
    std::cout << "Theoretical overlap gain: " << theoretical << "\n";
    std::cout << "Match: " << (std::abs(expectedGain - theoretical) < 0.0001f ? "YES" : "NO") << "\n";
    
    // For perfect reconstruction, we need normalization factor
    float normalizationFactor = 1.0f / expectedGain;
    std::cout << "Normalization factor needed: " << normalizationFactor << "\n";
    std::cout << "After normalization, gain would be: " << expectedGain * normalizationFactor << "\n";
    
    return 0;
}