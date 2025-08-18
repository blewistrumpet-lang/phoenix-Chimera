/**
 * REAL Chimera Phoenix Engine Test Harness
 * 
 * This actually tests the REAL engine implementations from the project
 * Not fake placeholder engines!
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <fstream>

// Include the ACTUAL engine headers from the project
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

// Test result structure
struct TestResult {
    std::string engineName;
    int engineId;
    bool creationSuccess = false;
    bool initSuccess = false;
    bool processSuccess = false;
    bool parameterTest = false;
    bool stabilityTest = false;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    float qualityScore = 0.0f;
    
    bool isPassing() const {
        return creationSuccess && initSuccess && processSuccess && errors.empty();
    }
    
    std::string getSummary() const {
        std::stringstream ss;
        ss << "Engine #" << std::setw(2) << engineId << " - ";
        
        if (!creationSuccess) {
            ss << "❌ FAILED TO CREATE";
        } else if (!initSuccess) {
            ss << "⚠️  FAILED TO INIT";
        } else if (!processSuccess) {
            ss << "⚠️  PROCESS ERRORS";
        } else if (!errors.empty()) {
            ss << "⚠️  HAS ISSUES (" << errors.size() << " errors)";
        } else {
            ss << "✅ WORKING";
        }
        
        ss << " | Quality: " << std::fixed << std::setprecision(1) << qualityScore << "%";
        
        return ss.str();
    }
};

class RealEngineTestHarness {
private:
    std::vector<TestResult> results;
    bool verbose;
    double sampleRate = 44100.0;
    int bufferSize = 512;
    
    // Check for NaN or Inf in audio buffer
    bool checkForNaNOrInf(float** buffer, int numChannels, int numSamples) {
        for (int ch = 0; ch < numChannels; ++ch) {
            if (!buffer[ch]) continue;
            
            for (int s = 0; s < numSamples; ++s) {
                float sample = buffer[ch][s];
                if (std::isnan(sample) || std::isinf(sample)) {
                    return true;
                }
            }
        }
        return false;
    }
    
    // Create test audio buffer
    float** createTestBuffer(int numChannels, int numSamples, bool fillWithSignal = true) {
        float** buffer = new float*[numChannels];
        for (int ch = 0; ch < numChannels; ++ch) {
            buffer[ch] = new float[numSamples];
            
            if (fillWithSignal) {
                // Fill with 440Hz sine wave
                for (int s = 0; s < numSamples; ++s) {
                    buffer[ch][s] = 0.5f * std::sin(2.0f * M_PI * 440.0f * s / sampleRate);
                }
            } else {
                // Fill with silence
                std::fill(buffer[ch], buffer[ch] + numSamples, 0.0f);
            }
        }
        return buffer;
    }
    
    void deleteTestBuffer(float** buffer, int numChannels) {
        for (int ch = 0; ch < numChannels; ++ch) {
            delete[] buffer[ch];
        }
        delete[] buffer;
    }

public:
    RealEngineTestHarness(bool verboseMode = false) : verbose(verboseMode) {}
    
    TestResult testEngine(int engineId) {
        TestResult result;
        result.engineId = engineId;
        
        if (verbose) {
            std::cout << "\n==============================" << std::endl;
            std::cout << "Testing Engine #" << engineId << std::endl;
            std::cout << "==============================" << std::endl;
        }
        
        // Test 1: Can we create the engine?
        std::unique_ptr<EngineBase> engine;
        try {
            engine = EngineFactory::createEngine(engineId);
            
            if (engine) {
                result.creationSuccess = true;
                result.engineName = engine->getName();
                
                if (verbose) {
                    std::cout << "✓ Created: " << result.engineName << std::endl;
                }
            } else {
                result.errors.push_back("EngineFactory returned nullptr");
                if (verbose) {
                    std::cout << "✗ Creation failed: nullptr returned" << std::endl;
                }
                return result;
            }
        } catch (const std::exception& e) {
            result.errors.push_back("Exception during creation: " + std::string(e.what()));
            if (verbose) {
                std::cout << "✗ Exception: " << e.what() << std::endl;
            }
            return result;
        }
        
        // Test 2: Can we initialize it?
        try {
            engine->prepareToPlay(sampleRate, bufferSize);
            result.initSuccess = true;
            
            if (verbose) {
                std::cout << "✓ Initialized at " << sampleRate << "Hz" << std::endl;
            }
        } catch (const std::exception& e) {
            result.errors.push_back("Init failed: " + std::string(e.what()));
            result.initSuccess = false;
            if (verbose) {
                std::cout << "✗ Init failed: " << e.what() << std::endl;
            }
            return result;
        }
        
        // Test 3: Can we process audio without crashing?
        try {
            float** testBuffer = createTestBuffer(2, bufferSize, true);
            
            // Process 100 blocks to test stability
            bool hasNaNOrInf = false;
            for (int i = 0; i < 100; ++i) {
                engine->processBlock(testBuffer, 2, bufferSize);
                
                if (checkForNaNOrInf(testBuffer, 2, bufferSize)) {
                    hasNaNOrInf = true;
                    result.warnings.push_back("NaN/Inf detected in output at block " + std::to_string(i));
                    break;
                }
            }
            
            if (!hasNaNOrInf) {
                result.processSuccess = true;
                if (verbose) {
                    std::cout << "✓ Processed 100 blocks without NaN/Inf" << std::endl;
                }
            } else {
                result.processSuccess = false;
                if (verbose) {
                    std::cout << "⚠ NaN/Inf detected in output" << std::endl;
                }
            }
            
            deleteTestBuffer(testBuffer, 2);
            
        } catch (const std::exception& e) {
            result.errors.push_back("Process failed: " + std::string(e.what()));
            result.processSuccess = false;
            if (verbose) {
                std::cout << "✗ Process failed: " << e.what() << std::endl;
            }
        }
        
        // Test 4: Parameter testing (basic)
        try {
            // Try to get parameter count
            auto paramCount = engine->getNumParameters();
            
            if (paramCount > 0) {
                // Test first parameter
                float originalValue = engine->getParameter(0);
                engine->setParameter(0, 0.5f);
                float newValue = engine->getParameter(0);
                
                if (std::abs(newValue - 0.5f) < 0.01f) {
                    result.parameterTest = true;
                    if (verbose) {
                        std::cout << "✓ Parameter handling works (" << paramCount << " params)" << std::endl;
                    }
                } else {
                    result.warnings.push_back("Parameter set/get mismatch");
                }
            } else {
                result.parameterTest = true; // No params is OK
                if (verbose) {
                    std::cout << "✓ No parameters (bypass engine?)" << std::endl;
                }
            }
        } catch (const std::exception& e) {
            result.warnings.push_back("Parameter test failed: " + std::string(e.what()));
        }
        
        // Test 5: Test with silence
        try {
            float** silentBuffer = createTestBuffer(2, bufferSize, false);
            engine->processBlock(silentBuffer, 2, bufferSize);
            
            if (!checkForNaNOrInf(silentBuffer, 2, bufferSize)) {
                result.stabilityTest = true;
                if (verbose) {
                    std::cout << "✓ Handles silence correctly" << std::endl;
                }
            } else {
                result.warnings.push_back("Produces NaN/Inf with silent input");
                if (verbose) {
                    std::cout << "⚠ NaN/Inf with silent input" << std::endl;
                }
            }
            
            deleteTestBuffer(silentBuffer, 2);
            
        } catch (const std::exception& e) {
            result.warnings.push_back("Silence test failed: " + std::string(e.what()));
        }
        
        // Calculate quality score
        result.qualityScore = 0.0f;
        if (result.creationSuccess) result.qualityScore += 20.0f;
        if (result.initSuccess) result.qualityScore += 20.0f;
        if (result.processSuccess) result.qualityScore += 30.0f;
        if (result.parameterTest) result.qualityScore += 15.0f;
        if (result.stabilityTest) result.qualityScore += 15.0f;
        
        return result;
    }
    
    void runAllTests() {
        std::cout << "\n==========================================" << std::endl;
        std::cout << "  REAL Chimera Phoenix Engine Testing    " << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "Testing 57 ACTUAL engine implementations..." << std::endl;
        std::cout << "Sample Rate: " << sampleRate << " Hz" << std::endl;
        std::cout << "Buffer Size: " << bufferSize << " samples" << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i <= 56; ++i) {
            results.push_back(testEngine(i));
            
            if (!verbose) {
                // Show progress
                if (results.back().creationSuccess) {
                    std::cout << "✓";
                } else {
                    std::cout << "✗";
                }
                
                if ((i + 1) % 10 == 0) {
                    std::cout << " [" << (i + 1) << "/57]" << std::endl;
                }
                std::cout.flush();
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        std::cout << "\n\nTesting completed in " << duration.count() << " ms" << std::endl;
    }
    
    void printDetailedResults() {
        std::cout << "\n==========================================" << std::endl;
        std::cout << "         DETAILED TEST RESULTS            " << std::endl;
        std::cout << "==========================================" << std::endl;
        
        int working = 0;
        int partiallyWorking = 0;
        int failing = 0;
        
        std::vector<int> criticalFailures;
        std::vector<int> partialFailures;
        
        for (const auto& result : results) {
            std::cout << result.getSummary() << std::endl;
            
            if (!result.creationSuccess) {
                failing++;
                criticalFailures.push_back(result.engineId);
            } else if (!result.processSuccess || !result.errors.empty()) {
                partiallyWorking++;
                partialFailures.push_back(result.engineId);
            } else {
                working++;
            }
            
            // Show errors and warnings
            for (const auto& error : result.errors) {
                std::cout << "    ERROR: " << error << std::endl;
            }
            for (const auto& warning : result.warnings) {
                std::cout << "    WARN:  " << warning << std::endl;
            }
        }
        
        std::cout << "\n==========================================" << std::endl;
        std::cout << "              FINAL SUMMARY               " << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << "✅ Fully Working:     " << working << "/57" << std::endl;
        std::cout << "⚠️  Partially Working: " << partiallyWorking << "/57" << std::endl;
        std::cout << "❌ Not Working:       " << failing << "/57" << std::endl;
        
        if (!criticalFailures.empty()) {
            std::cout << "\n❌ Critical Failures (won't create): ";
            for (size_t i = 0; i < criticalFailures.size(); ++i) {
                std::cout << "#" << criticalFailures[i];
                if (i < criticalFailures.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        
        if (!partialFailures.empty()) {
            std::cout << "\n⚠️  Partial Failures (create but have issues): ";
            for (size_t i = 0; i < partialFailures.size(); ++i) {
                std::cout << "#" << partialFailures[i];
                if (i < partialFailures.size() - 1) std::cout << ", ";
            }
            std::cout << std::endl;
        }
        
        float avgQuality = 0.0f;
        for (const auto& result : results) {
            avgQuality += result.qualityScore;
        }
        avgQuality /= results.size();
        
        std::cout << "\nOverall Quality Score: " << std::fixed << std::setprecision(1) 
                  << avgQuality << "%" << std::endl;
        
        std::cout << "==========================================" << std::endl;
    }
};

// Main function
int main(int argc, char* argv[]) {
    bool verbose = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --verbose, -v     Show detailed output for each engine" << std::endl;
            std::cout << "  --help, -h        Show this help message" << std::endl;
            return 0;
        }
    }
    
    try {
        RealEngineTestHarness harness(verbose);
        harness.runAllTests();
        harness.printDetailedResults();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ FATAL ERROR: " << e.what() << std::endl;
        return 1;
    }
}