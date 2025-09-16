// Comprehensive parameter mapping test for all pitch engines
#include "JUCE_Plugin/Source/PitchShifter.h"
#include "JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/DetuneDoubler.h"
#include "JUCE_Plugin/Source/FrequencyShifter.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>

const int SAMPLE_RATE = 44100;
const int BUFFER_SIZE = 4096;

// Helper to generate test signal
juce::AudioBuffer<float> generateTestSignal(float freq, int numSamples, float sampleRate) {
    juce::AudioBuffer<float> buffer(1, numSamples);
    for (int i = 0; i < numSamples; ++i) {
        float sample = std::sin(2.0f * M_PI * freq * i / sampleRate) * 0.5f;
        buffer.setSample(0, i, sample);
    }
    return buffer;
}

// Detect if output has signal
float measureRMS(const juce::AudioBuffer<float>& buffer) {
    float rms = 0.0f;
    const float* data = buffer.getReadPointer(0);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        rms += data[i] * data[i];
    }
    return std::sqrt(rms / buffer.getNumSamples());
}

void testPitchShifterParameters() {
    std::cout << "\n=== Testing PitchShifter Parameters ===" << std::endl;
    PitchShifter shifter;
    shifter.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test parameter ranges
    std::cout << "\nParameter Names and Display Strings:" << std::endl;
    for (int i = 0; i < 8; ++i) {
        std::string name = shifter.getParameterName(i).toStdString();
        if (!name.empty()) {
            std::cout << "  [" << i << "] " << name << ":" << std::endl;
            
            // Test a few values
            float testValues[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
            for (float v : testValues) {
                std::string display = shifter.getParameterDisplayString(i, v).toStdString();
                std::cout << "      " << std::fixed << std::setprecision(2) << v 
                          << " -> " << display << std::endl;
            }
        }
    }
    
    // Test critical parameter combinations
    std::cout << "\nTesting Parameter Combinations:" << std::endl;
    
    struct TestCase {
        const char* name;
        std::map<int, float> params;
        float expectedPitchRatio;
    };
    
    TestCase tests[] = {
        {"Unison (no shift)", {{0, 0.5f}, {1, 1.0f}}, 1.0f},
        {"Octave up", {{0, 1.0f}, {1, 1.0f}}, 2.0f},
        {"Octave down", {{0, 0.0f}, {1, 1.0f}}, 0.5f},
        {"Fifth up", {{0, 0.792f}, {1, 1.0f}}, 1.5f}, // 7 semitones = 2^(7/12) ≈ 1.5
        {"Dry signal only", {{0, 0.5f}, {1, 0.0f}}, 1.0f},
        {"50% mix", {{0, 0.75f}, {1, 0.5f}}, 1.414f}, // 6 semitones up
    };
    
    for (const auto& test : tests) {
        shifter.reset();
        shifter.updateParameters(test.params);
        
        auto buffer = generateTestSignal(440.0f, BUFFER_SIZE, SAMPLE_RATE);
        shifter.process(buffer);
        
        float rms = measureRMS(buffer);
        std::cout << "  " << std::setw(20) << test.name << ": ";
        std::cout << "RMS=" << std::fixed << std::setprecision(3) << rms;
        
        if (rms > 0.001f) {
            std::cout << " ✓ Producing output";
        } else {
            std::cout << " ✗ No output!";
        }
        std::cout << std::endl;
    }
}

void testIntelligentHarmonizerParameters() {
    std::cout << "\n=== Testing IntelligentHarmonizer Parameters ===" << std::endl;
    IntelligentHarmonizer harmonizer;
    harmonizer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test parameter display strings
    std::cout << "\nParameter Names and Display Strings:" << std::endl;
    for (int i = 0; i < 15; ++i) {
        std::string name = harmonizer.getParameterName(i).toStdString();
        if (!name.empty()) {
            std::cout << "  [" << i << "] " << name << ":" << std::endl;
            
            // Test specific values
            if (i == 1) { // Chord Type
                float chordValues[] = {0.0f, 0.083f, 0.167f, 0.25f, 0.333f, 0.5f, 0.667f, 0.833f, 1.0f};
                for (float v : chordValues) {
                    std::string display = harmonizer.getParameterDisplayString(i, v).toStdString();
                    std::cout << "      " << std::fixed << std::setprecision(3) << v 
                              << " -> " << display << std::endl;
                }
            } else if (i == 2) { // Root Key
                float keyValues[] = {0.0f, 0.083f, 0.167f, 0.25f, 0.333f, 0.417f, 0.5f, 0.583f, 0.667f, 0.75f, 0.833f, 0.917f};
                for (float v : keyValues) {
                    std::string display = harmonizer.getParameterDisplayString(i, v).toStdString();
                    std::cout << "      " << std::fixed << std::setprecision(3) << v 
                              << " -> " << display << std::endl;
                }
            } else if (i == 3) { // Scale
                float scaleValues[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
                for (float v : scaleValues) {
                    std::string display = harmonizer.getParameterDisplayString(i, v).toStdString();
                    std::cout << "      " << std::fixed << std::setprecision(1) << v 
                              << " -> " << display << std::endl;
                }
            }
        }
    }
    
    // Test chord combinations
    std::cout << "\nTesting Chord Type Combinations:" << std::endl;
    
    struct ChordTest {
        const char* name;
        float chordParam;
        int expectedIntervals[3];
    };
    
    ChordTest chordTests[] = {
        {"Major Triad", 0.0f, {0, 4, 7}},
        {"Minor Triad", 0.083f, {0, 3, 7}},
        {"Diminished", 0.167f, {0, 3, 6}},
        {"Augmented", 0.25f, {0, 4, 8}},
        {"Major 7th", 0.333f, {0, 4, 7}}, // Plus 11
        {"Minor 7th", 0.417f, {0, 3, 7}}, // Plus 10
        {"Dominant 7th", 0.5f, {0, 4, 7}}, // Plus 10
        {"Sus2", 0.75f, {0, 2, 7}},
        {"Sus4", 0.833f, {0, 5, 7}},
        {"Custom", 1.0f, {0, 0, 0}}
    };
    
    for (const auto& test : chordTests) {
        harmonizer.reset();
        
        std::map<int, float> params;
        params[0] = 1.0f;       // 3 voices
        params[1] = test.chordParam;  // Chord type
        params[2] = 0.0f;       // Root key C
        params[3] = 1.0f;       // Chromatic scale
        params[4] = 1.0f;       // Full mix
        
        harmonizer.updateParameters(params);
        
        auto buffer = generateTestSignal(440.0f, BUFFER_SIZE, SAMPLE_RATE);
        harmonizer.process(buffer);
        
        float rms = measureRMS(buffer);
        std::cout << "  " << std::setw(20) << test.name << ": ";
        std::cout << "RMS=" << std::fixed << std::setprecision(3) << rms;
        
        if (rms > 0.001f) {
            std::cout << " ✓ Output";
        } else {
            std::cout << " ✗ Silent!";
        }
        std::cout << std::endl;
    }
    
    // Test scale quantization
    std::cout << "\nTesting Scale Quantization:" << std::endl;
    
    float scales[] = {0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f};
    const char* scaleNames[] = {"Major", "Minor", "Harmonic Minor", "Melodic Minor", 
                                 "Dorian", "Phrygian", "Lydian", "Mixolydian", 
                                 "Locrian", "Pentatonic", "Chromatic"};
    
    for (int s = 0; s <= 10; ++s) {
        harmonizer.reset();
        
        std::map<int, float> params;
        params[0] = 0.33f;      // 1 voice
        params[1] = 0.0f;       // Major triad
        params[2] = 0.0f;       // Root key C
        params[3] = scales[s];  // Scale
        params[4] = 1.0f;       // Full mix
        
        harmonizer.updateParameters(params);
        
        auto buffer = generateTestSignal(440.0f, BUFFER_SIZE, SAMPLE_RATE);
        harmonizer.process(buffer);
        
        float rms = measureRMS(buffer);
        std::cout << "  " << std::setw(15) << scaleNames[s] << ": ";
        std::cout << "RMS=" << std::fixed << std::setprecision(3) << rms;
        
        if (rms > 0.001f) {
            std::cout << " ✓";
        } else {
            std::cout << " ✗";
        }
        std::cout << std::endl;
    }
}

void testShimmerReverbParameters() {
    std::cout << "\n=== Testing ShimmerReverb Parameters ===" << std::endl;
    ShimmerReverb shimmer;
    shimmer.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test parameter combinations
    std::cout << "\nTesting Shimmer Parameter Combinations:" << std::endl;
    
    struct ShimmerTest {
        const char* name;
        std::map<int, float> params;
    };
    
    ShimmerTest tests[] = {
        {"Dry only", {{0, 0.5f}, {1, 0.5f}, {2, 0.0f}, {3, 0.5f}, {4, 0.0f}}},
        {"Full shimmer", {{0, 0.5f}, {1, 0.5f}, {2, 1.0f}, {3, 0.5f}, {4, 1.0f}}},
        {"Octave up shimmer", {{0, 0.5f}, {1, 0.5f}, {2, 0.7f}, {3, 1.0f}, {4, 0.7f}}},
        {"Octave down shimmer", {{0, 0.5f}, {1, 0.5f}, {2, 0.7f}, {3, 0.0f}, {4, 0.7f}}},
        {"Long reverb", {{0, 1.0f}, {1, 0.2f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f}}},
        {"Short reverb", {{0, 0.1f}, {1, 0.8f}, {2, 0.5f}, {3, 0.5f}, {4, 0.5f}}},
    };
    
    for (const auto& test : tests) {
        shimmer.reset();
        shimmer.updateParameters(test.params);
        
        auto buffer = generateTestSignal(440.0f, BUFFER_SIZE, SAMPLE_RATE);
        shimmer.process(buffer);
        
        float rms = measureRMS(buffer);
        std::cout << "  " << std::setw(25) << test.name << ": ";
        std::cout << "RMS=" << std::fixed << std::setprecision(3) << rms;
        
        if (rms > 0.001f) {
            std::cout << " ✓";
        } else {
            std::cout << " ✗";
        }
        std::cout << std::endl;
    }
}

void testDetuneDoublerParameters() {
    std::cout << "\n=== Testing DetuneDoubler Parameters ===" << std::endl;
    AudioDSP::DetuneDoubler doubler;
    doubler.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Get parameter info
    std::cout << "\nParameter Names and Ranges:" << std::endl;
    for (int i = 0; i < 5; ++i) {
        std::string name = doubler.getParameterName(i).toStdString();
        if (!name.empty()) {
            std::cout << "  [" << i << "] " << name << ":" << std::endl;
            
            // Test display strings at key points
            float testValues[] = {0.0f, 0.5f, 1.0f};
            for (float v : testValues) {
                std::string display = doubler.getParameterDisplayString(i, v).toStdString();
                std::cout << "      " << v << " -> " << display << std::endl;
            }
        }
    }
    
    // Test combinations
    std::cout << "\nTesting Detune Combinations:" << std::endl;
    
    struct DetuneTest {
        const char* name;
        std::map<int, float> params;
    };
    
    DetuneTest tests[] = {
        {"No detune", {{0, 0.0f}, {1, 0.2f}, {2, 0.5f}, {3, 0.3f}, {4, 1.0f}}},
        {"Subtle detune", {{0, 0.1f}, {1, 0.2f}, {2, 0.5f}, {3, 0.3f}, {4, 1.0f}}},
        {"Maximum detune", {{0, 1.0f}, {1, 0.2f}, {2, 0.5f}, {3, 0.3f}, {4, 1.0f}}},
        {"Wide stereo", {{0, 0.3f}, {1, 0.2f}, {2, 1.0f}, {3, 0.3f}, {4, 1.0f}}},
        {"Thick chorus", {{0, 0.3f}, {1, 0.2f}, {2, 0.5f}, {3, 1.0f}, {4, 1.0f}}},
        {"Dry bypass", {{0, 0.3f}, {1, 0.2f}, {2, 0.5f}, {3, 0.3f}, {4, 0.0f}}},
    };
    
    for (const auto& test : tests) {
        doubler.reset();
        doubler.updateParameters(test.params);
        
        auto buffer = generateTestSignal(440.0f, BUFFER_SIZE, SAMPLE_RATE);
        
        // DetuneDoubler needs stereo
        if (buffer.getNumChannels() == 1) {
            buffer.setSize(2, BUFFER_SIZE, true, true, false);
            buffer.copyFrom(1, 0, buffer, 0, 0, BUFFER_SIZE);
        }
        
        doubler.process(buffer);
        
        float rmsL = measureRMS(buffer);
        std::cout << "  " << std::setw(20) << test.name << ": ";
        std::cout << "RMS=" << std::fixed << std::setprecision(3) << rmsL;
        
        if (rmsL > 0.001f) {
            std::cout << " ✓";
        } else {
            std::cout << " ✗";
        }
        std::cout << std::endl;
    }
}

void testFrequencyShifterParameters() {
    std::cout << "\n=== Testing FrequencyShifter Parameters ===" << std::endl;
    FrequencyShifter shifter;
    shifter.prepareToPlay(SAMPLE_RATE, BUFFER_SIZE);
    
    // Test combinations
    std::cout << "\nTesting Frequency Shift Combinations:" << std::endl;
    
    struct FreqTest {
        const char* name;
        std::map<int, float> params;
    };
    
    FreqTest tests[] = {
        {"No shift", {{0, 0.5f}, {1, 0.0f}, {2, 1.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 0.5f}}},
        {"Shift up 50Hz", {{0, 0.75f}, {1, 0.0f}, {2, 1.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 0.5f}}},
        {"Shift down 50Hz", {{0, 0.25f}, {1, 0.0f}, {2, 1.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 0.5f}}},
        {"With feedback", {{0, 0.6f}, {1, 0.5f}, {2, 1.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 0.5f}}},
        {"With modulation", {{0, 0.5f}, {1, 0.0f}, {2, 1.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.5f}, {6, 0.5f}, {7, 0.5f}}},
        {"Stereo spread", {{0, 0.5f}, {1, 0.0f}, {2, 1.0f}, {3, 0.5f}, {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 0.5f}}},
        {"Up only", {{0, 0.6f}, {1, 0.0f}, {2, 1.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 1.0f}}},
        {"Down only", {{0, 0.4f}, {1, 0.0f}, {2, 1.0f}, {3, 0.0f}, {4, 0.0f}, {5, 0.0f}, {6, 0.0f}, {7, 0.0f}}},
    };
    
    for (const auto& test : tests) {
        shifter.reset();
        shifter.updateParameters(test.params);
        
        auto buffer = generateTestSignal(440.0f, BUFFER_SIZE, SAMPLE_RATE);
        shifter.process(buffer);
        
        float rms = measureRMS(buffer);
        std::cout << "  " << std::setw(20) << test.name << ": ";
        std::cout << "RMS=" << std::fixed << std::setprecision(3) << rms;
        
        if (rms > 0.001f) {
            std::cout << " ✓";
        } else {
            std::cout << " ✗";
        }
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "=== COMPREHENSIVE PARAMETER MAPPING TEST ===" << std::endl;
    std::cout << "Testing all parameter combinations for pitch engines\n" << std::endl;
    
    testPitchShifterParameters();
    testIntelligentHarmonizerParameters();
    testShimmerReverbParameters();
    testDetuneDoublerParameters();
    testFrequencyShifterParameters();
    
    std::cout << "\n=== PARAMETER TEST COMPLETE ===" << std::endl;
    std::cout << "Check for any ✗ marks above - those indicate potential issues." << std::endl;
    
    return 0;
}