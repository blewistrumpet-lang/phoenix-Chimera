// Comprehensive Studio Quality Test Harness
// Tests all DSP engines for professional audio quality standards

#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <fstream>
#include <chrono>
#include <map>
#include <iomanip>

class StudioQualityTestHarness {
public:
    struct TestResult {
        bool passed = true;
        std::vector<std::string> failures;
        std::vector<std::string> warnings;
        
        // Performance metrics
        double cpuUsagePercent = 0.0;
        double maxLatencyMs = 0.0;
        
        // Audio quality metrics
        double thd = 0.0;  // Total harmonic distortion
        double snr = 0.0;  // Signal-to-noise ratio
        double dcOffset = 0.0;
        int denormalCount = 0;
        int nanInfCount = 0;
        int clipCount = 0;
        
        // Stability metrics
        bool crashOnExtreme = false;
        bool memoryLeak = false;
        bool threadSafe = true;
    };
    
    StudioQualityTestHarness() {
        factory = std::make_unique<EngineFactory>();
    }
    
    void runAllTests() {
        std::cout << "\n=== STUDIO QUALITY TEST HARNESS ===\n\n";
        
        // Get all available engines
        auto engineNames = getEngineList();
        
        int totalEngines = engineNames.size();
        int passedEngines = 0;
        int criticalFailures = 0;
        
        std::ofstream report("studio_test_results.md");
        report << "# Studio Quality Test Results\n\n";
        report << "Test Date: " << getCurrentTimestamp() << "\n\n";
        
        for (const auto& engineName : engineNames) {
            std::cout << "Testing: " << engineName << "..." << std::flush;
            
            TestResult result = testEngine(engineName);
            
            if (result.passed) {
                passedEngines++;
                std::cout << " ✅ PASSED\n";
            } else {
                if (!result.failures.empty()) {
                    criticalFailures++;
                    std::cout << " ❌ FAILED\n";
                } else {
                    std::cout << " ⚠️  WARNINGS\n";
                }
            }
            
            // Write detailed results to report
            writeEngineReport(report, engineName, result);
            
            results[engineName] = result;
        }
        
        // Summary
        std::cout << "\n=== TEST SUMMARY ===\n";
        std::cout << "Total Engines: " << totalEngines << "\n";
        std::cout << "Passed: " << passedEngines << "\n";
        std::cout << "Critical Failures: " << criticalFailures << "\n";
        std::cout << "Pass Rate: " << (100.0 * passedEngines / totalEngines) << "%\n";
        
        // Write summary to report
        writeSummary(report, totalEngines, passedEngines, criticalFailures);
        
        report.close();
        std::cout << "\nDetailed report: studio_test_results.md\n";
    }
    
private:
    std::unique_ptr<EngineFactory> factory;
    std::map<std::string, TestResult> results;
    
    static constexpr double SAMPLE_RATE = 48000.0;
    static constexpr int BLOCK_SIZE = 512;
    static constexpr int TEST_DURATION_BLOCKS = 100;
    
    std::vector<std::string> getEngineList() {
        // This should match your actual engine list
        return {
            "Bypass", "Vintage Opto", "Classic Compressor Pro",
            "Noise Gate", "Vintage Tube Preamp", "K-Style Overdrive",
            "Rodent Distortion", "Muff Fuzz", "Vintage Console EQ",
            "Parametric EQ", "State Variable Filter", "Ladder Filter",
            "Envelope Filter", "Formant Filter", "Vocal Formant",
            "Stereo Chorus", "Resonant Chorus", "Analog Phaser",
            "Classic Tremolo", "Harmonic Tremolo", "Rotary Speaker",
            "Digital Delay", "Tape Echo", "Bucket Brigade Delay",
            "Magnetic Drum Echo", "Plate Reverb", "Spring Reverb",
            "Gated Reverb", "Shimmer Reverb", "Convolution Reverb",
            "Harmonic Exciter", "Dimension Expander", "Stereo Widener",
            "Stereo Imager", "Bit Crusher", "Analog Ring Mod",
            "Frequency Shifter", "Pitch Shifter", "Intelligent Harmonizer",
            "Granular Cloud", "Spectral Freeze", "Spectral Gate",
            "Phased Vocoder", "Buffer Repeat", "Chaos Generator",
            "Wave Folder", "Comb Resonator", "Feedback Network",
            "Multiband Saturator", "Dynamic EQ", "Detune Doubler"
            // Add Platinum engines if needed
        };
    }
    
    TestResult testEngine(const std::string& engineName) {
        TestResult result;
        
        try {
            // Create engine instance
            auto engine = createEngine(engineName);
            if (!engine) {
                result.passed = false;
                result.failures.push_back("Failed to create engine instance");
                return result;
            }
            
            // Prepare engine
            engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
            
            // Run test suites
            testBasicFunctionality(engine.get(), result);
            testAudioQuality(engine.get(), result);
            testStability(engine.get(), result);
            testPerformance(engine.get(), result);
            testParameterHandling(engine.get(), result);
            testEdgeCases(engine.get(), result);
            
            // Determine overall pass/fail
            result.passed = result.failures.empty();
            
        } catch (const std::exception& e) {
            result.passed = false;
            result.failures.push_back("Exception: " + std::string(e.what()));
            result.crashOnExtreme = true;
        } catch (...) {
            result.passed = false;
            result.failures.push_back("Unknown exception occurred");
            result.crashOnExtreme = true;
        }
        
        return result;
    }
    
    void testBasicFunctionality(EngineBase* engine, TestResult& result) {
        // Test 1: Process silence without issues
        juce::AudioBuffer<float> silentBuffer(2, BLOCK_SIZE);
        silentBuffer.clear();
        
        engine->process(silentBuffer);
        
        // Check for NaN/Inf in output
        for (int ch = 0; ch < 2; ++ch) {
            const float* data = silentBuffer.getReadPointer(ch);
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                if (std::isnan(data[i]) || std::isinf(data[i])) {
                    result.failures.push_back("NaN/Inf detected on silence");
                    result.nanInfCount++;
                    break;
                }
            }
        }
        
        // Test 2: Reset functionality
        engine->reset();
        
        // Test 3: Process simple sine wave
        juce::AudioBuffer<float> sineBuffer(2, BLOCK_SIZE);
        generateSineWave(sineBuffer, 1000.0f, 0.5f);
        
        engine->process(sineBuffer);
        
        // Check output is reasonable
        float maxLevel = sineBuffer.getMagnitude(0, BLOCK_SIZE);
        if (maxLevel > 10.0f) {
            result.failures.push_back("Output level unreasonably high: " + std::to_string(maxLevel));
        }
    }
    
    void testAudioQuality(EngineBase* engine, TestResult& result) {
        // Test for DC offset
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE * 10);
        generateSineWave(testBuffer, 100.0f, 0.7f);
        
        engine->reset();
        
        // Process multiple blocks
        for (int block = 0; block < 10; ++block) {
            juce::AudioBuffer<float> blockBuffer(2, BLOCK_SIZE);
            for (int ch = 0; ch < 2; ++ch) {
                blockBuffer.copyFrom(ch, 0, testBuffer, ch, block * BLOCK_SIZE, BLOCK_SIZE);
            }
            engine->process(blockBuffer);
            
            // Measure DC offset
            for (int ch = 0; ch < 2; ++ch) {
                float sum = 0.0f;
                const float* data = blockBuffer.getReadPointer(ch);
                for (int i = 0; i < BLOCK_SIZE; ++i) {
                    sum += data[i];
                }
                float dcOffset = std::abs(sum / BLOCK_SIZE);
                if (dcOffset > 0.01f) {
                    result.dcOffset = std::max(result.dcOffset, (double)dcOffset);
                }
            }
        }
        
        if (result.dcOffset > 0.01) {
            result.warnings.push_back("DC offset detected: " + std::to_string(result.dcOffset));
        }
        
        // Test for aliasing (simplified)
        testAliasing(engine, result);
        
        // Test noise floor
        testNoiseFloor(engine, result);
    }
    
    void testStability(EngineBase* engine, TestResult& result) {
        // Test with extreme inputs
        juce::AudioBuffer<float> extremeBuffer(2, BLOCK_SIZE);
        
        // Test 1: Very loud signal
        extremeBuffer.clear();
        for (int ch = 0; ch < 2; ++ch) {
            float* data = extremeBuffer.getWritePointer(ch);
            for (int i = 0; i < BLOCK_SIZE; ++i) {
                data[i] = (i % 2) ? 0.99f : -0.99f;  // Square wave at Nyquist
            }
        }
        
        engine->reset();
        engine->process(extremeBuffer);
        
        // Check for crashes or NaN/Inf
        checkForAnomalies(extremeBuffer, result);
        
        // Test 2: Impulse response
        extremeBuffer.clear();
        extremeBuffer.setSample(0, 0, 1.0f);  // Impulse
        
        engine->reset();
        engine->process(extremeBuffer);
        
        checkForAnomalies(extremeBuffer, result);
        
        // Test 3: Rapid parameter changes
        testRapidParameterChanges(engine, result);
    }
    
    void testPerformance(EngineBase* engine, TestResult& result) {
        juce::AudioBuffer<float> perfBuffer(2, BLOCK_SIZE);
        generateWhiteNoise(perfBuffer, 0.5f);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Process many blocks
        for (int i = 0; i < 1000; ++i) {
            engine->process(perfBuffer);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Calculate CPU usage
        double totalSamples = 1000.0 * BLOCK_SIZE;
        double totalTime = totalSamples / SAMPLE_RATE;  // Time in seconds
        double processingTime = duration.count() / 1000000.0;  // Convert to seconds
        
        result.cpuUsagePercent = (processingTime / totalTime) * 100.0;
        
        if (result.cpuUsagePercent > 50.0) {
            result.warnings.push_back("High CPU usage: " + std::to_string(result.cpuUsagePercent) + "%");
        }
        
        // Estimate latency (simplified)
        result.maxLatencyMs = (BLOCK_SIZE / SAMPLE_RATE) * 1000.0;
    }
    
    void testParameterHandling(EngineBase* engine, TestResult& result) {
        // Test all parameters with extreme values
        std::map<int, float> params;
        
        int numParams = engine->getNumParameters();
        
        // Test minimum values
        for (int i = 0; i < numParams; ++i) {
            params[i] = 0.0f;
        }
        engine->updateParameters(params);
        
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
        generateSineWave(testBuffer, 440.0f, 0.5f);
        engine->process(testBuffer);
        checkForAnomalies(testBuffer, result);
        
        // Test maximum values
        for (int i = 0; i < numParams; ++i) {
            params[i] = 1.0f;
        }
        engine->updateParameters(params);
        engine->process(testBuffer);
        checkForAnomalies(testBuffer, result);
        
        // Test rapid changes
        for (int change = 0; change < 10; ++change) {
            for (int i = 0; i < numParams; ++i) {
                params[i] = (change % 2) ? 0.0f : 1.0f;
            }
            engine->updateParameters(params);
            engine->process(testBuffer);
        }
    }
    
    void testEdgeCases(EngineBase* engine, TestResult& result) {
        // Test with single channel
        juce::AudioBuffer<float> monoBuffer(1, BLOCK_SIZE);
        generateSineWave(monoBuffer, 220.0f, 0.3f);
        
        try {
            engine->process(monoBuffer);
        } catch (...) {
            result.warnings.push_back("Failed to process mono buffer");
        }
        
        // Test with very small buffer
        juce::AudioBuffer<float> tinyBuffer(2, 1);
        tinyBuffer.clear();
        
        try {
            engine->process(tinyBuffer);
        } catch (...) {
            result.warnings.push_back("Failed to process single-sample buffer");
        }
        
        // Test with large buffer
        juce::AudioBuffer<float> largeBuffer(2, 4096);
        generateWhiteNoise(largeBuffer, 0.1f);
        
        try {
            engine->process(largeBuffer);
            checkForAnomalies(largeBuffer, result);
        } catch (...) {
            result.failures.push_back("Failed to process large buffer");
        }
    }
    
    // Helper functions
    void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude) {
        const int numSamples = buffer.getNumSamples();
        const double phaseInc = 2.0 * M_PI * frequency / SAMPLE_RATE;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = amplitude * std::sin(i * phaseInc);
            }
        }
    }
    
    void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude) {
        juce::Random random;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = amplitude * (2.0f * random.nextFloat() - 1.0f);
            }
        }
    }
    
    void checkForAnomalies(const juce::AudioBuffer<float>& buffer, TestResult& result) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (std::isnan(data[i])) {
                    result.nanInfCount++;
                    result.failures.push_back("NaN detected in output");
                    return;
                }
                if (std::isinf(data[i])) {
                    result.nanInfCount++;
                    result.failures.push_back("Inf detected in output");
                    return;
                }
                if (std::abs(data[i]) > 1.5f) {
                    result.clipCount++;
                }
                
                // Check for denormals
                if (data[i] != 0.0f && std::abs(data[i]) < 1e-30f) {
                    result.denormalCount++;
                }
            }
        }
        
        if (result.clipCount > 10) {
            result.warnings.push_back("Excessive clipping detected");
        }
        if (result.denormalCount > 0) {
            result.warnings.push_back("Denormal values detected");
        }
    }
    
    void testAliasing(EngineBase* engine, TestResult& result) {
        // Generate high-frequency test signal
        juce::AudioBuffer<float> hfBuffer(2, BLOCK_SIZE);
        float testFreq = SAMPLE_RATE * 0.45f;  // Near Nyquist
        generateSineWave(hfBuffer, testFreq, 0.5f);
        
        engine->reset();
        engine->process(hfBuffer);
        
        // Simple aliasing check - look for unexpected low frequencies
        // (This is simplified - real aliasing detection would use FFT)
        float rms = hfBuffer.getRMSLevel(0, 0, BLOCK_SIZE);
        if (rms > 1.0f) {
            result.warnings.push_back("Possible aliasing detected");
        }
    }
    
    void testNoiseFloor(EngineBase* engine, TestResult& result) {
        // Process silence and measure noise floor
        juce::AudioBuffer<float> silentBuffer(2, BLOCK_SIZE * 10);
        silentBuffer.clear();
        
        engine->reset();
        
        float maxNoise = 0.0f;
        for (int block = 0; block < 10; ++block) {
            juce::AudioBuffer<float> blockBuffer(2, BLOCK_SIZE);
            blockBuffer.clear();
            engine->process(blockBuffer);
            
            float rms = blockBuffer.getRMSLevel(0, 0, BLOCK_SIZE);
            maxNoise = std::max(maxNoise, rms);
        }
        
        // Convert to dB
        float noiseFloorDb = 20.0f * std::log10(std::max(1e-10f, maxNoise));
        result.snr = -noiseFloorDb;  // Simplified SNR
        
        if (noiseFloorDb > -60.0f) {
            result.warnings.push_back("High noise floor: " + std::to_string(noiseFloorDb) + " dB");
        }
    }
    
    void testRapidParameterChanges(EngineBase* engine, TestResult& result) {
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
        generateSineWave(testBuffer, 440.0f, 0.5f);
        
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        
        // Rapidly change parameters
        for (int i = 0; i < 20; ++i) {
            for (int p = 0; p < numParams; ++p) {
                params[p] = (float)(i % 2);
            }
            engine->updateParameters(params);
            engine->process(testBuffer);
            
            // Check for clicks/pops (simplified)
            float peak = testBuffer.getMagnitude(0, BLOCK_SIZE);
            if (peak > 2.0f) {
                result.warnings.push_back("Possible click/pop on parameter change");
                break;
            }
        }
    }
    
    std::unique_ptr<EngineBase> createEngine(const std::string& name) {
        // Map engine names to factory indices
        // This is simplified - adjust based on your actual factory implementation
        static std::map<std::string, int> engineMap = {
            {"Bypass", 0},
            {"Vintage Opto", 1},
            {"Classic Compressor Pro", 2},
            // ... add all engines
        };
        
        auto it = engineMap.find(name);
        if (it != engineMap.end()) {
            return factory->createEngine(it->second);
        }
        
        // Try to find by partial match
        for (int i = 0; i < 60; ++i) {
            auto engine = factory->createEngine(i);
            if (engine && engine->getName().toStdString().find(name) != std::string::npos) {
                return engine;
            }
        }
        
        return nullptr;
    }
    
    void writeEngineReport(std::ofstream& report, const std::string& name, const TestResult& result) {
        report << "## " << name << "\n\n";
        
        if (result.passed) {
            report << "**Status**: ✅ PASSED\n\n";
        } else if (!result.failures.empty()) {
            report << "**Status**: ❌ FAILED\n\n";
        } else {
            report << "**Status**: ⚠️ WARNINGS\n\n";
        }
        
        // Performance metrics
        report << "### Performance\n";
        report << "- CPU Usage: " << std::fixed << std::setprecision(2) << result.cpuUsagePercent << "%\n";
        report << "- Latency: " << result.maxLatencyMs << " ms\n\n";
        
        // Audio quality
        report << "### Audio Quality\n";
        report << "- SNR: " << result.snr << " dB\n";
        report << "- DC Offset: " << result.dcOffset << "\n";
        report << "- Denormals: " << result.denormalCount << "\n";
        report << "- NaN/Inf: " << result.nanInfCount << "\n";
        report << "- Clipping: " << result.clipCount << "\n\n";
        
        // Issues
        if (!result.failures.empty()) {
            report << "### Failures\n";
            for (const auto& failure : result.failures) {
                report << "- " << failure << "\n";
            }
            report << "\n";
        }
        
        if (!result.warnings.empty()) {
            report << "### Warnings\n";
            for (const auto& warning : result.warnings) {
                report << "- " << warning << "\n";
            }
            report << "\n";
        }
        
        report << "---\n\n";
    }
    
    void writeSummary(std::ofstream& report, int total, int passed, int critical) {
        report << "## Summary\n\n";
        report << "- Total Engines Tested: " << total << "\n";
        report << "- Passed: " << passed << "\n";
        report << "- Critical Failures: " << critical << "\n";
        report << "- Pass Rate: " << std::fixed << std::setprecision(1) 
               << (100.0 * passed / total) << "%\n\n";
        
        // Priority recommendations
        report << "## Priority Fixes\n\n";
        
        std::vector<std::string> criticalEngines;
        for (const auto& [name, result] : results) {
            if (!result.failures.empty()) {
                criticalEngines.push_back(name);
            }
        }
        
        if (!criticalEngines.empty()) {
            report << "### Critical (Fix Immediately)\n";
            for (const auto& name : criticalEngines) {
                report << "1. " << name << "\n";
            }
        }
    }
    
    std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

int main() {
    StudioQualityTestHarness harness;
    harness.runAllTests();
    return 0;
}