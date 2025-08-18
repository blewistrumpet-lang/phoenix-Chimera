/**
 * Current Engine Status Test
 * Date: August 17, 2025
 * Purpose: Verify the ACTUAL current status of all 57 engines
 */

#include <iostream>
#include <iomanip>
#include <memory>
#include <vector>
#include <cmath>
#include <chrono>
#include <JuceHeader.h>

#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"

struct TestResult {
    int id;
    std::string name;
    bool loaded;
    bool processed;
    bool hasNaN;
    bool hasInf;
    bool hung;
    std::string error;
};

class EngineStatusTest {
private:
    const int SAMPLE_RATE = 48000;
    const int BLOCK_SIZE = 512;
    const int TEST_DURATION_MS = 100; // Short test to detect hangs
    
    ChimeraAudioProcessor processor;
    std::vector<TestResult> results;
    
    bool containsNaN(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (std::isnan(data[i])) return true;
            }
        }
        return false;
    }
    
    bool containsInf(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (std::isinf(data[i])) return true;
            }
        }
        return false;
    }
    
public:
    void testEngine(int engineID, const std::string& name) {
        TestResult result;
        result.id = engineID;
        result.name = name;
        result.loaded = false;
        result.processed = false;
        result.hasNaN = false;
        result.hasInf = false;
        result.hung = false;
        
        std::cout << "[" << std::setw(2) << engineID << "] Testing " 
                  << std::setw(30) << std::left << name << " ... ";
        std::cout.flush();
        
        try {
            // Create engine
            auto engine = EngineFactory::createEngine(engineID);
            if (!engine) {
                result.error = "Failed to create";
                std::cout << "❌ FAILED (couldn't create)\n";
                results.push_back(result);
                return;
            }
            result.loaded = true;
            
            // Initialize
            engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
            engine->reset();
            
            // Set mix parameter to ensure processing
            std::map<int, float> params;
            int mixIndex = processor.getMixParameterIndex(engineID);
            if (mixIndex >= 0) {
                params[mixIndex] = 1.0f; // 100% wet
            }
            engine->updateParameters(params);
            
            // Create test buffer with various signals
            juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
            
            // Test 1: Silence
            buffer.clear();
            auto start = std::chrono::high_resolution_clock::now();
            engine->process(buffer);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            
            if (duration > TEST_DURATION_MS) {
                result.hung = true;
                result.error = "Hung on silence";
                std::cout << "❌ HUNG\n";
                results.push_back(result);
                return;
            }
            
            if (containsNaN(buffer)) {
                result.hasNaN = true;
                result.error = "NaN on silence";
            }
            if (containsInf(buffer)) {
                result.hasInf = true;
                result.error = "Inf on silence";
            }
            
            // Test 2: Impulse
            buffer.clear();
            buffer.setSample(0, 0, 1.0f);
            buffer.setSample(1, 0, 1.0f);
            
            start = std::chrono::high_resolution_clock::now();
            engine->process(buffer);
            end = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            
            if (duration > TEST_DURATION_MS) {
                result.hung = true;
                result.error = "Hung on impulse";
                std::cout << "❌ HUNG\n";
                results.push_back(result);
                return;
            }
            
            if (containsNaN(buffer)) {
                result.hasNaN = true;
                if (result.error.empty()) result.error = "NaN on impulse";
            }
            if (containsInf(buffer)) {
                result.hasInf = true;
                if (result.error.empty()) result.error = "Inf on impulse";
            }
            
            // Test 3: Sine wave
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                float sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }
            
            start = std::chrono::high_resolution_clock::now();
            engine->process(buffer);
            end = std::chrono::high_resolution_clock::now();
            duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            
            if (duration > TEST_DURATION_MS) {
                result.hung = true;
                result.error = "Hung on sine";
                std::cout << "❌ HUNG\n";
                results.push_back(result);
                return;
            }
            
            if (containsNaN(buffer)) {
                result.hasNaN = true;
                if (result.error.empty()) result.error = "NaN on sine";
            }
            if (containsInf(buffer)) {
                result.hasInf = true;
                if (result.error.empty()) result.error = "Inf on sine";
            }
            
            result.processed = true;
            
            // Final verdict
            if (result.hasNaN || result.hasInf || result.hung) {
                std::cout << "❌ FAILED (" << result.error << ")\n";
            } else {
                std::cout << "✅ PASS\n";
            }
            
        } catch (const std::exception& e) {
            result.error = e.what();
            std::cout << "❌ EXCEPTION (" << e.what() << ")\n";
        } catch (...) {
            result.error = "Unknown exception";
            std::cout << "❌ EXCEPTION (unknown)\n";
        }
        
        results.push_back(result);
    }
    
    void runAllTests() {
        std::cout << "\n==========================================\n";
        std::cout << "  PHOENIX-CHIMERA ENGINE STATUS TEST\n";
        std::cout << "  Date: " << __DATE__ << " " << __TIME__ << "\n";
        std::cout << "==========================================\n\n";
        
        // Test all 57 engines
        const std::vector<std::pair<int, std::string>> engines = {
            {0, "NoneEngine"},
            {1, "ClassicCompressor"},
            {2, "VintageOptoCompressor_Platinum"},
            {3, "VCA_Compressor"},
            {4, "NoiseGate_Platinum"},
            {5, "TransientShaper_Platinum"},
            {6, "MasteringLimiter_Platinum"},
            {7, "ParametricEQ"},
            {8, "VintageConsoleEQ"},
            {9, "DynamicEQ"},
            {10, "AnalogPhaser"},
            {11, "EnvelopeFilter"},
            {12, "StateVariableFilter"},
            {13, "FormantFilter"},
            {14, "LadderFilter"},
            {15, "VintageTubePreamp"},
            {16, "TapeDistortion"},
            {17, "KStyleOverdrive"},
            {18, "BitCrusher"},
            {19, "WaveFolder"},
            {20, "MuffFuzz"},
            {21, "RodentDistortion"},
            {22, "MultibandSaturator"},
            {23, "StereoChorus"},
            {24, "VintageFlanger"},
            {25, "ClassicTremolo"},
            {26, "HarmonicTremolo"},
            {27, "RotarySpeaker"},
            {28, "RingModulator"},
            {29, "FrequencyShifter"},
            {30, "PitchShifter"},
            {31, "HarmonicExciter"},
            {32, "VocalFormant"},
            {33, "ResonantChorus"},
            {34, "DigitalDelay"},
            {35, "TapeEcho"},
            {36, "BucketBrigadeDelay"},
            {37, "MagneticDrumEcho"},
            {38, "BufferRepeat"},
            {39, "PlateReverb"},
            {40, "SpringReverb_Platinum"},
            {41, "ConvolutionReverb"},
            {42, "ShimmerReverb"},
            {43, "GatedReverb"},
            {44, "StereoWidener"},
            {45, "StereoImager"},
            {46, "MidSideProcessor"},
            {47, "DimensionExpander"},
            {48, "CombResonator"},
            {49, "SpectralFreeze"},
            {50, "GranularCloud"},
            {51, "ChaosGenerator"},
            {52, "FeedbackNetwork"},
            {53, "PhaseAlign_Platinum"},
            {54, "GainUtility"},
            {55, "MonoMaker"},
            {56, "SpectralGate"}
        };
        
        for (const auto& [id, name] : engines) {
            testEngine(id, name);
        }
        
        // Generate summary
        std::cout << "\n==========================================\n";
        std::cout << "              TEST SUMMARY\n";
        std::cout << "==========================================\n\n";
        
        int passed = 0;
        int failed = 0;
        std::vector<int> failedEngines;
        
        for (const auto& result : results) {
            if (result.loaded && result.processed && 
                !result.hasNaN && !result.hasInf && !result.hung) {
                passed++;
            } else {
                failed++;
                failedEngines.push_back(result.id);
            }
        }
        
        std::cout << "Total Engines: 57\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Success Rate: " << std::fixed << std::setprecision(1) 
                  << (passed * 100.0 / 57.0) << "%\n\n";
        
        if (failed > 0) {
            std::cout << "Failed Engines:\n";
            for (const auto& result : results) {
                if (!result.loaded || !result.processed || 
                    result.hasNaN || result.hasInf || result.hung) {
                    std::cout << "  [" << result.id << "] " << result.name 
                              << " - " << result.error << "\n";
                }
            }
        } else {
            std::cout << "✅ ALL ENGINES PASSED!\n";
        }
        
        std::cout << "\n==========================================\n";
        std::cout << "         TEST COMPLETE\n";
        std::cout << "==========================================\n\n";
    }
};

int main() {
    EngineStatusTest tester;
    tester.runAllTests();
    return 0;
}