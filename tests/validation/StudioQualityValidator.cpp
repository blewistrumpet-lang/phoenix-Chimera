// Studio Quality Validator - Integrates with existing test harness
// Tests all engines against professional audio standards

#include "JuceHeader.h"
#include "EngineFactory.h"
#include "EngineBase.h"
#include "DspEngineUtilities.h"
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>
#include <map>
#include <chrono>

class StudioQualityValidator {
public:
    struct ValidationResult {
        std::string engineName;
        bool passed = true;
        
        // Critical Safety Checks
        bool hasDenormalProtection = false;
        bool hasNaNInfProtection = false;
        bool hasDCBlocking = false;
        bool hasBufferSafety = false;
        
        // Quality Metrics
        double noiseFloorDb = 0.0;
        double thd = 0.0;  // Total harmonic distortion
        double dcOffset = 0.0;
        int clicksAndPops = 0;
        int denormalCount = 0;
        
        // Performance
        double cpuPercent = 0.0;
        bool canHandle48k = true;
        bool canHandle96k = true;
        
        std::vector<std::string> issues;
        std::vector<std::string> recommendations;
    };
    
    ValidationResult validateEngine(std::unique_ptr<EngineBase>& engine, const std::string& name) {
        ValidationResult result;
        result.engineName = name;
        
        std::cout << "\nValidating: " << name << "\n";
        std::cout << "------------------------\n";
        
        // Prepare engine
        engine->prepareToPlay(48000.0, 512);
        
        // Run validation tests
        validateSafety(engine.get(), result);
        validateQuality(engine.get(), result);
        validatePerformance(engine.get(), result);
        validateStability(engine.get(), result);
        
        // Generate recommendations
        generateRecommendations(result);
        
        // Determine overall pass/fail
        result.passed = result.hasDenormalProtection && 
                       result.hasNaNInfProtection &&
                       result.noiseFloorDb < -60.0 &&
                       result.dcOffset < 0.001 &&
                       result.clicksAndPops == 0 &&
                       result.cpuPercent < 30.0;
        
        printResult(result);
        
        return result;
    }
    
private:
    void validateSafety(EngineBase* engine, ValidationResult& result) {
        std::cout << "  Safety checks...\n";
        
        // Test 1: Denormal handling
        juce::AudioBuffer<float> denormalBuffer(2, 512);
        for (int ch = 0; ch < 2; ++ch) {
            float* data = denormalBuffer.getWritePointer(ch);
            for (int i = 0; i < 512; ++i) {
                data[i] = 1e-35f;  // Denormal value
            }
        }
        
        auto startTime = std::chrono::high_resolution_clock::now();
        engine->process(denormalBuffer);
        auto endTime = std::chrono::high_resolution_clock::now();
        
        // Check if denormals were handled efficiently
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
        if (duration.count() < 5000) {  // Should process quickly if denormals are handled
            result.hasDenormalProtection = true;
        }
        
        // Count remaining denormals
        for (int ch = 0; ch < 2; ++ch) {
            const float* data = denormalBuffer.getReadPointer(ch);
            for (int i = 0; i < 512; ++i) {
                if (data[i] != 0.0f && std::abs(data[i]) < 1e-30f) {
                    result.denormalCount++;
                }
            }
        }
        
        // Test 2: NaN/Inf handling
        juce::AudioBuffer<float> nanBuffer(2, 512);
        nanBuffer.getWritePointer(0)[0] = std::numeric_limits<float>::quiet_NaN();
        nanBuffer.getWritePointer(0)[1] = std::numeric_limits<float>::infinity();
        
        engine->reset();
        engine->process(nanBuffer);
        
        bool hasNaN = false, hasInf = false;
        for (int ch = 0; ch < 2; ++ch) {
            const float* data = nanBuffer.getReadPointer(ch);
            for (int i = 0; i < 512; ++i) {
                if (std::isnan(data[i])) hasNaN = true;
                if (std::isinf(data[i])) hasInf = true;
            }
        }
        
        result.hasNaNInfProtection = !hasNaN && !hasInf;
        
        // Test 3: DC offset handling
        juce::AudioBuffer<float> dcBuffer(2, 4096);
        for (int ch = 0; ch < 2; ++ch) {
            dcBuffer.addFrom(ch, 0, dcBuffer, ch, 0, 4096, 0.1f);  // Add DC
        }
        
        engine->reset();
        
        // Process multiple blocks
        for (int block = 0; block < 8; ++block) {
            juce::AudioBuffer<float> blockBuffer(2, 512);
            blockBuffer.copyFrom(0, 0, dcBuffer, 0, block * 512, 512);
            blockBuffer.copyFrom(1, 0, dcBuffer, 1, block * 512, 512);
            engine->process(blockBuffer);
            
            // Measure DC
            for (int ch = 0; ch < 2; ++ch) {
                float sum = 0.0f;
                const float* data = blockBuffer.getReadPointer(ch);
                for (int i = 0; i < 512; ++i) {
                    sum += data[i];
                }
                result.dcOffset = std::max(result.dcOffset, std::abs(sum / 512.0));
            }
        }
        
        result.hasDCBlocking = result.dcOffset < 0.001;
        
        // Test 4: Buffer overrun safety
        try {
            juce::AudioBuffer<float> hugeBuffer(2, 16384);
            engine->process(hugeBuffer);
            
            juce::AudioBuffer<float> tinyBuffer(2, 1);
            engine->process(tinyBuffer);
            
            result.hasBufferSafety = true;
        } catch (...) {
            result.hasBufferSafety = false;
            result.issues.push_back("Buffer size handling issues");
        }
    }
    
    void validateQuality(EngineBase* engine, ValidationResult& result) {
        std::cout << "  Quality metrics...\n";
        
        // Test noise floor
        juce::AudioBuffer<float> silentBuffer(2, 4096);
        silentBuffer.clear();
        
        engine->reset();
        engine->process(silentBuffer);
        
        float maxNoise = 0.0f;
        for (int ch = 0; ch < 2; ++ch) {
            float rms = silentBuffer.getRMSLevel(ch, 0, 4096);
            maxNoise = std::max(maxNoise, rms);
        }
        
        result.noiseFloorDb = 20.0f * std::log10(std::max(1e-10f, maxNoise));
        
        // Test for clicks and pops with parameter changes
        juce::AudioBuffer<float> testBuffer(2, 512);
        generateSine(testBuffer, 440.0f, 0.5f, 48000.0);
        
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        
        // Rapidly change parameters and check for discontinuities
        float lastSample[2] = {0.0f, 0.0f};
        
        for (int change = 0; change < 10; ++change) {
            // Toggle all parameters
            for (int i = 0; i < numParams; ++i) {
                params[i] = (change % 2) ? 0.0f : 1.0f;
            }
            engine->updateParameters(params);
            
            engine->process(testBuffer);
            
            // Check for large jumps (clicks)
            for (int ch = 0; ch < 2; ++ch) {
                const float* data = testBuffer.getReadPointer(ch);
                if (std::abs(data[0] - lastSample[ch]) > 0.5f) {
                    result.clicksAndPops++;
                }
                lastSample[ch] = data[511];
            }
        }
        
        // Simplified THD test
        testBuffer.clear();
        generateSine(testBuffer, 1000.0f, 0.7f, 48000.0);
        
        engine->reset();
        engine->process(testBuffer);
        
        // Very simplified THD calculation (real would use FFT)
        float fundamental = 0.0f;
        float harmonics = 0.0f;
        
        for (int ch = 0; ch < 2; ++ch) {
            float rms = testBuffer.getRMSLevel(ch, 0, 512);
            fundamental += rms;
        }
        
        // Estimate distortion by comparing to expected sine RMS
        float expectedRMS = 0.7f * 0.707f;  // Amplitude * RMS factor for sine
        result.thd = std::abs(fundamental / 2.0f - expectedRMS) / expectedRMS;
    }
    
    void validatePerformance(EngineBase* engine, ValidationResult& result) {
        std::cout << "  Performance metrics...\n";
        
        juce::AudioBuffer<float> perfBuffer(2, 512);
        generateNoise(perfBuffer, 0.5f);
        
        // Test at 48kHz
        engine->prepareToPlay(48000.0, 512);
        
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; ++i) {
            engine->process(perfBuffer);
        }
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double realTime = (1000.0 * 512.0) / 48000.0;  // Seconds
        double cpuTime = duration.count() / 1000000.0;  // Seconds
        
        result.cpuPercent = (cpuTime / realTime) * 100.0;
        result.canHandle48k = result.cpuPercent < 50.0;
        
        // Test at 96kHz
        try {
            engine->prepareToPlay(96000.0, 512);
            
            start = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < 1000; ++i) {
                engine->process(perfBuffer);
            }
            end = std::chrono::high_resolution_clock::now();
            
            duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            realTime = (1000.0 * 512.0) / 96000.0;
            cpuTime = duration.count() / 1000000.0;
            
            double cpu96k = (cpuTime / realTime) * 100.0;
            result.canHandle96k = cpu96k < 70.0;
        } catch (...) {
            result.canHandle96k = false;
        }
    }
    
    void validateStability(EngineBase* engine, ValidationResult& result) {
        std::cout << "  Stability tests...\n";
        
        // Test with extreme signals
        juce::AudioBuffer<float> extremeBuffer(2, 512);
        
        // Square wave at Nyquist
        for (int ch = 0; ch < 2; ++ch) {
            float* data = extremeBuffer.getWritePointer(ch);
            for (int i = 0; i < 512; ++i) {
                data[i] = (i % 2) ? 0.99f : -0.99f;
            }
        }
        
        try {
            engine->reset();
            engine->process(extremeBuffer);
            
            // Check output is bounded
            float maxLevel = extremeBuffer.getMagnitude(0, 512);
            if (maxLevel > 2.0f) {
                result.issues.push_back("Unbounded output with extreme input");
            }
        } catch (...) {
            result.issues.push_back("Crash with extreme input");
            result.passed = false;
        }
        
        // Test mono compatibility
        try {
            juce::AudioBuffer<float> monoBuffer(1, 512);
            generateSine(monoBuffer, 440.0f, 0.5f, 48000.0);
            engine->process(monoBuffer);
        } catch (...) {
            result.issues.push_back("Mono processing not supported");
        }
    }
    
    void generateRecommendations(ValidationResult& result) {
        if (!result.hasDenormalProtection) {
            result.recommendations.push_back("Add DenormalGuard to process()");
        }
        
        if (!result.hasNaNInfProtection) {
            result.recommendations.push_back("Add scrubBuffer() at end of process()");
        }
        
        if (!result.hasDCBlocking && result.dcOffset > 0.001) {
            result.recommendations.push_back("Add DCBlocker for each channel");
        }
        
        if (result.noiseFloorDb > -60.0) {
            result.recommendations.push_back("Reduce noise floor (currently " + 
                std::to_string(result.noiseFloorDb) + " dB)");
        }
        
        if (result.clicksAndPops > 0) {
            result.recommendations.push_back("Implement parameter smoothing");
        }
        
        if (result.cpuPercent > 30.0) {
            result.recommendations.push_back("Optimize processing (currently " + 
                std::to_string(result.cpuPercent) + "% CPU)");
        }
        
        if (result.denormalCount > 0) {
            result.recommendations.push_back("Flush denormals (" + 
                std::to_string(result.denormalCount) + " found)");
        }
    }
    
    void printResult(const ValidationResult& result) {
        std::cout << "\n  RESULT: " << (result.passed ? "✅ PASSED" : "❌ FAILED") << "\n";
        
        std::cout << "\n  Safety:\n";
        std::cout << "    Denormal Protection: " << (result.hasDenormalProtection ? "✓" : "✗") << "\n";
        std::cout << "    NaN/Inf Protection: " << (result.hasNaNInfProtection ? "✓" : "✗") << "\n";
        std::cout << "    DC Blocking: " << (result.hasDCBlocking ? "✓" : "✗") << "\n";
        std::cout << "    Buffer Safety: " << (result.hasBufferSafety ? "✓" : "✗") << "\n";
        
        std::cout << "\n  Quality:\n";
        std::cout << "    Noise Floor: " << result.noiseFloorDb << " dB\n";
        std::cout << "    DC Offset: " << result.dcOffset << "\n";
        std::cout << "    THD: " << (result.thd * 100.0) << "%\n";
        std::cout << "    Clicks/Pops: " << result.clicksAndPops << "\n";
        
        std::cout << "\n  Performance:\n";
        std::cout << "    CPU Usage: " << result.cpuPercent << "%\n";
        std::cout << "    48kHz: " << (result.canHandle48k ? "✓" : "✗") << "\n";
        std::cout << "    96kHz: " << (result.canHandle96k ? "✓" : "✗") << "\n";
        
        if (!result.issues.empty()) {
            std::cout << "\n  Issues:\n";
            for (const auto& issue : result.issues) {
                std::cout << "    - " << issue << "\n";
            }
        }
        
        if (!result.recommendations.empty()) {
            std::cout << "\n  Recommendations:\n";
            for (const auto& rec : result.recommendations) {
                std::cout << "    - " << rec << "\n";
            }
        }
        
        std::cout << "\n";
    }
    
    // Helper functions
    void generateSine(juce::AudioBuffer<float>& buffer, float freq, float amp, double sampleRate) {
        const double phaseInc = 2.0 * M_PI * freq / sampleRate;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = amp * std::sin(i * phaseInc);
            }
        }
    }
    
    void generateNoise(juce::AudioBuffer<float>& buffer, float amp) {
        juce::Random random;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = amp * (2.0f * random.nextFloat() - 1.0f);
            }
        }
    }
};

// Main test runner
int main() {
    std::cout << "=== STUDIO QUALITY VALIDATION ===\n";
    std::cout << "Testing all engines for professional standards\n\n";
    
    EngineFactory factory;
    StudioQualityValidator validator;
    
    std::vector<ValidationResult> results;
    int passCount = 0;
    int totalEngines = 0;
    
    // Test critical engines first
    std::vector<int> criticalEngines = {
        1,  // Vintage Opto
        2,  // Classic Compressor
        3,  // Noise Gate
        4,  // Vintage Tube Preamp
        5,  // K-Style Overdrive
        8,  // Vintage Console EQ
        9,  // Parametric EQ
        14, // Stereo Chorus
        25, // Plate Reverb
        26, // Spring Reverb
    };
    
    std::cout << "Testing Critical Engines:\n";
    std::cout << "=========================\n";
    
    for (int engineId : criticalEngines) {
        auto engine = factory.createEngine(engineId);
        if (engine) {
            std::string name = engine->getName().toStdString();
            auto result = validator.validateEngine(engine, name);
            results.push_back(result);
            
            if (result.passed) passCount++;
            totalEngines++;
        }
    }
    
    // Summary
    std::cout << "\n=== VALIDATION SUMMARY ===\n";
    std::cout << "Total Engines Tested: " << totalEngines << "\n";
    std::cout << "Passed: " << passCount << "\n";
    std::cout << "Failed: " << (totalEngines - passCount) << "\n";
    std::cout << "Pass Rate: " << (100.0 * passCount / totalEngines) << "%\n\n";
    
    // List failed engines
    std::cout << "Failed Engines:\n";
    for (const auto& result : results) {
        if (!result.passed) {
            std::cout << "  - " << result.engineName << "\n";
        }
    }
    
    // Critical recommendations
    std::cout << "\nCritical Actions Required:\n";
    std::cout << "1. Add DenormalGuard to all process() methods\n";
    std::cout << "2. Add scrubBuffer() at end of all process() methods\n";
    std::cout << "3. Implement parameter smoothing for zipper-free automation\n";
    std::cout << "4. Add DC blocking to dynamics and distortion engines\n";
    std::cout << "5. Ensure all engines have proper reset() implementation\n";
    
    return (passCount == totalEngines) ? 0 : 1;
}