#include "ComprehensiveTestHarness.h"
#include <future>
#include <thread>
#include <chrono>

namespace ChimeraTestHarness {

    // Performance Tests
    TestCategory ComprehensiveTestHarness::runPerformanceTests(EngineBase* engine, int engineID) {
        TestCategory category("Performance Tests");
        
        printProgressUpdate(engineID, engine->getName().toStdString(), "Running performance tests");
        
        // CPU usage test
        TestResult cpuResult = testCpuUsage(engine);
        category.addResult(cpuResult);
        
        // Latency test
        TestResult latencyResult = testLatency(engine);
        category.addResult(latencyResult);
        
        return category;
    }
    
    TestResult ComprehensiveTestHarness::testCpuUsage(EngineBase* engine) {
        TestResult result("CPU Usage");
        
        try {
            resetEngine(engine);
            
            auto testBuffer = m_signalCache.at("sine_440");
            
            // Warm up
            for (int i = 0; i < 10; ++i) {
                juce::AudioBuffer<float> warmupBuffer = testBuffer;
                engine->process(warmupBuffer);
            }
            
            // Measure processing time over multiple iterations
            const int numIterations = 100;
            std::vector<std::chrono::nanoseconds> processingTimes;
            processingTimes.reserve(numIterations);
            
            for (int i = 0; i < numIterations; ++i) {
                juce::AudioBuffer<float> processBuffer = testBuffer;
                
                auto measurement = PerformanceMeasurer::measureProcessingTime(
                    [&]() { engine->process(processBuffer); },
                    m_sampleRate, m_blockSize
                );
                
                processingTimes.push_back(measurement.processingTime);
                
                // Check for NaN/Inf output during performance testing
                if (ComprehensiveSignalGenerator::containsNanOrInf(processBuffer)) {
                    result.setFail(Severity::CRITICAL,
                                 "Engine produced NaN/Inf during CPU testing",
                                 {"Fix numerical instabilities",
                                  "Check for optimization-related bugs"});
                    return result;
                }
            }
            
            // Calculate statistics
            auto minTime = *std::min_element(processingTimes.begin(), processingTimes.end());
            auto maxTime = *std::max_element(processingTimes.begin(), processingTimes.end());
            
            auto totalTime = std::accumulate(processingTimes.begin(), processingTimes.end(), 
                                           std::chrono::nanoseconds{0});
            auto avgTime = totalTime / numIterations;
            
            float minCpu = PerformanceMeasurer::calculateCpuPercentage(minTime, m_blockSize, m_sampleRate);
            float maxCpu = PerformanceMeasurer::calculateCpuPercentage(maxTime, m_blockSize, m_sampleRate);
            float avgCpu = PerformanceMeasurer::calculateCpuPercentage(avgTime, m_blockSize, m_sampleRate);
            
            result.metrics["min_cpu_percent"] = minCpu;
            result.metrics["max_cpu_percent"] = maxCpu;
            result.metrics["avg_cpu_percent"] = avgCpu;
            result.metrics["min_time_ns"] = static_cast<float>(minTime.count());
            result.metrics["max_time_ns"] = static_cast<float>(maxTime.count());
            result.metrics["avg_time_ns"] = static_cast<float>(avgTime.count());
            
            bool realTimeCapable = PerformanceMeasurer::isRealTimeCapable(maxTime, m_blockSize, m_sampleRate);
            result.metrics["real_time_capable"] = realTimeCapable ? 1.0f : 0.0f;
            
            // Assess CPU usage
            if (maxCpu > 80.0f) {
                result.setFail(Severity::CRITICAL,
                             "Excessive CPU usage: " + std::to_string(maxCpu) + "%",
                             {"Optimize processing algorithms",
                              "Reduce computational complexity",
                              "Consider using lookup tables for expensive operations",
                              "Profile code to identify bottlenecks"});
            }
            else if (maxCpu > 50.0f) {
                result.setFail(Severity::WARNING,
                             "High CPU usage: " + std::to_string(maxCpu) + "%",
                             {"Consider optimization opportunities",
                              "Monitor CPU usage under different parameter settings",
                              "Check for unnecessary calculations"});
                result.score = 60.0f;
            }
            else if (!realTimeCapable) {
                result.setFail(Severity::ERROR,
                             "Not real-time capable at current settings",
                             {"Optimize critical processing paths",
                              "Reduce algorithm complexity",
                              "Consider approximate algorithms for heavy computations"});
            }
            else {
                result.setPass("CPU usage within acceptable limits");
                result.score = std::max(0.0f, 100.0f - avgCpu); // Higher score for lower CPU usage
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during CPU testing: " + std::string(e.what()),
                         {"Fix runtime errors in processing loop"});
        }
        
        return result;
    }
    
    TestResult ComprehensiveTestHarness::testLatency(EngineBase* engine) {
        TestResult result("Latency");
        
        try {
            resetEngine(engine);
            
            // Create impulse signal
            juce::AudioBuffer<float> impulseBuffer(2, m_blockSize * 4); // Longer buffer to catch delays
            impulseBuffer.clear();
            impulseBuffer.setSample(0, 0, 1.0f);  // Impulse at start
            impulseBuffer.setSample(1, 0, 1.0f);
            
            // Process the impulse
            engine->process(impulseBuffer);
            
            // Find the first significant output sample
            int latencySamples = -1;
            float threshold = 0.01f; // 1% of input level
            
            for (int channel = 0; channel < impulseBuffer.getNumChannels(); ++channel) {
                const auto* channelData = impulseBuffer.getReadPointer(channel);
                
                for (int sample = 1; sample < impulseBuffer.getNumSamples(); ++sample) {
                    if (std::abs(channelData[sample]) > threshold) {
                        if (latencySamples < 0 || sample < latencySamples) {
                            latencySamples = sample;
                        }
                        break;
                    }
                }
            }
            
            float latencyMs = 0.0f;
            if (latencySamples >= 0) {
                latencyMs = (latencySamples * 1000.0f) / static_cast<float>(m_sampleRate);
            }
            
            result.metrics["latency_samples"] = static_cast<float>(latencySamples);
            result.metrics["latency_ms"] = latencyMs;
            
            // Assess latency
            if (latencySamples < 0) {
                // No output detected - might be completely processed away or very high latency
                float outputPeak = ComprehensiveSignalGenerator::calculatePeak(impulseBuffer);
                if (outputPeak < 1e-6f) {
                    result.setFail(Severity::WARNING,
                                 "Impulse completely attenuated - cannot measure latency",
                                 {"Check if engine processes impulses correctly",
                                  "Verify engine is not completely muting input",
                                  "Consider if this is expected behavior"});
                } else {
                    result.setFail(Severity::WARNING,
                                 "Latency too high to measure with current buffer size",
                                 {"Check for excessive algorithmic delay",
                                  "Verify processing doesn't introduce unnecessary delays"});
                }
            }
            else if (latencyMs > 50.0f) {
                result.setFail(Severity::ERROR,
                             "Excessive latency: " + std::to_string(latencyMs) + "ms",
                             {"Reduce algorithmic delay",
                              "Optimize processing to minimize latency",
                              "Consider real-time processing requirements"});
            }
            else if (latencyMs > 20.0f) {
                result.setFail(Severity::WARNING,
                             "High latency: " + std::to_string(latencyMs) + "ms",
                             {"Consider reducing processing latency",
                              "Check if latency is necessary for algorithm"});
                result.score = 70.0f;
            }
            else {
                result.setPass("Latency within acceptable limits: " + std::to_string(latencyMs) + "ms");
                result.score = std::max(0.0f, 100.0f - latencyMs * 2); // Lower latency = higher score
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during latency testing: " + std::string(e.what()),
                         {"Fix runtime errors in impulse processing"});
        }
        
        return result;
    }
    
    // Stability Tests
    TestCategory ComprehensiveTestHarness::runStabilityTests(EngineBase* engine, int engineID) {
        TestCategory category("Stability Tests");
        
        printProgressUpdate(engineID, engine->getName().toStdString(), "Running stability tests");
        
        // Mix parameter linearity test
        TestResult mixResult = testMixParameterLinearity(engine);
        category.addResult(mixResult);
        
        // Rapid parameter changes test
        TestResult rapidResult = testRapidParameterChanges(engine);
        category.addResult(rapidResult);
        
        // Bypass stability test
        TestResult bypassResult = testBypassStability(engine);
        category.addResult(bypassResult);
        
        return category;
    }
    
    TestResult ComprehensiveTestHarness::testMixParameterLinearity(EngineBase* engine) {
        TestResult result("Mix Parameter Linearity");
        
        try {
            resetEngine(engine);
            
            // Look for a parameter that might be a mix/blend parameter
            // Common names: "Mix", "Blend", "Dry/Wet", "Amount", etc.
            int mixParamIndex = -1;
            int numParams = engine->getNumParameters();
            
            for (int i = 0; i < numParams; ++i) {
                std::string paramName = engine->getParameterName(i).toStdString();
                std::transform(paramName.begin(), paramName.end(), paramName.begin(), ::tolower);
                
                if (paramName.find("mix") != std::string::npos ||
                    paramName.find("blend") != std::string::npos ||
                    paramName.find("wet") != std::string::npos ||
                    paramName.find("dry") != std::string::npos ||
                    paramName.find("amount") != std::string::npos) {
                    mixParamIndex = i;
                    break;
                }
            }
            
            if (mixParamIndex < 0) {
                result.setFail(Severity::INFO,
                             "No obvious mix parameter found - test skipped",
                             {"Consider adding a mix/blend parameter if appropriate",
                              "Ensure parameter names clearly indicate their function"});
                result.score = 80.0f; // Not a failure, just no mix parameter
                return result;
            }
            
            auto testSignal = m_signalCache.at("sine_440");
            
            // Test linearity at different mix values
            std::vector<float> mixValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
            std::vector<float> outputLevels;
            
            for (float mixValue : mixValues) {
                resetEngine(engine);
                
                std::map<int, float> params;
                params[mixParamIndex] = mixValue;
                engine->updateParameters(params);
                
                juce::AudioBuffer<float> processBuffer = testSignal;
                engine->process(processBuffer);
                
                if (ComprehensiveSignalGenerator::containsNanOrInf(processBuffer)) {
                    result.setFail(Severity::ERROR,
                                 "Mix parameter value " + std::to_string(mixValue) + " caused NaN/Inf",
                                 {"Fix mix parameter implementation",
                                  "Add bounds checking for mix calculations"});
                    return result;
                }
                
                float outputRMS = ComprehensiveSignalGenerator::calculateRMS(processBuffer);
                outputLevels.push_back(outputRMS);
            }
            
            // Analyze linearity
            bool isMonotonic = true;
            float totalVariation = 0.0f;
            
            for (size_t i = 1; i < outputLevels.size(); ++i) {
                float diff = outputLevels[i] - outputLevels[i-1];
                totalVariation += std::abs(diff);
                
                // Check for non-monotonic behavior (levels should generally increase or decrease)
                if (i > 1) {
                    float prevDiff = outputLevels[i-1] - outputLevels[i-2];
                    if ((diff > 0 && prevDiff < 0) || (diff < 0 && prevDiff > 0)) {
                        if (std::abs(diff) > 0.01f && std::abs(prevDiff) > 0.01f) {
                            isMonotonic = false;
                        }
                    }
                }
            }
            
            result.metrics["total_variation"] = totalVariation;
            result.metrics["is_monotonic"] = isMonotonic ? 1.0f : 0.0f;
            result.metrics["min_level"] = *std::min_element(outputLevels.begin(), outputLevels.end());
            result.metrics["max_level"] = *std::max_element(outputLevels.begin(), outputLevels.end());
            
            if (!isMonotonic) {
                result.setFail(Severity::WARNING,
                             "Mix parameter behavior is not monotonic",
                             {"Review mix parameter implementation for smoothness",
                              "Ensure mix parameter follows expected behavior",
                              "Check for unexpected interactions with other parameters"});
                result.score = 50.0f;
            }
            else if (totalVariation < 0.01f) {
                result.setFail(Severity::WARNING,
                             "Mix parameter appears to have minimal effect",
                             {"Verify mix parameter is properly connected",
                              "Check parameter scaling and range",
                              "Ensure mix parameter affects audio output"});
                result.score = 40.0f;
            }
            else {
                result.setPass("Mix parameter shows good linearity");
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during mix parameter testing: " + std::string(e.what()),
                         {"Fix runtime errors in parameter handling"});
        }
        
        return result;
    }
    
    TestResult ComprehensiveTestHarness::testRapidParameterChanges(EngineBase* engine) {
        TestResult result("Rapid Parameter Changes");
        
        try {
            resetEngine(engine);
            
            if (engine->getNumParameters() == 0) {
                result.setFail(Severity::INFO,
                             "No parameters to test - skipping rapid change test",
                             {"Consider adding parameters if appropriate"});
                result.score = 80.0f;
                return result;
            }
            
            auto testSignal = m_signalCache.at("sine_440");
            juce::AudioBuffer<float> processBuffer = testSignal;
            
            // Test rapid parameter changes while processing
            const int numChanges = 50;
            bool hasError = false;
            std::string errorDetails;
            
            for (int change = 0; change < numChanges && !hasError; ++change) {
                // Randomize all parameters
                std::map<int, float> params;
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> dist(0.0f, 1.0f);
                
                for (int paramIdx = 0; paramIdx < engine->getNumParameters(); ++paramIdx) {
                    params[paramIdx] = dist(gen);
                }
                
                // Update parameters
                engine->updateParameters(params);
                
                // Process a small chunk
                int chunkSize = m_blockSize / 4;
                for (int pos = 0; pos < processBuffer.getNumSamples() - chunkSize; pos += chunkSize) {
                    juce::AudioBuffer<float> chunk(processBuffer.getNumChannels(), chunkSize);
                    
                    // Copy chunk
                    for (int ch = 0; ch < chunk.getNumChannels(); ++ch) {
                        chunk.copyFrom(ch, 0, processBuffer, ch, pos, chunkSize);
                    }
                    
                    // Process chunk
                    engine->process(chunk);
                    
                    // Check for problems
                    if (ComprehensiveSignalGenerator::containsNanOrInf(chunk)) {
                        hasError = true;
                        errorDetails = "NaN/Inf output during rapid parameter change " + std::to_string(change);
                        break;
                    }
                    
                    float peak = ComprehensiveSignalGenerator::calculatePeak(chunk);
                    if (peak > 5.0f) { // Allow some headroom for transients
                        hasError = true;
                        errorDetails = "Excessive output level (" + std::to_string(peak) + ") during parameter change";
                        break;
                    }
                }
            }
            
            if (hasError) {
                result.setFail(Severity::ERROR,
                             "Rapid parameter changes caused instability: " + errorDetails,
                             {"Add parameter smoothing/interpolation",
                              "Avoid sudden parameter jumps in processing",
                              "Use atomic parameter updates",
                              "Add bounds checking for extreme parameter combinations"});
            }
            else {
                result.setPass("Engine handles rapid parameter changes stably");
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during rapid parameter testing: " + std::string(e.what()),
                         {"Fix runtime errors in parameter update handling"});
        }
        
        return result;
    }
    
    TestResult ComprehensiveTestHarness::testBypassStability(EngineBase* engine) {
        TestResult result("Bypass Stability");
        
        try {
            resetEngine(engine);
            
            // Look for bypass parameter
            int bypassParamIndex = -1;
            int numParams = engine->getNumParameters();
            
            for (int i = 0; i < numParams; ++i) {
                std::string paramName = engine->getParameterName(i).toStdString();
                std::transform(paramName.begin(), paramName.end(), paramName.begin(), ::tolower);
                
                if (paramName.find("bypass") != std::string::npos ||
                    paramName.find("enable") != std::string::npos ||
                    paramName.find("on") != std::string::npos) {
                    bypassParamIndex = i;
                    break;
                }
            }
            
            if (bypassParamIndex < 0) {
                result.setFail(Severity::INFO,
                             "No bypass parameter found - testing manual bypass behavior",
                             {"Consider adding a bypass parameter",
                              "Ensure engine can be cleanly disabled when needed"});
                result.score = 70.0f;
                
                // Test basic stability without bypass
                auto testBuffer = m_signalCache.at("sine_440");
                juce::AudioBuffer<float> processBuffer = testBuffer;
                engine->process(processBuffer);
                
                if (ComprehensiveSignalGenerator::containsNanOrInf(processBuffer)) {
                    result.setFail(Severity::ERROR,
                                 "Engine produces NaN/Inf in normal operation",
                                 {"Fix basic processing stability"});
                }
                
                return result;
            }
            
            auto testSignal = m_signalCache.at("sine_440");
            
            // Test bypass transitions
            std::vector<float> bypassValues = {1.0f, 0.0f, 1.0f, 0.0f}; // On/Off/On/Off
            bool hasError = false;
            std::string errorDetails;
            
            for (size_t i = 0; i < bypassValues.size() && !hasError; ++i) {
                std::map<int, float> params;
                params[bypassParamIndex] = bypassValues[i];
                engine->updateParameters(params);
                
                // Process several blocks to check stability
                for (int block = 0; block < 5; ++block) {
                    juce::AudioBuffer<float> processBuffer = testSignal;
                    engine->process(processBuffer);
                    
                    if (ComprehensiveSignalGenerator::containsNanOrInf(processBuffer)) {
                        hasError = true;
                        errorDetails = "NaN/Inf during bypass state " + std::to_string(bypassValues[i]);
                        break;
                    }
                    
                    float peak = ComprehensiveSignalGenerator::calculatePeak(processBuffer);
                    if (peak > 2.0f) {
                        hasError = true;
                        errorDetails = "Excessive output (" + std::to_string(peak) + ") during bypass transition";
                        break;
                    }
                }
            }
            
            // Test rapid bypass toggling
            if (!hasError) {
                for (int toggle = 0; toggle < 20; ++toggle) {
                    float bypassValue = (toggle % 2 == 0) ? 1.0f : 0.0f;
                    std::map<int, float> params;
                    params[bypassParamIndex] = bypassValue;
                    engine->updateParameters(params);
                    
                    juce::AudioBuffer<float> processBuffer = testSignal;
                    engine->process(processBuffer);
                    
                    if (ComprehensiveSignalGenerator::containsNanOrInf(processBuffer)) {
                        hasError = true;
                        errorDetails = "NaN/Inf during rapid bypass toggling";
                        break;
                    }
                }
            }
            
            if (hasError) {
                result.setFail(Severity::ERROR,
                             "Bypass functionality is unstable: " + errorDetails,
                             {"Fix bypass parameter implementation",
                              "Ensure clean switching between bypassed/active states",
                              "Add proper state initialization for bypass transitions",
                              "Consider crossfading for smooth bypass transitions"});
            }
            else {
                result.setPass("Bypass functionality works stably");
            }
            
        } catch (const std::exception& e) {
            result.setFail(Severity::ERROR,
                         "Exception during bypass testing: " + std::string(e.what()),
                         {"Fix runtime errors in bypass handling"});
        }
        
        return result;
    }

} // namespace ChimeraTestHarness