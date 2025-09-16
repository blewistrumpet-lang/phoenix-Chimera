#include "TransientShaper_Platinum.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class TransientTestSuite {
private:
    TransientShaper_Platinum processor;
    const double sampleRate = 44100.0;
    const int blockSize = 2048;
    
    // Signal generators
    std::vector<float> generateKickDrum(int samples) {
        std::vector<float> signal(samples, 0.0f);
        
        // Sharp transient attack (0-100 samples)
        for (int i = 0; i < std::min(100, samples); ++i) {
            float envelope = std::exp(-i * 0.05f);  // Fast exponential decay
            float noise = ((rand() / float(RAND_MAX)) * 2.0f - 1.0f) * 0.3f;
            float tone = std::sin(2.0f * M_PI * 80.0f * i / sampleRate) * 0.4f; // 80Hz kick fundamental
            signal[i] = envelope * (noise + tone) * 0.8f;
        }
        
        // Body/sustain (100-800 samples) 
        for (int i = 100; i < std::min(800, samples); ++i) {
            float envelope = 0.4f * std::exp(-(i-100) * 0.003f);  // Slower decay
            float tone = std::sin(2.0f * M_PI * 60.0f * i / sampleRate);  // Lower fundamental for body
            signal[i] = envelope * tone * 0.6f;
        }
        
        return signal;
    }
    
    std::vector<float> generateSustainedTone(int samples) {
        std::vector<float> signal(samples, 0.0f);
        
        for (int i = 0; i < samples; ++i) {
            // Sustained 440Hz sine wave with gentle attack
            float envelope = (i < 200) ? (i / 200.0f) : 1.0f;  // 200-sample attack
            signal[i] = envelope * std::sin(2.0f * M_PI * 440.0f * i / sampleRate) * 0.5f;
        }
        
        return signal;
    }
    
    std::vector<float> generateMixedSignal(int samples) {
        std::vector<float> signal(samples, 0.0f);
        auto kick = generateKickDrum(samples);
        auto tone = generateSustainedTone(samples);
        
        for (int i = 0; i < samples; ++i) {
            signal[i] = kick[i] * 0.7f + tone[i] * 0.3f;
        }
        
        return signal;
    }
    
    // Analysis functions
    float calculateRMS(const std::vector<float>& signal, int start = 0, int length = -1) {
        if (length == -1) length = signal.size() - start;
        float sum = 0.0f;
        for (int i = start; i < start + length && i < signal.size(); ++i) {
            sum += signal[i] * signal[i];
        }
        return std::sqrt(sum / length);
    }
    
    float calculateTransientRMS(const std::vector<float>& signal) {
        // RMS of first 100 samples (transient portion)
        return calculateRMS(signal, 0, std::min(100, (int)signal.size()));
    }
    
    float calculateSustainRMS(const std::vector<float>& signal) {
        // RMS of samples 100-400 (sustain portion)
        int start = 100;
        int length = std::min(300, (int)signal.size() - start);
        return calculateRMS(signal, start, length);
    }
    
    std::vector<float> processSignal(const std::vector<float>& input, const std::map<int, float>& params) {
        // Update parameters
        processor.updateParameters(params);
        
        // Create JUCE buffer
        juce::AudioBuffer<float> buffer(1, input.size());
        std::copy(input.begin(), input.end(), buffer.getWritePointer(0));
        
        // Process in blocks
        for (int start = 0; start < input.size(); start += blockSize) {
            int currentBlockSize = std::min(blockSize, (int)input.size() - start);
            juce::AudioBuffer<float> blockBuffer = buffer.getRegion(0, start, 1, currentBlockSize);
            processor.process(blockBuffer);
        }
        
        // Extract processed signal
        std::vector<float> output(input.size());
        std::copy(buffer.getReadPointer(0), buffer.getReadPointer(0) + input.size(), output.begin());
        
        return output;
    }
    
public:
    TransientTestSuite() {
        processor.prepareToPlay(sampleRate, blockSize);
    }
    
    void runComprehensiveTest() {
        std::cout << "=== COMPREHENSIVE TRANSIENT SHAPER TEST ===\n";
        std::cout << std::fixed << std::setprecision(4);
        
        // Test 1: Attack Parameter with Kick Drum
        std::cout << "\n1. ATTACK PARAMETER TEST (Kick Drum Simulation)\n";
        std::cout << "================================================\n";
        
        auto kickSignal = generateKickDrum(blockSize);
        float originalTransientRMS = calculateTransientRMS(kickSignal);
        
        std::cout << "Original transient RMS: " << originalTransientRMS << "\n";
        
        // Attack at 0 (should reduce transients)
        std::map<int, float> params;
        params[TransientShaper_Platinum::Attack] = 0.0f;  // -15dB
        params[TransientShaper_Platinum::Sustain] = 0.5f; // 0dB (unity)
        params[TransientShaper_Platinum::Mix] = 1.0f;     // 100% wet
        
        auto attackMin = processSignal(kickSignal, params);
        float transientRMS_AttackMin = calculateTransientRMS(attackMin);
        
        // Attack at 1 (should boost transients)
        params[TransientShaper_Platinum::Attack] = 1.0f;  // +15dB
        auto attackMax = processSignal(kickSignal, params);
        float transientRMS_AttackMax = calculateTransientRMS(attackMax);
        
        // Unity test
        params[TransientShaper_Platinum::Attack] = 0.5f;  // 0dB
        auto attackUnity = processSignal(kickSignal, params);
        float transientRMS_AttackUnity = calculateTransientRMS(attackUnity);
        
        std::cout << "Attack=0.0 (cut):   " << transientRMS_AttackMin << " (ratio: " 
                  << transientRMS_AttackMin/originalTransientRMS << ")\n";
        std::cout << "Attack=0.5 (unity): " << transientRMS_AttackUnity << " (ratio: " 
                  << transientRMS_AttackUnity/originalTransientRMS << ")\n";
        std::cout << "Attack=1.0 (boost): " << transientRMS_AttackMax << " (ratio: " 
                  << transientRMS_AttackMax/originalTransientRMS << ")\n";
        
        float expectedRatio = transientRMS_AttackMax / transientRMS_AttackMin;
        std::cout << "Cut-to-Boost Ratio: " << expectedRatio << " (expected ~5.6 for 15dB range)\n";
        
        bool attackTest = (transientRMS_AttackMin < transientRMS_AttackUnity) && 
                         (transientRMS_AttackUnity < transientRMS_AttackMax) &&
                         (expectedRatio > 3.0f);
        std::cout << "ATTACK TEST: " << (attackTest ? "PASS" : "FAIL") << "\n";
        
        // Test 2: Sustain Parameter with Sustained Tone
        std::cout << "\n2. SUSTAIN PARAMETER TEST (Sustained Tone)\n";
        std::cout << "==========================================\n";
        
        auto toneSignal = generateSustainedTone(blockSize);
        float originalSustainRMS = calculateSustainRMS(toneSignal);
        
        std::cout << "Original sustain RMS: " << originalSustainRMS << "\n";
        
        // Reset attack to unity for sustain testing
        params[TransientShaper_Platinum::Attack] = 0.5f;  // 0dB (unity)
        
        // Sustain at 0 (should reduce sustain)
        params[TransientShaper_Platinum::Sustain] = 0.0f; // -24dB
        auto sustainMin = processSignal(toneSignal, params);
        float sustainRMS_Min = calculateSustainRMS(sustainMin);
        
        // Sustain at 1 (should boost sustain)
        params[TransientShaper_Platinum::Sustain] = 1.0f; // +24dB
        auto sustainMax = processSignal(toneSignal, params);
        float sustainRMS_Max = calculateSustainRMS(sustainMax);
        
        // Unity test
        params[TransientShaper_Platinum::Sustain] = 0.5f; // 0dB
        auto sustainUnity = processSignal(toneSignal, params);
        float sustainRMS_Unity = calculateSustainRMS(sustainUnity);
        
        std::cout << "Sustain=0.0 (cut):   " << sustainRMS_Min << " (ratio: " 
                  << sustainRMS_Min/originalSustainRMS << ")\n";
        std::cout << "Sustain=0.5 (unity): " << sustainRMS_Unity << " (ratio: " 
                  << sustainRMS_Unity/originalSustainRMS << ")\n";
        std::cout << "Sustain=1.0 (boost): " << sustainRMS_Max << " (ratio: " 
                  << sustainRMS_Max/originalSustainRMS << ")\n";
        
        float sustainRatio = sustainRMS_Max / sustainRMS_Min;
        std::cout << "Cut-to-Boost Ratio: " << sustainRatio << " (expected ~15.8 for 24dB range)\n";
        
        bool sustainTest = (sustainRMS_Min < sustainRMS_Unity) && 
                          (sustainRMS_Unity < sustainRMS_Max) &&
                          (sustainRatio > 5.0f);
        std::cout << "SUSTAIN TEST: " << (sustainTest ? "PASS" : "FAIL") << "\n";
        
        // Test 3: Mix Parameter Test
        std::cout << "\n3. MIX PARAMETER TEST\n";
        std::cout << "=====================\n";
        
        auto mixedSignal = generateMixedSignal(blockSize);
        float originalMixedRMS = calculateRMS(mixedSignal);
        
        // Set extreme parameters for obvious effect
        params[TransientShaper_Platinum::Attack] = 1.0f;  // +15dB boost
        params[TransientShaper_Platinum::Sustain] = 0.0f; // -24dB cut
        
        std::cout << "Original signal RMS: " << originalMixedRMS << "\n";
        
        // Test different mix levels
        std::vector<float> mixLevels = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        std::vector<float> mixResults;
        
        for (float mixLevel : mixLevels) {
            params[TransientShaper_Platinum::Mix] = mixLevel;
            auto mixResult = processSignal(mixedSignal, params);
            float mixRMS = calculateRMS(mixResult);
            mixResults.push_back(mixRMS);
            
            std::cout << "Mix=" << mixLevel << ": RMS=" << mixRMS 
                     << " (blend: " << (mixRMS/originalMixedRMS) << ")\n";
        }
        
        // Check if mix levels create expected progression
        bool mixTest = true;
        for (int i = 1; i < mixResults.size(); ++i) {
            if (std::abs(mixResults[i] - mixResults[i-1]) < 0.001f) {
                mixTest = false;
                break;
            }
        }
        
        // Mix=0 should be original, Mix=1 should be fully processed
        float dryWetDifference = std::abs(mixResults[4] - mixResults[0]);
        std::cout << "Dry-to-Wet difference: " << dryWetDifference << "\n";
        mixTest = mixTest && (dryWetDifference > 0.01f);
        
        std::cout << "MIX TEST: " << (mixTest ? "PASS" : "FAIL") << "\n";
        
        // Test 4: Combined Parameter Test with Real-World Signal
        std::cout << "\n4. COMBINED PARAMETER TEST (Mixed Signal)\n";
        std::cout << "========================================\n";
        
        // Test realistic parameter combinations
        struct TestCase {
            std::string name;
            float attack;
            float sustain;
            std::string description;
        };
        
        std::vector<TestCase> testCases = {
            {"Drum Enhancer", 0.8f, 0.3f, "Boost transients, cut body"},
            {"Drum Softener", 0.2f, 0.7f, "Cut transients, boost body"},
            {"Unity", 0.5f, 0.5f, "No change (reference)"},
            {"Extreme Boost", 1.0f, 1.0f, "Boost everything"},
            {"Extreme Cut", 0.0f, 0.0f, "Cut everything"}
        };
        
        params[TransientShaper_Platinum::Mix] = 1.0f; // Full wet for clear results
        
        for (const auto& test : testCases) {
            params[TransientShaper_Platinum::Attack] = test.attack;
            params[TransientShaper_Platinum::Sustain] = test.sustain;
            
            auto result = processSignal(mixedSignal, params);
            float transientRMS = calculateTransientRMS(result);
            float sustainRMS = calculateSustainRMS(result);
            float totalRMS = calculateRMS(result);
            
            std::cout << test.name << " (" << test.description << "):\n";
            std::cout << "  Transient RMS: " << transientRMS << "\n";
            std::cout << "  Sustain RMS:   " << sustainRMS << "\n";
            std::cout << "  Total RMS:     " << totalRMS << "\n\n";
        }
        
        // Test 5: Parameter Interaction Verification
        std::cout << "5. PARAMETER INTERACTION VERIFICATION\n";
        std::cout << "====================================\n";
        
        // Test that attack and sustain work independently
        params[TransientShaper_Platinum::Attack] = 1.0f;  // Max attack
        params[TransientShaper_Platinum::Sustain] = 0.0f; // Min sustain
        auto attackBoostSustainCut = processSignal(mixedSignal, params);
        
        params[TransientShaper_Platinum::Attack] = 0.0f;  // Min attack  
        params[TransientShaper_Platinum::Sustain] = 1.0f; // Max sustain
        auto attackCutSustainBoost = processSignal(mixedSignal, params);
        
        float case1_transient = calculateTransientRMS(attackBoostSustainCut);
        float case1_sustain = calculateSustainRMS(attackBoostSustainCut);
        float case2_transient = calculateTransientRMS(attackCutSustainBoost);
        float case2_sustain = calculateSustainRMS(attackCutSustainBoost);
        
        std::cout << "Attack Boost + Sustain Cut:\n";
        std::cout << "  Transient: " << case1_transient << ", Sustain: " << case1_sustain << "\n";
        std::cout << "Attack Cut + Sustain Boost:\n";
        std::cout << "  Transient: " << case2_transient << ", Sustain: " << case2_sustain << "\n";
        
        bool interactionTest = (case1_transient > case2_transient) && (case2_sustain > case1_sustain);
        std::cout << "INTERACTION TEST: " << (interactionTest ? "PASS" : "FAIL") << "\n";
        
        // Final Summary
        std::cout << "\n=== TEST SUMMARY ===\n";
        std::cout << "Attack Parameter:    " << (attackTest ? "PASS" : "FAIL") << "\n";
        std::cout << "Sustain Parameter:   " << (sustainTest ? "PASS" : "FAIL") << "\n";
        std::cout << "Mix Parameter:       " << (mixTest ? "PASS" : "FAIL") << "\n";
        std::cout << "Parameter Interaction: " << (interactionTest ? "PASS" : "FAIL") << "\n";
        
        bool allTestsPass = attackTest && sustainTest && mixTest && interactionTest;
        std::cout << "\nOVERALL RESULT: " << (allTestsPass ? "ALL TESTS PASS" : "SOME TESTS FAILED") << "\n";
        
        if (allTestsPass) {
            std::cout << "✓ TransientShaper_Platinum parameters are working correctly!\n";
            std::cout << "✓ Attack parameter properly controls transient levels\n";
            std::cout << "✓ Sustain parameter properly controls sustain/body levels\n";
            std::cout << "✓ Mix parameter properly blends dry/wet signals\n";
            std::cout << "✓ Parameters work independently and as expected\n";
        } else {
            std::cout << "✗ Some parameters may not be working as expected\n";
            std::cout << "✗ Check the implementation for proper parameter scaling\n";
        }
    }
};

int main() {
    std::cout << "TransientShaper_Platinum Comprehensive Test Suite\n";
    std::cout << "================================================\n";
    std::cout << "Sample Rate: 44.1 kHz\n";
    std::cout << "Block Size: 2048 samples\n";
    std::cout << "Expected Attack Range: ±15dB (ratio ~5.6)\n";  
    std::cout << "Expected Sustain Range: ±24dB (ratio ~15.8)\n\n";
    
    try {
        TransientTestSuite testSuite;
        testSuite.runComprehensiveTest();
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}