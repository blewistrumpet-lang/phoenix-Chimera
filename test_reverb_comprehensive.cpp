#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <string>
#include <map>
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

struct TestResult {
    bool passed = true;
    std::string details;
    float value = 0.0f;
};

class ReverbTester {
public:
    TestResult testDryPassthrough(EngineBase* reverb) {
        TestResult result;
        
        reverb->prepareToPlay(44100, 512);
        reverb->reset();
        
        // Set mix to 0 for dry passthrough
        std::map<int, float> params;
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            params[i] = 0.0f;
        }
        reverb->updateParameters(params);
        
        // Test signal
        juce::AudioBuffer<float> buffer(2, 512);
        for (int i = 0; i < 512; ++i) {
            buffer.setSample(0, i, 0.5f);
            buffer.setSample(1, i, 0.5f);
        }
        
        reverb->process(buffer);
        
        // Check output matches input
        float error = 0.0f;
        for (int i = 0; i < 512; ++i) {
            error += std::abs(buffer.getSample(0, i) - 0.5f);
        }
        error /= 512.0f;
        
        result.value = error;
        result.passed = (error < 0.001f);
        result.details = result.passed ? "Perfect dry passthrough" : "Dry signal altered";
        return result;
    }
    
    TestResult testStability(EngineBase* reverb) {
        TestResult result;
        
        reverb->prepareToPlay(44100, 512);
        reverb->reset();
        
        // Set high feedback parameters
        std::map<int, float> params;
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            juce::String paramName = reverb->getParameterName(i).toLowerCase();
            if (paramName.contains("mix")) {
                params[i] = 0.8f;
            } else if (paramName.contains("size") || paramName.contains("room") || 
                      paramName.contains("feedback") || paramName.contains("time")) {
                params[i] = 0.9f;  // High settings
            } else if (paramName.contains("damp")) {
                params[i] = 0.2f;  // Low damping
            } else {
                params[i] = 0.5f;
            }
        }
        reverb->updateParameters(params);
        
        // Send loud signal
        juce::AudioBuffer<float> buffer(2, 512);
        bool stable = true;
        float maxLevel = 0.0f;
        
        for (int block = 0; block < 100; ++block) {
            for (int i = 0; i < 512; ++i) {
                float sample = 0.9f * std::sin(2.0f * M_PI * 440.0f * (block * 512 + i) / 44100.0f);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }
            
            reverb->process(buffer);
            
            for (int i = 0; i < 512; ++i) {
                float sample = buffer.getSample(0, i);
                maxLevel = std::max(maxLevel, std::abs(sample));
                if (!std::isfinite(sample) || std::abs(sample) > 5.0f) {
                    stable = false;
                    break;
                }
            }
            if (!stable) break;
        }
        
        result.value = maxLevel;
        result.passed = stable && maxLevel < 2.0f;
        result.details = stable ? 
            "Stable at max level: " + std::to_string(maxLevel) : 
            "Unstable/exploding";
        return result;
    }
    
    TestResult testReverbTail(EngineBase* reverb) {
        TestResult result;
        
        reverb->prepareToPlay(44100, 512);
        reverb->reset();
        
        // Set medium reverb parameters
        std::map<int, float> params;
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            juce::String paramName = reverb->getParameterName(i).toLowerCase();
            if (paramName.contains("mix")) {
                params[i] = 1.0f;  // Full wet to measure tail
            } else if (paramName.contains("size") || paramName.contains("room")) {
                params[i] = 0.7f;
            } else if (paramName.contains("time")) {
                params[i] = 0.7f;
            } else if (paramName.contains("damp")) {
                params[i] = 0.3f;
            } else {
                params[i] = 0.5f;
            }
        }
        reverb->updateParameters(params);
        
        // Send impulse
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        reverb->process(buffer);
        
        // Find initial response
        float initialPeak = 0.0f;
        for (int i = 0; i < 512; ++i) {
            initialPeak = std::max(initialPeak, std::abs(buffer.getSample(0, i)));
        }
        
        // Measure tail
        float tailTime = 0.0f;
        bool hasProperTail = false;
        
        for (int block = 0; block < 300; ++block) {  // ~3.5 seconds
            buffer.clear();
            reverb->process(buffer);
            
            float peak = 0.0f;
            for (int i = 0; i < 512; ++i) {
                peak = std::max(peak, std::abs(buffer.getSample(0, i)));
            }
            
            if (peak > initialPeak * 0.01f) {  // -40dB threshold
                tailTime = (block + 1) * 512.0f / 44100.0f;
                hasProperTail = true;
            }
            
            if (peak < initialPeak * 0.001f) {  // -60dB reached
                break;
            }
        }
        
        result.value = tailTime;
        result.passed = hasProperTail;
        result.details = hasProperTail ? 
            "Tail duration: " + std::to_string(tailTime) + "s" :
            "No proper reverb tail detected";
        return result;
    }
    
    TestResult testMusicalResponse(EngineBase* reverb) {
        TestResult result;
        
        reverb->prepareToPlay(44100, 512);
        reverb->reset();
        
        // Realistic musical settings
        std::map<int, float> params;
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            juce::String paramName = reverb->getParameterName(i).toLowerCase();
            if (paramName.contains("mix")) {
                params[i] = 0.25f;  // 25% wet - typical
            } else if (paramName.contains("size") || paramName.contains("room")) {
                params[i] = 0.5f;
            } else if (paramName.contains("time")) {
                params[i] = 0.5f;
            } else {
                params[i] = 0.5f;
            }
        }
        reverb->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 512);
        
        // Play musical phrase (C-E-G arpeggio)
        float frequencies[] = {261.63f, 329.63f, 392.0f};
        float maxOutput = 0.0f;
        bool noClipping = true;
        
        for (int note = 0; note < 3; ++note) {
            // Play note for 200ms
            for (int block = 0; block < 17; ++block) {
                for (int i = 0; i < 512; ++i) {
                    float envelope = (block < 2) ? (block / 2.0f) : 
                                   (block > 14) ? ((17 - block) / 3.0f) : 1.0f;
                    float sample = 0.3f * envelope * 
                        std::sin(2.0f * M_PI * frequencies[note] * (block * 512 + i) / 44100.0f);
                    buffer.setSample(0, i, sample);
                    buffer.setSample(1, i, sample);
                }
                reverb->process(buffer);
                
                for (int i = 0; i < 512; ++i) {
                    float out = std::abs(buffer.getSample(0, i));
                    maxOutput = std::max(maxOutput, out);
                    if (out > 1.0f) noClipping = false;
                }
            }
        }
        
        result.value = maxOutput;
        result.passed = noClipping && maxOutput > 0.1f && maxOutput < 0.8f;
        result.details = "Max output: " + std::to_string(maxOutput) + 
                        (noClipping ? " (no clipping)" : " (CLIPPING!)");
        return result;
    }
    
    TestResult testParameterResponse(EngineBase* reverb) {
        TestResult result;
        
        reverb->prepareToPlay(44100, 512);
        reverb->reset();
        
        // Test if parameters actually affect output
        juce::AudioBuffer<float> buffer(2, 512);
        
        // First test with all parameters at minimum
        std::map<int, float> params;
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            params[i] = 0.0f;
        }
        // Set mix to 1.0 to hear the effect
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            if (reverb->getParameterName(i).toLowerCase().contains("mix")) {
                params[i] = 1.0f;
                break;
            }
        }
        reverb->updateParameters(params);
        
        // Send test signal
        for (int i = 0; i < 512; ++i) {
            buffer.setSample(0, i, 0.5f);
            buffer.setSample(1, i, 0.5f);
        }
        reverb->process(buffer);
        
        float outputMin = 0.0f;
        for (int i = 0; i < 512; ++i) {
            outputMin += std::abs(buffer.getSample(0, i));
        }
        
        // Now test with parameters at maximum
        for (int i = 0; i < reverb->getNumParameters(); ++i) {
            juce::String paramName = reverb->getParameterName(i).toLowerCase();
            if (!paramName.contains("mix")) {
                params[i] = 1.0f;
            }
        }
        reverb->updateParameters(params);
        reverb->reset();
        
        for (int i = 0; i < 512; ++i) {
            buffer.setSample(0, i, 0.5f);
            buffer.setSample(1, i, 0.5f);
        }
        reverb->process(buffer);
        
        float outputMax = 0.0f;
        for (int i = 0; i < 512; ++i) {
            outputMax += std::abs(buffer.getSample(0, i));
        }
        
        // Parameters should make a noticeable difference
        float difference = std::abs(outputMax - outputMin);
        result.value = difference;
        result.passed = difference > 10.0f;  // Significant difference expected
        result.details = "Parameter effect: " + std::to_string(difference);
        return result;
    }
    
    void runAllTests(EngineBase* reverb, const std::string& name) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "  " << name << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        struct Test {
            std::string name;
            TestResult (ReverbTester::*func)(EngineBase*);
        };
        
        std::vector<Test> tests = {
            {"Dry Passthrough    ", &ReverbTester::testDryPassthrough},
            {"Stability Test     ", &ReverbTester::testStability},
            {"Reverb Tail        ", &ReverbTester::testReverbTail},
            {"Musical Response   ", &ReverbTester::testMusicalResponse},
            {"Parameter Response ", &ReverbTester::testParameterResponse}
        };
        
        int passed = 0;
        int total = tests.size();
        
        for (const auto& test : tests) {
            TestResult result = (this->*test.func)(reverb);
            
            std::cout << "  " << test.name << ": ";
            if (result.passed) {
                std::cout << "✓ PASS";
                passed++;
            } else {
                std::cout << "✗ FAIL";
            }
            std::cout << " - " << result.details << std::endl;
        }
        
        std::cout << "\n  Overall: " << passed << "/" << total << " tests passed";
        if (passed == total) {
            std::cout << " ✓✓✓ FULLY FUNCTIONAL ✓✓✓";
        } else if (passed >= total - 1) {
            std::cout << " - Mostly working";
        } else {
            std::cout << " - Needs attention";
        }
        std::cout << std::endl;
    }
};

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║        COMPREHENSIVE REVERB ENGINE VALIDATION             ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    ReverbTester tester;
    
    // Test all reverb engines
    PlateReverb plate;
    tester.runAllTests(&plate, "PLATE REVERB");
    
    ShimmerReverb shimmer;
    tester.runAllTests(&shimmer, "SHIMMER REVERB");
    
    SpringReverb spring;
    tester.runAllTests(&spring, "SPRING REVERB");
    
    GatedReverb gated;
    tester.runAllTests(&gated, "GATED REVERB");
    
    ConvolutionReverb conv;
    tester.runAllTests(&conv, "CONVOLUTION REVERB");
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  TEST SUITE COMPLETE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "\nAll reverb engines have been comprehensively tested for:\n";
    std::cout << "  • Dry signal passthrough (mix=0)\n";
    std::cout << "  • Stability under extreme settings\n";
    std::cout << "  • Proper reverb tail generation\n";
    std::cout << "  • Musical response with typical settings\n";
    std::cout << "  • Parameter responsiveness\n";
    std::cout << std::endl;
    
    return 0;
}