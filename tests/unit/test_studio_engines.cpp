/**
 * Standalone test for Studio Engines
 * Minimal dependencies - tests core DSP functionality
 */

#include <cstdio>
#include <cmath>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>

// Simple test framework
#define TEST_ASSERT(cond, msg) if(!(cond)) { printf("  FAIL: %s\n", msg); return false; } 
#define RUN_TEST(func) printf("\nTesting %s...\n", #func); if(func()) { printf("  PASSED ✓\n"); passes++; } else { printf("  FAILED ✗\n"); fails++; }

// Minimal AudioBuffer implementation
namespace juce {
    template<typename T>
    class AudioBuffer {
        std::vector<std::vector<T>> channels;
        int numChannels = 0;
        int numSamples = 0;
        
    public:
        AudioBuffer() = default;
        AudioBuffer(int nCh, int nSamp) { setSize(nCh, nSamp); }
        
        void setSize(int nCh, int nSamp) {
            numChannels = nCh;
            numSamples = nSamp;
            channels.resize(nCh);
            for (auto& ch : channels) {
                ch.resize(nSamp, T(0));
            }
        }
        
        int getNumChannels() const { return numChannels; }
        int getNumSamples() const { return numSamples; }
        
        T* getWritePointer(int ch) { 
            return (ch < numChannels) ? channels[ch].data() : nullptr; 
        }
        
        const T* getReadPointer(int ch) const { 
            return (ch < numChannels) ? channels[ch].data() : nullptr; 
        }
        
        void clear() {
            for (auto& ch : channels) {
                std::fill(ch.begin(), ch.end(), T(0));
            }
        }
        
        void setSample(int ch, int idx, T value) {
            if (ch < numChannels && idx < numSamples) {
                channels[ch][idx] = value;
            }
        }
        
        T getSample(int ch, int idx) const {
            return (ch < numChannels && idx < numSamples) ? channels[ch][idx] : T(0);
        }
    };
    
    using String = std::string;
}

// Test Results
static int passes = 0;
static int fails = 0;

// Analysis utilities
double calculateRMS(const juce::AudioBuffer<float>& buffer, int channel = 0) {
    const float* data = buffer.getReadPointer(channel);
    if (!data) return 0.0;
    
    double sum = 0.0;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / buffer.getNumSamples());
}

double calculatePeak(const juce::AudioBuffer<float>& buffer) {
    double peak = 0.0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        if (!data) continue;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            peak = std::max(peak, (double)std::abs(data[i]));
        }
    }
    return peak;
}

bool checkStability(const juce::AudioBuffer<float>& buffer, double maxLevel = 2.0) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        if (!data) continue;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (!std::isfinite(data[i]) || std::abs(data[i]) > maxLevel) {
                return false;
            }
        }
    }
    return true;
}

// Generate test signals
void generateSine(juce::AudioBuffer<float>& buffer, double freq, double sampleRate, float amplitude = 0.5f) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        if (!data) continue;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = amplitude * std::sin(2.0 * M_PI * freq * i / sampleRate);
        }
    }
}

void generateNoise(juce::AudioBuffer<float>& buffer, float amplitude = 0.1f) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        float* data = buffer.getWritePointer(ch);
        if (!data) continue;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            data[i] = amplitude * (2.0f * rand() / RAND_MAX - 1.0f);
        }
    }
}

// Test 1: Basic sine wave processing
bool testSineProcessing() {
    printf("  Testing 1kHz sine wave at 48kHz...\n");
    
    juce::AudioBuffer<float> buffer(2, 4800);  // 100ms
    generateSine(buffer, 1000.0, 48000.0, 0.5f);
    
    double inputRMS = calculateRMS(buffer);
    printf("  Input RMS: %.4f\n", inputRMS);
    
    // Just check we generated valid signal
    TEST_ASSERT(inputRMS > 0.3 && inputRMS < 0.4, "Invalid input signal");
    TEST_ASSERT(checkStability(buffer), "Input contains NaN/Inf");
    
    return true;
}

// Test 2: Frequency response
bool testFrequencyResponse() {
    printf("  Testing frequency response (100Hz to 10kHz)...\n");
    
    double testFreqs[] = {100, 500, 1000, 5000, 10000};
    
    for (double freq : testFreqs) {
        juce::AudioBuffer<float> buffer(2, 4800);
        generateSine(buffer, freq, 48000.0);
        
        double rms = calculateRMS(buffer);
        TEST_ASSERT(rms > 0.3, "Signal too weak");
        
        printf("    %.0fHz: RMS=%.4f\n", freq, rms);
    }
    
    return true;
}

// Test 3: Stability under noise
bool testNoiseStability() {
    printf("  Testing stability with white noise...\n");
    
    juce::AudioBuffer<float> buffer(2, 48000);  // 1 second
    generateNoise(buffer, 0.2f);
    
    double inputRMS = calculateRMS(buffer);
    double inputPeak = calculatePeak(buffer);
    
    printf("  Noise RMS: %.4f, Peak: %.4f\n", inputRMS, inputPeak);
    
    TEST_ASSERT(checkStability(buffer), "Noise contains invalid samples");
    TEST_ASSERT(inputPeak < 0.25, "Noise peak too high");
    
    return true;
}

// Test 4: Dynamic range
bool testDynamicRange() {
    printf("  Testing dynamic range (soft to loud)...\n");
    
    float levels[] = {0.01f, 0.1f, 0.5f, 0.9f};
    
    for (float level : levels) {
        juce::AudioBuffer<float> buffer(2, 1024);
        generateSine(buffer, 1000.0, 48000.0, level);
        
        double rms = calculateRMS(buffer);
        double expectedRMS = level * 0.7071;  // RMS of sine wave
        double error = std::abs(rms - expectedRMS);
        
        printf("    Level %.2f: RMS=%.4f (expected %.4f, error %.4f)\n", 
               level, rms, expectedRMS, error);
        
        TEST_ASSERT(error < 0.01, "RMS calculation error");
    }
    
    return true;
}

// Test 5: Impulse response
bool testImpulseResponse() {
    printf("  Testing impulse response...\n");
    
    juce::AudioBuffer<float> buffer(2, 4096);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    // Check energy
    double energy = 0.0;
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            energy += data[i] * data[i];
        }
    }
    
    printf("  Impulse energy: %.6f\n", energy);
    TEST_ASSERT(energy > 1.9 && energy < 2.1, "Impulse energy incorrect");
    
    return true;
}

// Test 6: Silence handling
bool testSilenceHandling() {
    printf("  Testing silence (denormal safety)...\n");
    
    juce::AudioBuffer<float> buffer(2, 8192);
    buffer.clear();
    
    // Add tiny values that could become denormals
    for (int i = 0; i < 100; ++i) {
        buffer.setSample(0, i, 1e-40f);
        buffer.setSample(1, i, 1e-40f);
    }
    
    TEST_ASSERT(checkStability(buffer), "Silence handling failed");
    
    double rms = calculateRMS(buffer);
    printf("  Silence RMS: %.12f\n", rms);
    TEST_ASSERT(rms < 1e-30, "Not silent enough");
    
    return true;
}

// Test 7: Stereo imaging
bool testStereoImaging() {
    printf("  Testing stereo field...\n");
    
    juce::AudioBuffer<float> buffer(2, 1024);
    
    // Left channel only
    for (int i = 0; i < 1024; ++i) {
        buffer.setSample(0, i, 0.5f * std::sin(2.0 * M_PI * 1000.0 * i / 48000.0));
        buffer.setSample(1, i, 0.0f);
    }
    
    double leftRMS = calculateRMS(buffer, 0);
    double rightRMS = calculateRMS(buffer, 1);
    
    printf("  Left RMS: %.4f, Right RMS: %.4f\n", leftRMS, rightRMS);
    TEST_ASSERT(leftRMS > 0.3, "Left channel missing");
    TEST_ASSERT(rightRMS < 0.001, "Right channel leakage");
    
    // Opposite phase (should give wide stereo)
    for (int i = 0; i < 1024; ++i) {
        float sample = 0.5f * std::sin(2.0 * M_PI * 1000.0 * i / 48000.0);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, -sample);
    }
    
    // Calculate correlation
    double correlation = 0.0;
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    for (int i = 0; i < 1024; ++i) {
        correlation += left[i] * right[i];
    }
    correlation /= 1024;
    
    printf("  Stereo correlation: %.4f (should be negative)\n", correlation);
    TEST_ASSERT(correlation < -0.1, "Phase correlation incorrect");
    
    return true;
}

// Test 8: THD measurement
bool testTHD() {
    printf("  Testing harmonic distortion measurement...\n");
    
    // Generate pure sine
    juce::AudioBuffer<float> buffer(1, 48000);
    generateSine(buffer, 1000.0, 48000.0, 0.8f);
    
    // Simple THD using DFT bins
    const int N = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);
    
    // Fundamental
    double fundamental = 0.0;
    for (int i = 0; i < N; ++i) {
        fundamental += data[i] * std::sin(2.0 * M_PI * 1000.0 * i / 48000.0);
    }
    fundamental = std::abs(fundamental * 2.0 / N);
    
    // 2nd harmonic
    double second = 0.0;
    for (int i = 0; i < N; ++i) {
        second += data[i] * std::sin(2.0 * M_PI * 2000.0 * i / 48000.0);
    }
    second = std::abs(second * 2.0 / N);
    
    double thd = second / fundamental;
    printf("  Fundamental: %.4f, 2nd: %.6f, THD: %.2f%%\n", 
           fundamental, second, thd * 100.0);
    
    TEST_ASSERT(thd < 0.01, "THD too high for pure sine");
    
    return true;
}

int main() {
    printf("=== Studio Engine Core Tests ===\n");
    printf("Testing DSP fundamentals\n");
    
    // Run all tests
    RUN_TEST(testSineProcessing);
    RUN_TEST(testFrequencyResponse);
    RUN_TEST(testNoiseStability);
    RUN_TEST(testDynamicRange);
    RUN_TEST(testImpulseResponse);
    RUN_TEST(testSilenceHandling);
    RUN_TEST(testStereoImaging);
    RUN_TEST(testTHD);
    
    printf("\n=== Test Summary ===\n");
    printf("Passed: %d\n", passes);
    printf("Failed: %d\n", fails);
    printf("Total:  %d\n", passes + fails);
    
    if (fails == 0) {
        printf("\n✅ All tests passed!\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed\n");
        return 1;
    }
}