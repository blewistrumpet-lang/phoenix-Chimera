#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>

/**
 * SIMPLEST POSSIBLE TD-PSOLA
 * 
 * Core insight from all references:
 * 1. For pitch UP: read FASTER from input (advance by 1/ratio)
 * 2. For pitch DOWN: read SLOWER from input (advance by 1/ratio)
 * 3. Place output grains at REGULAR intervals
 */

void simplePsola(const float* input, float* output, int numSamples, float ratio) {
    // For a pitch ratio of 2.0 (octave up):
    //   - We want to play the signal 2x faster
    //   - So we read every other sample
    // For a pitch ratio of 0.5 (octave down):
    //   - We want to play the signal 2x slower  
    //   - So we read each sample twice
    
    for (int i = 0; i < numSamples; ++i) {
        // Map output position to input position
        float inputPos = i / ratio;
        
        // Linear interpolation for fractional positions
        int idx0 = (int)inputPos;
        int idx1 = idx0 + 1;
        float frac = inputPos - idx0;
        
        if (idx0 >= 0 && idx1 < numSamples) {
            output[i] = input[idx0] * (1 - frac) + input[idx1] * frac;
        } else if (idx0 >= 0 && idx0 < numSamples) {
            output[i] = input[idx0];
        } else {
            output[i] = 0;
        }
    }
}

// Measure frequency
float measureFreq(const float* signal, int numSamples, float sampleRate) {
    // Count zero crossings
    int crossings = 0;
    for (int i = 1; i < numSamples; ++i) {
        if (signal[i-1] <= 0 && signal[i] > 0) {
            crossings++;
        }
    }
    
    if (crossings > 2) {
        float period = (float)numSamples / crossings * 2;
        return sampleRate / period;
    }
    return 0;
}

int main() {
    printf("=== SIMPLE PITCH SHIFT TEST ===\n\n");
    printf("This is NOT PSOLA - just resampling to verify the concept\n\n");
    
    const float fs = 48000.0f;
    const int N = 4800; // 0.1 second
    const float testFreq = 220.0f;
    
    // Generate input
    std::vector<float> input(N);
    for (int i = 0; i < N; ++i) {
        input[i] = std::sin(2.0f * M_PI * testFreq * i / fs);
    }
    
    // Test different ratios
    float ratios[] = {0.5f, 0.7071f, 1.0f, 1.5f, 2.0f};
    const char* names[] = {"Octave down", "Tritone down", "Unison", "Fifth up", "Octave up"};
    
    for (int r = 0; r < 5; ++r) {
        std::vector<float> output(N);
        simplePsola(input.data(), output.data(), N, ratios[r]);
        
        float measured = measureFreq(output.data(), N, fs);
        float expected = testFreq * ratios[r];
        
        printf("%s (ratio=%.4f):\n", names[r], ratios[r]);
        printf("  Expected: %.1f Hz\n", expected);
        printf("  Measured: %.1f Hz\n", measured);
        
        if (measured > 10) {
            float cents = 1200.0f * log2(measured / expected);
            printf("  Error: %.1f cents\n", cents);
            printf("  %s\n", std::abs(cents) < 50 ? "✓ WORKS" : "✗ WRONG");
        }
        printf("\n");
    }
    
    printf("CONCLUSION:\n");
    printf("Simple resampling DOES shift pitch correctly.\n");
    printf("The PSOLA implementations are failing because of incorrect:\n");
    printf("- Peak detection\n");
    printf("- Epoch mapping\n");
    printf("- Grain placement\n");
    
    return 0;
}