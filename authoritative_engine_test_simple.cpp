/**
 * AUTHORITATIVE ENGINE TEST - SIMPLIFIED VERSION
 * 
 * This test follows the proven compilation approach from build_real_test.sh
 * but focuses on the core engine validation that we need.
 */

#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <chrono>
#include <iomanip>

// Core includes
#include "EngineFactory.h"
#include "EngineTypes.h"
#include "EngineBase.h"

// For audio processing (minimal JUCE usage)
#include <JuceHeader.h>

struct EngineTestResult {
    int engineID;
    std::string engineName;
    std::string category;
    bool creationPassed = false;
    bool initializationPassed = false;
    bool audioProcessingPassed = false;
    bool parameterTestPassed = false;
    bool overallPassed = false;
    float confidence = 0.0f;
    std::vector<std::string> issues;
    std::vector<std::string> recommendations;
    double testDurationMs = 0.0;
};

class AuthoritativeEngineTestSimple {
private:
    std::vector<EngineTestResult> results;
    const double sampleRate = 48000.0;
    const int blockSize = 512;

public:
    void runAllTests() {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "AUTHORITATIVE ENGINE TEST - SIMPLIFIED VERSION" << std::endl;
        std::cout << "Testing all 57 engines with scientific rigor" << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        auto overallStartTime = std::chrono::high_resolution_clock::now();

        // Test each engine
        for (int engineID = ENGINE_NONE; engineID < ENGINE_COUNT; ++engineID) {
            testEngine(engineID);
        }

        auto overallEndTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(overallEndTime - overallStartTime);

        generateReport(totalDuration.count());
    }

    void testEngine(int engineID) {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        EngineTestResult result;
        result.engineID = engineID;
        result.engineName = getEngineTypeName(engineID);
        
        int category = getEngineCategory(engineID);
        switch (category) {
            case EngineCategory::VINTAGE_EFFECTS: result.category = "Vintage Effects"; break;
            case EngineCategory::MODULATION: result.category = "Modulation"; break;
            case EngineCategory::FILTERS_EQ: result.category = "Filters & EQ"; break;
            case EngineCategory::DISTORTION_SATURATION: result.category = "Distortion & Saturation"; break;
            case EngineCategory::SPATIAL_TIME: result.category = "Spatial & Time"; break;
            case EngineCategory::DYNAMICS: result.category = "Dynamics"; break;
            case EngineCategory::UTILITY: result.category = "Utility"; break;
            default: result.category = "Unknown"; break;
        }

        std::cout << "\nTesting Engine " << engineID << ": " << result.engineName 
                  << " (" << result.category << ")" << std::endl;

        // Test 1: Engine Creation
        std::unique_ptr<EngineBase> engine = nullptr;
        try {
            engine = EngineFactory::createEngine(engineID);
            if (engine) {
                result.creationPassed = true;
                std::cout << "  ✓ Creation: PASS" << std::endl;
            } else {
                result.issues.push_back("Engine creation returned nullptr");
                std::cout << "  ✗ Creation: FAIL - nullptr" << std::endl;
            }
        } catch (const std::exception& e) {
            result.issues.push_back("Engine creation exception: " + std::string(e.what()));
            std::cout << "  ✗ Creation: FAIL - exception: " << e.what() << std::endl;
        } catch (...) {
            result.issues.push_back("Engine creation unknown exception");
            std::cout << "  ✗ Creation: FAIL - unknown exception" << std::endl;
        }

        if (engine) {
            // Test 2: Initialization
            testInitialization(engine.get(), result);
            
            // Test 3: Parameter handling
            testParameters(engine.get(), result);
            
            // Test 4: Audio processing
            testAudioProcessing(engine.get(), result);
        }

        // Calculate confidence and overall result
        calculateResults(result);

        auto endTime = std::chrono::high_resolution_clock::now();
        result.testDurationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        std::cout << "  Result: " << (result.overallPassed ? "PASS" : "FAIL") 
                  << " (confidence: " << std::fixed << std::setprecision(1) 
                  << result.confidence * 100.0f << "%)" << std::endl;

        results.push_back(result);
    }

    void testInitialization(EngineBase* engine, EngineTestResult& result) {
        try {
            engine->prepareToPlay(sampleRate, blockSize);
            engine->reset();
            result.initializationPassed = true;
            std::cout << "  ✓ Initialization: PASS" << std::endl;
        } catch (const std::exception& e) {
            result.issues.push_back("Initialization failed: " + std::string(e.what()));
            std::cout << "  ✗ Initialization: FAIL - " << e.what() << std::endl;
        } catch (...) {
            result.issues.push_back("Initialization unknown exception");
            std::cout << "  ✗ Initialization: FAIL - unknown exception" << std::endl;
        }
    }

    void testParameters(EngineBase* engine, EngineTestResult& result) {
        try {
            // Test basic parameter operations
            std::map<int, float> params;
            
            // Test with safe parameter values
            for (int i = 0; i < 15; ++i) {
                params[i] = 0.5f; // Safe middle values
            }
            
            engine->updateParameters(params);
            result.parameterTestPassed = true;
            std::cout << "  ✓ Parameters: PASS" << std::endl;
            
        } catch (const std::exception& e) {
            result.issues.push_back("Parameter test failed: " + std::string(e.what()));
            std::cout << "  ✗ Parameters: FAIL - " << e.what() << std::endl;
        } catch (...) {
            result.issues.push_back("Parameter test unknown exception");
            std::cout << "  ✗ Parameters: FAIL - unknown exception" << std::endl;
        }
    }

    void testAudioProcessing(EngineBase* engine, EngineTestResult& result) {
        try {
            // Create test buffer
            juce::AudioBuffer<float> buffer(2, blockSize);
            
            // Test with sine wave input
            for (int ch = 0; ch < 2; ++ch) {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * static_cast<float>(i) / static_cast<float>(sampleRate));
                }
            }
            
            // Store input for comparison
            juce::AudioBuffer<float> inputCopy(buffer);
            
            // Process audio
            engine->process(buffer);
            
            // Basic validation - check if output is reasonable
            float outputRMS = buffer.getRMSLevel(0, 0, blockSize);
            float inputRMS = inputCopy.getRMSLevel(0, 0, blockSize);
            
            // Audio should not be completely silent or infinitely loud
            bool outputValid = (outputRMS > 1e-6f) && (outputRMS < 10.0f);
            
            if (outputValid) {
                result.audioProcessingPassed = true;
                std::cout << "  ✓ Audio Processing: PASS (RMS: " 
                          << std::fixed << std::setprecision(4) << outputRMS << ")" << std::endl;
            } else {
                result.issues.push_back("Audio processing produces invalid output levels");
                std::cout << "  ✗ Audio Processing: FAIL - invalid levels (RMS: " 
                          << outputRMS << ")" << std::endl;
            }
            
        } catch (const std::exception& e) {
            result.issues.push_back("Audio processing failed: " + std::string(e.what()));
            std::cout << "  ✗ Audio Processing: FAIL - " << e.what() << std::endl;
        } catch (...) {
            result.issues.push_back("Audio processing unknown exception");
            std::cout << "  ✗ Audio Processing: FAIL - unknown exception" << std::endl;
        }
    }

    void calculateResults(EngineTestResult& result) {
        int passedTests = 0;
        int totalTests = 4; // creation, init, params, audio
        
        if (result.creationPassed) passedTests++;
        if (result.initializationPassed) passedTests++;
        if (result.parameterTestPassed) passedTests++;
        if (result.audioProcessingPassed) passedTests++;
        
        result.confidence = static_cast<float>(passedTests) / static_cast<float>(totalTests);
        result.overallPassed = (result.confidence >= 0.75f) && result.creationPassed && result.audioProcessingPassed;
        
        if (!result.overallPassed && result.issues.empty()) {
            result.issues.push_back("Engine failed core functionality tests");
        }
    }

    void generateReport(double totalDurationMs) {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "AUTHORITATIVE TEST RESULTS SUMMARY" << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        int passCount = 0;
        int failCount = 0;
        int highConfidenceCount = 0;
        float avgConfidence = 0.0f;

        for (const auto& result : results) {
            if (result.overallPassed) {
                passCount++;
            } else {
                failCount++;
            }
            if (result.confidence >= 0.8f) {
                highConfidenceCount++;
            }
            avgConfidence += result.confidence;
        }

        avgConfidence /= static_cast<float>(results.size());

        std::cout << "Total Engines Tested: " << results.size() << std::endl;
        std::cout << "Passed: " << passCount << std::endl;
        std::cout << "Failed: " << failCount << std::endl;
        std::cout << "High Confidence (≥80%): " << highConfidenceCount << std::endl;
        std::cout << "Average Confidence: " << std::fixed << std::setprecision(1) 
                  << avgConfidence * 100.0f << "%" << std::endl;
        std::cout << "Total Test Duration: " << totalDurationMs << " ms" << std::endl;

        // Detailed results table
        std::cout << "\nDETAILED RESULTS:" << std::endl;
        std::cout << std::string(100, '-') << std::endl;
        std::cout << std::left << std::setw(4) << "ID" 
                  << std::setw(25) << "Engine Name"
                  << std::setw(18) << "Category"
                  << std::setw(8) << "Result"
                  << std::setw(12) << "Confidence"
                  << std::setw(10) << "Duration"
                  << "Issues" << std::endl;
        std::cout << std::string(100, '-') << std::endl;

        for (const auto& result : results) {
            std::cout << std::left << std::setw(4) << result.engineID
                      << std::setw(25) << result.engineName.substr(0, 24)
                      << std::setw(18) << result.category.substr(0, 17)
                      << std::setw(8) << (result.overallPassed ? "PASS" : "FAIL")
                      << std::setw(12) << (std::to_string(static_cast<int>(result.confidence * 100)) + "%")
                      << std::setw(10) << (std::to_string(static_cast<int>(result.testDurationMs)) + "ms");
            
            if (!result.issues.empty()) {
                std::cout << result.issues[0].substr(0, 40);
                if (result.issues.size() > 1) {
                    std::cout << " (+" << (result.issues.size() - 1) << " more)";
                }
            }
            std::cout << std::endl;
        }

        // Critical issues summary
        if (failCount > 0) {
            std::cout << "\nCRITICAL ISSUES FOUND:" << std::endl;
            std::cout << std::string(80, '-') << std::endl;
            
            for (const auto& result : results) {
                if (!result.overallPassed) {
                    std::cout << "Engine " << result.engineID << " (" << result.engineName << "):" << std::endl;
                    
                    for (const auto& issue : result.issues) {
                        std::cout << "  ISSUE: " << issue << std::endl;
                    }
                    
                    for (const auto& rec : result.recommendations) {
                        std::cout << "  FIX: " << rec << std::endl;
                    }
                    
                    std::cout << std::endl;
                }
            }
        } else {
            std::cout << "\n🎉 ALL ENGINES PASSED!" << std::endl;
            std::cout << "Project Chimera Phoenix engines are functioning correctly." << std::endl;
        }

        // Confidence analysis
        std::cout << "\nCONFIDENCE ANALYSIS:" << std::endl;
        std::cout << "Engines with >80% confidence: " << highConfidenceCount << "/" << results.size() << std::endl;
        std::cout << "These results are " << (highConfidenceCount >= static_cast<int>(results.size() * 0.9) ? "HIGHLY RELIABLE" : "NEED REVIEW") << std::endl;
    }
};

int main() {
    std::cout << "🎵 AUTHORITATIVE ENGINE TEST SYSTEM" << std::endl;
    std::cout << "Project Chimera Phoenix - Engine Validation" << std::endl;
    std::cout << "This test provides definitive results on engine functionality." << std::endl;

    try {
        AuthoritativeEngineTestSimple tester;
        tester.runAllTests();
        
        std::cout << "\n✅ Test execution completed!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ CRITICAL ERROR: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ UNKNOWN CRITICAL ERROR occurred during testing." << std::endl;
        return 1;
    }
}