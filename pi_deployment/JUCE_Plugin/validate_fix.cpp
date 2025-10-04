// Mathematical validation of the SpectralFreeze window fix
#include <iostream>
#include <array>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Constants from SpectralFreeze
constexpr int FFT_ORDER = 11;
constexpr int FFT_SIZE = 1 << FFT_ORDER;  // 2048
constexpr int HOP_SIZE = FFT_SIZE / 4;     // 512 (75% overlap)

// Test the window overlap compensation math
void testWindowOverlapCompensation() {
    std::cout << "Testing Window Overlap Compensation Math\n";
    std::cout << "=======================================\n";
    std::cout << "FFT_SIZE: " << FFT_SIZE << "\n";
    std::cout << "HOP_SIZE: " << HOP_SIZE << "\n";
    std::cout << "Overlap ratio: " << (1.0f - (float)HOP_SIZE / FFT_SIZE) * 100 << "%\n\n";
    
    // Step 1: Generate Hann window
    std::array<float, FFT_SIZE> window;
    for (int i = 0; i < FFT_SIZE; ++i) {
        window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
    }
    
    // Step 2: Calculate overlap compensation (as done in SpectralFreeze)
    std::array<float, FFT_SIZE> overlapCompensation;
    overlapCompensation.fill(0.0f);
    
    // Sum overlapping windows
    for (int hop = 0; hop < FFT_SIZE; hop += HOP_SIZE) {
        for (int i = 0; i < FFT_SIZE; ++i) {
            int idx = (hop + i) % FFT_SIZE;
            overlapCompensation[idx] += window[i] * window[i];
        }
    }
    
    // Step 3: Create normalized window (FIXED - removed * FFT_SIZE)
    std::array<float, FFT_SIZE> windowNormalized;
    for (int i = 0; i < FFT_SIZE; ++i) {
        float compensation = (overlapCompensation[i] > 0.0f) ? 
                           1.0f / overlapCompensation[i] : 0.0f;
        windowNormalized[i] = window[i] * compensation;
    }
    
    // Step 4: Test the OLD (buggy) validation function
    std::cout << "Testing OLD validation method:\n";
    float oldResult = 0.0f;
    for (int hop = 0; hop < FFT_SIZE; hop += HOP_SIZE) {
        for (int i = 0; i < FFT_SIZE; ++i) {
            int idx = (hop + i) % FFT_SIZE;
            if (idx < HOP_SIZE) {
                oldResult += windowNormalized[i] * windowNormalized[i] * FFT_SIZE;
            }
        }
    }
    oldResult /= HOP_SIZE;
    std::cout << "Old validation result: " << oldResult << " (should be 1.0)\n";
    std::cout << "Error: " << std::abs(oldResult - 1.0f) << "\n\n";
    
    // Step 5: Test the NEW (fixed) validation function
    std::cout << "Testing NEW validation method:\n";
    float testGain = 0.0f;
    
    for (int testPos = 0; testPos < HOP_SIZE; ++testPos) {
        float overlap = 0.0f;
        
        // Sum contributions from all hops that affect this output position
        for (int hop = 0; hop < FFT_SIZE; hop += HOP_SIZE) {
            for (int i = 0; i < FFT_SIZE; ++i) {
                int outputPos = (hop + i) % FFT_SIZE;
                if (outputPos == testPos) {
                    overlap += windowNormalized[i] * windowNormalized[i];
                }
            }
        }
        
        testGain += overlap;
        
        if (testPos < 10) {  // Show first 10 positions
            std::cout << "Position " << testPos << ": overlap = " << overlap << "\n";
        }
    }
    
    float newResult = testGain / HOP_SIZE;
    std::cout << "...\n";
    std::cout << "New validation result: " << newResult << " (should be 1.0)\n";
    std::cout << "New method error: " << std::abs(newResult - 1.0f) << "\n\n";
    
    // Summary
    std::cout << "SUMMARY:\n";
    std::cout << "========\n";
    std::cout << "Old method error: " << std::abs(oldResult - 1.0f) << "\n";
    std::cout << "New method error: " << std::abs(newResult - 1.0f) << "\n";
    
    if (std::abs(newResult - 1.0f) < 0.001f) {
        std::cout << "SUCCESS: New validation method shows proper overlap-add compensation!\n";
    } else {
        std::cout << "WARNING: Overlap compensation may have issues.\n";
    }
    
    if (std::abs(oldResult - 1.0f) > 0.001f) {
        std::cout << "CONFIRMED: Old validation method was indeed buggy.\n";
    }
}

int main() {
    testWindowOverlapCompensation();
    return 0;
}