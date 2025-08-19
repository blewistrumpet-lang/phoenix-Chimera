//==============================================================================
// Utility Engines Parameter Verification Test
// 
// This test verifies that the utility engines have the correct parameter counts
// and that their parameter databases are properly synchronized.
//==============================================================================

#include "JuceHeader.h"
#include "MidSideProcessor_Platinum.h"
#include "GainUtility_Platinum.h" 
#include "MonoMaker_Platinum.h"
#include "UnifiedDefaultParameters.h"
#include "EngineTypes.h"
#include <iostream>
#include <memory>

//==============================================================================
// Test Results Structure
//==============================================================================
struct TestResult {
    std::string engineName;
    int expectedParams;
    int actualParams;
    int databaseParams;
    bool parametersMatch;
    bool databaseMatch;
    std::vector<std::string> parameterNames;
    std::string status;
    
    TestResult(const std::string& name) : engineName(name) {
        expectedParams = 0;
        actualParams = 0;
        databaseParams = 0;
        parametersMatch = false;
        databaseMatch = false;
        status = "UNKNOWN";
    }
};

//==============================================================================
// Test Functions
//==============================================================================

TestResult testMidSideProcessor() {
    TestResult result("Mid-Side Processor (ENGINE_ID: 53)");
    
    // Create engine instance
    auto engine = std::make_unique<MidSideProcessor_Platinum>();
    
    // Get actual parameter count from engine
    result.actualParams = engine->getNumParameters();
    result.expectedParams = 10; // As documented in the implementation
    
    // Get parameter count from database
    result.databaseParams = UnifiedDefaultParameters::getParameterCount(ENGINE_MID_SIDE_PROCESSOR);
    
    // Test parameter names
    for (int i = 0; i < result.actualParams; ++i) {
        juce::String paramName = engine->getParameterName(i);
        result.parameterNames.push_back(paramName.toStdString());
    }
    
    // Verify consistency
    result.parametersMatch = (result.actualParams == result.expectedParams);
    result.databaseMatch = (result.databaseParams == result.actualParams);
    
    if (result.parametersMatch && result.databaseMatch) {
        result.status = "PASS";
    } else {
        result.status = "FAIL";
    }
    
    return result;
}

TestResult testGainUtility() {
    TestResult result("Gain Utility (ENGINE_ID: 54)");
    
    // Create engine instance
    auto engine = std::make_unique<GainUtility_Platinum>();
    
    // Get actual parameter count from engine
    result.actualParams = engine->getNumParameters();
    result.expectedParams = 10; // As documented in the implementation
    
    // Get parameter count from database
    result.databaseParams = UnifiedDefaultParameters::getParameterCount(ENGINE_GAIN_UTILITY);
    
    // Test parameter names
    for (int i = 0; i < result.actualParams; ++i) {
        juce::String paramName = engine->getParameterName(i);
        result.parameterNames.push_back(paramName.toStdString());
    }
    
    // Verify consistency
    result.parametersMatch = (result.actualParams == result.expectedParams);
    result.databaseMatch = (result.databaseParams == result.actualParams);
    
    if (result.parametersMatch && result.databaseMatch) {
        result.status = "PASS";
    } else {
        result.status = "FAIL";
    }
    
    return result;
}

TestResult testMonoMaker() {
    TestResult result("Mono Maker (ENGINE_ID: 55)");
    
    // Create engine instance
    auto engine = std::make_unique<MonoMaker_Platinum>();
    
    // Get actual parameter count from engine
    result.actualParams = engine->getNumParameters();
    result.expectedParams = 8; // As documented in the implementation
    
    // Get parameter count from database
    result.databaseParams = UnifiedDefaultParameters::getParameterCount(ENGINE_MONO_MAKER);
    
    // Test parameter names
    for (int i = 0; i < result.actualParams; ++i) {
        juce::String paramName = engine->getParameterName(i);
        result.parameterNames.push_back(paramName.toStdString());
    }
    
    // Verify consistency
    result.parametersMatch = (result.actualParams == result.expectedParams);
    result.databaseMatch = (result.databaseParams == result.actualParams);
    
    if (result.parametersMatch && result.databaseMatch) {
        result.status = "PASS";
    } else {
        result.status = "FAIL";
    }
    
    return result;
}

//==============================================================================
// Test Engine Functionality
//==============================================================================

bool testEngineProcessing() {
    std::cout << "\n=== Testing Engine Processing Functionality ===\n";
    
    // Prepare test audio buffer
    juce::AudioBuffer<float> testBuffer(2, 512); // Stereo, 512 samples
    testBuffer.clear();
    
    // Add test signal (sine wave)
    for (int sample = 0; sample < 512; ++sample) {
        float testSignal = std::sin(2.0f * M_PI * 440.0f * sample / 48000.0f) * 0.5f;
        testBuffer.setSample(0, sample, testSignal);  // Left
        testBuffer.setSample(1, sample, testSignal * 0.8f);  // Right (slightly different)
    }
    
    bool allTestsPassed = true;
    
    // Test Mid-Side Processor
    {
        auto engine = std::make_unique<MidSideProcessor_Platinum>();
        engine->prepareToPlay(48000.0, 512);
        
        // Test parameter updates
        std::map<int, float> params;
        params[0] = 0.6f; // Mid Gain
        params[1] = 0.4f; // Side Gain  
        params[2] = 0.7f; // Width
        engine->updateParameters(params);
        
        // Process audio
        auto testBufferCopy = testBuffer;
        engine->process(testBufferCopy);
        
        // Verify output is not silent and not clipped
        float maxLevel = testBufferCopy.getMagnitude(0, 512);
        bool validOutput = (maxLevel > 0.001f && maxLevel < 2.0f);
        
        std::cout << "Mid-Side Processor: " << (validOutput ? "PASS" : "FAIL") 
                  << " (max level: " << maxLevel << ")\n";
        allTestsPassed &= validOutput;
    }
    
    // Test Gain Utility
    {
        auto engine = std::make_unique<GainUtility_Platinum>();
        engine->prepareToPlay(48000.0, 512);
        
        // Test parameter updates
        std::map<int, float> params;
        params[0] = 0.7f; // Main Gain
        params[5] = 0.0f; // Mode (stereo)
        engine->updateParameters(params);
        
        // Process audio
        auto testBufferCopy = testBuffer;
        engine->process(testBufferCopy);
        
        // Verify output is not silent and not clipped
        float maxLevel = testBufferCopy.getMagnitude(0, 512);
        bool validOutput = (maxLevel > 0.001f && maxLevel < 2.0f);
        
        std::cout << "Gain Utility: " << (validOutput ? "PASS" : "FAIL") 
                  << " (max level: " << maxLevel << ")\n";
        allTestsPassed &= validOutput;
    }
    
    // Test Mono Maker
    {
        auto engine = std::make_unique<MonoMaker_Platinum>();
        engine->prepareToPlay(48000.0, 512);
        
        // Test parameter updates
        std::map<int, float> params;
        params[0] = 0.3f; // Frequency
        params[3] = 1.0f; // Bass Mono amount
        engine->updateParameters(params);
        
        // Process audio
        auto testBufferCopy = testBuffer;
        engine->process(testBufferCopy);
        
        // Verify output is not silent and not clipped
        float maxLevel = testBufferCopy.getMagnitude(0, 512);
        bool validOutput = (maxLevel > 0.001f && maxLevel < 2.0f);
        
        std::cout << "Mono Maker: " << (validOutput ? "PASS" : "FAIL") 
                  << " (max level: " << maxLevel << ")\n";
        allTestsPassed &= validOutput;
    }
    
    return allTestsPassed;
}

//==============================================================================
// Print Test Results
//==============================================================================

void printTestResult(const TestResult& result) {
    std::cout << "\n--- " << result.engineName << " ---\n";
    std::cout << "Expected Parameters: " << result.expectedParams << "\n";
    std::cout << "Actual Parameters: " << result.actualParams << "\n";
    std::cout << "Database Parameters: " << result.databaseParams << "\n";
    std::cout << "Parameters Match: " << (result.parametersMatch ? "YES" : "NO") << "\n";
    std::cout << "Database Match: " << (result.databaseMatch ? "YES" : "NO") << "\n";
    std::cout << "Status: " << result.status << "\n";
    
    std::cout << "Parameter Names:\n";
    for (size_t i = 0; i < result.parameterNames.size(); ++i) {
        std::cout << "  [" << i << "] " << result.parameterNames[i] << "\n";
    }
}

//==============================================================================
// Main Test Function
//==============================================================================

int main() {
    std::cout << "=== Utility Engines Parameter Verification Test ===\n";
    std::cout << "Testing parameter count synchronization between engine implementations\n";
    std::cout << "and parameter database for utility engines.\n";
    
    // Run parameter count tests
    std::vector<TestResult> results;
    results.push_back(testMidSideProcessor());
    results.push_back(testGainUtility());
    results.push_back(testMonoMaker());
    
    // Print results
    for (const auto& result : results) {
        printTestResult(result);
    }
    
    // Summary
    int passCount = 0;
    for (const auto& result : results) {
        if (result.status == "PASS") passCount++;
    }
    
    std::cout << "\n=== SUMMARY ===\n";
    std::cout << "Tests Passed: " << passCount << "/" << results.size() << "\n";
    
    if (passCount == results.size()) {
        std::cout << "✅ ALL PARAMETER COUNT TESTS PASSED!\n";
        std::cout << "The utility engines now have correct parameter mappings.\n";
    } else {
        std::cout << "❌ SOME TESTS FAILED!\n";
        std::cout << "Parameter database needs further correction.\n";
    }
    
    // Test engine processing functionality
    bool processingTestsPassed = testEngineProcessing();
    
    std::cout << "\n=== FINAL RESULT ===\n";
    if (passCount == results.size() && processingTestsPassed) {
        std::cout << "🎉 ALL TESTS PASSED! Utility engines are working correctly.\n";
        return 0;
    } else {
        std::cout << "⚠️  Some issues remain. Check the output above for details.\n";
        return 1;
    }
}

//==============================================================================
// Compilation Instructions:
// 
// g++ -std=c++17 -I./JuceLibraryCode -I./Source \
//     utility_engines_parameter_test.cpp \
//     Source/MidSideProcessor_Platinum.cpp \
//     Source/GainUtility_Platinum.cpp \
//     Source/MonoMaker_Platinum.cpp \
//     Source/UnifiedDefaultParameters.cpp \
//     Source/DspEngineUtilities.cpp \
//     -ljuce_core -ljuce_audio_basics -ljuce_dsp \
//     -o utility_engines_test
//==============================================================================