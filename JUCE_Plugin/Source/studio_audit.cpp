/**
 * Studio Engine Audit - Lightweight test harness
 * Tests all three studio engines without full JUCE dependencies
 */

#include <cstdio>
#include <cmath>
#include <cassert>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>

// Minimal AudioBuffer stub for testing
namespace juce {
    template<typename T>
    class AudioBuffer {
        std::vector<std::vector<T>> channels;
        int numChannels = 0;
        int numSamples = 0;
        
    public:
        AudioBuffer() = default;
        AudioBuffer(int nCh, int nSamp) : numChannels(nCh), numSamples(nSamp) {
            channels.resize(nCh);
            for (auto& ch : channels) {
                ch.resize(nSamp, T(0));
            }
        }
        
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
            if (ch < numChannels && idx < numSamples) {
                return channels[ch][idx];
            }
            return T(0);
        }
        
        void copyFrom(int destCh, int destStart, const AudioBuffer& src, int srcCh, int srcStart, int num) {
            if (destCh >= numChannels || srcCh >= src.numChannels) return;
            int toCopy = std::min(num, std::min(numSamples - destStart, src.numSamples - srcStart));
            std::memcpy(&channels[destCh][destStart], &src.channels[srcCh][srcStart], toCopy * sizeof(T));
        }
        
        void makeCopyOf(const AudioBuffer& other) {
            setSize(other.numChannels, other.numSamples);
            for (int ch = 0; ch < numChannels; ++ch) {
                std::memcpy(channels[ch].data(), other.channels[ch].data(), numSamples * sizeof(T));
            }
        }
        
        T getMagnitude(int start, int num) const {
            T maxMag = 0;
            for (int ch = 0; ch < numChannels; ++ch) {
                for (int i = start; i < start + num && i < numSamples; ++i) {
                    maxMag = std::max(maxMag, std::abs(channels[ch][i]));
                }
            }
            return maxMag;
        }
    };
    
    using String = std::string;
    
    template<typename T>
    T jlimit(T min, T val, T max) {
        return std::clamp(val, min, max);
    }
}

// Include the studio engines
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1

// Stub the DspEngineUtilities
namespace Dsp {
    struct DenormalGuard {
        DenormalGuard() {
            #ifdef __SSE__
            // Set FTZ and DAZ flags
            _mm_setcsr(_mm_getcsr() | 0x8040);
            #endif
        }
        ~DenormalGuard() {}
    };
    
    struct DCBlocker {
        float x1 = 0, y1 = 0;
        void reset() { x1 = y1 = 0; }
        float process(float x) {
            float y = x - x1 + 0.995f * y1;
            x1 = x;
            y1 = y;
            return y;
        }
    };
    
    inline void scrubBuffer(juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (!std::isfinite(data[i])) {
                    data[i] = 0.0f;
                }
            }
        }
    }
    
    struct ParamAccess {
        static float get(const std::map<int, float>& params, int id, float defaultVal) {
            auto it = params.find(id);
            return (it != params.end()) ? it->second : defaultVal;
        }
    };
}

// Simple EngineBase stub
class EngineBase {
public:
    virtual ~EngineBase() = default;
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock) = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void reset() = 0;
    virtual void updateParameters(const std::map<int, float>& params) = 0;
    virtual juce::String getName() const = 0;
    virtual int getNumParameters() const = 0;
    virtual juce::String getParameterName(int index) const = 0;
};

// Include the actual implementations
#include "ParametricEQ_Studio.cpp"
#include "VintageConsoleEQ_Studio.cpp"
#include "VintageTubePreamp_Studio.cpp"

// Test utilities
static double calculateRMS(const juce::AudioBuffer<float>& buffer, int channel = 0) {
    const float* data = buffer.getReadPointer(channel);
    double sum = 0.0;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / buffer.getNumSamples());
}

static double calculateTHD(const juce::AudioBuffer<float>& buffer, double sampleRate, double fundamental) {
    // Simplified THD calculation using DFT bins
    const int N = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(0);
    
    // Calculate magnitude at fundamental
    double fundMag = 0.0;
    double phase = 0.0;
    double phaseInc = 2.0 * M_PI * fundamental / sampleRate;
    
    for (int i = 0; i < N; ++i) {
        fundMag += data[i] * std::sin(phase);
        phase += phaseInc;
    }
    fundMag = std::abs(fundMag * 2.0 / N);
    
    // Calculate harmonics (2nd through 5th)
    double harmonicSum = 0.0;
    for (int h = 2; h <= 5; ++h) {
        double harmMag = 0.0;
        phase = 0.0;
        phaseInc = 2.0 * M_PI * fundamental * h / sampleRate;
        
        for (int i = 0; i < N; ++i) {
            harmMag += data[i] * std::sin(phase);
            phase += phaseInc;
        }
        harmMag = std::abs(harmMag * 2.0 / N);
        harmonicSum += harmMag * harmMag;
    }
    
    return std::sqrt(harmonicSum) / std::max(fundMag, 1e-10);
}

// Test functions
void testParametricEQ() {
    printf("\n=== Testing ParametricEQ_Studio ===\n");
    
    ParametricEQ_Studio eq;
    eq.prepareToPlay(48000.0, 512);
    
    // Test 1: Basic processing without crashes
    juce::AudioBuffer<float> buffer(2, 1024);
    for (int i = 0; i < 1024; ++i) {
        buffer.setSample(0, i, std::sin(2.0 * M_PI * 1000.0 * i / 48000.0) * 0.5f);
        buffer.setSample(1, i, std::sin(2.0 * M_PI * 1000.0 * i / 48000.0) * 0.5f);
    }
    
    double inputRMS = calculateRMS(buffer);
    
    // Set up a boost at 1kHz
    std::map<int, float> params;
    params[ParametricEQ_Studio::kGlobalBypass] = 0.0f;
    params[ParametricEQ_Studio::kWetDry] = 1.0f;
    params[ParametricEQ_Studio::kBandBase + 0] = 1.0f;  // Enable
    params[ParametricEQ_Studio::kBandBase + 1] = 1000.0f;  // Freq
    params[ParametricEQ_Studio::kBandBase + 2] = 6.0f;  // Gain
    params[ParametricEQ_Studio::kBandBase + 3] = 2.0f;  // Q
    
    eq.updateParameters(params);
    eq.process(buffer);
    
    double outputRMS = calculateRMS(buffer);
    double gainDB = 20.0 * std::log10(outputRMS / inputRMS);
    
    printf("  Input RMS: %.6f\n", inputRMS);
    printf("  Output RMS: %.6f\n", outputRMS);
    printf("  Measured gain: %.2f dB (expected ~6dB)\n", gainDB);
    
    // Check for NaN/Inf
    bool hasNaN = false;
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (!std::isfinite(data[i])) {
                hasNaN = true;
                break;
            }
        }
    }
    
    printf("  Stability check: %s\n", hasNaN ? "FAILED" : "PASSED");
    printf("  Result: %s\n", (std::abs(gainDB - 6.0) < 2.0 && !hasNaN) ? "PASSED ✓" : "FAILED ✗");
}

void testVintageConsoleEQ() {
    printf("\n=== Testing VintageConsoleEQ_Studio ===\n");
    
    VintageConsoleEQ_Studio eq;
    eq.prepareToPlay(48000.0, 512);
    
    // Test with Neve voicing
    eq.selectConsole(VintageConsoleEQ_Studio::ConsoleType::NEVE_1073);
    
    juce::AudioBuffer<float> buffer(2, 4096);
    // Generate white noise
    for (int i = 0; i < 4096; ++i) {
        float sample = (rand() / (float)RAND_MAX) * 0.2f - 0.1f;
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    
    double inputRMS = calculateRMS(buffer);
    
    // Set parameters for typical Neve curve
    std::map<int, float> params;
    params[VintageConsoleEQ_Studio::kConsoleType] = 0.0f;  // Neve
    params[VintageConsoleEQ_Studio::kLow_Gain_dB] = 8.0f;  // Bass boost
    params[VintageConsoleEQ_Studio::kHigh_Gain_dB] = 6.0f;  // Treble boost
    params[VintageConsoleEQ_Studio::kDrive] = 0.3f;  // Some warmth
    
    eq.updateParameters(params);
    eq.process(buffer);
    
    double outputRMS = calculateRMS(buffer);
    double thd = calculateTHD(buffer, 48000.0, 1000.0);
    
    printf("  Console: Neve 1073\n");
    printf("  Input RMS: %.6f\n", inputRMS);
    printf("  Output RMS: %.6f\n", outputRMS);
    printf("  THD: %.4f%%\n", thd * 100.0);
    
    // Check stability
    bool stable = true;
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (!std::isfinite(data[i]) || std::abs(data[i]) > 2.0f) {
                stable = false;
                break;
            }
        }
    }
    
    printf("  Stability: %s\n", stable ? "PASSED" : "FAILED");
    printf("  Result: %s\n", stable ? "PASSED ✓" : "FAILED ✗");
}

void testVintageTubePreamp() {
    printf("\n=== Testing VintageTubePreamp_Studio ===\n");
    
    VintageTubePreamp_Studio preamp;
    preamp.prepareToPlay(48000.0, 512);
    
    // Generate clean sine wave
    juce::AudioBuffer<float> buffer(2, 8192);
    for (int i = 0; i < 8192; ++i) {
        float sample = std::sin(2.0 * M_PI * 1000.0 * i / 48000.0) * 0.3f;
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    
    double inputRMS = calculateRMS(buffer);
    
    // Set moderate drive with Fender voicing
    std::map<int, float> params;
    params[VintageTubePreamp_Studio::kBypass] = 0.0f;
    params[VintageTubePreamp_Studio::kVoicing] = 1.0f;  // Fender
    params[VintageTubePreamp_Studio::kDrive] = 0.5f;
    params[VintageTubePreamp_Studio::kBass] = 0.6f;
    params[VintageTubePreamp_Studio::kMid] = 0.4f;
    params[VintageTubePreamp_Studio::kTreble] = 0.6f;
    params[VintageTubePreamp_Studio::kPresence] = 0.3f;
    params[VintageTubePreamp_Studio::kOSMode] = 0.0f;  // Auto OS
    
    preamp.updateParameters(params);
    preamp.process(buffer);
    
    double outputRMS = calculateRMS(buffer);
    double thd = calculateTHD(buffer, 48000.0, 1000.0);
    
    printf("  Voicing: Fender Deluxe\n");
    printf("  Input RMS: %.6f\n", inputRMS);
    printf("  Output RMS: %.6f\n", outputRMS);
    printf("  THD: %.2f%% (tube warmth)\n", thd * 100.0);
    
    // Check for reasonable THD (should have some harmonics from tube modeling)
    bool hasWarmth = (thd > 0.001 && thd < 0.2);  // 0.1% to 20% THD
    
    // Check stability
    bool stable = true;
    float maxSample = 0.0f;
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (!std::isfinite(data[i])) {
                stable = false;
                break;
            }
            maxSample = std::max(maxSample, std::abs(data[i]));
        }
    }
    
    printf("  Peak level: %.3f\n", maxSample);
    printf("  Tube character: %s\n", hasWarmth ? "Present" : "Missing");
    printf("  Stability: %s\n", stable ? "PASSED" : "FAILED");
    printf("  Result: %s\n", (stable && hasWarmth) ? "PASSED ✓" : "FAILED ✗");
}

void testAutomation() {
    printf("\n=== Testing Parameter Automation ===\n");
    
    VintageConsoleEQ_Studio eq;
    eq.prepareToPlay(48000.0, 256);
    
    // Generate test signal
    juce::AudioBuffer<float> buffer(2, 256);
    for (int i = 0; i < 256; ++i) {
        float sample = (rand() / (float)RAND_MAX) * 0.1f - 0.05f;
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    
    // Rapidly change parameters
    bool smooth = true;
    for (int iter = 0; iter < 10; ++iter) {
        std::map<int, float> params;
        params[VintageConsoleEQ_Studio::kLow_Gain_dB] = (iter % 2) ? 10.0f : -10.0f;
        params[VintageConsoleEQ_Studio::kHigh_Gain_dB] = (iter % 2) ? -8.0f : 8.0f;
        params[VintageConsoleEQ_Studio::kDrive] = iter * 0.1f;
        
        eq.updateParameters(params);
        eq.process(buffer);
        
        // Check for clicks (large sample-to-sample deltas)
        const float* data = buffer.getReadPointer(0);
        for (int i = 1; i < buffer.getNumSamples(); ++i) {
            float delta = std::abs(data[i] - data[i-1]);
            if (delta > 0.5f) {  // Threshold for click detection
                smooth = false;
                printf("  Click detected at sample %d (delta: %.3f)\n", i, delta);
                break;
            }
        }
    }
    
    printf("  Automation smoothness: %s\n", smooth ? "PASSED ✓" : "FAILED ✗");
}

int main() {
    printf("=== Studio Engine Quality Audit ===\n");
    printf("Testing Dr. Sarah Chen's implementations\n");
    
    // Test each engine
    testParametricEQ();
    testVintageConsoleEQ();
    testVintageTubePreamp();
    testAutomation();
    
    printf("\n=== Audit Complete ===\n");
    printf("All critical functionality tested.\n");
    
    return 0;
}