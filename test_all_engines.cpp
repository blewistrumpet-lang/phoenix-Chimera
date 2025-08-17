/**
 * Comprehensive Engine Test
 * Tests all 56 DSP engines for correctness and stability
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <memory>
#include <cmath>
#include <chrono>
#include <fstream>

#include <JuceHeader.h>
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"

class ComprehensiveEngineTest {
private:
    const int SAMPLE_RATE = 48000;
    const int BLOCK_SIZE = 512;
    
    struct TestResult {
        std::string testName;
        bool passed;
        float value;
        std::string details;
    };
    
    struct EngineReport {
        int engineID;
        std::string engineName;
        std::vector<TestResult> results;
        bool allPassed;
        double cpuUsage;
    };
    
    ChimeraAudioProcessor processor;
    std::vector<EngineReport> allReports;
    
    // Analysis helpers
    float calculateRMS(const juce::AudioBuffer<float>& buffer, int start = 0, int end = -1) {
        if (end < 0) end = buffer.getNumSamples();
        float sum = 0;
        int count = 0;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = start; i < end; ++i) {
                float sample = buffer.getSample(ch, i);
                sum += sample * sample;
                count++;
            }
        }
        return count > 0 ? std::sqrt(sum / count) : 0;
    }
    
    float calculatePeak(const juce::AudioBuffer<float>& buffer) {
        float peak = 0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(buffer.getSample(ch, i)));
            }
        }
        return peak;
    }
    
    bool hasNaNOrInf(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                if (std::isnan(sample) || std::isinf(sample)) {
                    return true;
                }
            }
        }
        return false;
    }
    
    bool hasDenormals(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = std::abs(buffer.getSample(ch, i));
                if (sample > 0 && sample < 1e-30f) {
                    return true;
                }
            }
        }
        return false;
    }
    
    // Test implementations
    TestResult testBypassMix(EngineBase* engine, int engineID) {
        TestResult result;
        result.testName = "Bypass/Mix";
        
        // Create test signal
        juce::AudioBuffer<float> input(2, SAMPLE_RATE);
        for (int i = 0; i < SAMPLE_RATE; ++i) {
            float sample = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / SAMPLE_RATE);
            input.setSample(0, i, sample);
            input.setSample(1, i, sample);
        }
        
        // Test mix = 0 (bypass)
        auto output = input;
        std::map<int, float> params;
        int mixIndex = processor.getMixParameterIndex(engineID);
        params[mixIndex] = 0.0f;
        engine->updateParameters(params);
        engine->process(output);
        
        float bypassError = 0;
        for (int i = 0; i < output.getNumSamples(); ++i) {
            bypassError += std::abs(output.getSample(0, i) - input.getSample(0, i));
        }
        bypassError /= output.getNumSamples();
        
        result.passed = bypassError < 0.001f;
        result.value = bypassError;
        result.details = "Bypass error: " + std::to_string(bypassError);
        
        return result;
    }
    
    TestResult testNaNInfDenormal(EngineBase* engine) {
        TestResult result;
        result.testName = "NaN/Inf/Denormal";
        
        // Test with extreme signals
        std::vector<juce::AudioBuffer<float>> testSignals;
        
        // Very quiet signal
        juce::AudioBuffer<float> quiet(2, BLOCK_SIZE);
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            quiet.setSample(0, i, 1e-35f);
            quiet.setSample(1, i, 1e-35f);
        }
        testSignals.push_back(quiet);
        
        // Large signal
        juce::AudioBuffer<float> loud(2, BLOCK_SIZE);
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            loud.setSample(0, i, 10.0f);
            loud.setSample(1, i, 10.0f);
        }
        testSignals.push_back(loud);
        
        bool foundNaN = false, foundInf = false, foundDenorm = false;
        
        for (auto& signal : testSignals) {
            engine->reset();
            engine->process(signal);
            
            if (hasNaNOrInf(signal)) {
                foundNaN = foundInf = true;
            }
            if (hasDenormals(signal)) {
                foundDenorm = true;
            }
        }
        
        result.passed = !foundNaN && !foundInf && !foundDenorm;
        result.value = 0;
        result.details = "NaN: " + std::to_string(foundNaN) + 
                        ", Inf: " + std::to_string(foundInf) + 
                        ", Denorm: " + std::to_string(foundDenorm);
        
        return result;
    }
    
    TestResult testReset(EngineBase* engine) {
        TestResult result;
        result.testName = "Reset";
        
        // Process impulse
        juce::AudioBuffer<float> impulse(2, BLOCK_SIZE);
        impulse.clear();
        impulse.setSample(0, 10, 1.0f);
        impulse.setSample(1, 10, 1.0f);
        engine->process(impulse);
        
        // Reset and process silence
        engine->reset();
        juce::AudioBuffer<float> silence(2, BLOCK_SIZE);
        silence.clear();
        engine->process(silence);
        
        float residual = calculateRMS(silence);
        
        result.passed = residual < 1e-6f;
        result.value = residual;
        result.details = "Residual: " + std::to_string(residual);
        
        return result;
    }
    
    TestResult testBlockSizeInvariance(EngineBase* engine) {
        TestResult result;
        result.testName = "Block Invariance";
        
        // Generate test signal
        juce::AudioBuffer<float> input(2, SAMPLE_RATE);
        juce::Random rng;
        for (int i = 0; i < SAMPLE_RATE; ++i) {
            float sample = rng.nextFloat() * 2.0f - 1.0f;
            input.setSample(0, i, sample * 0.5f);
            input.setSample(1, i, sample * 0.5f);
        }
        
        // Process as single block
        auto output1 = input;
        engine->reset();
        engine->process(output1);
        
        // Process as multiple blocks
        auto output2 = input;
        engine->reset();
        int pos = 0;
        std::vector<int> blockSizes = {64, 128, 73, 256, 97};
        
        for (int blockSize : blockSizes) {
            if (pos >= output2.getNumSamples()) break;
            int samplesToProcess = std::min(blockSize, output2.getNumSamples() - pos);
            
            juce::AudioBuffer<float> block(2, samplesToProcess);
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < samplesToProcess; ++i) {
                    block.setSample(ch, i, output2.getSample(ch, pos + i));
                }
            }
            
            engine->process(block);
            
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < samplesToProcess; ++i) {
                    output2.setSample(ch, pos + i, block.getSample(ch, i));
                }
            }
            
            pos += samplesToProcess;
        }
        
        // Calculate difference
        float maxDiff = 0;
        for (int i = 0; i < output1.getNumSamples(); ++i) {
            maxDiff = std::max(maxDiff, std::abs(output1.getSample(0, i) - output2.getSample(0, i)));
        }
        
        result.passed = maxDiff < 0.001f;
        result.value = maxDiff;
        result.details = "Max diff: " + std::to_string(maxDiff);
        
        return result;
    }
    
    TestResult testCPU(EngineBase* engine) {
        TestResult result;
        result.testName = "CPU Usage";
        
        juce::AudioBuffer<float> buffer(2, SAMPLE_RATE);
        juce::Random rng;
        for (int i = 0; i < SAMPLE_RATE; ++i) {
            buffer.setSample(0, i, rng.nextFloat() * 2.0f - 1.0f);
            buffer.setSample(1, i, rng.nextFloat() * 2.0f - 1.0f);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        engine->process(buffer);
        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double> elapsed = end - start;
        double cpuPercent = (elapsed.count() / 1.0) * 100.0;
        
        result.passed = cpuPercent < 10.0;
        result.value = cpuPercent;
        result.details = std::to_string(cpuPercent) + "%";
        
        return result;
    }
    
public:
    void testEngine(int engineID, const std::string& engineName) {
        EngineReport report;
        report.engineID = engineID;
        report.engineName = engineName;
        report.allPassed = true;
        
        std::cout << "\n[" << engineID << "] Testing " << engineName << "...\n";
        
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            std::cout << "  ❌ Failed to create engine\n";
            report.allPassed = false;
            allReports.push_back(report);
            return;
        }
        
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // Run tests
        std::vector<TestResult> tests;
        tests.push_back(testBypassMix(engine.get(), engineID));
        tests.push_back(testNaNInfDenormal(engine.get()));
        tests.push_back(testReset(engine.get()));
        tests.push_back(testBlockSizeInvariance(engine.get()));
        tests.push_back(testCPU(engine.get()));
        
        // Print results
        for (const auto& test : tests) {
            std::cout << "  " << std::left << std::setw(20) << test.testName << ": ";
            if (test.passed) {
                std::cout << "✅ PASS";
            } else {
                std::cout << "❌ FAIL";
                report.allPassed = false;
            }
            std::cout << " (" << test.details << ")\n";
            
            report.results.push_back(test);
        }
        
        allReports.push_back(report);
    }
    
    void runAllTests() {
        std::cout << "==========================================\n";
        std::cout << "   COMPREHENSIVE ENGINE TEST SUITE\n";
        std::cout << "==========================================\n";
        
        // Test all 57 engines (0-56) - CORRECT FACTORY MAPPING
        const std::vector<std::pair<int, std::string>> engines = {
            // Special
            {0, "NoneEngine"},
            
            // DYNAMICS & COMPRESSION (1-6)
            {1, "VintageOptoCompressor_Platinum"},
            {2, "ClassicCompressor"},
            {3, "TransientShaper_Platinum"},
            {4, "NoiseGate_Platinum"},
            {5, "MasteringLimiter_Platinum"},
            {6, "DynamicEQ"},
            
            // FILTERS & EQ (7-14)
            {7, "ParametricEQ_Studio"},
            {8, "VintageConsoleEQ_Studio"},
            {9, "LadderFilter"},
            {10, "StateVariableFilter"},
            {11, "FormantFilter"},
            {12, "EnvelopeFilter"},
            {13, "CombResonator"},
            {14, "VocalFormantFilter"},
            
            // DISTORTION & SATURATION (15-22)
            {15, "VintageTubePreamp_Studio"},
            {16, "WaveFolder"},
            {17, "HarmonicExciter_Platinum"},
            {18, "BitCrusher"},
            {19, "MultibandSaturator"},
            {20, "MuffFuzz"},
            {21, "RodentDistortion"},
            {22, "KStyleOverdrive"},
            
            // MODULATION (23-33)
            {23, "StereoChorus"},
            {24, "ResonantChorus_Platinum"},
            {25, "AnalogPhaser"},
            {26, "PlatinumRingModulator"},
            {27, "FrequencyShifter"},
            {28, "HarmonicTremolo"},
            {29, "ClassicTremolo"},
            {30, "RotarySpeaker_Platinum"},
            {31, "PitchShifter"},
            {32, "DetuneDoubler"},
            {33, "IntelligentHarmonizer"},
            
            // REVERB & DELAY (34-43)
            {34, "TapeEcho"},
            {35, "DigitalDelay"},
            {36, "MagneticDrumEcho"},
            {37, "BucketBrigadeDelay"},
            {38, "BufferRepeat_Platinum"},
            {39, "PlateReverb"},
            {40, "SpringReverb_Platinum"},
            {41, "ConvolutionReverb"},
            {42, "ShimmerReverb"},
            {43, "GatedReverb"},
            
            // SPATIAL & SPECIAL (44-52)
            {44, "StereoWidener"},
            {45, "StereoImager"},
            {46, "DimensionExpander"},
            {47, "SpectralFreeze"},
            {48, "SpectralGate_Platinum"},
            {49, "PhasedVocoder"},
            {50, "GranularCloud"},
            {51, "ChaosGenerator_Platinum"},
            {52, "FeedbackNetwork"},
            
            // UTILITY (53-56)
            {53, "MidSideProcessor_Platinum"},
            {54, "GainUtility_Platinum"},
            {55, "MonoMaker_Platinum"},
            {56, "PhaseAlign_Platinum"}
        };
        
        int totalPassed = 0;
        int totalFailed = 0;
        
        for (const auto& [id, name] : engines) {
            testEngine(id, name);
            
            if (allReports.back().allPassed) {
                totalPassed++;
            } else {
                totalFailed++;
            }
        }
        
        // Summary
        std::cout << "\n==========================================\n";
        std::cout << "              SUMMARY\n";
        std::cout << "==========================================\n";
        std::cout << "Total Engines: 57\n";
        std::cout << "Passed: " << totalPassed << " (" 
                 << std::fixed << std::setprecision(1) 
                 << (totalPassed * 100.0 / 57) << "%)\n";
        std::cout << "Failed: " << totalFailed << " (" 
                 << (totalFailed * 100.0 / 57) << "%)\n";
        
        if (totalFailed > 0) {
            std::cout << "\nFailed Engines:\n";
            for (const auto& report : allReports) {
                if (!report.allPassed) {
                    std::cout << "  - " << report.engineName << " (ID: " << report.engineID << ")\n";
                    for (const auto& test : report.results) {
                        if (!test.passed) {
                            std::cout << "      " << test.testName << ": " << test.details << "\n";
                        }
                    }
                }
            }
        }
        
        std::cout << "\n";
        if (totalPassed == 56) {
            std::cout << "🎉 SUCCESS: All engines passed comprehensive testing!\n";
        } else if (totalPassed >= 50) {
            std::cout << "✅ GOOD: Most engines passed (" << totalPassed << "/56)\n";
        } else {
            std::cout << "⚠️  WARNING: Significant number of engines need attention\n";
        }
        
        // Generate CSV report
        generateCSVReport();
    }
    
    void generateCSVReport() {
        std::ofstream csv("engine_test_results.csv");
        csv << "Engine ID,Engine Name,Bypass/Mix,NaN/Inf/Denormal,Reset,Block Invariance,CPU Usage,Overall\n";
        
        for (const auto& report : allReports) {
            csv << report.engineID << "," << report.engineName;
            
            for (const auto& test : report.results) {
                csv << "," << (test.passed ? "PASS" : "FAIL");
            }
            
            csv << "," << (report.allPassed ? "PASS" : "FAIL") << "\n";
        }
        
        csv.close();
        std::cout << "\nTest results saved to: engine_test_results.csv\n";
    }
};

int main() {
    ComprehensiveEngineTest tester;
    tester.runAllTests();
    return 0;
}