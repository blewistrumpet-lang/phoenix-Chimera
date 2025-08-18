#include "ComprehensiveTestHarness.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <future>

namespace ChimeraTestHarness {

    // ComprehensiveSignalGenerator implementation
    juce::AudioBuffer<float> ComprehensiveSignalGenerator::generateSignal(
        SignalType type, double sampleRate, float durationSeconds, float amplitude,
        const std::map<std::string, float>& params) {
        
        int numSamples = static_cast<int>(sampleRate * durationSeconds);
        juce::AudioBuffer<float> buffer(2, numSamples); // Stereo
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<float> noise(0.0f, amplitude);
        std::uniform_real_distribution<float> uniform(-amplitude, amplitude);
        
        switch (type) {
            case SignalType::DC_OFFSET: {
                float dcValue = params.count("dc") ? params.at("dc") : amplitude;
                buffer.clear();
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    juce::FloatVectorOperations::fill(buffer.getWritePointer(ch), dcValue, numSamples);
                }
                break;
            }
            
            case SignalType::SINE_WAVE: {
                float frequency = params.count("frequency") ? params.at("frequency") : 440.0f;
                float phase = params.count("phase") ? params.at("phase") : 0.0f;
                
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    auto* channelData = buffer.getWritePointer(ch);
                    float channelPhase = phase + (ch * juce::MathConstants<float>::pi * 0.1f); // Slight stereo separation
                    
                    for (int i = 0; i < numSamples; ++i) {
                        float samplePhase = channelPhase + (float(i) / sampleRate) * frequency * juce::MathConstants<float>::twoPi;
                        channelData[i] = amplitude * std::sin(samplePhase);
                    }
                }
                break;
            }
            
            case SignalType::WHITE_NOISE: {
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    auto* channelData = buffer.getWritePointer(ch);
                    for (int i = 0; i < numSamples; ++i) {
                        channelData[i] = uniform(gen);
                    }
                }
                break;
            }
            
            case SignalType::PINK_NOISE: {
                // Simple pink noise approximation using cascaded filters
                buffer.clear();
                
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    auto* channelData = buffer.getWritePointer(ch);
                    float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
                    
                    for (int i = 0; i < numSamples; ++i) {
                        float white = uniform(gen);
                        b0 = 0.99886f * b0 + white * 0.0555179f;
                        b1 = 0.99332f * b1 + white * 0.0750759f;
                        b2 = 0.96900f * b2 + white * 0.1538520f;
                        b3 = 0.86650f * b3 + white * 0.3104856f;
                        b4 = 0.55000f * b4 + white * 0.5329522f;
                        b5 = -0.7616f * b5 - white * 0.0168980f;
                        float pink = b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f;
                        b6 = white * 0.115926f;
                        channelData[i] = pink * amplitude * 0.11f; // Scale to appropriate level
                    }
                }
                break;
            }
            
            case SignalType::IMPULSE: {
                buffer.clear();
                if (numSamples > 0) {
                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                        buffer.setSample(ch, 0, amplitude);
                    }
                }
                break;
            }
            
            case SignalType::STEP: {
                float stepTime = params.count("stepTime") ? params.at("stepTime") : durationSeconds * 0.1f;
                int stepSample = static_cast<int>(sampleRate * stepTime);
                
                buffer.clear();
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    auto* channelData = buffer.getWritePointer(ch);
                    for (int i = stepSample; i < numSamples; ++i) {
                        channelData[i] = amplitude;
                    }
                }
                break;
            }
            
            case SignalType::CHIRP_SWEEP: {
                float startFreq = params.count("startFreq") ? params.at("startFreq") : 20.0f;
                float endFreq = params.count("endFreq") ? params.at("endFreq") : 20000.0f;
                
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    auto* channelData = buffer.getWritePointer(ch);
                    
                    for (int i = 0; i < numSamples; ++i) {
                        float t = float(i) / sampleRate;
                        float normalizedTime = t / durationSeconds;
                        float instantFreq = startFreq * std::pow(endFreq / startFreq, normalizedTime);
                        float phase = startFreq * t + (endFreq - startFreq) * t * t / (2.0f * durationSeconds);
                        channelData[i] = amplitude * std::sin(juce::MathConstants<float>::twoPi * phase);
                    }
                }
                break;
            }
            
            case SignalType::MULTITONE: {
                // Generate multiple sine waves at different frequencies
                std::vector<float> frequencies = {220.0f, 440.0f, 880.0f, 1760.0f};
                if (params.count("numTones")) {
                    int numTones = static_cast<int>(params.at("numTones"));
                    frequencies.clear();
                    for (int i = 0; i < numTones; ++i) {
                        frequencies.push_back(220.0f * std::pow(2.0f, i));
                    }
                }
                
                buffer.clear();
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    auto* channelData = buffer.getWritePointer(ch);
                    
                    for (const auto freq : frequencies) {
                        for (int i = 0; i < numSamples; ++i) {
                            float phase = (float(i) / sampleRate) * freq * juce::MathConstants<float>::twoPi;
                            channelData[i] += (amplitude / frequencies.size()) * std::sin(phase);
                        }
                    }
                }
                break;
            }
            
            case SignalType::DRUM_TRANSIENT: {
                // Simulate drum hit with fast attack, exponential decay
                buffer.clear();
                
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    auto* channelData = buffer.getWritePointer(ch);
                    
                    for (int i = 0; i < numSamples; ++i) {
                        float t = float(i) / sampleRate;
                        float envelope = amplitude * std::exp(-t * 30.0f); // Fast decay
                        float noise = uniform(gen);
                        float tone = std::sin(juce::MathConstants<float>::twoPi * 80.0f * t); // Low frequency thump
                        channelData[i] = envelope * (0.7f * noise + 0.3f * tone);
                    }
                }
                break;
            }
            
            case SignalType::EXTREME_LEVELS: {
                // Test with near-clipping levels
                float extremeAmp = params.count("amplitude") ? params.at("amplitude") : 0.99f;
                buffer.clear();
                
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                    auto* channelData = buffer.getWritePointer(ch);
                    for (int i = 0; i < numSamples; ++i) {
                        float phase = (float(i) / sampleRate) * 440.0f * juce::MathConstants<float>::twoPi;
                        channelData[i] = extremeAmp * std::sin(phase);
                    }
                }
                break;
            }
            
            case SignalType::SILENCE:
            default:
                buffer.clear();
                break;
        }
        
        return buffer;
    }
    
    std::vector<float> ComprehensiveSignalGenerator::generateParameterSweep(int numSteps, float min, float max) {
        std::vector<float> values;
        values.reserve(numSteps);
        
        if (numSteps <= 1) {
            values.push_back((min + max) * 0.5f);
            return values;
        }
        
        for (int i = 0; i < numSteps; ++i) {
            float t = float(i) / (numSteps - 1);
            values.push_back(min + t * (max - min));
        }
        
        return values;
    }
    
    bool ComprehensiveSignalGenerator::containsNanOrInf(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const auto* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (!std::isfinite(channelData[i])) {
                    return true;
                }
            }
        }
        return false;
    }
    
    float ComprehensiveSignalGenerator::calculateRMS(const juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumSamples() == 0) return 0.0f;
        
        double sumSquares = 0.0;
        int totalSamples = 0;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const auto* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                sumSquares += channelData[i] * channelData[i];
                totalSamples++;
            }
        }
        
        return totalSamples > 0 ? static_cast<float>(std::sqrt(sumSquares / totalSamples)) : 0.0f;
    }
    
    float ComprehensiveSignalGenerator::calculatePeak(const juce::AudioBuffer<float>& buffer) {
        float peak = 0.0f;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const auto* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(channelData[i]));
            }
        }
        
        return peak;
    }
    
    float ComprehensiveSignalGenerator::calculateCrestFactor(const juce::AudioBuffer<float>& buffer) {
        float rms = calculateRMS(buffer);
        float peak = calculatePeak(buffer);
        return rms > 0.0f ? (peak / rms) : 0.0f;
    }
    
    float ComprehensiveSignalGenerator::calculateStereoCorrelation(const juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumChannels() < 2 || buffer.getNumSamples() == 0) {
            return 1.0f; // Perfect correlation for mono or empty buffer
        }
        
        const auto* leftData = buffer.getReadPointer(0);
        const auto* rightData = buffer.getReadPointer(1);
        
        double sumL = 0.0, sumR = 0.0, sumLR = 0.0, sumL2 = 0.0, sumR2 = 0.0;
        int numSamples = buffer.getNumSamples();
        
        for (int i = 0; i < numSamples; ++i) {
            sumL += leftData[i];
            sumR += rightData[i];
            sumLR += leftData[i] * rightData[i];
            sumL2 += leftData[i] * leftData[i];
            sumR2 += rightData[i] * rightData[i];
        }
        
        double meanL = sumL / numSamples;
        double meanR = sumR / numSamples;
        double covariance = (sumLR / numSamples) - (meanL * meanR);
        double stdL = std::sqrt((sumL2 / numSamples) - (meanL * meanL));
        double stdR = std::sqrt((sumR2 / numSamples) - (meanR * meanR));
        
        return (stdL > 0.0 && stdR > 0.0) ? static_cast<float>(covariance / (stdL * stdR)) : 1.0f;
    }
    
    // PerformanceMeasurer implementation
    PerformanceMeasurer::Measurement PerformanceMeasurer::measureProcessingTime(
        std::function<void()> processingFunction, double sampleRate, int blockSize) {
        
        Measurement result;
        
        auto startTime = std::chrono::high_resolution_clock::now();
        processingFunction();
        auto endTime = std::chrono::high_resolution_clock::now();
        
        result.processingTime = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime);
        result.cpuPercentage = calculateCpuPercentage(result.processingTime, blockSize, sampleRate);
        result.realTimeCapable = isRealTimeCapable(result.processingTime, blockSize, sampleRate);
        
        return result;
    }
    
    float PerformanceMeasurer::calculateCpuPercentage(std::chrono::nanoseconds processingTime, 
                                                     int blockSize, double sampleRate) {
        if (sampleRate <= 0.0 || blockSize <= 0) return 100.0f;
        
        double blockDurationNs = (double(blockSize) / sampleRate) * 1e9;
        return static_cast<float>((processingTime.count() / blockDurationNs) * 100.0);
    }
    
    bool PerformanceMeasurer::isRealTimeCapable(std::chrono::nanoseconds processingTime,
                                               int blockSize, double sampleRate, float safetyMargin) {
        float cpuPercentage = calculateCpuPercentage(processingTime, blockSize, sampleRate);
        return cpuPercentage <= (100.0f * safetyMargin);
    }
    
    // ComprehensiveTestHarness implementation
    ComprehensiveTestHarness::ComprehensiveTestHarness() {
        cacheCommonSignals();
    }
    
    void ComprehensiveTestHarness::cacheCommonSignals() {
        // Cache commonly used test signals to improve performance
        m_signalCache.clear();
        
        // Silence
        m_signalCache["silence"] = ComprehensiveSignalGenerator::generateSignal(
            ComprehensiveSignalGenerator::SignalType::SILENCE, m_sampleRate, m_testDuration);
            
        // 440Hz sine wave
        m_signalCache["sine_440"] = ComprehensiveSignalGenerator::generateSignal(
            ComprehensiveSignalGenerator::SignalType::SINE_WAVE, m_sampleRate, m_testDuration, 0.5f,
            {{"frequency", 440.0f}});
            
        // White noise
        m_signalCache["white_noise"] = ComprehensiveSignalGenerator::generateSignal(
            ComprehensiveSignalGenerator::SignalType::WHITE_NOISE, m_sampleRate, m_testDuration, 0.5f);
            
        // Impulse
        m_signalCache["impulse"] = ComprehensiveSignalGenerator::generateSignal(
            ComprehensiveSignalGenerator::SignalType::IMPULSE, m_sampleRate, m_testDuration, 1.0f);
    }
    
    TestSuiteResults ComprehensiveTestHarness::testAllEngines() {
        auto startTime = std::chrono::high_resolution_clock::now();
        
        TestSuiteResults suiteResults;
        suiteResults.engineResults.reserve(ENGINE_COUNT);
        
        if (m_verbose) {
            std::cout << "Starting comprehensive test of all " << ENGINE_COUNT << " engines...\n";
            std::cout << "Sample Rate: " << m_sampleRate << " Hz\n";
            std::cout << "Block Size: " << m_blockSize << " samples\n";
            std::cout << "Test Duration: " << m_testDuration << " seconds\n\n";
        }
        
        // Test all engines (0-56)
        for (int engineID = 0; engineID < ENGINE_COUNT; ++engineID) {
            if (m_shouldStop.load()) {
                break;
            }
            
            EngineTestResults engineResults = testSingleEngine(engineID);
            suiteResults.engineResults.push_back(std::move(engineResults));
            
            if (m_verbose) {
                const auto& result = suiteResults.engineResults.back();
                std::cout << "Engine " << engineID << " (" << result.engineName << "): ";
                std::cout << ReportUtils::formatScore(result.overallScore) << " - ";
                std::cout << (result.allTestsPassed ? "PASS" : "FAIL");
                if (result.criticalIssues > 0) std::cout << " (" << result.criticalIssues << " critical issues)";
                std::cout << "\n";
            }
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        suiteResults.totalExecutionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        suiteResults.calculateSummary();
        
        if (m_verbose) {
            printSummaryToConsole(suiteResults);
        }
        
        return suiteResults;
    }
    
    EngineTestResults ComprehensiveTestHarness::testSingleEngine(int engineID) {
        EngineTestResults results;
        results.engineID = engineID;
        results.engineName = getEngineTypeName(engineID);
        
        auto engineStartTime = std::chrono::high_resolution_clock::now();
        
        // Attempt to create the engine
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            results.engineCreated = false;
            TestResult creationFailure("Engine Creation");
            creationFailure.setFail(Severity::CRITICAL, 
                                   "Failed to create engine instance",
                                   {"Check engine factory implementation", 
                                    "Verify engine class exists and compiles",
                                    "Check for missing dependencies"});
            results.safetyTests.addResult(creationFailure);
            results.calculateOverallScore();
            return results;
        }
        
        results.engineCreated = true;
        
        // Prepare the engine
        if (!prepareEngine(engine.get())) {
            TestResult prepFailure("Engine Preparation");
            prepFailure.setFail(Severity::ERROR, 
                               "Engine failed to prepare properly",
                               {"Check prepareToPlay implementation",
                                "Verify sample rate and block size handling"});
            results.safetyTests.addResult(prepFailure);
        }
        
        // Run all test categories
        try {
            results.parameterSweepTests = runParameterSweepTests(engine.get(), engineID);
            results.safetyTests = runSafetyTests(engine.get(), engineID);
            results.audioQualityTests = runAudioQualityTests(engine.get(), engineID);
            results.performanceTests = runPerformanceTests(engine.get(), engineID);
            results.stabilityTests = runStabilityTests(engine.get(), engineID);
        } catch (const std::exception& e) {
            TestResult exceptionFailure("Test Execution");
            exceptionFailure.setFail(Severity::CRITICAL, 
                                   std::string("Exception during testing: ") + e.what(),
                                   {"Fix runtime errors in engine implementation",
                                    "Add proper error handling",
                                    "Test engine manually before automated testing"});
            results.safetyTests.addResult(exceptionFailure);
        }
        
        auto engineEndTime = std::chrono::high_resolution_clock::now();
        results.totalTestTime = std::chrono::duration_cast<std::chrono::milliseconds>(engineEndTime - engineStartTime);
        
        results.calculateOverallScore();
        
        return results;
    }
    
    bool ComprehensiveTestHarness::prepareEngine(EngineBase* engine) {
        try {
            engine->prepareToPlay(m_sampleRate, m_blockSize);
            engine->reset();
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void ComprehensiveTestHarness::resetEngine(EngineBase* engine) {
        try {
            engine->reset();
        } catch (...) {
            // Ignore reset failures for now
        }
    }
    
    void ComprehensiveTestHarness::logMessage(const std::string& message) {
        std::lock_guard<std::mutex> lock(m_logMutex);
        if (m_verbose) {
            std::cout << message << std::endl;
        }
    }
    
    void ComprehensiveTestHarness::printProgressUpdate(int engineID, const std::string& engineName, 
                                                      const std::string& currentTest) {
        if (m_verbose) {
            logMessage("Engine " + std::to_string(engineID) + " (" + engineName + "): " + currentTest);
        }
    }
    
    void ComprehensiveTestHarness::printSummaryToConsole(const TestSuiteResults& results) {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << "COMPREHENSIVE TEST HARNESS SUMMARY\n";
        std::cout << std::string(80, '=') << "\n";
        std::cout << "Total Engines: " << results.totalEngines << "\n";
        std::cout << "Working Engines: " << results.workingEngines << "\n";
        std::cout << "Failed to Create: " << results.failedEngines << "\n";
        std::cout << "Engines with Critical Issues: " << results.enginesWithCriticalIssues << "\n";
        std::cout << "Engines with Errors: " << results.enginesWithErrors << "\n";
        std::cout << "Engines with Warnings: " << results.enginesWithWarnings << "\n";
        std::cout << "Average Score: " << ReportUtils::formatScore(results.averageScore) << "\n";
        std::cout << "Average CPU Usage: " << ReportUtils::formatPercentage(results.averageCpuUsage) << "\n";
        std::cout << "Worst CPU Usage: " << ReportUtils::formatPercentage(results.worstCpuUsage) << "\n";
        std::cout << "Total Test Time: " << ReportUtils::formatDuration(results.totalExecutionTime) << "\n";
        
        auto problematic = results.getProblematicEngines();
        if (!problematic.empty()) {
            std::cout << "\nPROBLEMATIC ENGINES (Top 10):\n";
            std::cout << std::string(50, '-') << "\n";
            
            int count = 0;
            for (const auto& engine : problematic) {
                if (++count > 10) break;
                
                std::cout << std::setw(3) << engine.engineID << ": " 
                         << std::setw(25) << std::left << engine.engineName 
                         << " Score: " << std::setw(6) << ReportUtils::formatScore(engine.overallScore);
                         
                if (engine.criticalIssues > 0) std::cout << " [" << engine.criticalIssues << " critical]";
                else if (engine.errorIssues > 0) std::cout << " [" << engine.errorIssues << " errors]";
                else if (engine.warningIssues > 0) std::cout << " [" << engine.warningIssues << " warnings]";
                
                std::cout << "\n";
            }
        }
        
        std::cout << std::string(80, '=') << "\n\n";
    }
    
    // Report utility functions
    namespace ReportUtils {
        std::string formatDuration(std::chrono::milliseconds duration) {
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
            auto ms = duration - seconds;
            
            std::ostringstream oss;
            oss << seconds.count() << "." << std::setfill('0') << std::setw(3) << ms.count() << "s";
            return oss.str();
        }
        
        std::string formatScore(float score) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << score << "%";
            return oss.str();
        }
        
        std::string formatPercentage(float percentage) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << percentage << "%";
            return oss.str();
        }
        
        std::string severityToString(Severity severity) {
            switch (severity) {
                case Severity::INFO: return "INFO";
                case Severity::WARNING: return "WARNING";
                case Severity::ERROR: return "ERROR";
                case Severity::CRITICAL: return "CRITICAL";
                default: return "UNKNOWN";
            }
        }
        
        std::string escapeHTML(const std::string& text) {
            std::string escaped;
            escaped.reserve(text.length() * 2);
            
            for (char c : text) {
                switch (c) {
                    case '<': escaped += "&lt;"; break;
                    case '>': escaped += "&gt;"; break;
                    case '&': escaped += "&amp;"; break;
                    case '"': escaped += "&quot;"; break;
                    case '\'': escaped += "&#39;"; break;
                    default: escaped += c; break;
                }
            }
            
            return escaped;
        }
        
        std::string generateProgressBar(float percentage, int width) {
            int filled = static_cast<int>((percentage / 100.0f) * width);
            std::string bar = "[";
            
            for (int i = 0; i < width; ++i) {
                if (i < filled) {
                    bar += "=";
                } else if (i == filled && percentage < 100.0f) {
                    bar += ">";
                } else {
                    bar += " ";
                }
            }
            
            bar += "]";
            return bar;
        }
    }

} // namespace ChimeraTestHarness