// Direct Engine Test - Analyzes all Chimera engines for quality and safety
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <chrono>
#include <cmath>
#include <iomanip>

// Include JUCE and engine headers
#include "JuceHeader.h"
#include "Source/EngineBase.h"
#include "Source/EngineFactory.h"
#include "Source/EngineTypes.h"

struct EngineIssue {
    std::string engineName;
    int engineId;
    std::vector<std::string> problems;
    bool critical = false;
};

class DirectEngineTest {
private:
    std::vector<EngineIssue> issues;
    EngineFactory factory;
    
    // Test helpers
    bool containsNaN(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (std::isnan(data[i])) return true;
            }
        }
        return false;
    }
    
    bool containsInf(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                if (std::isinf(data[i])) return true;
            }
        }
        return false;
    }
    
    float calculateRMS(const juce::AudioBuffer<float>& buffer) {
        float sum = 0.0f;
        int totalSamples = buffer.getNumChannels() * buffer.getNumSamples();
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                sum += data[i] * data[i];
            }
        }
        
        return std::sqrt(sum / totalSamples);
    }
    
    float calculatePeak(const juce::AudioBuffer<float>& buffer) {
        float peak = 0.0f;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(data[i]));
            }
        }
        
        return peak;
    }
    
    void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude = 0.5f) {
        const float sampleRate = 44100.0f;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = amplitude * std::sin(2.0f * M_PI * frequency * i / sampleRate);
            }
        }
    }
    
    void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude = 0.5f) {
        juce::Random random;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = random.nextFloat() * 2.0f * amplitude - amplitude;
            }
        }
    }
    
public:
    void testEngine(int engineId) {
        std::unique_ptr<EngineBase> engine = factory.createEngine(engineId);
        
        if (!engine) {
            // Skip engines that don't exist
            return;
        }
        
        EngineIssue issue;
        issue.engineId = engineId;
        issue.engineName = engine->getName().toStdString();
        
        // Prepare engine
        const double sampleRate = 44100.0;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);
        
        // Create test buffers
        juce::AudioBuffer<float> testBuffer(2, blockSize);
        juce::AudioBuffer<float> originalBuffer(2, blockSize);
        
        // Test 1: NaN input handling
        testBuffer.clear();
        testBuffer.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
        
        try {
            engine->process(testBuffer);
            if (containsNaN(testBuffer)) {
                issue.problems.push_back("CRITICAL: Does not handle NaN input - output contains NaN");
                issue.critical = true;
            }
        } catch (...) {
            issue.problems.push_back("CRITICAL: Crashes on NaN input");
            issue.critical = true;
        }
        
        // Test 2: Infinity input handling
        testBuffer.clear();
        testBuffer.setSample(0, 0, std::numeric_limits<float>::infinity());
        
        try {
            engine->process(testBuffer);
            if (containsInf(testBuffer)) {
                issue.problems.push_back("CRITICAL: Does not handle infinity input - output contains infinity");
                issue.critical = true;
            }
        } catch (...) {
            issue.problems.push_back("CRITICAL: Crashes on infinity input");
            issue.critical = true;
        }
        
        // Test 3: Extreme parameter values
        std::map<int, float> extremeParams;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            extremeParams[i] = 1.0f; // Max value
        }
        engine->updateParameters(extremeParams);
        
        generateSineWave(testBuffer, 440.0f);
        originalBuffer = testBuffer;
        
        try {
            engine->process(testBuffer);
            
            if (containsNaN(testBuffer) || containsInf(testBuffer)) {
                issue.problems.push_back("Produces NaN/Inf with extreme parameter values");
            }
            
            float outputRMS = calculateRMS(testBuffer);
            float inputRMS = calculateRMS(originalBuffer);
            float gain = outputRMS / (inputRMS + 0.00001f);
            
            if (gain > 10.0f) {
                issue.problems.push_back("Excessive gain with extreme parameters: " + 
                                       std::to_string(gain) + "x");
            }
            
            float peak = calculatePeak(testBuffer);
            if (peak > 1.0f) {
                issue.problems.push_back("Output clips (peak: " + std::to_string(peak) + ")");
            }
            
        } catch (...) {
            issue.problems.push_back("CRITICAL: Crashes with extreme parameter values");
            issue.critical = true;
        }
        
        // Test 4: Normal operation
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            extremeParams[i] = 0.5f; // Default values
        }
        engine->updateParameters(extremeParams);
        
        generateWhiteNoise(testBuffer, 0.3f);
        originalBuffer = testBuffer;
        
        try {
            engine->process(testBuffer);
            
            float outputRMS = calculateRMS(testBuffer);
            float inputRMS = calculateRMS(originalBuffer);
            float gain = outputRMS / (inputRMS + 0.00001f);
            
            if (gain > 5.0f) {
                issue.problems.push_back("High gain in normal operation: " + std::to_string(gain) + "x");
            }
            
        } catch (...) {
            issue.problems.push_back("CRITICAL: Crashes during normal operation");
            issue.critical = true;
        }
        
        // Test 5: Silent input
        testBuffer.clear();
        engine->process(testBuffer);
        
        float silenceRMS = calculateRMS(testBuffer);
        if (silenceRMS > 0.01f) {
            issue.problems.push_back("Generates noise with silent input (RMS: " + 
                                   std::to_string(silenceRMS) + ")");
        }
        
        // Only record if there are issues
        if (!issue.problems.empty()) {
            issues.push_back(issue);
        }
    }
    
    void runAllTests() {
        std::cout << "Testing all Chimera engines..." << std::endl << std::endl;
        
        // Test all engines from 0 to 56
        for (int engineId = 0; engineId <= 56; ++engineId) {
            std::cout << "Testing engine " << engineId << "... ";
            std::cout.flush();
            
            testEngine(engineId);
            
            // Check if we found issues with this engine
            bool foundIssue = false;
            for (const auto& issue : issues) {
                if (issue.engineId == engineId) {
                    foundIssue = true;
                    break;
                }
            }
            
            if (foundIssue) {
                std::cout << "ISSUES FOUND" << std::endl;
            } else {
                std::cout << "OK" << std::endl;
            }
        }
        
        std::cout << std::endl;
    }
    
    void generateReport() {
        // Sort issues by criticality
        std::sort(issues.begin(), issues.end(), 
                  [](const EngineIssue& a, const EngineIssue& b) {
                      if (a.critical != b.critical) return a.critical > b.critical;
                      return a.problems.size() > b.problems.size();
                  });
        
        // Console summary
        std::cout << "=== ENGINE TEST SUMMARY ===" << std::endl;
        std::cout << "Total engines tested: 57" << std::endl;
        std::cout << "Engines with issues: " << issues.size() << std::endl;
        
        int criticalCount = 0;
        for (const auto& issue : issues) {
            if (issue.critical) criticalCount++;
        }
        
        std::cout << "Critical issues: " << criticalCount << std::endl;
        std::cout << std::endl;
        
        // Detailed report
        std::ofstream report("chimera_engine_test_report.txt");
        
        report << "CHIMERA ENGINE TEST REPORT" << std::endl;
        report << "=========================" << std::endl;
        report << "Generated: " << getCurrentDateTime() << std::endl << std::endl;
        
        report << "SUMMARY" << std::endl;
        report << "-------" << std::endl;
        report << "Total engines: 57" << std::endl;
        report << "Engines with issues: " << issues.size() << std::endl;
        report << "Critical issues: " << criticalCount << std::endl;
        report << "Pass rate: " << std::fixed << std::setprecision(1) 
               << ((57.0 - issues.size()) / 57.0 * 100.0) << "%" << std::endl << std::endl;
        
        // Critical issues first
        if (criticalCount > 0) {
            report << "CRITICAL ISSUES (Fix immediately)" << std::endl;
            report << "---------------------------------" << std::endl;
            
            for (const auto& issue : issues) {
                if (issue.critical) {
                    report << "Engine " << issue.engineId << ": " << issue.engineName << std::endl;
                    for (const auto& problem : issue.problems) {
                        report << "  - " << problem << std::endl;
                    }
                    report << std::endl;
                }
            }
        }
        
        // Non-critical issues
        report << "OTHER ISSUES" << std::endl;
        report << "------------" << std::endl;
        
        for (const auto& issue : issues) {
            if (!issue.critical) {
                report << "Engine " << issue.engineId << ": " << issue.engineName << std::endl;
                for (const auto& problem : issue.problems) {
                    report << "  - " << problem << std::endl;
                }
                report << std::endl;
            }
        }
        
        // Console output of critical issues
        if (criticalCount > 0) {
            std::cout << "CRITICAL ISSUES FOUND:" << std::endl;
            std::cout << "---------------------" << std::endl;
            
            for (const auto& issue : issues) {
                if (issue.critical) {
                    std::cout << "Engine " << issue.engineId << " (" << issue.engineName << "):" << std::endl;
                    for (const auto& problem : issue.problems) {
                        std::cout << "  - " << problem << std::endl;
                    }
                }
            }
            std::cout << std::endl;
        }
        
        report.close();
        std::cout << "Detailed report saved to: chimera_engine_test_report.txt" << std::endl;
    }
    
private:
    std::string getCurrentDateTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

int main() {
    std::cout << "Chimera Engine Quality Test" << std::endl;
    std::cout << "===========================" << std::endl << std::endl;
    
    DirectEngineTest tester;
    tester.runAllTests();
    tester.generateReport();
    
    return 0;
}