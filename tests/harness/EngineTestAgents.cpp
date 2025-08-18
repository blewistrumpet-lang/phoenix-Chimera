#include "EngineTestAgents.h"
#include "EngineFactory.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace EngineTestAgents {

// =============================================================================
// TestAgentBase Implementation
// =============================================================================

TestResult TestAgentBase::createBasicFunctionTest(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Basic Function Test";
    result.description = "Verifies engine processes audio without crashing";
    result.units = "boolean";
    
    try {
        // Prepare engine
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Generate test signal
        auto testBuffer = TestSignalGenerator::generateSineWave(1000.0f, 1.0f, sampleRate, 0.5f);
        
        // Process audio
        engine->process(testBuffer);
        
        // Check for valid output
        float rms = AudioMeasurements::measureRMS(testBuffer);
        result.passed = !std::isnan(rms) && !std::isinf(rms);
        result.measuredValue = result.passed ? 1.0f : 0.0f;
        
        if (!result.passed) {
            result.failureReason = "Engine produced invalid output (NaN or Inf)";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception thrown: " + std::string(e.what());
    }
    
    return result;
}

TestResult TestAgentBase::createParameterResponseTest(EngineBase* engine, int paramIndex, double sampleRate) {
    TestResult result;
    result.testName = "Parameter Response Test";
    result.description = "Verifies parameter " + std::to_string(paramIndex) + " affects output";
    result.units = "% change";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Generate test signal
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 0.5f, sampleRate, 0.3f);
        
        // Test with default parameter
        auto buffer1 = testSignal;
        engine->process(buffer1);
        float rms1 = AudioMeasurements::measureRMS(buffer1);
        
        // Reset and change parameter
        engine->reset();
        std::map<int, float> params;
        params[paramIndex] = 0.8f; // Set to high value
        engine->updateParameters(params);
        
        auto buffer2 = testSignal;
        engine->process(buffer2);
        float rms2 = AudioMeasurements::measureRMS(buffer2);
        
        // Calculate change
        float change = std::abs(rms2 - rms1) / (rms1 + 1e-6f) * 100.0f;
        result.measuredValue = change;
        result.expectedRange[0] = 1.0f; // Expect at least 1% change
        result.expectedRange[1] = 1000.0f; // Up to 1000% change is reasonable
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Parameter change too small: " + std::to_string(change) + "%";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult TestAgentBase::createSilenceTest(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Silence Test";
    result.description = "Verifies engine doesn't generate noise from silence";
    result.units = "dB";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Generate silence
        auto silenceBuffer = TestSignalGenerator::generateSilence(1.0f, sampleRate);
        
        // Process silence
        engine->process(silenceBuffer);
        
        // Measure output level
        float rms = AudioMeasurements::measureRMS(silenceBuffer);
        float dB = TestSignalGenerator::linearTodB(rms);
        
        result.measuredValue = dB;
        result.expectedRange[0] = -120.0f; // Very quiet
        result.expectedRange[1] = -60.0f;  // Still acceptable
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Output too loud for silence: " + std::to_string(dB) + " dB";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult TestAgentBase::createLatencyTest(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Latency Test";
    result.description = "Measures processing latency";
    result.units = "samples";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Generate impulse
        auto impulse = TestSignalGenerator::generateImpulse(sampleRate, 1.0f);
        auto originalImpulse = impulse;
        
        // Process impulse
        engine->process(impulse);
        
        // Measure latency
        float latency = AudioMeasurements::measureLatency(originalImpulse, impulse, sampleRate);
        
        result.measuredValue = latency;
        result.expectedRange[0] = 0.0f;
        result.expectedRange[1] = 2048.0f; // Up to 2048 samples is reasonable
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Latency too high: " + std::to_string(latency) + " samples";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

bool TestAgentBase::detectProcessingActivity(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output) {
    float inputRMS = AudioMeasurements::measureRMS(input);
    float outputRMS = AudioMeasurements::measureRMS(output);
    
    // Check if there's a significant change or if output exists when input doesn't
    return std::abs(outputRMS - inputRMS) > 0.001f || (inputRMS < 0.001f && outputRMS > 0.001f);
}

// =============================================================================
// DynamicsTestAgent Implementation
// =============================================================================

EngineTestSuite DynamicsTestAgent::runTests(std::unique_ptr<EngineBase> engine, double sampleRate) {
    EngineTestSuite suite;
    suite.engineName = engine->getName().toStdString();
    suite.engineType = -1; // Will be set by caller
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Run base tests
    suite.results.push_back(createBasicFunctionTest(engine.get(), sampleRate));
    suite.results.push_back(createSilenceTest(engine.get(), sampleRate));
    suite.results.push_back(createLatencyTest(engine.get(), sampleRate));
    
    // Run dynamics-specific tests
    suite.results.push_back(testGainReduction(engine.get(), sampleRate));
    suite.results.push_back(testThresholdResponse(engine.get(), sampleRate));
    suite.results.push_back(testAttackRelease(engine.get(), sampleRate));
    suite.results.push_back(testRatioResponse(engine.get(), sampleRate));
    suite.results.push_back(testMakeupGain(engine.get(), sampleRate));
    suite.results.push_back(testKneeResponse(engine.get(), sampleRate));
    
    // Test all parameters
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        suite.results.push_back(createParameterResponseTest(engine.get(), i, sampleRate));
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    suite.processingTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    suite.calculateOverallResult();
    
    return suite;
}

TestResult DynamicsTestAgent::testGainReduction(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Gain Reduction Test";
    result.description = "Tests if dynamics processor reduces gain above threshold";
    result.units = "dB";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set aggressive compression settings
        std::map<int, float> params;
        params[0] = 0.3f; // Low threshold
        params[1] = 0.8f; // High ratio
        params[2] = 0.1f; // Fast attack
        params[3] = 0.2f; // Medium release
        engine->updateParameters(params);
        
        // Generate loud signal
        auto loudSignal = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.8f);
        auto originalSignal = loudSignal;
        
        // Process signal
        engine->process(loudSignal);
        
        // Measure gain reduction
        float gainReduction = AudioMeasurements::measureGainReduction(originalSignal, loudSignal);
        
        result.measuredValue = gainReduction;
        result.expectedRange[0] = 0.5f; // At least 0.5dB reduction
        result.expectedRange[1] = 40.0f; // Up to 40dB is reasonable
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Insufficient gain reduction: " + std::to_string(gainReduction) + " dB";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DynamicsTestAgent::testThresholdResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Threshold Response Test";
    result.description = "Tests threshold parameter affects compression point";
    result.units = "dB difference";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Test with high threshold
        engine->reset();
        std::map<int, float> params1;
        params1[0] = 0.9f; // High threshold
        params1[1] = 0.5f; // Medium ratio
        engine->updateParameters(params1);
        
        auto testSignal1 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.6f);
        auto original1 = testSignal1;
        engine->process(testSignal1);
        float gr1 = AudioMeasurements::measureGainReduction(original1, testSignal1);
        
        // Test with low threshold
        engine->reset();
        std::map<int, float> params2;
        params2[0] = 0.1f; // Low threshold
        params2[1] = 0.5f; // Medium ratio
        engine->updateParameters(params2);
        
        auto testSignal2 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.6f);
        auto original2 = testSignal2;
        engine->process(testSignal2);
        float gr2 = AudioMeasurements::measureGainReduction(original2, testSignal2);
        
        // Low threshold should cause more gain reduction
        float difference = gr2 - gr1;
        result.measuredValue = difference;
        result.expectedRange[0] = 0.5f; // At least 0.5dB more reduction
        result.expectedRange[1] = 50.0f; // Up to 50dB difference
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Threshold not affecting compression: " + std::to_string(difference) + " dB";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DynamicsTestAgent::testAttackRelease(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Attack/Release Test";
    result.description = "Tests attack and release timing parameters";
    result.units = "ms";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set fast attack/release
        std::map<int, float> params;
        params[0] = 0.2f; // Medium threshold
        params[1] = 0.8f; // High ratio
        params[2] = 0.0f; // Very fast attack
        params[3] = 0.1f; // Fast release
        engine->updateParameters(params);
        
        // Generate burst signal
        auto burstSignal = TestSignalGenerator::generateBurst(0.05f, 0.1f, 0.5f, sampleRate);
        auto originalBurst = burstSignal;
        
        // Process signal
        engine->process(burstSignal);
        
        // Measure envelope timing
        auto timing = AudioMeasurements::measureEnvelopeTiming(burstSignal, sampleRate);
        
        result.measuredValue = timing.first; // Attack time
        result.expectedRange[0] = 0.1f; // At least 0.1ms
        result.expectedRange[1] = 100.0f; // Up to 100ms
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Attack time out of range: " + std::to_string(timing.first) + " ms";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DynamicsTestAgent::testRatioResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Ratio Response Test";
    result.description = "Tests compression ratio affects gain reduction amount";
    result.units = "ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Test with low ratio
        engine->reset();
        std::map<int, float> params1;
        params1[0] = 0.3f; // Low threshold
        params1[1] = 0.2f; // Low ratio
        engine->updateParameters(params1);
        
        auto testSignal1 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.8f);
        auto original1 = testSignal1;
        engine->process(testSignal1);
        float gr1 = AudioMeasurements::measureGainReduction(original1, testSignal1);
        
        // Test with high ratio
        engine->reset();
        std::map<int, float> params2;
        params2[0] = 0.3f; // Low threshold
        params2[1] = 0.9f; // High ratio
        engine->updateParameters(params2);
        
        auto testSignal2 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.8f);
        auto original2 = testSignal2;
        engine->process(testSignal2);
        float gr2 = AudioMeasurements::measureGainReduction(original2, testSignal2);
        
        // High ratio should cause more gain reduction
        float ratio = gr2 / (gr1 + 0.1f); // Avoid division by zero
        result.measuredValue = ratio;
        result.expectedRange[0] = 1.2f; // At least 20% more reduction
        result.expectedRange[1] = 10.0f; // Up to 10x more reduction
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Ratio not affecting compression: " + std::to_string(ratio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DynamicsTestAgent::testMakeupGain(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Makeup Gain Test";
    result.description = "Tests makeup gain parameter increases output level";
    result.units = "dB";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Test without makeup gain
        engine->reset();
        std::map<int, float> params1;
        if (engine->getNumParameters() > 5) {
            params1[5] = 0.0f; // No makeup gain
        }
        engine->updateParameters(params1);
        
        auto testSignal1 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.5f);
        engine->process(testSignal1);
        float rms1 = AudioMeasurements::measureRMS(testSignal1);
        
        // Test with makeup gain
        engine->reset();
        std::map<int, float> params2;
        if (engine->getNumParameters() > 5) {
            params2[5] = 0.8f; // High makeup gain
        }
        engine->updateParameters(params2);
        
        auto testSignal2 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.5f);
        engine->process(testSignal2);
        float rms2 = AudioMeasurements::measureRMS(testSignal2);
        
        float gainIncrease = TestSignalGenerator::linearTodB(rms2) - TestSignalGenerator::linearTodB(rms1);
        result.measuredValue = gainIncrease;
        result.expectedRange[0] = 1.0f; // At least 1dB increase
        result.expectedRange[1] = 40.0f; // Up to 40dB increase
        result.passed = result.isInRange() || engine->getNumParameters() <= 5; // Pass if no makeup gain param
        
        if (!result.passed) {
            result.failureReason = "Makeup gain not working: " + std::to_string(gainIncrease) + " dB";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DynamicsTestAgent::testKneeResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Knee Response Test";
    result.description = "Tests knee parameter affects compression curve smoothness";
    result.units = "smoothness factor";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Generate signal with varying levels
        auto sweepSignal = TestSignalGenerator::generateSweep(20.0f, 20000.0f, 1.0f, sampleRate, 0.6f);
        auto originalSweep = sweepSignal;
        
        // Process with knee setting if available
        std::map<int, float> params;
        if (engine->getNumParameters() > 4) {
            params[4] = 0.8f; // Soft knee
        }
        engine->updateParameters(params);
        
        engine->process(sweepSignal);
        
        // Measure smoothness of response
        float rmsOriginal = AudioMeasurements::measureRMS(originalSweep);
        float rmsProcessed = AudioMeasurements::measureRMS(sweepSignal);
        float smoothness = rmsProcessed / (rmsOriginal + 1e-6f);
        
        result.measuredValue = smoothness;
        result.expectedRange[0] = 0.1f; // Some processing occurred
        result.expectedRange[1] = 2.0f; // Not excessive gain
        result.passed = result.isInRange() || engine->getNumParameters() <= 4; // Pass if no knee param
        
        if (!result.passed) {
            result.failureReason = "Knee response abnormal: " + std::to_string(smoothness);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

// =============================================================================
// FilterTestAgent Implementation
// =============================================================================

EngineTestSuite FilterTestAgent::runTests(std::unique_ptr<EngineBase> engine, double sampleRate) {
    EngineTestSuite suite;
    suite.engineName = engine->getName().toStdString();
    suite.engineType = -1;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Run base tests
    suite.results.push_back(createBasicFunctionTest(engine.get(), sampleRate));
    suite.results.push_back(createSilenceTest(engine.get(), sampleRate));
    suite.results.push_back(createLatencyTest(engine.get(), sampleRate));
    
    // Run filter-specific tests
    suite.results.push_back(testFrequencyResponse(engine.get(), sampleRate));
    suite.results.push_back(testCutoffSweep(engine.get(), sampleRate));
    suite.results.push_back(testResonanceEffect(engine.get(), sampleRate));
    suite.results.push_back(testFilterStability(engine.get(), sampleRate));
    suite.results.push_back(testGainResponse(engine.get(), sampleRate));
    suite.results.push_back(testQualityFactor(engine.get(), sampleRate));
    
    // Test all parameters
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        suite.results.push_back(createParameterResponseTest(engine.get(), i, sampleRate));
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    suite.processingTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    suite.calculateOverallResult();
    
    return suite;
}

TestResult FilterTestAgent::testFrequencyResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Frequency Response Test";
    result.description = "Tests filter affects frequency spectrum";
    result.units = "dB range";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set filter parameters
        std::map<int, float> params;
        params[0] = 0.3f; // Mid frequency
        params[1] = 0.5f; // Medium resonance/Q
        engine->updateParameters(params);
        
        // Generate pink noise for frequency response test
        auto noiseSignal = TestSignalGenerator::generatePinkNoise(2.0f, sampleRate, 0.3f);
        auto originalNoise = noiseSignal;
        
        // Process signal
        engine->process(noiseSignal);
        
        // Compute frequency response
        auto originalSpectrum = AudioMeasurements::computeFrequencyResponse(originalNoise, sampleRate);
        auto processedSpectrum = AudioMeasurements::computeFrequencyResponse(noiseSignal, sampleRate);
        
        // Find maximum change in frequency response
        float maxChange = 0.0f;
        for (size_t i = 0; i < std::min(originalSpectrum.magnitudes.size(), processedSpectrum.magnitudes.size()); ++i) {
            float change = std::abs(processedSpectrum.magnitudes[i] - originalSpectrum.magnitudes[i]);
            maxChange = std::max(maxChange, change);
        }
        
        result.measuredValue = TestSignalGenerator::linearTodB(maxChange + 1e-6f);
        result.expectedRange[0] = 1.0f; // At least 1dB change somewhere
        result.expectedRange[1] = 60.0f; // Up to 60dB change
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Insufficient frequency response: " + std::to_string(result.measuredValue) + " dB";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult FilterTestAgent::testCutoffSweep(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Cutoff Sweep Test";
    result.description = "Tests cutoff frequency parameter sweeps correctly";
    result.units = "% response change";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Test with low cutoff
        engine->reset();
        std::map<int, float> params1;
        params1[0] = 0.1f; // Low cutoff
        engine->updateParameters(params1);
        
        auto testSignal1 = TestSignalGenerator::generateSweep(20.0f, 20000.0f, 1.0f, sampleRate, 0.3f);
        engine->process(testSignal1);
        float rms1 = AudioMeasurements::measureRMS(testSignal1);
        
        // Test with high cutoff
        engine->reset();
        std::map<int, float> params2;
        params2[0] = 0.9f; // High cutoff
        engine->updateParameters(params2);
        
        auto testSignal2 = TestSignalGenerator::generateSweep(20.0f, 20000.0f, 1.0f, sampleRate, 0.3f);
        engine->process(testSignal2);
        float rms2 = AudioMeasurements::measureRMS(testSignal2);
        
        // Calculate percentage change
        float change = std::abs(rms2 - rms1) / (rms1 + 1e-6f) * 100.0f;
        result.measuredValue = change;
        result.expectedRange[0] = 5.0f; // At least 5% change
        result.expectedRange[1] = 500.0f; // Up to 500% change
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Cutoff sweep insufficient: " + std::to_string(change) + "%";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult FilterTestAgent::testResonanceEffect(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Resonance Effect Test";
    result.description = "Tests resonance/Q parameter creates peak";
    result.units = "dB peak";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set high resonance/Q
        std::map<int, float> params;
        params[0] = 0.5f; // Mid frequency
        if (engine->getNumParameters() > 1) {
            params[1] = 0.9f; // High resonance/Q
        }
        engine->updateParameters(params);
        
        // Generate pink noise to test resonant peak
        auto noiseSignal = TestSignalGenerator::generatePinkNoise(1.0f, sampleRate, 0.3f);
        auto originalNoise = noiseSignal;
        
        // Process signal
        engine->process(noiseSignal);
        
        // Measure peak enhancement
        float originalRMS = AudioMeasurements::measureRMS(originalNoise);
        float processedRMS = AudioMeasurements::measureRMS(noiseSignal);
        float peak = AudioMeasurements::measurePeak(noiseSignal);
        
        // Calculate resonant peak
        float peakToRMS = TestSignalGenerator::linearTodB(peak / (processedRMS + 1e-6f));
        result.measuredValue = peakToRMS;
        result.expectedRange[0] = 3.0f; // At least 3dB peak
        result.expectedRange[1] = 40.0f; // Up to 40dB peak
        result.passed = result.isInRange() || engine->getNumParameters() <= 1; // Pass if no Q param
        
        if (!result.passed) {
            result.failureReason = "Resonance effect weak: " + std::to_string(peakToRMS) + " dB";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult FilterTestAgent::testFilterStability(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Filter Stability Test";
    result.description = "Tests filter doesn't oscillate or become unstable";
    result.units = "boolean";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set extreme parameters
        std::map<int, float> params;
        params[0] = 0.95f; // High frequency
        if (engine->getNumParameters() > 1) {
            params[1] = 0.95f; // High resonance
        }
        engine->updateParameters(params);
        
        // Test with impulse
        auto impulseSignal = TestSignalGenerator::generateImpulse(sampleRate, 1.0f);
        
        // Extend buffer for stability test
        juce::AudioBuffer<float> extendedBuffer(impulseSignal.getNumChannels(), static_cast<int>(sampleRate * 2)); // 2 seconds
        extendedBuffer.clear();
        
        // Copy impulse to start
        for (int ch = 0; ch < impulseSignal.getNumChannels(); ++ch) {
            extendedBuffer.copyFrom(ch, 0, impulseSignal, ch, 0, impulseSignal.getNumSamples());
        }
        
        // Process extended buffer
        engine->process(extendedBuffer);
        
        // Check for sustained oscillation
        bool hasOscillation = AudioMeasurements::detectSustainedOscillation(extendedBuffer, sampleRate);
        
        result.measuredValue = hasOscillation ? 0.0f : 1.0f;
        result.passed = !hasOscillation;
        
        if (!result.passed) {
            result.failureReason = "Filter became unstable and oscillated";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult FilterTestAgent::testGainResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Gain Response Test";
    result.description = "Tests gain parameter affects output level";
    result.units = "dB";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Find gain parameter (usually last or near end)
        int gainParam = engine->getNumParameters() - 1;
        
        // Test with low gain
        engine->reset();
        std::map<int, float> params1;
        params1[gainParam] = 0.1f; // Low gain
        engine->updateParameters(params1);
        
        auto testSignal1 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.3f);
        engine->process(testSignal1);
        float rms1 = AudioMeasurements::measureRMS(testSignal1);
        
        // Test with high gain
        engine->reset();
        std::map<int, float> params2;
        params2[gainParam] = 0.9f; // High gain
        engine->updateParameters(params2);
        
        auto testSignal2 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.3f);
        engine->process(testSignal2);
        float rms2 = AudioMeasurements::measureRMS(testSignal2);
        
        float gainChange = TestSignalGenerator::linearTodB(rms2) - TestSignalGenerator::linearTodB(rms1);
        result.measuredValue = gainChange;
        result.expectedRange[0] = 1.0f; // At least 1dB change
        result.expectedRange[1] = 60.0f; // Up to 60dB change
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Gain response insufficient: " + std::to_string(gainChange) + " dB";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult FilterTestAgent::testQualityFactor(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Quality Factor Test";
    result.description = "Tests Q parameter affects filter bandwidth";
    result.units = "bandwidth ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Test with low Q
        engine->reset();
        std::map<int, float> params1;
        params1[0] = 0.5f; // Mid frequency
        if (engine->getNumParameters() > 1) {
            params1[1] = 0.1f; // Low Q
        }
        engine->updateParameters(params1);
        
        auto noiseSignal1 = TestSignalGenerator::generatePinkNoise(1.0f, sampleRate, 0.3f);
        engine->process(noiseSignal1);
        auto spectrum1 = AudioMeasurements::computeFrequencyResponse(noiseSignal1, sampleRate);
        
        // Test with high Q
        engine->reset();
        std::map<int, float> params2;
        params2[0] = 0.5f; // Mid frequency
        if (engine->getNumParameters() > 1) {
            params2[1] = 0.9f; // High Q
        }
        engine->updateParameters(params2);
        
        auto noiseSignal2 = TestSignalGenerator::generatePinkNoise(1.0f, sampleRate, 0.3f);
        engine->process(noiseSignal2);
        auto spectrum2 = AudioMeasurements::computeFrequencyResponse(noiseSignal2, sampleRate);
        
        // Compare bandwidth (simplified: measure spectral spread)
        float spread1 = 0.0f, spread2 = 0.0f;
        for (size_t i = 0; i < spectrum1.magnitudes.size(); ++i) {
            spread1 += spectrum1.magnitudes[i];
        }
        for (size_t i = 0; i < spectrum2.magnitudes.size(); ++i) {
            spread2 += spectrum2.magnitudes[i];
        }
        
        float bandwidthRatio = spread1 / (spread2 + 1e-6f);
        result.measuredValue = bandwidthRatio;
        result.expectedRange[0] = 1.1f; // At least 10% difference
        result.expectedRange[1] = 10.0f; // Up to 10x difference
        result.passed = result.isInRange() || engine->getNumParameters() <= 1; // Pass if no Q param
        
        if (!result.passed) {
            result.failureReason = "Q factor not affecting bandwidth: " + std::to_string(bandwidthRatio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

// =============================================================================
// TimeBasedTestAgent Implementation
// =============================================================================

EngineTestSuite TimeBasedTestAgent::runTests(std::unique_ptr<EngineBase> engine, double sampleRate) {
    EngineTestSuite suite;
    suite.engineName = engine->getName().toStdString();
    suite.engineType = -1;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Run base tests
    suite.results.push_back(createBasicFunctionTest(engine.get(), sampleRate));
    suite.results.push_back(createSilenceTest(engine.get(), sampleRate));
    suite.results.push_back(createLatencyTest(engine.get(), sampleRate));
    
    // Run time-based specific tests
    suite.results.push_back(testImpulseResponse(engine.get(), sampleRate));
    suite.results.push_back(testDelayTime(engine.get(), sampleRate));
    suite.results.push_back(testFeedbackResponse(engine.get(), sampleRate));
    suite.results.push_back(testDecayTime(engine.get(), sampleRate));
    suite.results.push_back(testDryWetMix(engine.get(), sampleRate));
    suite.results.push_back(testEchoClarity(engine.get(), sampleRate));
    
    // Test all parameters
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        suite.results.push_back(createParameterResponseTest(engine.get(), i, sampleRate));
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    suite.processingTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    suite.calculateOverallResult();
    
    return suite;
}

TestResult TimeBasedTestAgent::testImpulseResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Impulse Response Test";
    result.description = "Tests impulse response characteristics";
    result.units = "ms";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set moderate parameters
        std::map<int, float> params;
        params[0] = 0.5f; // Medium time/size
        if (engine->getNumParameters() > 1) {
            params[1] = 0.3f; // Medium feedback/decay
        }
        engine->updateParameters(params);
        
        // Generate impulse and extended buffer
        auto impulse = TestSignalGenerator::generateImpulse(sampleRate, 1.0f);
        juce::AudioBuffer<float> longBuffer(impulse.getNumChannels(), static_cast<int>(sampleRate * 3)); // 3 seconds
        longBuffer.clear();
        
        // Copy impulse to start
        for (int ch = 0; ch < impulse.getNumChannels(); ++ch) {
            longBuffer.copyFrom(ch, 0, impulse, ch, 0, impulse.getNumSamples());
        }
        
        // Process
        engine->process(longBuffer);
        
        // Measure RT60 (reverb time)
        float rt60 = AudioMeasurements::measureRT60(longBuffer, sampleRate);
        
        result.measuredValue = rt60 * 1000.0f; // Convert to ms
        result.expectedRange[0] = 10.0f; // At least 10ms
        result.expectedRange[1] = 10000.0f; // Up to 10 seconds
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "RT60 out of range: " + std::to_string(result.measuredValue) + " ms";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult TimeBasedTestAgent::testDelayTime(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Delay Time Test";
    result.description = "Tests delay time parameter";
    result.units = "ms";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set delay parameters
        std::map<int, float> params;
        params[0] = 0.5f; // Medium delay time
        if (engine->getNumParameters() > 1) {
            params[1] = 0.0f; // No feedback to isolate delay
        }
        engine->updateParameters(params);
        
        // Generate impulse
        auto impulse = TestSignalGenerator::generateImpulse(sampleRate, 1.0f);
        auto originalImpulse = impulse;
        
        // Extend buffer for delay measurement
        juce::AudioBuffer<float> extendedBuffer(impulse.getNumChannels(), static_cast<int>(sampleRate * 2));
        extendedBuffer.clear();
        for (int ch = 0; ch < impulse.getNumChannels(); ++ch) {
            extendedBuffer.copyFrom(ch, 0, impulse, ch, 0, impulse.getNumSamples());
        }
        
        // Process
        engine->process(extendedBuffer);
        
        // Measure delay time
        float delayTime = AudioMeasurements::measureDelayTime(originalImpulse, extendedBuffer, sampleRate);
        
        result.measuredValue = delayTime * 1000.0f; // Convert to ms
        result.expectedRange[0] = 1.0f; // At least 1ms delay
        result.expectedRange[1] = 2000.0f; // Up to 2 seconds
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Delay time out of range: " + std::to_string(result.measuredValue) + " ms";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult TimeBasedTestAgent::testFeedbackResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Feedback Response Test";
    result.description = "Tests feedback parameter affects decay";
    result.units = "decay ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Test with low feedback
        engine->reset();
        std::map<int, float> params1;
        params1[0] = 0.3f; // Medium time
        if (engine->getNumParameters() > 1) {
            params1[1] = 0.1f; // Low feedback
        }
        engine->updateParameters(params1);
        
        auto impulse1 = TestSignalGenerator::generateImpulse(sampleRate, 1.0f);
        juce::AudioBuffer<float> buffer1(impulse1.getNumChannels(), static_cast<int>(sampleRate * 2));
        buffer1.clear();
        for (int ch = 0; ch < impulse1.getNumChannels(); ++ch) {
            buffer1.copyFrom(ch, 0, impulse1, ch, 0, impulse1.getNumSamples());
        }
        engine->process(buffer1);
        float rt60_1 = AudioMeasurements::measureRT60(buffer1, sampleRate);
        
        // Test with high feedback
        engine->reset();
        std::map<int, float> params2;
        params2[0] = 0.3f; // Medium time
        if (engine->getNumParameters() > 1) {
            params2[1] = 0.8f; // High feedback
        }
        engine->updateParameters(params2);
        
        auto impulse2 = TestSignalGenerator::generateImpulse(sampleRate, 1.0f);
        juce::AudioBuffer<float> buffer2(impulse2.getNumChannels(), static_cast<int>(sampleRate * 2));
        buffer2.clear();
        for (int ch = 0; ch < impulse2.getNumChannels(); ++ch) {
            buffer2.copyFrom(ch, 0, impulse2, ch, 0, impulse2.getNumSamples());
        }
        engine->process(buffer2);
        float rt60_2 = AudioMeasurements::measureRT60(buffer2, sampleRate);
        
        float decayRatio = rt60_2 / (rt60_1 + 1e-6f);
        result.measuredValue = decayRatio;
        result.expectedRange[0] = 1.2f; // At least 20% longer decay
        result.expectedRange[1] = 20.0f; // Up to 20x longer
        result.passed = result.isInRange() || engine->getNumParameters() <= 1; // Pass if no feedback param
        
        if (!result.passed) {
            result.failureReason = "Feedback not affecting decay: " + std::to_string(decayRatio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult TimeBasedTestAgent::testDecayTime(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Decay Time Test";
    result.description = "Tests decay time parameter";
    result.units = "ms";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters for decay test
        std::map<int, float> params;
        if (engine->getNumParameters() > 2) {
            params[2] = 0.6f; // Decay time parameter
        }
        engine->updateParameters(params);
        
        // Generate impulse
        auto impulse = TestSignalGenerator::generateImpulse(sampleRate, 1.0f);
        juce::AudioBuffer<float> longBuffer(impulse.getNumChannels(), static_cast<int>(sampleRate * 5)); // 5 seconds
        longBuffer.clear();
        
        for (int ch = 0; ch < impulse.getNumChannels(); ++ch) {
            longBuffer.copyFrom(ch, 0, impulse, ch, 0, impulse.getNumSamples());
        }
        
        engine->process(longBuffer);
        
        // Measure actual decay time
        float decayTime = AudioMeasurements::measureRT60(longBuffer, sampleRate);
        
        result.measuredValue = decayTime * 1000.0f; // Convert to ms
        result.expectedRange[0] = 50.0f; // At least 50ms
        result.expectedRange[1] = 20000.0f; // Up to 20 seconds
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Decay time out of range: " + std::to_string(result.measuredValue) + " ms";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult TimeBasedTestAgent::testDryWetMix(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Dry/Wet Mix Test";
    result.description = "Tests dry/wet mix parameter";
    result.units = "mix ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Find mix parameter (often last parameter)
        int mixParam = engine->getNumParameters() - 1;
        
        // Test with dry signal (mix = 0)
        engine->reset();
        std::map<int, float> params1;
        params1[mixParam] = 0.0f; // Dry
        engine->updateParameters(params1);
        
        auto testSignal1 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.3f);
        auto original1 = testSignal1;
        engine->process(testSignal1);
        
        // Test with wet signal (mix = 1)
        engine->reset();
        std::map<int, float> params2;
        params2[mixParam] = 1.0f; // Wet
        engine->updateParameters(params2);
        
        auto testSignal2 = TestSignalGenerator::generateSineWave(1000.0f, 0.5f, sampleRate, 0.3f);
        auto original2 = testSignal2;
        engine->process(testSignal2);
        
        // Compare correlation with original (dry should be more correlated)
        float corr1 = AudioMeasurements::measureSNR(original1, testSignal1);
        float corr2 = AudioMeasurements::measureSNR(original2, testSignal2);
        
        float mixRatio = corr1 / (corr2 + 1e-6f);
        result.measuredValue = mixRatio;
        result.expectedRange[0] = 1.1f; // Dry should be more correlated
        result.expectedRange[1] = 100.0f; // Up to 100x more correlated
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Mix parameter not working: " + std::to_string(mixRatio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult TimeBasedTestAgent::testEchoClarity(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Echo Clarity Test";
    result.description = "Tests echo clarity and definition";
    result.units = "clarity index";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters for clear echo
        std::map<int, float> params;
        params[0] = 0.4f; // Medium delay
        if (engine->getNumParameters() > 1) {
            params[1] = 0.3f; // Moderate feedback
        }
        engine->updateParameters(params);
        
        // Generate transient signal
        auto drumHit = TestSignalGenerator::generateDrumHit(sampleRate);
        juce::AudioBuffer<float> extendedBuffer(drumHit.getNumChannels(), static_cast<int>(sampleRate * 2));
        extendedBuffer.clear();
        
        for (int ch = 0; ch < drumHit.getNumChannels(); ++ch) {
            extendedBuffer.copyFrom(ch, 0, drumHit, ch, 0, drumHit.getNumSamples());
        }
        
        engine->process(extendedBuffer);
        
        // Measure echo clarity (simplified: measure transient definition)
        float peak = AudioMeasurements::measurePeak(extendedBuffer);
        float rms = AudioMeasurements::measureRMS(extendedBuffer);
        float clarity = peak / (rms + 1e-6f);
        
        result.measuredValue = clarity;
        result.expectedRange[0] = 1.5f; // Some transient definition
        result.expectedRange[1] = 50.0f; // Very clear transients
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Echo clarity poor: " + std::to_string(clarity);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

// =============================================================================
// ModulationTestAgent Implementation
// =============================================================================

EngineTestSuite ModulationTestAgent::runTests(std::unique_ptr<EngineBase> engine, double sampleRate) {
    EngineTestSuite suite;
    suite.engineName = engine->getName().toStdString();
    suite.engineType = -1;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Run base tests
    suite.results.push_back(createBasicFunctionTest(engine.get(), sampleRate));
    suite.results.push_back(createSilenceTest(engine.get(), sampleRate));
    suite.results.push_back(createLatencyTest(engine.get(), sampleRate));
    
    // Run modulation-specific tests
    suite.results.push_back(testLFORate(engine.get(), sampleRate));
    suite.results.push_back(testModulationDepth(engine.get(), sampleRate));
    suite.results.push_back(testStereoWidth(engine.get(), sampleRate));
    suite.results.push_back(testModulationShape(engine.get(), sampleRate));
    suite.results.push_back(testPhaseResponse(engine.get(), sampleRate));
    suite.results.push_back(testChorusVoices(engine.get(), sampleRate));
    
    // Test all parameters
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        suite.results.push_back(createParameterResponseTest(engine.get(), i, sampleRate));
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    suite.processingTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    suite.calculateOverallResult();
    
    return suite;
}

TestResult ModulationTestAgent::testLFORate(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "LFO Rate Test";
    result.description = "Tests LFO rate parameter affects modulation speed";
    result.units = "Hz";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set moderate depth, vary rate
        std::map<int, float> params;
        params[0] = 0.7f; // High rate
        params[1] = 0.5f; // Medium depth
        engine->updateParameters(params);
        
        // Generate sustained tone
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 2.0f, sampleRate, 0.5f);
        engine->process(testSignal);
        
        // Extract modulation characteristics
        auto modProfile = AudioMeasurements::extractModulationProfile(testSignal, sampleRate);
        
        result.measuredValue = modProfile.rate;
        result.expectedRange[0] = 0.1f; // At least 0.1Hz
        result.expectedRange[1] = 50.0f; // Up to 50Hz
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "LFO rate out of range: " + std::to_string(modProfile.rate) + " Hz";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult ModulationTestAgent::testModulationDepth(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Modulation Depth Test";
    result.description = "Tests modulation depth parameter";
    result.units = "depth ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Test with low depth
        engine->reset();
        std::map<int, float> params1;
        params1[0] = 0.5f; // Medium rate
        if (engine->getNumParameters() > 1) {
            params1[1] = 0.1f; // Low depth
        }
        engine->updateParameters(params1);
        
        auto testSignal1 = TestSignalGenerator::generateSineWave(440.0f, 2.0f, sampleRate, 0.5f);
        engine->process(testSignal1);
        auto modProfile1 = AudioMeasurements::extractModulationProfile(testSignal1, sampleRate);
        
        // Test with high depth
        engine->reset();
        std::map<int, float> params2;
        params2[0] = 0.5f; // Medium rate
        if (engine->getNumParameters() > 1) {
            params2[1] = 0.9f; // High depth
        }
        engine->updateParameters(params2);
        
        auto testSignal2 = TestSignalGenerator::generateSineWave(440.0f, 2.0f, sampleRate, 0.5f);
        engine->process(testSignal2);
        auto modProfile2 = AudioMeasurements::extractModulationProfile(testSignal2, sampleRate);
        
        float depthRatio = modProfile2.depth / (modProfile1.depth + 1e-6f);
        result.measuredValue = depthRatio;
        result.expectedRange[0] = 1.5f; // At least 50% more depth
        result.expectedRange[1] = 20.0f; // Up to 20x more depth
        result.passed = result.isInRange() || engine->getNumParameters() <= 1; // Pass if no depth param
        
        if (!result.passed) {
            result.failureReason = "Modulation depth not changing: " + std::to_string(depthRatio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult ModulationTestAgent::testStereoWidth(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Stereo Width Test";
    result.description = "Tests stereo width of modulation effect";
    result.units = "correlation";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters for stereo effect
        std::map<int, float> params;
        params[0] = 0.5f; // Medium rate
        if (engine->getNumParameters() > 1) {
            params[1] = 0.7f; // High depth
        }
        if (engine->getNumParameters() > 2) {
            params[2] = 0.8f; // Stereo width parameter
        }
        engine->updateParameters(params);
        
        // Generate stereo test signal
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 2.0f, sampleRate, 0.5f);
        if (testSignal.getNumChannels() == 1) {
            // Make stereo
            juce::AudioBuffer<float> stereoSignal(2, testSignal.getNumSamples());
            stereoSignal.copyFrom(0, 0, testSignal, 0, 0, testSignal.getNumSamples());
            stereoSignal.copyFrom(1, 0, testSignal, 0, 0, testSignal.getNumSamples());
            testSignal = std::move(stereoSignal);
        }
        
        engine->process(testSignal);
        
        // Measure correlation between channels
        float correlation = 0.0f;
        if (testSignal.getNumChannels() >= 2) {
            correlation = AudioMeasurements::correlate(
                testSignal.getReadPointer(0), 
                testSignal.getReadPointer(1), 
                testSignal.getNumSamples()
            );
        }
        
        result.measuredValue = 1.0f - std::abs(correlation); // Lower correlation = wider stereo
        result.expectedRange[0] = 0.05f; // Some stereo width
        result.expectedRange[1] = 1.0f; // Full range
        result.passed = result.isInRange() || testSignal.getNumChannels() < 2; // Pass if mono
        
        if (!result.passed) {
            result.failureReason = "Stereo width insufficient: " + std::to_string(result.measuredValue);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult ModulationTestAgent::testModulationShape(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Modulation Shape Test";
    result.description = "Tests modulation waveform shape";
    result.units = "shape factor";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters for clear modulation
        std::map<int, float> params;
        params[0] = 0.2f; // Slow rate for clear measurement
        if (engine->getNumParameters() > 1) {
            params[1] = 0.8f; // High depth
        }
        if (engine->getNumParameters() > 3) {
            params[3] = 0.7f; // Waveform shape parameter
        }
        engine->updateParameters(params);
        
        // Generate test signal
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 5.0f, sampleRate, 0.5f);
        engine->process(testSignal);
        
        // Analyze modulation waveform shape
        auto modProfile = AudioMeasurements::extractModulationProfile(testSignal, sampleRate);
        
        result.measuredValue = modProfile.depth;
        result.expectedRange[0] = 0.1f; // Some modulation present
        result.expectedRange[1] = 1.0f; // Full modulation range
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Modulation shape unclear: " + std::to_string(modProfile.depth);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult ModulationTestAgent::testPhaseResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Phase Response Test";
    result.description = "Tests phase relationships in modulation";
    result.units = "radians";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters
        std::map<int, float> params;
        params[0] = 0.5f; // Medium rate
        if (engine->getNumParameters() > 1) {
            params[1] = 0.6f; // Medium depth
        }
        engine->updateParameters(params);
        
        // Generate test signal
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 2.0f, sampleRate, 0.5f);
        auto originalSignal = testSignal;
        engine->process(testSignal);
        
        // Compute phase response
        auto originalSpectrum = AudioMeasurements::computeFrequencyResponse(originalSignal, sampleRate);
        auto processedSpectrum = AudioMeasurements::computeFrequencyResponse(testSignal, sampleRate);
        
        // Measure phase difference
        float maxPhaseDiff = 0.0f;
        for (size_t i = 0; i < std::min(originalSpectrum.phases.size(), processedSpectrum.phases.size()); ++i) {
            float phaseDiff = std::abs(processedSpectrum.phases[i] - originalSpectrum.phases[i]);
            maxPhaseDiff = std::max(maxPhaseDiff, phaseDiff);
        }
        
        result.measuredValue = maxPhaseDiff;
        result.expectedRange[0] = 0.1f; // Some phase shift
        result.expectedRange[1] = 3.14f; // Up to pi radians
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Phase response out of range: " + std::to_string(maxPhaseDiff) + " rad";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult ModulationTestAgent::testChorusVoices(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Chorus Voices Test";
    result.description = "Tests chorus voices parameter";
    result.units = "voice count";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters for chorus
        std::map<int, float> params;
        params[0] = 0.3f; // Medium rate
        if (engine->getNumParameters() > 1) {
            params[1] = 0.5f; // Medium depth
        }
        if (engine->getNumParameters() > 4) {
            params[4] = 0.8f; // Many voices
        }
        engine->updateParameters(params);
        
        // Generate test signal
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 2.0f, sampleRate, 0.5f);
        engine->process(testSignal);
        
        // Measure spectral complexity as proxy for voice count
        auto spectrum = AudioMeasurements::computeFrequencyResponse(testSignal, sampleRate);
        
        // Count peaks in spectrum
        int peakCount = 0;
        for (size_t i = 1; i < spectrum.magnitudes.size() - 1; ++i) {
            if (spectrum.magnitudes[i] > spectrum.magnitudes[i-1] && 
                spectrum.magnitudes[i] > spectrum.magnitudes[i+1] &&
                spectrum.magnitudes[i] > 0.01f) {
                peakCount++;
            }
        }
        
        result.measuredValue = static_cast<float>(peakCount);
        result.expectedRange[0] = 1.0f; // At least 1 peak
        result.expectedRange[1] = 20.0f; // Up to 20 peaks
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Chorus voice count: " + std::to_string(peakCount);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

// =============================================================================
// DistortionTestAgent Implementation
// =============================================================================

EngineTestSuite DistortionTestAgent::runTests(std::unique_ptr<EngineBase> engine, double sampleRate) {
    EngineTestSuite suite;
    suite.engineName = engine->getName().toStdString();
    suite.engineType = -1;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Run base tests
    suite.results.push_back(createBasicFunctionTest(engine.get(), sampleRate));
    suite.results.push_back(createSilenceTest(engine.get(), sampleRate));
    suite.results.push_back(createLatencyTest(engine.get(), sampleRate));
    
    // Run distortion-specific tests
    suite.results.push_back(testHarmonicGeneration(engine.get(), sampleRate));
    suite.results.push_back(testTHDMeasurement(engine.get(), sampleRate));
    suite.results.push_back(testSaturationCurve(engine.get(), sampleRate));
    suite.results.push_back(testDriveResponse(engine.get(), sampleRate));
    suite.results.push_back(testToneShaping(engine.get(), sampleRate));
    suite.results.push_back(testOverdriveCharacter(engine.get(), sampleRate));
    
    // Test all parameters
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        suite.results.push_back(createParameterResponseTest(engine.get(), i, sampleRate));
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    suite.processingTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    suite.calculateOverallResult();
    
    return suite;
}

TestResult DistortionTestAgent::testHarmonicGeneration(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Harmonic Generation Test";
    result.description = "Tests generation of harmonic content";
    result.units = "harmonic count";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set drive for harmonic generation
        std::map<int, float> params;
        params[0] = 0.7f; // High drive
        engine->updateParameters(params);
        
        // Generate clean sine wave
        auto testSignal = TestSignalGenerator::generateSineWave(220.0f, 1.0f, sampleRate, 0.6f);
        engine->process(testSignal);
        
        // Measure harmonic content
        auto harmonics = AudioMeasurements::measureHarmonicContent(testSignal, 220.0f, sampleRate);
        
        result.measuredValue = static_cast<float>(harmonics.harmonicAmplitudes.size());
        result.expectedRange[0] = 2.0f; // At least 2 harmonics (fundamental + 1 harmonic)
        result.expectedRange[1] = 20.0f; // Up to 20 harmonics
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Insufficient harmonics: " + std::to_string(harmonics.harmonicAmplitudes.size());
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DistortionTestAgent::testTHDMeasurement(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "THD Measurement Test";
    result.description = "Tests total harmonic distortion";
    result.units = "% THD";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set moderate drive
        std::map<int, float> params;
        params[0] = 0.6f; // Medium drive
        engine->updateParameters(params);
        
        // Generate sine wave
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 1.0f, sampleRate, 0.5f);
        engine->process(testSignal);
        
        // Measure THD
        float thd = AudioMeasurements::measureTHD(testSignal, 440.0f, sampleRate);
        
        result.measuredValue = thd * 100.0f; // Convert to percentage
        result.expectedRange[0] = 0.1f; // At least 0.1% THD
        result.expectedRange[1] = 50.0f; // Up to 50% THD
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "THD out of range: " + std::to_string(result.measuredValue) + "%";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DistortionTestAgent::testSaturationCurve(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Saturation Curve Test";
    result.description = "Tests saturation response curve";
    result.units = "compression ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set high drive for saturation
        std::map<int, float> params;
        params[0] = 0.9f; // High drive
        engine->updateParameters(params);
        
        // Test with low amplitude
        auto lowSignal = TestSignalGenerator::generateSineWave(440.0f, 0.5f, sampleRate, 0.2f);
        auto lowOriginal = lowSignal;
        engine->process(lowSignal);
        float lowRMS = AudioMeasurements::measureRMS(lowSignal);
        float lowOriginalRMS = AudioMeasurements::measureRMS(lowOriginal);
        
        // Reset and test with high amplitude
        engine->reset();
        engine->updateParameters(params);
        auto highSignal = TestSignalGenerator::generateSineWave(440.0f, 0.5f, sampleRate, 0.8f);
        auto highOriginal = highSignal;
        engine->process(highSignal);
        float highRMS = AudioMeasurements::measureRMS(highSignal);
        float highOriginalRMS = AudioMeasurements::measureRMS(highOriginal);
        
        // Calculate compression ratio
        float lowGain = lowRMS / (lowOriginalRMS + 1e-6f);
        float highGain = highRMS / (highOriginalRMS + 1e-6f);
        float compressionRatio = lowGain / (highGain + 1e-6f);
        
        result.measuredValue = compressionRatio;
        result.expectedRange[0] = 1.1f; // Some saturation
        result.expectedRange[1] = 10.0f; // Strong saturation
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Saturation curve abnormal: " + std::to_string(compressionRatio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DistortionTestAgent::testDriveResponse(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Drive Response Test";
    result.description = "Tests drive parameter affects distortion amount";
    result.units = "THD ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        
        // Test with low drive
        engine->reset();
        std::map<int, float> params1;
        params1[0] = 0.1f; // Low drive
        engine->updateParameters(params1);
        
        auto testSignal1 = TestSignalGenerator::generateSineWave(440.0f, 1.0f, sampleRate, 0.5f);
        engine->process(testSignal1);
        float thd1 = AudioMeasurements::measureTHD(testSignal1, 440.0f, sampleRate);
        
        // Test with high drive
        engine->reset();
        std::map<int, float> params2;
        params2[0] = 0.9f; // High drive
        engine->updateParameters(params2);
        
        auto testSignal2 = TestSignalGenerator::generateSineWave(440.0f, 1.0f, sampleRate, 0.5f);
        engine->process(testSignal2);
        float thd2 = AudioMeasurements::measureTHD(testSignal2, 440.0f, sampleRate);
        
        float thdRatio = thd2 / (thd1 + 1e-6f);
        result.measuredValue = thdRatio;
        result.expectedRange[0] = 1.5f; // At least 50% more distortion
        result.expectedRange[1] = 100.0f; // Up to 100x more distortion
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Drive not affecting distortion: " + std::to_string(thdRatio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DistortionTestAgent::testToneShaping(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Tone Shaping Test";
    result.description = "Tests tone/filter parameters";
    result.units = "frequency response";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set tone parameters if available
        std::map<int, float> params;
        params[0] = 0.5f; // Medium drive
        if (engine->getNumParameters() > 1) {
            params[1] = 0.8f; // Bright tone
        }
        engine->updateParameters(params);
        
        // Generate broadband signal
        auto testSignal = TestSignalGenerator::generatePinkNoise(1.0f, sampleRate, 0.3f);
        auto originalSignal = testSignal;
        engine->process(testSignal);
        
        // Compare frequency response
        auto originalSpectrum = AudioMeasurements::computeFrequencyResponse(originalSignal, sampleRate);
        auto processedSpectrum = AudioMeasurements::computeFrequencyResponse(testSignal, sampleRate);
        
        // Calculate frequency response difference
        float maxDifference = 0.0f;
        for (size_t i = 0; i < std::min(originalSpectrum.magnitudes.size(), processedSpectrum.magnitudes.size()); ++i) {
            float diff = std::abs(processedSpectrum.magnitudes[i] - originalSpectrum.magnitudes[i]);
            maxDifference = std::max(maxDifference, diff);
        }
        
        result.measuredValue = TestSignalGenerator::linearTodB(maxDifference + 1e-6f);
        result.expectedRange[0] = 1.0f; // At least 1dB change
        result.expectedRange[1] = 40.0f; // Up to 40dB change
        result.passed = result.isInRange() || engine->getNumParameters() <= 1; // Pass if no tone param
        
        if (!result.passed) {
            result.failureReason = "Tone shaping insufficient: " + std::to_string(result.measuredValue) + " dB";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult DistortionTestAgent::testOverdriveCharacter(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Overdrive Character Test";
    result.description = "Tests overdrive character and warmth";
    result.units = "harmonic ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters for overdrive character
        std::map<int, float> params;
        params[0] = 0.6f; // Medium drive
        if (engine->getNumParameters() > 2) {
            params[2] = 0.7f; // Character/warmth parameter
        }
        engine->updateParameters(params);
        
        // Generate test signal
        auto testSignal = TestSignalGenerator::generateSineWave(220.0f, 1.0f, sampleRate, 0.5f);
        engine->process(testSignal);
        
        // Measure harmonic content for character analysis
        auto harmonics = AudioMeasurements::measureHarmonicContent(testSignal, 220.0f, sampleRate);
        
        // Calculate even/odd harmonic ratio (character indicator)
        float evenHarmonics = 0.0f;
        float oddHarmonics = 0.0f;
        
        for (size_t i = 0; i < harmonics.harmonicAmplitudes.size(); ++i) {
            if (i % 2 == 0) {
                evenHarmonics += harmonics.harmonicAmplitudes[i];
            } else {
                oddHarmonics += harmonics.harmonicAmplitudes[i];
            }
        }
        
        float harmonicRatio = (evenHarmonics + oddHarmonics) > 0 ? 
                             evenHarmonics / (evenHarmonics + oddHarmonics) : 0.0f;
        
        result.measuredValue = harmonicRatio;
        result.expectedRange[0] = 0.1f; // Some harmonic content
        result.expectedRange[1] = 0.9f; // Reasonable distribution
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Overdrive character abnormal: " + std::to_string(harmonicRatio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

// =============================================================================
// SpectralTestAgent Implementation
// =============================================================================

EngineTestSuite SpectralTestAgent::runTests(std::unique_ptr<EngineBase> engine, double sampleRate) {
    EngineTestSuite suite;
    suite.engineName = engine->getName().toStdString();
    suite.engineType = -1;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Run base tests
    suite.results.push_back(createBasicFunctionTest(engine.get(), sampleRate));
    suite.results.push_back(createSilenceTest(engine.get(), sampleRate));
    suite.results.push_back(createLatencyTest(engine.get(), sampleRate));
    
    // Run spectral-specific tests
    suite.results.push_back(testFrequencyShifting(engine.get(), sampleRate));
    suite.results.push_back(testPitchShifting(engine.get(), sampleRate));
    suite.results.push_back(testFormantPreservation(engine.get(), sampleRate));
    suite.results.push_back(testSpectralResolution(engine.get(), sampleRate));
    suite.results.push_back(testArtifactLevel(engine.get(), sampleRate));
    suite.results.push_back(testTransientHandling(engine.get(), sampleRate));
    
    // Test all parameters
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        suite.results.push_back(createParameterResponseTest(engine.get(), i, sampleRate));
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    suite.processingTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    suite.calculateOverallResult();
    
    return suite;
}

TestResult SpectralTestAgent::testFrequencyShifting(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Frequency Shifting Test";
    result.description = "Tests frequency shifting accuracy";
    result.units = "Hz";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set frequency shift parameters
        std::map<int, float> params;
        params[0] = 0.6f; // Frequency shift amount
        engine->updateParameters(params);
        
        // Generate sine wave
        float inputFreq = 440.0f;
        auto testSignal = TestSignalGenerator::generateSineWave(inputFreq, 2.0f, sampleRate, 0.5f);
        engine->process(testSignal);
        
        // Find peak frequency in output
        auto spectrum = AudioMeasurements::computeFrequencyResponse(testSignal, sampleRate);
        float peakFreq = AudioMeasurements::findPeakFrequency(spectrum.magnitudes, sampleRate);
        
        // Calculate shift amount
        float shift = std::abs(peakFreq - inputFreq);
        result.measuredValue = shift;
        result.expectedRange[0] = 10.0f; // At least 10Hz shift
        result.expectedRange[1] = 1000.0f; // Up to 1000Hz shift
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Frequency shift insufficient: " + std::to_string(shift) + " Hz";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult SpectralTestAgent::testPitchShifting(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Pitch Shifting Test";
    result.description = "Tests pitch shifting preserves harmonic relationships";
    result.units = "semitones";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set pitch shift parameters
        std::map<int, float> params;
        params[0] = 0.7f; // Pitch shift up
        engine->updateParameters(params);
        
        // Generate harmonic content
        auto testSignal = TestSignalGenerator::generateChord(261.63f, 2.0f, sampleRate); // C major
        auto originalSignal = testSignal;
        engine->process(testSignal);
        
        // Analyze harmonic relationships
        auto originalSpectrum = AudioMeasurements::computeFrequencyResponse(originalSignal, sampleRate);
        auto shiftedSpectrum = AudioMeasurements::computeFrequencyResponse(testSignal, sampleRate);
        
        // Find fundamental frequency shift
        float originalPeak = AudioMeasurements::findPeakFrequency(originalSpectrum.magnitudes, sampleRate);
        float shiftedPeak = AudioMeasurements::findPeakFrequency(shiftedSpectrum.magnitudes, sampleRate);
        
        // Calculate semitones
        float ratio = shiftedPeak / originalPeak;
        float semitones = 12.0f * std::log2(ratio);
        
        result.measuredValue = std::abs(semitones);
        result.expectedRange[0] = 0.5f; // At least 0.5 semitones
        result.expectedRange[1] = 24.0f; // Up to 2 octaves
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Pitch shift out of range: " + std::to_string(semitones) + " semitones";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult SpectralTestAgent::testFormantPreservation(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Formant Preservation Test";
    result.description = "Tests formant preservation in pitch shifting";
    result.units = "preservation factor";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters for formant preservation
        std::map<int, float> params;
        params[0] = 0.8f; // Large pitch shift
        if (engine->getNumParameters() > 1) {
            params[1] = 0.9f; // High formant preservation
        }
        engine->updateParameters(params);
        
        // Generate signal with formants (speech-like)
        auto testSignal = TestSignalGenerator::generateChord(150.0f, 2.0f, sampleRate);
        auto originalSignal = testSignal;
        engine->process(testSignal);
        
        // Analyze formant structure
        auto originalSpectrum = AudioMeasurements::computeFrequencyResponse(originalSignal, sampleRate);
        auto processedSpectrum = AudioMeasurements::computeFrequencyResponse(testSignal, sampleRate);
        
        // Measure spectral envelope similarity
        float similarity = 0.0f;
        int validBins = 0;
        for (size_t i = 0; i < std::min(originalSpectrum.magnitudes.size(), processedSpectrum.magnitudes.size()); ++i) {
            if (originalSpectrum.magnitudes[i] > 0.01f) {
                float ratio = processedSpectrum.magnitudes[i] / originalSpectrum.magnitudes[i];
                similarity += ratio;
                validBins++;
            }
        }
        
        if (validBins > 0) {
            similarity /= validBins;
        }
        
        result.measuredValue = similarity;
        result.expectedRange[0] = 0.3f; // Some preservation
        result.expectedRange[1] = 3.0f; // Reasonable range
        result.passed = result.isInRange() || engine->getNumParameters() <= 1; // Pass if no formant param
        
        if (!result.passed) {
            result.failureReason = "Formant preservation poor: " + std::to_string(similarity);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult SpectralTestAgent::testSpectralResolution(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Spectral Resolution Test";
    result.description = "Tests spectral processing resolution";
    result.units = "resolution factor";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters
        std::map<int, float> params;
        params[0] = 0.5f; // Medium processing
        if (engine->getNumParameters() > 2) {
            params[2] = 0.8f; // High resolution
        }
        engine->updateParameters(params);
        
        // Generate two-tone signal for resolution test
        auto testSignal = TestSignalGenerator::generateTwoTone(440.0f, 460.0f, 2.0f, sampleRate);
        auto originalSignal = testSignal;
        engine->process(testSignal);
        
        // Analyze spectral resolution
        auto originalSpectrum = AudioMeasurements::computeFrequencyResponse(originalSignal, sampleRate);
        auto processedSpectrum = AudioMeasurements::computeFrequencyResponse(testSignal, sampleRate);
        
        // Measure spectral clarity (peak-to-noise ratio)
        float maxMagnitude = *std::max_element(processedSpectrum.magnitudes.begin(), processedSpectrum.magnitudes.end());
        float avgMagnitude = 0.0f;
        for (float mag : processedSpectrum.magnitudes) {
            avgMagnitude += mag;
        }
        avgMagnitude /= processedSpectrum.magnitudes.size();
        
        float clarity = maxMagnitude / (avgMagnitude + 1e-6f);
        result.measuredValue = clarity;
        result.expectedRange[0] = 2.0f; // Some spectral focus
        result.expectedRange[1] = 100.0f; // High resolution
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Spectral resolution poor: " + std::to_string(clarity);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult SpectralTestAgent::testArtifactLevel(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Artifact Level Test";
    result.description = "Tests for processing artifacts";
    result.units = "dB SNR";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set moderate parameters
        std::map<int, float> params;
        params[0] = 0.5f; // Medium processing
        engine->updateParameters(params);
        
        // Generate clean sine wave
        auto testSignal = TestSignalGenerator::generateSineWave(440.0f, 2.0f, sampleRate, 0.3f);
        auto originalSignal = testSignal;
        engine->process(testSignal);
        
        // Measure artifacts as noise floor
        float noiseFloor = AudioMeasurements::measureNoiseFloor(testSignal);
        float signalLevel = AudioMeasurements::measureRMS(testSignal);
        float snr = TestSignalGenerator::linearTodB(signalLevel / (noiseFloor + 1e-6f));
        
        result.measuredValue = snr;
        result.expectedRange[0] = 20.0f; // At least 20dB SNR
        result.expectedRange[1] = 120.0f; // Up to 120dB SNR
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Artifact level too high: " + std::to_string(snr) + " dB SNR";
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

TestResult SpectralTestAgent::testTransientHandling(EngineBase* engine, double sampleRate) {
    TestResult result;
    result.testName = "Transient Handling Test";
    result.description = "Tests transient preservation in spectral processing";
    result.units = "transient ratio";
    
    try {
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();
        
        // Set parameters
        std::map<int, float> params;
        params[0] = 0.6f; // Medium processing
        engine->updateParameters(params);
        
        // Generate signal with transients
        auto testSignal = TestSignalGenerator::generateDrumHit(sampleRate);
        auto originalSignal = testSignal;
        engine->process(testSignal);
        
        // Measure transient preservation
        float originalPeak = AudioMeasurements::measurePeak(originalSignal);
        float processedPeak = AudioMeasurements::measurePeak(testSignal);
        
        float originalRMS = AudioMeasurements::measureRMS(originalSignal);
        float processedRMS = AudioMeasurements::measureRMS(testSignal);
        
        float originalCrest = originalPeak / (originalRMS + 1e-6f);
        float processedCrest = processedPeak / (processedRMS + 1e-6f);
        
        float transientRatio = processedCrest / (originalCrest + 1e-6f);
        result.measuredValue = transientRatio;
        result.expectedRange[0] = 0.3f; // Some transient preservation
        result.expectedRange[1] = 3.0f; // Reasonable range
        result.passed = result.isInRange();
        
        if (!result.passed) {
            result.failureReason = "Transient handling poor: " + std::to_string(transientRatio);
        }
    }
    catch (const std::exception& e) {
        result.passed = false;
        result.failureReason = "Exception: " + std::string(e.what());
    }
    
    return result;
}

// =============================================================================
// TestAgentFactory Implementation
// =============================================================================

std::unique_ptr<TestAgentBase> TestAgentFactory::createTestAgent(int engineType) {
    if (isDynamicsEffect(engineType)) {
        return std::make_unique<DynamicsTestAgent>();
    } else if (isFilterEffect(engineType)) {
        return std::make_unique<FilterTestAgent>();
    } else if (isTimeBasedEffect(engineType)) {
        return std::make_unique<TimeBasedTestAgent>();
    } else if (isModulationEffect(engineType)) {
        return std::make_unique<ModulationTestAgent>();
    } else if (isDistortionEffect(engineType)) {
        return std::make_unique<DistortionTestAgent>();
    } else if (isSpectralEffect(engineType)) {
        return std::make_unique<SpectralTestAgent>();
    } else {
        // Default to filter test agent for unknown types
        return std::make_unique<FilterTestAgent>();
    }
}

std::string TestAgentFactory::getEffectCategoryName(int engineType) {
    if (isDynamicsEffect(engineType)) return "Dynamics";
    if (isFilterEffect(engineType)) return "Filter/EQ";
    if (isTimeBasedEffect(engineType)) return "Time-based";
    if (isModulationEffect(engineType)) return "Modulation";
    if (isDistortionEffect(engineType)) return "Distortion";
    if (isSpectralEffect(engineType)) return "Spectral";
    return "Unknown";
}

bool TestAgentFactory::isDynamicsEffect(int engineType) {
    return engineType == ENGINE_OPTO_COMPRESSOR ||
           engineType == ENGINE_VCA_COMPRESSOR ||
           engineType == ENGINE_MASTERING_LIMITER ||
           engineType == ENGINE_NOISE_GATE ||
           engineType == ENGINE_TRANSIENT_SHAPER;
}

bool TestAgentFactory::isFilterEffect(int engineType) {
    return engineType == ENGINE_PARAMETRIC_EQ ||
           engineType == ENGINE_VINTAGE_CONSOLE_EQ ||
           engineType == ENGINE_LADDER_FILTER ||
           engineType == ENGINE_STATE_VARIABLE_FILTER ||
           engineType == ENGINE_FORMANT_FILTER ||
           engineType == ENGINE_ENVELOPE_FILTER ||
           engineType == ENGINE_COMB_RESONATOR ||
           engineType == ENGINE_DYNAMIC_EQ;
}

bool TestAgentFactory::isTimeBasedEffect(int engineType) {
    return engineType == ENGINE_TAPE_ECHO ||
           engineType == ENGINE_DIGITAL_DELAY ||
           engineType == ENGINE_BUCKET_BRIGADE_DELAY ||
           engineType == ENGINE_MAGNETIC_DRUM_ECHO ||
           engineType == ENGINE_SHIMMER_REVERB ||
           engineType == ENGINE_PLATE_REVERB ||
           engineType == ENGINE_SPRING_REVERB ||
           engineType == ENGINE_CONVOLUTION_REVERB ||
           engineType == ENGINE_GATED_REVERB;
}

bool TestAgentFactory::isModulationEffect(int engineType) {
    return engineType == ENGINE_DIGITAL_CHORUS ||
           engineType == ENGINE_RESONANT_CHORUS ||
           engineType == ENGINE_ANALOG_PHASER ||
           engineType == ENGINE_CLASSIC_TREMOLO ||
           engineType == ENGINE_HARMONIC_TREMOLO ||
           engineType == ENGINE_RING_MODULATOR ||
           engineType == ENGINE_DIMENSION_EXPANDER ||
           engineType == ENGINE_ROTARY_SPEAKER ||
           engineType == ENGINE_STEREO_WIDENER ||
           engineType == ENGINE_STEREO_IMAGER;
}

bool TestAgentFactory::isDistortionEffect(int engineType) {
    return engineType == ENGINE_VINTAGE_TUBE ||
           engineType == ENGINE_WAVE_FOLDER ||
           engineType == ENGINE_HARMONIC_EXCITER ||
           engineType == ENGINE_BIT_CRUSHER ||
           engineType == ENGINE_MULTIBAND_SATURATOR ||
           engineType == ENGINE_MUFF_FUZZ ||
           engineType == ENGINE_RODENT_DISTORTION ||
           engineType == ENGINE_K_STYLE;
}

bool TestAgentFactory::isSpectralEffect(int engineType) {
    return engineType == ENGINE_PITCH_SHIFTER ||
           engineType == ENGINE_FREQUENCY_SHIFTER ||
           engineType == ENGINE_PHASED_VOCODER ||
           engineType == ENGINE_SPECTRAL_FREEZE ||
           engineType == ENGINE_SPECTRAL_GATE ||
           engineType == ENGINE_VOCAL_FORMANT ||
           engineType == ENGINE_GRANULAR_CLOUD ||
           engineType == ENGINE_INTELLIGENT_HARMONIZER ||
           engineType == ENGINE_DETUNE_DOUBLER;
}

// =============================================================================
// TestUtils Implementation
// =============================================================================

namespace TestUtils {

bool validateParameterRange(float value, float min, float max) {
    return value >= min && value <= max;
}

float calculatePercentageChange(float original, float modified) {
    return std::abs(modified - original) / (std::abs(original) + 1e-6f) * 100.0f;
}

std::vector<float> generateParameterSweep(float min, float max, int steps) {
    std::vector<float> values;
    values.reserve(steps);
    
    for (int i = 0; i < steps; ++i) {
        float t = static_cast<float>(i) / (steps - 1);
        values.push_back(min + t * (max - min));
    }
    
    return values;
}

juce::AudioBuffer<float> generateTestSignalForCategory(const std::string& category, double sampleRate) {
    if (category == "Dynamics") {
        return TestSignalGenerator::generateSineWave(1000.0f, 1.0f, sampleRate, 0.7f);
    } else if (category == "Filter/EQ") {
        return TestSignalGenerator::generatePinkNoise(2.0f, sampleRate, 0.3f);
    } else if (category == "Time-based") {
        return TestSignalGenerator::generateImpulse(sampleRate, 1.0f);
    } else if (category == "Modulation") {
        return TestSignalGenerator::generateSineWave(440.0f, 2.0f, sampleRate, 0.5f);
    } else if (category == "Distortion") {
        return TestSignalGenerator::generateSineWave(220.0f, 1.0f, sampleRate, 0.6f);
    } else if (category == "Spectral") {
        return TestSignalGenerator::generateChord(261.63f, 2.0f, sampleRate); // C major chord
    } else {
        return TestSignalGenerator::generateSineWave(1000.0f, 1.0f, sampleRate, 0.5f);
    }
}

bool isSignificantChange(float before, float after, float threshold) {
    return std::abs(after - before) / (std::abs(before) + 1e-6f) > threshold;
}

std::string formatMeasurement(float value, const std::string& units, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << value << " " << units;
    return oss.str();
}

} // namespace TestUtils

} // namespace EngineTestAgents