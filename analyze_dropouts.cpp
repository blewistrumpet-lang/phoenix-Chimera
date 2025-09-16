#include <cstdio>
#include <vector>
#include <cmath>
#include <algorithm>
#include "psola_engine_final.h"

/**
 * Analyze why PSOLA engine produces dropouts
 * Focus on the synthesis timing and grain overlap
 */

void testOverlapFactors() {
    printf("=== ANALYZING DROPOUT CAUSES ===\n\n");
    
    const float fs = 48000.0f;
    const float testFreq = 220.0f;
    const float period = fs / testFreq;
    const int blockSize = 512;
    
    // Test different ratios
    float ratios[] = {0.5f, 0.7071f, 1.0f, 1.5f, 2.0f};
    
    for (float ratio : ratios) {
        printf("\nRatio %.4f:\n", ratio);
        printf("----------------\n");
        
        // Calculate synthesis parameters
        float synHop = period / ratio;
        float overlap = period / synHop;
        
        printf("  Input period: %.1f samples (%.1f Hz)\n", period, testFreq);
        printf("  Synthesis hop: %.1f samples\n", synHop);
        printf("  Overlap factor: %.2f\n", overlap);
        printf("  Grains per period: %.2f\n", period / synHop);
        
        // Check for potential gaps
        if (overlap < 1.5f) {
            printf("  ⚠️  WARNING: Low overlap factor - may cause dropouts!\n");
        }
        
        // Grain density calculation
        float grainDensity = 1.0f / synHop * period;
        printf("  Grain density: %.2f grains/period\n", grainDensity);
        
        // Energy compensation factor
        float energyComp = std::sqrt(1.0f / overlap);
        printf("  Energy compensation: %.3f\n", energyComp);
        
        // Check if grains will leave gaps
        float grainWidth = period * 2.0f; // Typical grain size
        float coverage = grainWidth / synHop;
        printf("  Coverage ratio: %.2f (should be > 2.0)\n", coverage);
        
        if (coverage < 2.0f) {
            printf("  ⚠️  WARNING: Insufficient coverage - gaps between grains!\n");
        }
    }
    
    printf("\n\n=== DROPOUT MECHANISM ===\n");
    printf("The dropouts occur because:\n");
    printf("1. Synthesis hop is too large relative to grain size\n");
    printf("2. Energy compensation formula sqrt(1/overlap) becomes too small\n");
    printf("3. Grains don't overlap sufficiently to maintain continuous output\n");
    printf("4. The 60%% core window for WSOLA reduces effective grain size\n");
    
    printf("\n=== PROPOSED FIXES ===\n");
    printf("1. Ensure minimum overlap factor of 2.0\n");
    printf("2. Use full-size grains (not 60%% core) for synthesis\n");
    printf("3. Adjust energy compensation to maintain constant RMS\n");
    printf("4. Add safety checks for synthesis hop size\n");
}

int main() {
    testOverlapFactors();
    
    printf("\n\n=== TESTING CURRENT IMPLEMENTATION ===\n");
    
    // Create engine
    PsolaEngineWithFixes engine;
    engine.prepare(48000.0, 0.6);
    
    // Generate test signal
    const int testSamples = 4800; // 0.1 second
    std::vector<float> input(testSamples);
    std::vector<float> output(testSamples);
    
    for (int i = 0; i < testSamples; ++i) {
        input[i] = 0.8f * sin(2.0f * M_PI * 220.0f * i / 48000.0f);
    }
    
    // Process with different ratios
    float testRatios[] = {0.5f, 1.0f, 2.0f};
    
    for (float ratio : testRatios) {
        engine.resetSynthesis();
        engine.pushBlock(input.data(), testSamples);
        
        // Add some epochs
        std::vector<int> epochs;
        float period = 48000.0f / 220.0f;
        for (int i = 0; i < testSamples; i += (int)period) {
            epochs.push_back(i);
        }
        engine.appendEpochs(epochs, 0, period, true);
        
        // Render
        engine.renderBlock(ratio, output.data(), testSamples);
        
        // Count zero samples (dropouts)
        int zeroCount = 0;
        float totalEnergy = 0;
        for (int i = 0; i < testSamples; ++i) {
            if (std::abs(output[i]) < 1e-6f) zeroCount++;
            totalEnergy += output[i] * output[i];
        }
        
        float rms = std::sqrt(totalEnergy / testSamples);
        float dropoutPercent = 100.0f * zeroCount / testSamples;
        
        printf("\nRatio %.2f: RMS=%.3f, Dropouts=%.1f%%\n", 
               ratio, rms, dropoutPercent);
        
        if (dropoutPercent > 10.0f) {
            printf("  ⚠️  EXCESSIVE DROPOUTS DETECTED!\n");
        }
    }
    
    return 0;
}