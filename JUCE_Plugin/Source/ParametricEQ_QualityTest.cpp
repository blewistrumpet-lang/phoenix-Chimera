/**
 * ParametricEQ_QualityTest.cpp
 * Dr. Sarah Chen - Comprehensive validation suite for ParametricEQ_Studio
 * 
 * Tests:
 * 1. Frequency response accuracy
 * 2. Phase response
 * 3. Impulse response 
 * 4. Parameter automation smoothness
 * 5. Numerical stability at extremes
 * 6. CPU performance benchmarks
 * 7. Null test (inverse EQ cancellation)
 */

#include "JuceHeader.h"
#include "ParametricEQ_Studio.h"
#include <cmath>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <complex>

class ParametricEQ_QualityTest {
public:
    static constexpr double SAMPLE_RATE = 48000.0;
    static constexpr int BLOCK_SIZE = 512;
    
    // Test tolerances
    static constexpr double FREQ_TOLERANCE_DB = 0.5;   // ±0.5dB frequency response accuracy
    static constexpr double NULL_THRESHOLD_DB = -90.0; // Null test residual
    static constexpr double DC_THRESHOLD = 0.001;      // DC offset threshold
    static constexpr double CLICK_THRESHOLD = 0.5;     // Max sample-to-sample delta
    
    void runAllTests() {
        printf("\n=== ParametricEQ_Studio Quality Tests ===\n");
        printf("Dr. Sarah Chen - Studio Quality Validation\n\n");
        
        bool allPassed = true;
        
        allPassed &= testFrequencyResponse();
        allPassed &= testPhaseResponse();
        allPassed &= testImpulseResponse();
        allPassed &= testParameterSmoothness();
        allPassed &= testNumericalStability();
        allPassed &= testCPUPerformance();
        allPassed &= testNullCancellation();
        allPassed &= testMidSideProcessing();
        allPassed &= testVintageMode();
        
        printf("\n=== FINAL RESULT: %s ===\n", allPassed ? "ALL TESTS PASSED ✅" : "SOME TESTS FAILED ❌");
    }
    
private:
    // Helper: Generate test signal
    void generateSineWave(juce::AudioBuffer<float>& buffer, double freq, float amplitude) {
        const int numSamples = buffer.getNumSamples();
        const double phaseInc = 2.0 * M_PI * freq / SAMPLE_RATE;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = amplitude * std::sin(i * phaseInc);
            }
        }
    }
    
    // Helper: Calculate RMS in dB
    double calculateRMSdB(const juce::AudioBuffer<float>& buffer, int channel = 0) {
        const int N = buffer.getNumSamples();
        const float* data = buffer.getReadPointer(channel);
        
        double sum = 0.0;
        for (int i = 0; i < N; ++i) {
            sum += data[i] * data[i];
        }
        
        double rms = std::sqrt(sum / N);
        return 20.0 * std::log10(std::max(1e-12, rms));
    }
    
    // Helper: Goertzel algorithm for specific frequency bin
    double goertzelMagnitudedB(const float* data, int N, double targetFreq) {
        const double k = std::round((N * targetFreq) / SAMPLE_RATE);
        const double omega = 2.0 * M_PI * k / N;
        const double coeff = 2.0 * std::cos(omega);
        
        double s0 = 0.0, s1 = 0.0, s2 = 0.0;
        for (int n = 0; n < N; ++n) {
            s0 = data[n] + coeff * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        
        double real = s1 - s2 * std::cos(omega);
        double imag = s2 * std::sin(omega);
        double magnitude = std::sqrt(real * real + imag * imag) / N;
        
        return 20.0 * std::log10(std::max(1e-12, magnitude));
    }
    
    // Test 1: Frequency Response Accuracy
    bool testFrequencyResponse() {
        printf("1. Frequency Response Test...\n");
        
        ParametricEQ_Studio eq;
        eq.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // Test frequencies and expected gains
        struct TestPoint {
            double freq;
            float gain;
            float q;
        };
        
        TestPoint testPoints[] = {
            {100.0, 6.0, 2.0},
            {1000.0, -6.0, 4.0},
            {5000.0, 12.0, 1.0},
            {10000.0, -12.0, 8.0}
        };
        
        bool passed = true;
        
        for (const auto& test : testPoints) {
            // Configure single band
            std::map<int, float> params;
            params[ParametricEQ_Studio::kGlobalBypass] = 0.0f;
            params[ParametricEQ_Studio::kWetDry] = 1.0f;
            params[ParametricEQ_Studio::kOutputTrim_dB] = 0.0f;
            
            // Band 0 settings
            const int base = ParametricEQ_Studio::kBandBase;
            params[base + 0] = 1.0f;  // Enabled
            params[base + 1] = test.freq;
            params[base + 2] = test.gain;
            params[base + 3] = test.q;
            
            eq.updateParameters(params);
            
            // Generate test signal at target frequency
            const int numSamples = 32768;  // Long enough for accurate measurement
            juce::AudioBuffer<float> buffer(2, numSamples);
            generateSineWave(buffer, test.freq, 0.5f);
            
            // Measure input level
            double inputDB = calculateRMSdB(buffer);
            
            // Process
            eq.process(buffer);
            
            // Measure output level
            double outputDB = calculateRMSdB(buffer);
            double measuredGain = outputDB - inputDB;
            
            // Check accuracy
            double error = std::abs(measuredGain - test.gain);
            if (error > FREQ_TOLERANCE_DB) {
                printf("   ❌ Freq %.0fHz: Expected %.1fdB, got %.2fdB (error: %.2fdB)\n",
                       test.freq, test.gain, measuredGain, error);
                passed = false;
            } else {
                printf("   ✓ Freq %.0fHz: %.1fdB gain (error: %.2fdB)\n",
                       test.freq, measuredGain, error);
            }
        }
        
        printf("   %s\n\n", passed ? "PASSED" : "FAILED");
        return passed;
    }
    
    // Test 2: Phase Response
    bool testPhaseResponse() {
        printf("2. Phase Response Test...\n");
        
        ParametricEQ_Studio eq;
        eq.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // Configure for unity gain (should have minimal phase shift)
        std::map<int, float> params;
        params[ParametricEQ_Studio::kGlobalBypass] = 0.0f;
        params[ParametricEQ_Studio::kWetDry] = 1.0f;
        
        const int base = ParametricEQ_Studio::kBandBase;
        params[base + 0] = 1.0f;
        params[base + 1] = 1000.0f;
        params[base + 2] = 0.0f;  // 0dB gain
        params[base + 3] = 1.0f;
        
        eq.updateParameters(params);
        
        // Impulse response for phase analysis
        juce::AudioBuffer<float> impulse(2, 4096);
        impulse.clear();
        impulse.setSample(0, 0, 1.0f);
        impulse.setSample(1, 0, 1.0f);
        
        eq.process(impulse);
        
        // Check that energy is preserved (no excessive phase distortion)
        double energy = 0.0;
        for (int i = 0; i < impulse.getNumSamples(); ++i) {
            float sample = impulse.getSample(0, i);
            energy += sample * sample;
        }
        
        bool passed = (energy > 0.9 && energy < 1.1);  // Within 10% of unity
        
        printf("   Energy preservation: %.3f %s\n", energy, passed ? "✓" : "✗");
        printf("   %s\n\n", passed ? "PASSED" : "FAILED");
        
        return passed;
    }
    
    // Test 3: Impulse Response
    bool testImpulseResponse() {
        printf("3. Impulse Response Test...\n");
        
        ParametricEQ_Studio eq;
        eq.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // High Q boost - challenging for stability
        std::map<int, float> params;
        const int base = ParametricEQ_Studio::kBandBase;
        params[base + 0] = 1.0f;
        params[base + 1] = 2000.0f;
        params[base + 2] = 12.0f;
        params[base + 3] = 10.0f;
        
        eq.updateParameters(params);
        
        juce::AudioBuffer<float> impulse(2, 8192);
        impulse.clear();
        impulse.setSample(0, 0, 1.0f);
        impulse.setSample(1, 0, 1.0f);
        
        eq.process(impulse);
        
        // Check for NaN/Inf
        bool hasNaN = false;
        bool hasInf = false;
        float maxValue = 0.0f;
        
        for (int ch = 0; ch < 2; ++ch) {
            const float* data = impulse.getReadPointer(ch);
            for (int i = 0; i < impulse.getNumSamples(); ++i) {
                if (std::isnan(data[i])) hasNaN = true;
                if (std::isinf(data[i])) hasInf = true;
                maxValue = std::max(maxValue, std::abs(data[i]));
            }
        }
        
        bool stable = !hasNaN && !hasInf && maxValue < 2.0f;
        
        printf("   NaN: %s, Inf: %s, Peak: %.3f\n",
               hasNaN ? "YES ✗" : "NO ✓",
               hasInf ? "YES ✗" : "NO ✓",
               maxValue);
        printf("   %s\n\n", stable ? "PASSED" : "FAILED");
        
        return stable;
    }
    
    // Test 4: Parameter Smoothness
    bool testParameterSmoothness() {
        printf("4. Parameter Automation Test...\n");
        
        ParametricEQ_Studio eq;
        eq.prepareToPlay(SAMPLE_RATE, 256);  // Small blocks for more updates
        
        // Generate test signal
        juce::AudioBuffer<float> buffer(2, 4096);
        generateSineWave(buffer, 440.0f, 0.5f);
        
        // Initial state
        std::map<int, float> params;
        const int base = ParametricEQ_Studio::kBandBase;
        params[base + 0] = 1.0f;
        params[base + 1] = 1000.0f;
        params[base + 2] = 0.0f;
        params[base + 3] = 1.0f;
        eq.updateParameters(params);
        
        // Process first half
        juce::AudioBuffer<float> firstHalf(2, 2048);
        for (int ch = 0; ch < 2; ++ch) {
            firstHalf.copyFrom(ch, 0, buffer, ch, 0, 2048);
        }
        eq.process(firstHalf);
        
        // Sudden parameter change
        params[base + 1] = 5000.0f;  // Jump frequency
        params[base + 2] = 12.0f;    // Jump gain
        params[base + 3] = 10.0f;    // Jump Q
        eq.updateParameters(params);
        
        // Process second half
        juce::AudioBuffer<float> secondHalf(2, 2048);
        for (int ch = 0; ch < 2; ++ch) {
            secondHalf.copyFrom(ch, 0, buffer, ch, 2048, 2048);
        }
        eq.process(secondHalf);
        
        // Check for clicks at the boundary
        float lastSample = firstHalf.getSample(0, 2047);
        float firstSample = secondHalf.getSample(0, 0);
        float click = std::abs(firstSample - lastSample);
        
        // Check for zipper noise
        float maxDelta = 0.0f;
        const float* data = secondHalf.getReadPointer(0);
        for (int i = 1; i < 256; ++i) {  // Check first 256 samples
            float delta = std::abs(data[i] - data[i-1]);
            maxDelta = std::max(maxDelta, delta);
        }
        
        bool smooth = click < CLICK_THRESHOLD && maxDelta < CLICK_THRESHOLD;
        
        printf("   Boundary click: %.4f %s\n", click, click < CLICK_THRESHOLD ? "✓" : "✗");
        printf("   Max delta: %.4f %s\n", maxDelta, maxDelta < CLICK_THRESHOLD ? "✓" : "✗");
        printf("   %s\n\n", smooth ? "PASSED" : "FAILED");
        
        return smooth;
    }
    
    // Test 5: Numerical Stability
    bool testNumericalStability() {
        printf("5. Numerical Stability Test (Extreme Settings)...\n");
        
        ParametricEQ_Studio eq;
        eq.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // Most challenging: Q=20, f=20Hz, +18dB
        std::map<int, float> params;
        const int base = ParametricEQ_Studio::kBandBase;
        params[base + 0] = 1.0f;
        params[base + 1] = 20.0f;   // Minimum frequency
        params[base + 2] = 18.0f;   // Maximum gain
        params[base + 3] = 20.0f;   // Maximum Q
        
        eq.updateParameters(params);
        
        // Process 2 seconds of audio (long-term stability)
        const int totalSamples = SAMPLE_RATE * 2;
        const int numBlocks = totalSamples / BLOCK_SIZE;
        
        bool stable = true;
        
        for (int block = 0; block < numBlocks; ++block) {
            juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
            generateSineWave(buffer, 20.0f, 0.7f);
            
            eq.process(buffer);
            
            // Check for instability
            for (int ch = 0; ch < 2; ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < BLOCK_SIZE; ++i) {
                    if (std::isnan(data[i]) || std::isinf(data[i])) {
                        stable = false;
                        break;
                    }
                    if (std::abs(data[i]) > 1.5f) {
                        stable = false;  // Reasonable headroom exceeded
                        break;
                    }
                }
            }
            
            if (!stable) break;
        }
        
        printf("   Extreme Q=20, f=20Hz, +18dB: %s\n", stable ? "STABLE ✓" : "UNSTABLE ✗");
        
        // Test all bands simultaneously
        params.clear();
        for (int band = 0; band < 6; ++band) {
            const int b = ParametricEQ_Studio::kBandBase + band * 4;
            params[b + 0] = 1.0f;
            params[b + 1] = 100.0f * std::pow(2.0f, band);  // Spaced octaves
            params[b + 2] = (band % 2) ? 12.0f : -12.0f;   // Alternating boost/cut
            params[b + 3] = 4.0f;
        }
        
        eq.updateParameters(params);
        
        juce::AudioBuffer<float> noise(2, 4096);
        for (int ch = 0; ch < 2; ++ch) {
            float* data = noise.getWritePointer(ch);
            for (int i = 0; i < 4096; ++i) {
                data[i] = (rand() / (float)RAND_MAX) * 0.5f - 0.25f;
            }
        }
        
        eq.process(noise);
        
        float peak = noise.getMagnitude(0, 4096);
        bool allBandsStable = peak < 2.0f;
        
        printf("   All 6 bands active: %s (peak: %.2f)\n",
               allBandsStable ? "STABLE ✓" : "UNSTABLE ✗", peak);
        
        printf("   %s\n\n", (stable && allBandsStable) ? "PASSED" : "FAILED");
        
        return stable && allBandsStable;
    }
    
    // Test 6: CPU Performance
    bool testCPUPerformance() {
        printf("6. CPU Performance Test...\n");
        
        ParametricEQ_Studio eq;
        eq.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // All bands active
        std::map<int, float> params;
        for (int band = 0; band < 6; ++band) {
            const int base = ParametricEQ_Studio::kBandBase + band * 4;
            params[base + 0] = 1.0f;
            params[base + 1] = 200.0f * std::pow(2.0f, band);
            params[base + 2] = 6.0f;
            params[base + 3] = 2.0f;
        }
        params[ParametricEQ_Studio::kVintageOn] = 1.0f;  // Enable vintage mode too
        
        eq.updateParameters(params);
        
        // Prepare test buffer
        juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                data[i] = (rand() / (float)RAND_MAX) * 0.5f - 0.25f;
            }
        }
        
        // Warmup
        for (int i = 0; i < 100; ++i) {
            eq.process(buffer);
        }
        
        // Benchmark
        const int iterations = 10000;
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            eq.process(buffer);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        double totalSamples = iterations * BLOCK_SIZE;
        double totalTime = totalSamples / SAMPLE_RATE;  // Real-time seconds
        double processingTime = duration.count() / 1000000.0;  // Processing seconds
        double cpuPercent = (processingTime / totalTime) * 100.0;
        
        bool efficient = cpuPercent < 2.0;  // Target: <2% CPU
        
        printf("   Processing time: %.3f ms per block\n", 
               (duration.count() / 1000.0) / iterations);
        printf("   CPU usage: %.2f%% %s\n", cpuPercent, efficient ? "✓" : "✗");
        printf("   %s\n\n", efficient ? "PASSED" : "FAILED");
        
        return efficient;
    }
    
    // Test 7: Null Cancellation
    bool testNullCancellation() {
        printf("7. Null Test (Inverse EQ)...\n");
        
        ParametricEQ_Studio eqA, eqB;
        eqA.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        eqB.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // Configure opposite EQs
        std::map<int, float> paramsA, paramsB;
        const int base = ParametricEQ_Studio::kBandBase;
        
        // EQ A: +12dB at 1kHz
        paramsA[base + 0] = 1.0f;
        paramsA[base + 1] = 1000.0f;
        paramsA[base + 2] = 12.0f;
        paramsA[base + 3] = 4.0f;
        
        // EQ B: -12dB at 1kHz (should cancel)
        paramsB[base + 0] = 1.0f;
        paramsB[base + 1] = 1000.0f;
        paramsB[base + 2] = -12.0f;
        paramsB[base + 3] = 4.0f;
        
        eqA.updateParameters(paramsA);
        eqB.updateParameters(paramsB);
        
        // Generate test signal
        const int numSamples = 32768;
        juce::AudioBuffer<float> original(2, numSamples);
        for (int ch = 0; ch < 2; ++ch) {
            float* data = original.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = (rand() / (float)RAND_MAX) * 0.5f - 0.25f;
            }
        }
        
        // Process through both EQs
        juce::AudioBuffer<float> processed(original);
        eqA.process(processed);
        eqB.process(processed);
        
        // Calculate residual
        for (int ch = 0; ch < 2; ++ch) {
            float* proc = processed.getWritePointer(ch);
            const float* orig = original.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                proc[i] -= orig[i];  // Difference signal
            }
        }
        
        double residualDB = calculateRMSdB(processed);
        bool nulled = residualDB < NULL_THRESHOLD_DB;
        
        printf("   Residual: %.1f dB %s\n", residualDB, nulled ? "✓" : "✗");
        printf("   %s\n\n", nulled ? "PASSED" : "FAILED");
        
        return nulled;
    }
    
    // Test 8: Mid/Side Processing
    bool testMidSideProcessing() {
        printf("8. Mid/Side Processing Test...\n");
        
        ParametricEQ_Studio eq;
        eq.prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // Enable M/S mode, boost mids only
        std::map<int, float> params;
        params[ParametricEQ_Studio::kMidSideOn] = 1.0f;
        
        const int base = ParametricEQ_Studio::kBandBase;
        params[base + 0] = 1.0f;
        params[base + 1] = 1000.0f;
        params[base + 2] = 6.0f;
        params[base + 3] = 2.0f;
        
        eq.updateParameters(params);
        eq.setBandMSMode(0, 1);  // M-only
        
        // Create stereo signal with different L/R content
        juce::AudioBuffer<float> buffer(2, 4096);
        generateSineWave(buffer, 1000.0f, 0.5f);
        
        // Make R different phase (creates S content)
        float* R = buffer.getWritePointer(1);
        for (int i = 0; i < 4096; ++i) {
            R[i] *= -1.0f;  // Invert right channel
        }
        
        // Store original for comparison
        juce::AudioBuffer<float> original(buffer);
        
        eq.process(buffer);
        
        // In M/S mode with M-only boost, the center (M) should be boosted
        // but the sides (S) should remain unchanged
        
        // Calculate M and S from processed
        juce::AudioBuffer<float> processed_MS(2, 4096);
        for (int i = 0; i < 4096; ++i) {
            float L = buffer.getSample(0, i);
            float R = buffer.getSample(1, i);
            float M = (L + R) * 0.70710678f;
            float S = (L - R) * 0.70710678f;
            processed_MS.setSample(0, i, M);
            processed_MS.setSample(1, i, S);
        }
        
        // Original M/S
        juce::AudioBuffer<float> original_MS(2, 4096);
        for (int i = 0; i < 4096; ++i) {
            float L = original.getSample(0, i);
            float R = original.getSample(1, i);
            float M = (L + R) * 0.70710678f;
            float S = (L - R) * 0.70710678f;
            original_MS.setSample(0, i, M);
            original_MS.setSample(1, i, S);
        }
        
        // M should be boosted by ~6dB
        double originalM = calculateRMSdB(original_MS, 0);
        double processedM = calculateRMSdB(processed_MS, 0);
        double gainM = processedM - originalM;
        
        // S should be unchanged
        double originalS = calculateRMSdB(original_MS, 1);
        double processedS = calculateRMSdB(processed_MS, 1);
        double gainS = processedS - originalS;
        
        bool msCorrect = (std::abs(gainM - 6.0) < 1.0) && (std::abs(gainS) < 0.5);
        
        printf("   M gain: %.1f dB (expected 6dB) %s\n", 
               gainM, std::abs(gainM - 6.0) < 1.0 ? "✓" : "✗");
        printf("   S gain: %.1f dB (expected 0dB) %s\n", 
               gainS, std::abs(gainS) < 0.5 ? "✓" : "✗");
        printf("   %s\n\n", msCorrect ? "PASSED" : "FAILED");
        
        return msCorrect;
    }
    
    // Test 9: Vintage Mode
    bool testVintageMode() {
        printf("9. Vintage Mode Test...\n");
        
        ParametricEQ_Studio eq;
        eq.prepareToPlay(44100.0, BLOCK_SIZE);  // 44.1k to trigger oversampling
        
        // Enable vintage mode
        std::map<int, float> params;
        params[ParametricEQ_Studio::kVintageOn] = 1.0f;
        eq.updateParameters(params);
        
        // Generate pure sine
        juce::AudioBuffer<float> clean(2, 4096);
        const double freq = 1000.0;
        const double phaseInc = 2.0 * M_PI * freq / 44100.0;
        
        for (int ch = 0; ch < 2; ++ch) {
            float* data = clean.getWritePointer(ch);
            for (int i = 0; i < 4096; ++i) {
                data[i] = 0.8f * std::sin(i * phaseInc);  // High level for saturation
            }
        }
        
        juce::AudioBuffer<float> vintage(clean);
        eq.process(vintage);
        
        // Measure harmonic content (should have 3rd harmonic from cubic)
        double fundamental = goertzelMagnitudedB(vintage.getReadPointer(0), 4096, 1000.0);
        double third = goertzelMagnitudedB(vintage.getReadPointer(0), 4096, 3000.0);
        
        double thdPercent = std::pow(10.0, (third - fundamental) / 20.0) * 100.0;
        
        bool hasHarmonics = thdPercent > 0.5 && thdPercent < 5.0;  // Subtle coloration
        
        printf("   3rd harmonic: %.1f dB below fundamental\n", fundamental - third);
        printf("   THD: %.2f%% %s\n", thdPercent, hasHarmonics ? "✓" : "✗");
        printf("   %s\n\n", hasHarmonics ? "PASSED" : "FAILED");
        
        return hasHarmonics;
    }
};

// Standalone test runner
int main() {
    ParametricEQ_QualityTest tester;
    tester.runAllTests();
    return 0;
}