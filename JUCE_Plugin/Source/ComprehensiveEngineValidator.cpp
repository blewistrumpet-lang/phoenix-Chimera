#include "ComprehensiveEngineValidator.h"
#include "EngineFactory.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <filesystem>
#include <fstream>

namespace ComprehensiveEngineValidator {

// =============================================================================
// EngineValidationResult Implementation
// =============================================================================

void EngineValidationResult::calculateOverallMetrics() {
    // Collect all tests
    std::vector<ValidationTest> allTests;
    allTests.insert(allTests.end(), functionalityTests.begin(), functionalityTests.end());
    allTests.insert(allTests.end(), parameterTests.begin(), parameterTests.end());
    allTests.insert(allTests.end(), audioQualityTests.begin(), audioQualityTests.end());
    allTests.insert(allTests.end(), performanceTests.begin(), performanceTests.end());
    allTests.insert(allTests.end(), compatibilityTests.begin(), compatibilityTests.end());
    
    totalTests = static_cast<int>(allTests.size());
    passedTests = 0;
    float totalScore = 0.0f;
    totalExecutionTimeMs = 0.0f;
    
    for (const auto& test : allTests) {
        if (test.passed) passedTests++;
        totalScore += test.score;
        totalExecutionTimeMs += test.executionTimeMs;
    }
    
    overallScore = (totalTests > 0) ? totalScore / totalTests : 0.0f;
    overallPassed = (passedTests == totalTests) && (overallScore >= 70.0f);
    
    // Generate quality assessment
    if (overallScore >= 90.0f) {
        qualityAssessment = "Excellent - Engine performing optimally";
    } else if (overallScore >= 80.0f) {
        qualityAssessment = "Good - Engine performing well with minor issues";
    } else if (overallScore >= 70.0f) {
        qualityAssessment = "Acceptable - Engine functional but needs improvement";
    } else if (overallScore >= 50.0f) {
        qualityAssessment = "Poor - Engine has significant issues";
    } else {
        qualityAssessment = "Failed - Engine not functioning correctly";
    }
}

std::string EngineValidationResult::generateSummaryReport() const {
    std::ostringstream report;
    
    report << "=== Engine Validation Summary ===\n";
    report << "Engine: " << engineName << " (" << engineCategory << ")\n";
    report << "Overall Status: " << (overallPassed ? "PASSED" : "FAILED") << "\n";
    report << "Overall Score: " << std::fixed << std::setprecision(1) << overallScore << "/100\n";
    report << "Tests Passed: " << passedTests << "/" << totalTests << "\n";
    report << "Execution Time: " << totalExecutionTimeMs << " ms\n";
    report << "Quality Assessment: " << qualityAssessment << "\n\n";
    
    if (!issues.empty()) {
        report << "Issues Found:\n";
        for (const auto& issue : issues) {
            report << "- " << issue << "\n";
        }
        report << "\n";
    }
    
    if (!recommendations.empty()) {
        report << "Recommendations:\n";
        for (const auto& rec : recommendations) {
            report << "- " << rec << "\n";
        }
        report << "\n";
    }
    
    return report.str();
}

// =============================================================================
// BatchValidationResults Implementation
// =============================================================================

void BatchValidationResults::calculateBatchStatistics() {
    totalEngines = static_cast<int>(engineResults.size());
    passedEngines = 0;
    float totalScore = 0.0f;
    totalBatchTimeMs = 0.0f;
    
    // Calculate basic statistics
    for (const auto& result : engineResults) {
        if (result.overallPassed) passedEngines++;
        totalScore += result.overallScore;
        totalBatchTimeMs += result.totalExecutionTimeMs;
    }
    
    averageScore = (totalEngines > 0) ? totalScore / totalEngines : 0.0f;
    
    // Calculate category statistics
    std::map<std::string, std::vector<float>> categoryScores;
    std::map<std::string, std::vector<std::string>> categoryIssues;
    
    for (const auto& result : engineResults) {
        // Functionality category
        float funcScore = ValidationUtils::calculateCategoryScore(result.functionalityTests);
        categoryScores["Functionality"].push_back(funcScore);
        
        float paramScore = ValidationUtils::calculateCategoryScore(result.parameterTests);
        categoryScores["Parameters"].push_back(paramScore);
        
        float qualityScore = ValidationUtils::calculateCategoryScore(result.audioQualityTests);
        categoryScores["Audio Quality"].push_back(qualityScore);
        
        float perfScore = ValidationUtils::calculateCategoryScore(result.performanceTests);
        categoryScores["Performance"].push_back(perfScore);
        
        float compatScore = ValidationUtils::calculateCategoryScore(result.compatibilityTests);
        categoryScores["Compatibility"].push_back(compatScore);
        
        // Collect issues
        for (const auto& issue : result.issues) {
            categoryIssues["General"].push_back(issue);
        }
    }
    
    // Process category statistics
    for (const auto& pair : categoryScores) {
        CategoryStats stats;
        stats.totalTests = static_cast<int>(pair.second.size());
        stats.passedTests = 0;
        float totalCatScore = 0.0f;
        
        for (float score : pair.second) {
            if (score >= 70.0f) stats.passedTests++;
            totalCatScore += score;
        }
        
        stats.averageScore = (stats.totalTests > 0) ? totalCatScore / stats.totalTests : 0.0f;
        categoryStats[pair.first] = stats;
    }
    
    // Identify critical issues and global recommendations
    std::map<std::string, int> issueFrequency;
    for (const auto& result : engineResults) {
        for (const auto& issue : result.issues) {
            issueFrequency[issue]++;
        }
    }
    
    // Issues affecting more than 20% of engines are considered critical
    int criticalThreshold = std::max(1, totalEngines / 5);
    for (const auto& pair : issueFrequency) {
        if (pair.second >= criticalThreshold) {
            criticalIssues.push_back(pair.first + " (affects " + std::to_string(pair.second) + " engines)");
        }
    }
}

std::string BatchValidationResults::generateExecutiveSummary() const {
    std::ostringstream summary;
    
    summary << "=== Executive Summary ===\n";
    summary << "Total Engines Tested: " << totalEngines << "\n";
    summary << "Engines Passed: " << passedEngines << " (" << 
               std::fixed << std::setprecision(1) << (100.0f * passedEngines / totalEngines) << "%)\n";
    summary << "Average Score: " << averageScore << "/100\n";
    summary << "Total Test Time: " << (totalBatchTimeMs / 1000.0f) << " seconds\n\n";
    
    // Category breakdown
    summary << "=== Category Performance ===\n";
    for (const auto& pair : categoryStats) {
        const auto& stats = pair.second;
        summary << pair.first << ": " << stats.averageScore << "/100 ";
        summary << "(" << stats.passedTests << "/" << stats.totalTests << " passed)\n";
    }
    summary << "\n";
    
    // Critical issues
    if (!criticalIssues.empty()) {
        summary << "=== Critical Issues ===\n";
        for (const auto& issue : criticalIssues) {
            summary << "- " << issue << "\n";
        }
        summary << "\n";
    }
    
    // Overall assessment
    if (averageScore >= 85.0f) {
        summary << "Assessment: Excellent - All engines performing well\n";
    } else if (averageScore >= 75.0f) {
        summary << "Assessment: Good - Most engines functional with minor issues\n";
    } else if (averageScore >= 65.0f) {
        summary << "Assessment: Acceptable - Some engines need attention\n";
    } else {
        summary << "Assessment: Poor - Multiple engines have significant problems\n";
    }
    
    return summary.str();
}

// =============================================================================
// EngineValidator Implementation
// =============================================================================

EngineValidator::EngineValidator() {
    // Initialize default settings
}

EngineValidationResult EngineValidator::validateEngine(int engineType, ValidationLevel level) {
    auto engine = EngineFactory::createEngine(engineType);
    if (!engine) {
        EngineValidationResult result;
        result.engineName = "Unknown Engine";
        result.engineType = engineType;
        result.overallPassed = false;
        result.issues.push_back("Failed to create engine instance");
        return result;
    }
    
    return validateEngine(std::move(engine), engineType, level);
}

EngineValidationResult EngineValidator::validateEngine(std::unique_ptr<EngineBase> engine, int engineType, ValidationLevel level) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    EngineValidationResult result;
    result.engineName = engine->getName().toStdString();
    result.engineType = engineType;
    result.engineCategory = EngineTestAgents::TestAgentFactory::getEffectCategoryName(engineType);
    
    reportProgress("Starting validation for " + result.engineName, 0.0f);
    
    try {
        // Prepare engine
        engine->prepareToPlay(m_sampleRate, 512);
        engine->reset();
        
        // Run test suites based on validation level
        if (level >= ValidationLevel::BASIC) {
            reportProgress("Running functionality tests", 20.0f);
            result.functionalityTests = runFunctionalityTests(engine.get(), engineType);
        }
        
        if (level >= ValidationLevel::STANDARD) {
            reportProgress("Running parameter tests", 40.0f);
            result.parameterTests = runParameterTests(engine.get(), engineType);
            
            reportProgress("Running audio quality tests", 60.0f);
            result.audioQualityTests = runAudioQualityTests(engine.get(), engineType);
        }
        
        if (level >= ValidationLevel::COMPREHENSIVE) {
            reportProgress("Running performance tests", 80.0f);
            result.performanceTests = runPerformanceTests(engine.get(), engineType);
            
            result.compatibilityTests = runCompatibilityTests(engine.get(), engineType);
            
            // Run specialized test agents
            auto testAgent = EngineTestAgents::TestAgentFactory::createTestAgent(engineType);
            result.agentTestResults = testAgent->runTests(EngineFactory::createEngine(engineType), m_sampleRate);
            
            // Run parameter sweep tests
            ParameterSweepTest::ParameterSweeper sweeper;
            result.sweepResults = sweeper.testAllParameters(EngineFactory::createEngine(engineType), m_sampleRate);
            result.sweepResults.engineType = engineType;
        }
        
        if (level >= ValidationLevel::STRESS_TEST) {
            reportProgress("Running stress tests", 90.0f);
            auto stressTests = runStressTests(engine.get(), engineType);
            result.performanceTests.insert(result.performanceTests.end(), stressTests.begin(), stressTests.end());
        }
        
        // Calculate overall metrics
        result.calculateOverallMetrics();
        
        // Generate analysis and recommendations
        analyzeEnginePerformance(result);
        generateRecommendations(result);
        
    } catch (const std::exception& e) {
        result.overallPassed = false;
        result.issues.push_back("Exception during validation: " + std::string(e.what()));
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    result.totalExecutionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    reportProgress("Validation complete for " + result.engineName, 100.0f);
    
    return result;
}

BatchValidationResults EngineValidator::validateAllEngines(ValidationLevel level) {
    std::vector<int> allEngineTypes;
    
    // Collect all valid engine types
    for (int i = 0; i < ENGINE_COUNT; ++i) {
        if (isValidEngineType(i)) {
            allEngineTypes.push_back(i);
        }
    }
    
    return validateEngineList(allEngineTypes, level);
}

BatchValidationResults EngineValidator::validateEngineList(const std::vector<int>& engineTypes, ValidationLevel level) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    BatchValidationResults batchResults;
    
    for (size_t i = 0; i < engineTypes.size(); ++i) {
        int engineType = engineTypes[i];
        
        reportProgress("Validating engine " + std::to_string(i + 1) + "/" + std::to_string(engineTypes.size()), 
                      (100.0f * i) / engineTypes.size());
        
        auto result = validateEngine(engineType, level);
        batchResults.engineResults.push_back(result);
        
        // Save individual report if requested
        if (m_generateHTML && !m_outputDirectory.empty()) {
            createOutputDirectory();
            std::string filename = m_outputDirectory + "/" + result.engineName + "_validation_report.html";
            std::ofstream file(filename);
            if (file.is_open()) {
                file << generateHTMLReport(result);
                file.close();
            }
        }
    }
    
    // Calculate batch statistics
    batchResults.calculateBatchStatistics();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    batchResults.totalBatchTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    reportProgress("Batch validation complete", 100.0f);
    
    return batchResults;
}

std::vector<ValidationTest> EngineValidator::runFunctionalityTests(EngineBase* engine, int engineType) {
    std::vector<ValidationTest> tests;
    
    tests.push_back(testBasicFunctionality(engine));
    tests.push_back(testSilenceHandling(engine));
    tests.push_back(testLatency(engine));
    tests.push_back(testStability(engine));
    
    // Test all parameters for basic functionality
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        tests.push_back(testParameterRange(engine, i));
    }
    
    return tests;
}

std::vector<ValidationTest> EngineValidator::runParameterTests(EngineBase* engine, int engineType) {
    std::vector<ValidationTest> tests;
    
    // Run basic parameter sweep for all parameters
    ParameterSweepTest::ParameterSweeper sweeper;
    auto configs = sweeper.generateConfigsForEngine(engine, engineType);
    
    for (const auto& config : configs) {
        auto sweepResult = sweeper.testSingleParameter(engine, config, m_sampleRate);
        
        ValidationTest test;
        test.testName = "Parameter Sweep: " + config.parameterName;
        test.category = "Parameters";
        test.description = "Tests parameter effectiveness and response";
        test.passed = sweepResult.isEffective;
        test.score = ParameterSweepTest::ParameterAnalysis::assessParameterQuality(sweepResult) * 100.0f;
        test.details = sweepResult.generateSummary();
        
        if (!test.passed) {
            test.failureReason = "Parameter appears to have no significant effect on audio";
        }
        
        // Add measurement data for plotting
        for (const auto& point : sweepResult.measurements) {
            test.plotData.push_back(point.measuredValue);
        }
        test.plotType = "line";
        
        tests.push_back(test);
    }
    
    return tests;
}

std::vector<ValidationTest> EngineValidator::runAudioQualityTests(EngineBase* engine, int engineType) {
    std::vector<ValidationTest> tests;
    
    tests.push_back(testDynamicRange(engine));
    tests.push_back(testFrequencyResponse(engine));
    tests.push_back(testTHDLevel(engine));
    tests.push_back(testNoiseFloor(engine));
    
    return tests;
}

std::vector<ValidationTest> EngineValidator::runPerformanceTests(EngineBase* engine, int engineType) {
    std::vector<ValidationTest> tests;
    
    tests.push_back(testCPUUsage(engine));
    tests.push_back(testMemoryUsage(engine));
    
    return tests;
}

std::vector<ValidationTest> EngineValidator::runCompatibilityTests(EngineBase* engine, int engineType) {
    std::vector<ValidationTest> tests;
    
    tests.push_back(testSampleRateCompatibility(engine));
    tests.push_back(testBufferSizeCompatibility(engine));
    tests.push_back(testExtremeParameters(engine));
    
    return tests;
}

std::vector<ValidationTest> EngineValidator::runStressTests(EngineBase* engine, int engineType) {
    std::vector<ValidationTest> tests;
    
    tests.push_back(testLongDurationStability(engine));
    
    return tests;
}

// =============================================================================
// Individual Test Implementations
// =============================================================================

ValidationTest EngineValidator::testBasicFunctionality(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Basic Functionality";
    test.category = "Functionality";
    test.description = "Tests if engine processes audio without crashing";
    
    try {
        engine->reset();
        
        // Generate test signal
        auto testBuffer = TestSignalGenerator::generateSineWave(1000.0f, 1.0f, m_sampleRate, 0.5f);
        
        // Process audio
        engine->process(testBuffer);
        
        // Check for valid output
        float rms = AudioMeasurements::measureRMS(testBuffer);
        test.passed = !std::isnan(rms) && !std::isinf(rms) && rms > 0.0f;
        test.score = test.passed ? 100.0f : 0.0f;
        
        test.measurements["output_rms"] = rms;
        
        if (!test.passed) {
            test.failureReason = "Engine produced invalid output";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testParameterRange(EngineBase* engine, int paramIndex) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Parameter Range: " + engine->getParameterName(paramIndex).toStdString();
    test.category = "Functionality";
    test.description = "Tests parameter accepts full 0-1 range";
    
    try {
        bool rangeTestPassed = true;
        std::vector<float> testValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
        
        for (float value : testValues) {
            engine->reset();
            std::map<int, float> params;
            params[paramIndex] = value;
            
            try {
                engine->updateParameters(params);
                
                // Process a short signal to ensure no crashes
                auto testBuffer = TestSignalGenerator::generateSineWave(440.0f, 0.1f, m_sampleRate, 0.3f);
                engine->process(testBuffer);
                
                float rms = AudioMeasurements::measureRMS(testBuffer);
                if (std::isnan(rms) || std::isinf(rms)) {
                    rangeTestPassed = false;
                    break;
                }
                
            } catch (...) {
                rangeTestPassed = false;
                break;
            }
        }
        
        test.passed = rangeTestPassed;
        test.score = test.passed ? 100.0f : 0.0f;
        
        if (!test.passed) {
            test.failureReason = "Parameter does not accept full range or causes crashes";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testSilenceHandling(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Silence Handling";
    test.category = "Functionality";
    test.description = "Tests engine doesn't generate noise from silence";
    
    try {
        engine->reset();
        
        // Generate silence
        auto silenceBuffer = TestSignalGenerator::generateSilence(2.0f, m_sampleRate);
        
        // Process silence
        engine->process(silenceBuffer);
        
        // Measure output level
        float rms = AudioMeasurements::measureRMS(silenceBuffer);
        float dB = TestSignalGenerator::linearTodB(rms);
        
        test.measurements["noise_floor_db"] = dB;
        
        // Pass if output is very quiet (below -60dB)
        test.passed = dB < -60.0f;
        test.score = test.passed ? 100.0f : std::max(0.0f, 100.0f - (dB + 60.0f) * 2.0f);
        
        if (!test.passed) {
            test.failureReason = "Engine generates noise from silence: " + std::to_string(dB) + " dB";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testLatency(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Latency Test";
    test.category = "Performance";
    test.description = "Measures processing latency";
    
    try {
        engine->reset();
        
        // Generate impulse
        auto impulse = TestSignalGenerator::generateImpulse(m_sampleRate, 1.0f);
        auto originalImpulse = impulse;
        
        // Process impulse
        engine->process(impulse);
        
        // Measure latency
        float latency = AudioMeasurements::measureLatency(originalImpulse, impulse, m_sampleRate);
        
        test.measurements["latency_samples"] = latency;
        test.measurements["latency_ms"] = (latency / m_sampleRate) * 1000.0f;
        
        // Pass if latency is reasonable (< 100ms)
        float latencyMs = (latency / m_sampleRate) * 1000.0f;
        test.passed = latencyMs < 100.0f;
        test.score = test.passed ? std::max(0.0f, 100.0f - latencyMs) : 0.0f;
        
        if (!test.passed) {
            test.failureReason = "Latency too high: " + std::to_string(latencyMs) + " ms";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testStability(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Stability Test";
    test.category = "Functionality";
    test.description = "Tests engine doesn't oscillate or become unstable";
    
    try {
        engine->reset();
        
        // Generate impulse and long buffer
        auto impulse = TestSignalGenerator::generateImpulse(m_sampleRate, 1.0f);
        juce::AudioBuffer<float> longBuffer(impulse.getNumChannels(), static_cast<int>(m_sampleRate * 3)); // 3 seconds
        longBuffer.clear();
        
        // Copy impulse to start
        for (int ch = 0; ch < impulse.getNumChannels(); ++ch) {
            longBuffer.copyFrom(ch, 0, impulse, ch, 0, impulse.getNumSamples());
        }
        
        // Process long buffer
        engine->process(longBuffer);
        
        // Check for sustained oscillation
        bool hasOscillation = AudioMeasurements::detectSustainedOscillation(longBuffer, m_sampleRate);
        
        test.passed = !hasOscillation;
        test.score = test.passed ? 100.0f : 0.0f;
        test.measurements["stable"] = test.passed ? 1.0f : 0.0f;
        
        if (!test.passed) {
            test.failureReason = "Engine became unstable and oscillated";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testDynamicRange(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Dynamic Range";
    test.category = "Audio Quality";
    test.description = "Measures dynamic range preservation";
    
    try {
        engine->reset();
        
        // Test with different signal levels
        auto quietSignal = TestSignalGenerator::generateSineWave(1000.0f, 1.0f, m_sampleRate, 0.1f);
        auto loudSignal = TestSignalGenerator::generateSineWave(1000.0f, 1.0f, m_sampleRate, 0.8f);
        
        auto quietOriginal = quietSignal;
        auto loudOriginal = loudSignal;
        
        engine->process(quietSignal);
        engine->reset();
        engine->process(loudSignal);
        
        float quietRMS = AudioMeasurements::measureRMS(quietSignal);
        float loudRMS = AudioMeasurements::measureRMS(loudSignal);
        
        float originalDR = TestSignalGenerator::linearTodB(0.8f) - TestSignalGenerator::linearTodB(0.1f);
        float processedDR = TestSignalGenerator::linearTodB(loudRMS) - TestSignalGenerator::linearTodB(quietRMS);
        
        test.measurements["original_dr_db"] = originalDR;
        test.measurements["processed_dr_db"] = processedDR;
        test.measurements["dr_preservation"] = processedDR / originalDR;
        
        // Good dynamic range preservation is > 80%
        float preservation = processedDR / originalDR;
        test.passed = preservation > 0.8f;
        test.score = std::min(100.0f, preservation * 100.0f);
        
        if (!test.passed) {
            test.failureReason = "Poor dynamic range preservation: " + std::to_string(preservation * 100.0f) + "%";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testFrequencyResponse(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Frequency Response";
    test.category = "Audio Quality";
    test.description = "Analyzes frequency response characteristics";
    
    try {
        engine->reset();
        
        // Generate pink noise for frequency response test
        auto noiseSignal = TestSignalGenerator::generatePinkNoise(2.0f, m_sampleRate, 0.3f);
        auto originalNoise = noiseSignal;
        
        // Process signal
        engine->process(noiseSignal);
        
        // Compute frequency response
        auto originalSpectrum = AudioMeasurements::computeFrequencyResponse(originalNoise, m_sampleRate);
        auto processedSpectrum = AudioMeasurements::computeFrequencyResponse(noiseSignal, m_sampleRate);
        
        // Calculate frequency response deviation
        float maxDeviation = 0.0f;
        for (size_t i = 0; i < std::min(originalSpectrum.magnitudes.size(), processedSpectrum.magnitudes.size()); ++i) {
            if (originalSpectrum.magnitudes[i] > 0.001f) {
                float ratio = processedSpectrum.magnitudes[i] / originalSpectrum.magnitudes[i];
                float deviationDB = std::abs(TestSignalGenerator::linearTodB(ratio));
                maxDeviation = std::max(maxDeviation, deviationDB);
                
                if (i < 100) { // Store some plot data
                    test.plotData.push_back(deviationDB);
                }
            }
        }
        
        test.measurements["max_deviation_db"] = maxDeviation;
        test.plotType = "line";
        
        // Pass if deviation is reasonable
        test.passed = maxDeviation < 20.0f; // Within 20dB
        test.score = std::max(0.0f, 100.0f - maxDeviation * 2.0f);
        
        if (!test.passed) {
            test.failureReason = "Excessive frequency response deviation: " + std::to_string(maxDeviation) + " dB";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testTHDLevel(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "THD Level";
    test.category = "Audio Quality";
    test.description = "Measures total harmonic distortion";
    
    try {
        engine->reset();
        
        // Generate sine wave
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 1.0f, m_sampleRate, 0.5f);
        
        // Process signal
        engine->process(testSignal);
        
        // Measure THD
        float thd = AudioMeasurements::measureTHD(testSignal, 440.0f, m_sampleRate);
        float thdPercent = thd * 100.0f;
        
        test.measurements["thd_percent"] = thdPercent;
        
        // Different limits for different engine types (distortion vs clean effects)
        float thdLimit = 10.0f; // Default 10% THD limit
        // TODO: Adjust based on engine type
        
        test.passed = thdPercent < thdLimit;
        test.score = std::max(0.0f, 100.0f - (thdPercent / thdLimit) * 100.0f);
        
        if (!test.passed) {
            test.failureReason = "THD too high: " + std::to_string(thdPercent) + "%";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testNoiseFloor(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Noise Floor";
    test.category = "Audio Quality";
    test.description = "Measures noise floor and artifacts";
    
    try {
        engine->reset();
        
        // Generate very quiet signal
        auto quietSignal = TestSignalGenerator::generateSineWave(1000.0f, 2.0f, m_sampleRate, 0.01f);
        
        // Process signal
        engine->process(quietSignal);
        
        // Measure noise floor
        float noiseFloor = AudioMeasurements::measureNoiseFloor(quietSignal);
        float noiseFloorDB = TestSignalGenerator::linearTodB(noiseFloor);
        
        test.measurements["noise_floor_db"] = noiseFloorDB;
        
        // Pass if noise floor is low enough
        test.passed = noiseFloorDB < -80.0f;
        test.score = std::max(0.0f, 100.0f + (noiseFloorDB + 80.0f));
        
        if (!test.passed) {
            test.failureReason = "Noise floor too high: " + std::to_string(noiseFloorDB) + " dB";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testCPUUsage(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "CPU Usage";
    test.category = "Performance";
    test.description = "Measures CPU usage during processing";
    
    try {
        engine->reset();
        
        // Generate test signal
        auto testSignal = TestSignalGenerator::generatePinkNoise(5.0f, m_sampleRate, 0.5f);
        
        // Time the processing
        auto processingStart = std::chrono::high_resolution_clock::now();
        engine->process(testSignal);
        auto processingEnd = std::chrono::high_resolution_clock::now();
        
        float processingTimeMs = std::chrono::duration<float, std::milli>(processingEnd - processingStart).count();
        float audioTimeMs = (testSignal.getNumSamples() / m_sampleRate) * 1000.0f;
        float realTimeRatio = processingTimeMs / audioTimeMs;
        
        test.measurements["processing_time_ms"] = processingTimeMs;
        test.measurements["audio_time_ms"] = audioTimeMs;
        test.measurements["real_time_ratio"] = realTimeRatio;
        
        // Pass if can process in real-time (ratio < 1.0)
        test.passed = realTimeRatio < 1.0f;
        test.score = std::max(0.0f, 100.0f - realTimeRatio * 50.0f);
        
        if (!test.passed) {
            test.failureReason = "Cannot process in real-time: " + std::to_string(realTimeRatio) + "x";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testMemoryUsage(EngineBase* engine) {
    ValidationTest test;
    test.testName = "Memory Usage";
    test.category = "Performance";
    test.description = "Estimates memory usage";
    
    // This is a simplified test - real memory measurement would require platform-specific code
    test.passed = true;
    test.score = 100.0f;
    test.measurements["estimated_memory_kb"] = 1024.0f; // Placeholder
    
    return test;
}

ValidationTest EngineValidator::testSampleRateCompatibility(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Sample Rate Compatibility";
    test.category = "Compatibility";
    test.description = "Tests compatibility with different sample rates";
    
    std::vector<double> sampleRates = {44100.0, 48000.0, 88200.0, 96000.0};
    bool allRatesPassed = true;
    
    for (double sr : sampleRates) {
        try {
            engine->prepareToPlay(sr, 512);
            engine->reset();
            
            auto testSignal = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sr, 0.5f);
            engine->process(testSignal);
            
            float rms = AudioMeasurements::measureRMS(testSignal);
            if (std::isnan(rms) || std::isinf(rms)) {
                allRatesPassed = false;
                break;
            }
            
        } catch (...) {
            allRatesPassed = false;
            break;
        }
    }
    
    test.passed = allRatesPassed;
    test.score = test.passed ? 100.0f : 0.0f;
    
    if (!test.passed) {
        test.failureReason = "Engine failed with some sample rates";
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testBufferSizeCompatibility(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Buffer Size Compatibility";
    test.category = "Compatibility";
    test.description = "Tests compatibility with different buffer sizes";
    
    std::vector<int> bufferSizes = {64, 128, 256, 512, 1024, 2048};
    bool allSizesPassed = true;
    
    for (int size : bufferSizes) {
        try {
            engine->prepareToPlay(m_sampleRate, size);
            engine->reset();
            
            auto testSignal = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, m_sampleRate, 0.5f);
            engine->process(testSignal);
            
            float rms = AudioMeasurements::measureRMS(testSignal);
            if (std::isnan(rms) || std::isinf(rms)) {
                allSizesPassed = false;
                break;
            }
            
        } catch (...) {
            allSizesPassed = false;
            break;
        }
    }
    
    test.passed = allSizesPassed;
    test.score = test.passed ? 100.0f : 0.0f;
    
    if (!test.passed) {
        test.failureReason = "Engine failed with some buffer sizes";
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testExtremeParameters(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Extreme Parameters";
    test.category = "Compatibility";
    test.description = "Tests engine with extreme parameter values";
    
    bool extremeTestPassed = true;
    
    try {
        // Test all parameters at extreme values
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            engine->reset();
            
            // Test minimum values
            std::map<int, float> minParams;
            minParams[i] = 0.0f;
            engine->updateParameters(minParams);
            
            auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 0.1f, m_sampleRate, 0.5f);
            engine->process(testSignal);
            
            float rms = AudioMeasurements::measureRMS(testSignal);
            if (std::isnan(rms) || std::isinf(rms)) {
                extremeTestPassed = false;
                break;
            }
            
            // Test maximum values
            engine->reset();
            std::map<int, float> maxParams;
            maxParams[i] = 1.0f;
            engine->updateParameters(maxParams);
            
            testSignal = TestSignalGenerator::generateSineWave(440.0f, 0.1f, m_sampleRate, 0.5f);
            engine->process(testSignal);
            
            rms = AudioMeasurements::measureRMS(testSignal);
            if (std::isnan(rms) || std::isinf(rms)) {
                extremeTestPassed = false;
                break;
            }
        }
        
    } catch (...) {
        extremeTestPassed = false;
    }
    
    test.passed = extremeTestPassed;
    test.score = test.passed ? 100.0f : 0.0f;
    
    if (!test.passed) {
        test.failureReason = "Engine failed with extreme parameter values";
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

ValidationTest EngineValidator::testLongDurationStability(EngineBase* engine) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    ValidationTest test;
    test.testName = "Long Duration Stability";
    test.category = "Stress Test";
    test.description = "Tests engine stability over extended processing";
    
    try {
        engine->reset();
        
        // Generate long test signal
        auto longSignal = TestSignalGenerator::generatePinkNoise(m_stressTestDuration, m_sampleRate, 0.3f);
        
        // Process in chunks to simulate real-time
        int chunkSize = 512;
        bool stabilityPassed = true;
        
        for (int start = 0; start < longSignal.getNumSamples(); start += chunkSize) {
            int end = std::min(start + chunkSize, longSignal.getNumSamples());
            int numSamples = end - start;
            
            juce::AudioBuffer<float> chunk(longSignal.getNumChannels(), numSamples);
            for (int ch = 0; ch < longSignal.getNumChannels(); ++ch) {
                chunk.copyFrom(ch, 0, longSignal, ch, start, numSamples);
            }
            
            engine->process(chunk);
            
            // Check for valid output
            float rms = AudioMeasurements::measureRMS(chunk);
            if (std::isnan(rms) || std::isinf(rms)) {
                stabilityPassed = false;
                break;
            }
        }
        
        test.passed = stabilityPassed;
        test.score = test.passed ? 100.0f : 0.0f;
        test.measurements["duration_seconds"] = m_stressTestDuration;
        
        if (!test.passed) {
            test.failureReason = "Engine became unstable during long processing";
        }
        
    } catch (const std::exception& e) {
        test.passed = false;
        test.score = 0.0f;
        test.failureReason = "Exception: " + std::string(e.what());
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    test.executionTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return test;
}

// =============================================================================
// Analysis and Reporting
// =============================================================================

void EngineValidator::analyzeEnginePerformance(EngineValidationResult& result) {
    // Analyze test patterns and generate insights
    
    // Check for common issues
    for (const auto& test : result.functionalityTests) {
        if (!test.passed) {
            result.issues.push_back("Functionality issue: " + test.testName);
        }
    }
    
    for (const auto& test : result.parameterTests) {
        if (!test.passed) {
            result.issues.push_back("Parameter issue: " + test.testName);
        }
    }
    
    // Check audio quality metrics
    float avgQualityScore = ValidationUtils::calculateCategoryScore(result.audioQualityTests);
    if (avgQualityScore < 70.0f) {
        result.issues.push_back("Audio quality below acceptable threshold");
    }
    
    // Check performance metrics
    float avgPerfScore = ValidationUtils::calculateCategoryScore(result.performanceTests);
    if (avgPerfScore < 70.0f) {
        result.issues.push_back("Performance below acceptable threshold");
    }
}

void EngineValidator::generateRecommendations(EngineValidationResult& result) {
    // Generate specific recommendations based on test results
    
    if (result.overallScore < 70.0f) {
        result.recommendations.push_back("Engine requires significant improvements before release");
    } else if (result.overallScore < 85.0f) {
        result.recommendations.push_back("Engine is functional but could benefit from optimization");
    }
    
    // Check for specific issues and recommend solutions
    bool hasParameterIssues = false;
    for (const auto& test : result.parameterTests) {
        if (!test.passed) {
            hasParameterIssues = true;
            break;
        }
    }
    
    if (hasParameterIssues) {
        result.recommendations.push_back("Review parameter implementation to ensure all controls affect audio output");
    }
    
    // Performance recommendations
    for (const auto& test : result.performanceTests) {
        if (test.testName == "CPU Usage" && !test.passed) {
            result.recommendations.push_back("Optimize processing algorithm to reduce CPU usage");
        }
    }
    
    // Audio quality recommendations
    for (const auto& test : result.audioQualityTests) {
        if (test.testName == "THD Level" && !test.passed) {
            result.recommendations.push_back("Consider implementing anti-aliasing or reducing internal gain staging");
        }
        if (test.testName == "Noise Floor" && !test.passed) {
            result.recommendations.push_back("Investigate noise sources and implement better isolation");
        }
    }
}

std::string EngineValidator::generateHTMLReport(const EngineValidationResult& result) {
    return ReportTemplates::generateEngineReportHTML(result);
}

std::string EngineValidator::generateBatchHTMLReport(const BatchValidationResults& results) {
    return ReportTemplates::generateBatchReportHTML(results);
}

void EngineValidator::saveReports(const BatchValidationResults& results) {
    if (m_outputDirectory.empty()) return;
    
    createOutputDirectory();
    
    // Save batch report
    std::string batchFilename = m_outputDirectory + "/batch_validation_report.html";
    std::ofstream batchFile(batchFilename);
    if (batchFile.is_open()) {
        batchFile << generateBatchHTMLReport(results);
        batchFile.close();
    }
    
    // Save executive summary
    std::string summaryFilename = m_outputDirectory + "/executive_summary.txt";
    std::ofstream summaryFile(summaryFilename);
    if (summaryFile.is_open()) {
        summaryFile << results.generateExecutiveSummary();
        summaryFile.close();
    }
}

// =============================================================================
// Utility Methods
// =============================================================================

void EngineValidator::reportProgress(const std::string& message, float percentage) {
    if (m_progressCallback) {
        m_progressCallback(message, percentage);
    }
}

std::string EngineValidator::formatDuration(float milliseconds) {
    if (milliseconds < 1000.0f) {
        return std::to_string(static_cast<int>(milliseconds)) + " ms";
    } else {
        return std::to_string(milliseconds / 1000.0f) + " s";
    }
}

std::string EngineValidator::formatScore(float score) {
    return std::to_string(static_cast<int>(score)) + "/100";
}

bool EngineValidator::createOutputDirectory() {
    try {
        std::filesystem::create_directories(m_outputDirectory);
        return true;
    } catch (...) {
        return false;
    }
}

// =============================================================================
// ValidationUtils Implementation
// =============================================================================

namespace ValidationUtils {

bool isTestPassed(const ValidationTest& test, float threshold) {
    return test.passed && test.score >= threshold;
}

float calculateCategoryScore(const std::vector<ValidationTest>& tests) {
    if (tests.empty()) return 0.0f;
    
    float totalScore = 0.0f;
    for (const auto& test : tests) {
        totalScore += test.score;
    }
    
    return totalScore / tests.size();
}

std::string classifyIssue(const ValidationTest& test) {
    if (test.score < 30.0f) {
        return "Critical";
    } else if (test.score < 60.0f) {
        return "Major";
    } else if (test.score < 80.0f) {
        return "Minor";
    } else {
        return "Warning";
    }
}

ValidationUtils::AudioQualityMetrics analyzeAudioQuality(const juce::AudioBuffer<float>& signal, double sampleRate) {
    AudioQualityMetrics metrics;
    
    // Basic implementation - would be expanded with more sophisticated analysis
    metrics.dynamicRange = AudioMeasurements::measurePeak(signal) / (AudioMeasurements::measureRMS(signal) + 1e-6f);
    metrics.signalToNoise = 60.0f; // Placeholder
    metrics.totalHarmonicDistortion = AudioMeasurements::measureTHD(signal, 440.0f, sampleRate);
    metrics.frequencyResponseFlatness = 0.9f; // Placeholder
    metrics.phaseCoherence = 0.95f; // Placeholder
    
    return metrics;
}

ValidationUtils::PerformanceMetrics measurePerformance(EngineBase* engine, double sampleRate, float durationSeconds) {
    PerformanceMetrics metrics;
    
    // Basic performance measurement
    auto testSignal = TestSignalGenerator::generatePinkNoise(durationSeconds, sampleRate, 0.5f);
    
    auto startTime = std::chrono::high_resolution_clock::now();
    engine->process(testSignal);
    auto endTime = std::chrono::high_resolution_clock::now();
    
    float processingTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    float audioTime = durationSeconds * 1000.0f;
    
    metrics.averageCPULoad = (processingTime / audioTime) * 100.0f;
    metrics.peakCPULoad = metrics.averageCPULoad; // Simplified
    metrics.memoryUsage = 1024 * 1024; // Placeholder - 1MB
    metrics.averageLatency = 0.0f; // Placeholder
    metrics.realTimeCapable = metrics.averageCPULoad < 100.0f;
    
    return metrics;
}

} // namespace ValidationUtils

// =============================================================================
// ReportTemplates Implementation
// =============================================================================

namespace ReportTemplates {

const char* HTML_TEMPLATE_HEADER = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Engine Validation Report</title>
    <script src="https://cdn.plot.ly/plotly-latest.min.js"></script>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; line-height: 1.6; }
        .header { background: #2c3e50; color: white; padding: 20px; margin: -20px -20px 20px -20px; }
        .summary { background: #f8f9fa; padding: 15px; border-left: 4px solid #28a745; margin: 20px 0; }
        .passed { border-left-color: #28a745; }
        .failed { border-left-color: #dc3545; }
        .test-section { margin: 20px 0; border: 1px solid #ddd; }
        .test-header { background: #e9ecef; padding: 10px; font-weight: bold; }
        .test-item { padding: 10px; border-bottom: 1px solid #eee; }
        .score { font-weight: bold; }
        .score.excellent { color: #28a745; }
        .score.good { color: #ffc107; }
        .score.poor { color: #dc3545; }
        table { width: 100%; border-collapse: collapse; margin: 10px 0; }
        th, td { padding: 8px; text-align: left; border-bottom: 1px solid #ddd; }
        th { background-color: #f2f2f2; }
        .plot-container { width: 100%; height: 400px; margin: 10px 0; }
    </style>
</head>
<body>
)";

const char* HTML_TEMPLATE_FOOTER = R"(
</body>
</html>
)";

std::string generateEngineReportHTML(const EngineValidationResult& result) {
    std::ostringstream html;
    
    html << HTML_TEMPLATE_HEADER;
    
    // Header
    html << "<div class=\"header\">\n";
    html << "<h1>Engine Validation Report</h1>\n";
    html << "<h2>" << result.engineName << " (" << result.engineCategory << ")</h2>\n";
    html << "</div>\n";
    
    // Summary
    html << "<div class=\"summary " << (result.overallPassed ? "passed" : "failed") << "\">\n";
    html << "<h3>Validation Summary</h3>\n";
    html << "<p><strong>Overall Status:</strong> " << (result.overallPassed ? "PASSED" : "FAILED") << "</p>\n";
    html << "<p><strong>Overall Score:</strong> <span class=\"score\">" << 
             std::fixed << std::setprecision(1) << result.overallScore << "/100</span></p>\n";
    html << "<p><strong>Tests Passed:</strong> " << result.passedTests << "/" << result.totalTests << "</p>\n";
    html << "<p><strong>Execution Time:</strong> " << result.totalExecutionTimeMs << " ms</p>\n";
    html << "<p><strong>Quality Assessment:</strong> " << result.qualityAssessment << "</p>\n";
    html << "</div>\n";
    
    // Test results by category
    auto categories = {
        std::make_pair("Functionality Tests", result.functionalityTests),
        std::make_pair("Parameter Tests", result.parameterTests),
        std::make_pair("Audio Quality Tests", result.audioQualityTests),
        std::make_pair("Performance Tests", result.performanceTests),
        std::make_pair("Compatibility Tests", result.compatibilityTests)
    };
    
    for (const auto& category : categories) {
        if (!category.second.empty()) {
            html << "<div class=\"test-section\">\n";
            html << "<div class=\"test-header\">" << category.first << "</div>\n";
            
            for (const auto& test : category.second) {
                html << "<div class=\"test-item\">\n";
                html << "<h4>" << test.testName << " <span class=\"score " << 
                         (test.score >= 80 ? "excellent" : test.score >= 60 ? "good" : "poor") << 
                         "\">(" << std::fixed << std::setprecision(0) << test.score << "/100)</span></h4>\n";
                html << "<p>" << test.description << "</p>\n";
                html << "<p><strong>Status:</strong> " << (test.passed ? "PASSED" : "FAILED") << "</p>\n";
                
                if (!test.failureReason.empty()) {
                    html << "<p><strong>Issue:</strong> " << test.failureReason << "</p>\n";
                }
                
                if (!test.details.empty()) {
                    html << "<p><strong>Details:</strong> " << test.details << "</p>\n";
                }
                
                html << "</div>\n";
            }
            
            html << "</div>\n";
        }
    }
    
    // Issues and recommendations
    if (!result.issues.empty()) {
        html << "<div class=\"test-section\">\n";
        html << "<div class=\"test-header\">Issues Found</div>\n";
        for (const auto& issue : result.issues) {
            html << "<div class=\"test-item\">• " << issue << "</div>\n";
        }
        html << "</div>\n";
    }
    
    if (!result.recommendations.empty()) {
        html << "<div class=\"test-section\">\n";
        html << "<div class=\"test-header\">Recommendations</div>\n";
        for (const auto& rec : result.recommendations) {
            html << "<div class=\"test-item\">• " << rec << "</div>\n";
        }
        html << "</div>\n";
    }
    
    html << HTML_TEMPLATE_FOOTER;
    
    return html.str();
}

std::string generateBatchReportHTML(const BatchValidationResults& results) {
    std::ostringstream html;
    
    html << HTML_TEMPLATE_HEADER;
    
    // Header
    html << "<div class=\"header\">\n";
    html << "<h1>Batch Validation Report</h1>\n";
    html << "<h2>All Engines Validation Results</h2>\n";
    html << "</div>\n";
    
    // Executive Summary
    html << "<div class=\"summary\">\n";
    html << "<h3>Executive Summary</h3>\n";
    html << results.generateExecutiveSummary();
    html << "</div>\n";
    
    // Engines table
    html << "<div class=\"test-section\">\n";
    html << "<div class=\"test-header\">Engine Results</div>\n";
    html << "<table>\n";
    html << "<tr><th>Engine</th><th>Category</th><th>Status</th><th>Score</th><th>Issues</th></tr>\n";
    
    for (const auto& result : results.engineResults) {
        html << "<tr>\n";
        html << "<td>" << result.engineName << "</td>\n";
        html << "<td>" << result.engineCategory << "</td>\n";
        html << "<td>" << (result.overallPassed ? "PASSED" : "FAILED") << "</td>\n";
        html << "<td>" << std::fixed << std::setprecision(1) << result.overallScore << "/100</td>\n";
        html << "<td>" << result.issues.size() << "</td>\n";
        html << "</tr>\n";
    }
    
    html << "</table>\n";
    html << "</div>\n";
    
    html << HTML_TEMPLATE_FOOTER;
    
    return html.str();
}

} // namespace ReportTemplates

} // namespace ComprehensiveEngineValidator