#include "ComprehensiveTestHarness.h"
#include <future>
#include <thread>
#include <chrono>

namespace ChimeraTestHarness {

    // Parameter Sweep Tests
    TestCategory ComprehensiveTestHarness::runParameterSweepTests(EngineBase* engine, int engineID) {
        TestCategory category("Parameter Sweep Tests");
        
        printProgressUpdate(engineID, engine->getName().toStdString(), "Running parameter sweep tests");
        
        int numParams = engine->getNumParameters();
        if (numParams == 0) {
            TestResult noParamsResult("Parameter Count Check");
            noParamsResult.setFail(Severity::WARNING, 
                                 "Engine has no parameters to test",
                                 {"Verify that the engine should have parameters",
                                  "Check getNumParameters() implementation"});
            category.addResult(noParamsResult);
            return category;
        }
        
        // Test each parameter
        for (int paramIndex = 0; paramIndex < numParams; ++paramIndex) {
            TestResult paramResult = testParameterSweep(engine, paramIndex);
            category.addResult(paramResult);
        }
        
        // Overall parameter functionality test
        TestResult overallParamTest("Overall Parameter Functionality");
        try {
            // Test setting all parameters to various values
            std::map<int, float> testParams;
            for (int i = 0; i < numParams; ++i) {
                testParams[i] = 0.5f; // Middle values
            }
            
            engine->updateParameters(testParams);
            
            // Process some audio to ensure no crashes
            auto testBuffer = m_signalCache.at("silence");
            juce::AudioBuffer<float> processBuffer = testBuffer;
            engine->process(processBuffer);
            
            if (ComprehensiveSignalGenerator::containsNanOrInf(processBuffer)) {
                overallParamTest.setFail(Severity::ERROR,
                                       "Parameters caused NaN/Inf output",
                                       {"Check parameter validation in updateParameters()",
                                        "Add bounds checking for parameter values",
                                        "Ensure internal calculations handle edge cases"});
            } else {
                overallParamTest.setPass("All parameters updated successfully");
            }
            
        } catch (const std::exception& e) {
            overallParamTest.setFail(Severity::ERROR,
                                   "Exception during parameter update: " + std::string(e.what()),
                                   {"Add proper error handling in updateParameters()",
                                    "Check for null pointer access",
                                    "Validate parameter indices"});
        }
        
        category.addResult(overallParamTest);
        
        return category;
    }
    
    TestResult ComprehensiveTestHarness::testParameterSweep(EngineBase* engine, int paramIndex) {
        std::string paramName = engine->getParameterName(paramIndex).toStdString();
        TestResult result("Parameter Sweep: " + paramName);
        
        try {
            auto testSignal = m_signalCache.at("sine_440");
            
            // Generate parameter sweep values
            auto sweepValues = ComprehensiveSignalGenerator::generateParameterSweep(m_parameterSweepSteps);
            
            std::vector<float> outputRMS;
            outputRMS.reserve(sweepValues.size());
            
            bool hasVariation = false;
            float baselineRMS = 0.0f;
            
            for (size_t i = 0; i < sweepValues.size(); ++i) {
                // Reset engine state
                resetEngine(engine);
                
                // Set parameter value
                std::map<int, float> params;
                params[paramIndex] = sweepValues[i];
                engine->updateParameters(params);
                
                // Process test signal
                juce::AudioBuffer<float> processBuffer = testSignal;
                engine->process(processBuffer);
                
                // Check for NaN/Inf
                if (ComprehensiveSignalGenerator::containsNanOrInf(processBuffer)) {
                    result.setFail(Severity::ERROR,
                                 "Parameter value " + std::to_string(sweepValues[i]) + " caused NaN/Inf output",
                                 {"Add parameter bounds checking",
                                  "Fix mathematical operations that can produce NaN/Inf",
                                  "Add input validation"});
                    return result;
                }
                
                // Calculate RMS
                float rms = ComprehensiveSignalGenerator::calculateRMS(processBuffer);
                outputRMS.push_back(rms);
                
                if (i == 0) {
                    baselineRMS = rms;
                } else {
                    float diff = std::abs(rms - baselineRMS);
                    if (diff > 0.01f) { // 1% threshold for meaningful variation
                        hasVariation = true;
                    }
                }
            }
            
            // Analyze sweep results
            result.metrics["min_rms"] = *std::min_element(outputRMS.begin(), outputRMS.end());
            result.metrics["max_rms"] = *std::max_element(outputRMS.begin(), outputRMS.end());
            result.metrics["range"] = result.metrics["max_rms"] - result.metrics["min_rms"];
            result.metrics["baseline_rms"] = baselineRMS;
            
            if (!hasVariation && result.metrics["range"] < 0.001f) {
                result.setFail(Severity::WARNING,
                             "Parameter appears to have no effect on output",
                             {"Verify parameter is connected to processing",
                              "Check if parameter affects internal state",
                              "Consider if parameter only affects other parameters"});
                result.score = 30.0f; // Partial score for working but ineffective parameter
            } else {
                result.setPass("Parameter sweep completed successfully with variation");
                result.score = 100.0f;
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during parameter sweep: " + std::string(e.what()),
                         {"Fix runtime errors in parameter handling",
                          "Add proper error handling",
                          "Check for array bounds violations"});
        }
        
        return result;
    }
    
    // Safety Tests
    TestCategory ComprehensiveTestHarness::runSafetyTests(EngineBase* engine, int engineID) {
        TestCategory category("Safety Tests");
        
        printProgressUpdate(engineID, engine->getName().toStdString(), "Running safety tests");
        
        // NaN/Inf safety test
        TestResult nanInfResult = testNanInfSafety(engine);
        category.addResult(nanInfResult);
        
        // Buffer overrun safety test
        TestResult bufferResult = testBufferOverrunSafety(engine);
        category.addResult(bufferResult);
        
        // Thread safety test
        TestResult threadResult = testThreadSafety(engine);
        category.addResult(threadResult);
        
        return category;
    }
    
    TestResult ComprehensiveTestHarness::testNanInfSafety(EngineBase* engine) {
        TestResult result("NaN/Inf Safety");
        
        try {
            resetEngine(engine);
            
            // Test with various problematic inputs
            std::vector<std::pair<std::string, float>> testCases = {
                {"NaN input", std::numeric_limits<float>::quiet_NaN()},
                {"Positive infinity", std::numeric_limits<float>::infinity()},
                {"Negative infinity", -std::numeric_limits<float>::infinity()},
                {"Very large values", 1e10f},
                {"Very small values", 1e-10f},
                {"Denormal values", 1e-40f}
            };
            
            bool allPassed = true;
            std::string failureDetails;
            
            for (const auto& [description, testValue] : testCases) {
                // Create buffer with test value
                juce::AudioBuffer<float> testBuffer(2, m_blockSize);
                testBuffer.clear();
                
                if (std::isfinite(testValue)) {
                    // Fill buffer with test value
                    for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch) {
                        juce::FloatVectorOperations::fill(testBuffer.getWritePointer(ch), testValue, m_blockSize);
                    }
                } else {
                    // For NaN/Inf, just set first sample
                    testBuffer.setSample(0, 0, testValue);
                    testBuffer.setSample(1, 0, testValue);
                }
                
                // Process the buffer
                engine->process(testBuffer);
                
                // Check output for NaN/Inf
                if (ComprehensiveSignalGenerator::containsNanOrInf(testBuffer)) {
                    allPassed = false;
                    failureDetails += description + " produced NaN/Inf output; ";
                }
            }
            
            if (allPassed) {
                result.setPass("Engine handles problematic input values safely");
            } else {
                result.setFail(Severity::CRITICAL,
                             "Engine produces NaN/Inf output: " + failureDetails,
                             {"Add input sanitization to process() method",
                              "Check all mathematical operations for edge cases",
                              "Use std::isfinite() to validate intermediate calculations",
                              "Replace divisions with safe alternatives when denominator might be zero"});
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::CRITICAL,
                         "Exception during NaN/Inf testing: " + std::string(e.what()),
                         {"Add proper exception handling",
                          "Fix memory access violations",
                          "Check for null pointer dereferences"});
        }
        
        return result;
    }
    
    TestResult ComprehensiveTestHarness::testBufferOverrunSafety(EngineBase* engine) {
        TestResult result("Buffer Overrun Safety");
        
        try {
            resetEngine(engine);
            
            // Test with various buffer sizes
            std::vector<int> testSizes = {1, 64, 128, 512, 1024, 2048, 4096};
            bool allPassed = true;
            std::string failureDetails;
            
            for (int testSize : testSizes) {
                if (testSize == m_blockSize) continue; // Skip the size we prepared with
                
                // Create buffer of different size
                juce::AudioBuffer<float> testBuffer(2, testSize);
                testBuffer.clear();
                
                // Fill with test signal
                for (int ch = 0; ch < testBuffer.getNumChannels(); ++ch) {
                    auto* channelData = testBuffer.getWritePointer(ch);
                    for (int i = 0; i < testSize; ++i) {
                        float phase = (float(i) / m_sampleRate) * 440.0f * juce::MathConstants<float>::twoPi;
                        channelData[i] = 0.5f * std::sin(phase);
                    }
                }
                
                try {
                    engine->process(testBuffer);
                    
                    // Verify buffer hasn't been corrupted
                    if (ComprehensiveSignalGenerator::containsNanOrInf(testBuffer)) {
                        allPassed = false;
                        failureDetails += "Size " + std::to_string(testSize) + " caused NaN/Inf; ";
                    }
                    
                } catch (...) {
                    allPassed = false;
                    failureDetails += "Size " + std::to_string(testSize) + " caused exception; ";
                }
            }
            
            if (allPassed) {
                result.setPass("Engine handles various buffer sizes safely");
            } else {
                result.setFail(Severity::ERROR,
                             "Engine has buffer size issues: " + failureDetails,
                             {"Don't assume specific buffer sizes in process() method",
                              "Use buffer.getNumSamples() instead of fixed sizes",
                              "Add bounds checking for all buffer access",
                              "Handle dynamic buffer size changes gracefully"});
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during buffer testing: " + std::string(e.what()),
                         {"Fix buffer access violations",
                          "Add proper bounds checking",
                          "Handle edge cases in buffer processing"});
        }
        
        return result;
    }
    
    TestResult ComprehensiveTestHarness::testThreadSafety(EngineBase* engine) {
        TestResult result("Thread Safety");
        
        try {
            resetEngine(engine);
            
            // Prepare test data
            auto testBuffer1 = m_signalCache.at("sine_440");
            auto testBuffer2 = m_signalCache.at("white_noise");
            
            std::atomic<bool> hasError{false};
            std::atomic<int> completedTests{0};
            const int numThreads = 4;
            const int testsPerThread = 10;
            
            auto threadTest = [&](int threadId) {
                try {
                    for (int i = 0; i < testsPerThread && !hasError.load(); ++i) {
                        // Create separate buffer for this thread
                        juce::AudioBuffer<float> localBuffer = (threadId % 2 == 0) ? testBuffer1 : testBuffer2;
                        
                        // Simulate concurrent parameter changes
                        if (engine->getNumParameters() > 0) {
                            std::map<int, float> params;
                            params[0] = float(i) / testsPerThread;
                            engine->updateParameters(params);
                        }
                        
                        // Process audio
                        engine->process(localBuffer);
                        
                        // Check for corruption
                        if (ComprehensiveSignalGenerator::containsNanOrInf(localBuffer)) {
                            hasError.store(true);
                            return;
                        }
                        
                        completedTests.fetch_add(1);
                        
                        // Small delay to increase chances of race conditions
                        std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                } catch (...) {
                    hasError.store(true);
                }
            };
            
            // Launch threads
            std::vector<std::thread> threads;
            for (int i = 0; i < numThreads; ++i) {
                threads.emplace_back(threadTest, i);
            }
            
            // Wait for completion with timeout
            auto startTime = std::chrono::steady_clock::now();
            for (auto& thread : threads) {
                thread.join();
            }
            auto endTime = std::chrono::steady_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            
            if (hasError.load()) {
                result.setFail(Severity::WARNING,
                             "Thread safety issues detected",
                             {"Add proper synchronization to shared state",
                              "Use atomic operations for parameter updates",
                              "Consider thread-local storage for processing state",
                              "Add mutex protection for critical sections"});
            } else if (completedTests.load() < numThreads * testsPerThread) {
                result.setFail(Severity::WARNING,
                             "Some thread tests failed to complete",
                             {"Check for deadlocks in engine implementation",
                              "Ensure all operations can complete in reasonable time",
                              "Review thread synchronization logic"});
            } else {
                result.setPass("Thread safety tests passed");
                result.metrics["test_duration_ms"] = static_cast<float>(duration.count());
                result.metrics["completed_tests"] = static_cast<float>(completedTests.load());
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::WARNING,
                         "Exception during thread safety testing: " + std::string(e.what()),
                         {"Review thread safety implementation",
                          "Add proper exception handling for concurrent access"});
        }
        
        return result;
    }
    
    // Audio Quality Tests
    TestCategory ComprehensiveTestHarness::runAudioQualityTests(EngineBase* engine, int engineID) {
        TestCategory category("Audio Quality Tests");
        
        printProgressUpdate(engineID, engine->getName().toStdString(), "Running audio quality tests");
        
        // Sine wave response test
        TestResult sineResult = testSineWaveResponse(engine);
        category.addResult(sineResult);
        
        // Noise response test
        TestResult noiseResult = testNoiseResponse(engine);
        category.addResult(noiseResult);
        
        // Transient response test
        TestResult transientResult = testTransientResponse(engine);
        category.addResult(transientResult);
        
        return category;
    }
    
    TestResult ComprehensiveTestHarness::testSineWaveResponse(EngineBase* engine, float frequency) {
        TestResult result("Sine Wave Response (" + std::to_string(int(frequency)) + "Hz)");
        
        try {
            resetEngine(engine);
            
            // Generate sine wave
            auto sineWave = ComprehensiveSignalGenerator::generateSignal(
                ComprehensiveSignalGenerator::SignalType::SINE_WAVE,
                m_sampleRate, m_testDuration, 0.5f,
                {{"frequency", frequency}});
            
            // Store original for comparison
            auto originalBuffer = sineWave;
            
            // Process through engine
            engine->process(sineWave);
            
            // Analyze output
            if (ComprehensiveSignalGenerator::containsNanOrInf(sineWave)) {
                result.setFail(Severity::CRITICAL,
                             "Sine wave processing produced NaN/Inf",
                             {"Fix numerical instabilities in processing",
                              "Add input validation",
                              "Check for divide-by-zero errors"});
                return result;
            }
            
            float inputRMS = ComprehensiveSignalGenerator::calculateRMS(originalBuffer);
            float outputRMS = ComprehensiveSignalGenerator::calculateRMS(sineWave);
            float outputPeak = ComprehensiveSignalGenerator::calculatePeak(sineWave);
            
            result.metrics["input_rms"] = inputRMS;
            result.metrics["output_rms"] = outputRMS;
            result.metrics["output_peak"] = outputPeak;
            result.metrics["gain_change_db"] = 20.0f * std::log10(outputRMS / (inputRMS + 1e-10f));
            
            // Check for excessive gain
            if (outputPeak > 1.0f) {
                result.setFail(Severity::WARNING,
                             "Output exceeds full scale (peak: " + std::to_string(outputPeak) + ")",
                             {"Add output limiting/clipping protection",
                              "Check gain staging in processing chain",
                              "Ensure parameters don't cause excessive amplification"});
            }
            // Check for complete silence (might indicate broken processing)
            else if (outputRMS < 1e-6f && inputRMS > 1e-3f) {
                result.setFail(Severity::ERROR,
                             "Engine appears to be completely muting input",
                             {"Check if engine is properly initialized",
                              "Verify processing chain is connected",
                              "Check for bypassed processing state"});
            }
            else {
                result.setPass("Sine wave processed without issues");
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during sine wave testing: " + std::string(e.what()),
                         {"Fix runtime errors in audio processing",
                          "Add proper error handling"});
        }
        
        return result;
    }
    
    TestResult ComprehensiveTestHarness::testNoiseResponse(EngineBase* engine) {
        TestResult result("Noise Response");
        
        try {
            resetEngine(engine);
            
            auto noiseBuffer = m_signalCache.at("white_noise");
            auto originalBuffer = noiseBuffer;
            
            engine->process(noiseBuffer);
            
            if (ComprehensiveSignalGenerator::containsNanOrInf(noiseBuffer)) {
                result.setFail(Severity::CRITICAL,
                             "Noise processing produced NaN/Inf",
                             {"Fix numerical instabilities with noisy input",
                              "Add proper signal conditioning"});
                return result;
            }
            
            float inputRMS = ComprehensiveSignalGenerator::calculateRMS(originalBuffer);
            float outputRMS = ComprehensiveSignalGenerator::calculateRMS(noiseBuffer);
            float outputPeak = ComprehensiveSignalGenerator::calculatePeak(noiseBuffer);
            float crestFactor = ComprehensiveSignalGenerator::calculateCrestFactor(noiseBuffer);
            
            result.metrics["input_rms"] = inputRMS;
            result.metrics["output_rms"] = outputRMS;
            result.metrics["output_peak"] = outputPeak;
            result.metrics["crest_factor"] = crestFactor;
            
            // Analyze noise handling characteristics
            if (outputPeak > 1.2f) {
                result.setFail(Severity::ERROR,
                             "Excessive peak levels with noise input: " + std::to_string(outputPeak),
                             {"Add peak limiting for noisy signals",
                              "Check for gain issues with high crest factor signals"});
            }
            else if (crestFactor > 50.0f) {
                result.setFail(Severity::WARNING,
                             "Very high crest factor may indicate processing artifacts",
                             {"Check for transient artifacts",
                              "Verify noise handling doesn't create spikes"});
            }
            else {
                result.setPass("Noise processed appropriately");
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during noise testing: " + std::string(e.what()),
                         {"Fix runtime errors with noisy input"});
        }
        
        return result;
    }
    
    TestResult ComprehensiveTestHarness::testTransientResponse(EngineBase* engine) {
        TestResult result("Transient Response");
        
        try {
            resetEngine(engine);
            
            // Test with impulse response
            auto impulseBuffer = m_signalCache.at("impulse");
            auto originalBuffer = impulseBuffer;
            
            engine->process(impulseBuffer);
            
            if (ComprehensiveSignalGenerator::containsNanOrInf(impulseBuffer)) {
                result.setFail(Severity::CRITICAL,
                             "Impulse processing produced NaN/Inf",
                             {"Fix handling of impulse/transient signals",
                              "Check for numerical instabilities with sudden changes"});
                return result;
            }
            
            float outputPeak = ComprehensiveSignalGenerator::calculatePeak(impulseBuffer);
            float outputRMS = ComprehensiveSignalGenerator::calculateRMS(impulseBuffer);
            
            result.metrics["output_peak"] = outputPeak;
            result.metrics["output_rms"] = outputRMS;
            
            // Check transient handling
            if (outputPeak > 2.0f) {
                result.setFail(Severity::WARNING,
                             "Impulse caused excessive output level: " + std::to_string(outputPeak),
                             {"Add transient limiting",
                              "Check impulse response for instability",
                              "Verify filter stability with sharp transients"});
            }
            else {
                result.setPass("Transients handled appropriately");
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during transient testing: " + std::string(e.what()),
                         {"Fix runtime errors with transient input"});
        }
        
        return result;
    }

} // namespace ChimeraTestHarness