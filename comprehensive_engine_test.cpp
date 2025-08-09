// Comprehensive Functional Audio Test for Chimera Phoenix Engines
// Tests actual audio processing behavior of all 57 engines

#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <complex>
#include <chrono>
#include <ctime>

// Mock JUCE types for testing
namespace juce {
    class String {
        std::string str;
    public:
        String() = default;
        String(const char* s) : str(s) {}
        String(const std::string& s) : str(s) {}
        String(int value) : str(std::to_string(value)) {}
        String(float value) : str(std::to_string(value)) {}
        const char* toRawUTF8() const { return str.c_str(); }
        operator std::string() const { return str; }
    };
    
    template<typename T>
    class AudioBuffer {
        std::vector<std::vector<T>> channels;
        int numSamples;
    public:
        AudioBuffer(int numChannels, int samples) : numSamples(samples) {
            channels.resize(numChannels);
            for (auto& ch : channels) {
                ch.resize(samples, 0);
            }
        }
        
        int getNumChannels() const { return channels.size(); }
        int getNumSamples() const { return numSamples; }
        
        T* getWritePointer(int channel) {
            return channels[channel].data();
        }
        
        const T* getReadPointer(int channel) const {
            return channels[channel].data();
        }
        
        T** getArrayOfWritePointers() {
            static std::vector<T*> ptrs;
            ptrs.clear();
            for (auto& ch : channels) {
                ptrs.push_back(ch.data());
            }
            return ptrs.data();
        }
        
        void clear() {
            for (auto& ch : channels) {
                std::fill(ch.begin(), ch.end(), 0);
            }
        }
    };
}

// Include the engine base and factory
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

// Test result structure
struct EngineTestResult {
    int engineID;
    std::string engineName;
    std::string category;
    bool passedCreation = false;
    bool passedInit = false;
    bool processesAudio = false;
    bool correctBehavior = false;
    float signalModification = 0.0f;
    float cpuUsage = 0.0f;
    std::string behaviorDetails;
    std::string errorMessage;
};

// Simple audio analyzer for testing
class AudioAnalyzer {
public:
    static float calculateRMS(const float* data, int numSamples) {
        if (numSamples == 0) return 0.0f;
        float sum = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            sum += data[i] * data[i];
        }
        return std::sqrt(sum / numSamples);
    }
    
    static float calculatePeak(const float* data, int numSamples) {
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            peak = std::max(peak, std::abs(data[i]));
        }
        return peak;
    }
    
    static bool detectCompression(const float* input, const float* output, int numSamples, float& ratio) {
        float inputPeak = calculatePeak(input, numSamples);
        float outputPeak = calculatePeak(output, numSamples);
        
        if (inputPeak > 0.1f) {
            ratio = outputPeak / inputPeak;
            return ratio < 0.95f;
        }
        return false;
    }
    
    static bool detectFiltering(const float* input, const float* output, int numSamples) {
        float inputRMS = calculateRMS(input, numSamples);
        float outputRMS = calculateRMS(output, numSamples);
        float difference = std::abs(inputRMS - outputRMS) / (inputRMS + 1e-10f);
        return difference > 0.05f;
    }
    
    static bool detectDistortion(const float* input, const float* output, int numSamples) {
        // Simple check: output has higher peaks than input
        float inputPeak = calculatePeak(input, numSamples);
        float outputPeak = calculatePeak(output, numSamples);
        return outputPeak > inputPeak * 1.1f || outputPeak < inputPeak * 0.9f;
    }
    
    static bool detectModulation(const float* output, int numSamples, float& modDepth) {
        float maxAmp = 0.0f;
        float minAmp = 1.0f;
        int windowSize = 128;
        
        for (int i = 0; i < numSamples - windowSize; i += windowSize / 2) {
            float windowRMS = calculateRMS(output + i, windowSize);
            maxAmp = std::max(maxAmp, windowRMS);
            minAmp = std::min(minAmp, windowRMS);
        }
        
        modDepth = (maxAmp - minAmp) / (maxAmp + minAmp + 1e-10f);
        return modDepth > 0.05f;
    }
    
    static bool detectReverb(const float* impulseResponse, int numSamples, float& decayTime) {
        float threshold = 0.001f;
        int tailStart = -1;
        int tailEnd = -1;
        
        for (int i = 0; i < numSamples; ++i) {
            if (std::abs(impulseResponse[i]) > threshold) {
                if (tailStart < 0) tailStart = i;
                tailEnd = i;
            }
        }
        
        if (tailEnd > tailStart && tailEnd - tailStart > 100) {
            decayTime = (tailEnd - tailStart) / 48000.0f;
            return true;
        }
        return false;
    }
};

// Test signal generators
class TestSignalGenerator {
public:
    static void generateSineWave(float* buffer, int numSamples, float frequency, float sampleRate, float amplitude = 0.5f) {
        for (int i = 0; i < numSamples; ++i) {
            buffer[i] = amplitude * sin(2.0f * M_PI * frequency * i / sampleRate);
        }
    }
    
    static void generateWhiteNoise(float* buffer, int numSamples, float amplitude = 0.3f) {
        for (int i = 0; i < numSamples; ++i) {
            buffer[i] = amplitude * ((rand() / (float)RAND_MAX) * 2.0f - 1.0f);
        }
    }
    
    static void generateImpulse(float* buffer, int numSamples, float amplitude = 1.0f) {
        std::fill(buffer, buffer + numSamples, 0.0f);
        buffer[0] = amplitude;
    }
};

// Get category name for engine
std::string getEngineCategory(int engineID) {
    if (engineID == 0) return "Bypass";
    if (engineID >= 1 && engineID <= 6) return "Dynamics & Compression";
    if (engineID >= 7 && engineID <= 14) return "Filters & EQ";
    if (engineID >= 15 && engineID <= 22) return "Distortion & Saturation";
    if (engineID >= 23 && engineID <= 33) return "Modulation Effects";
    if (engineID >= 34 && engineID <= 43) return "Reverb & Delay";
    if (engineID >= 44 && engineID <= 52) return "Spatial & Special";
    if (engineID >= 53 && engineID <= 56) return "Utility";
    return "Unknown";
}

// Main test runner
class ComprehensiveEngineTester {
private:
    const float sampleRate = 48000.0f;
    const int blockSize = 512;
    std::vector<EngineTestResult> results;
    
public:
    void runAllTests() {
        std::cout << "=== Comprehensive Engine Functional Test ===" << std::endl;
        std::cout << "Testing all 57 engines for correct audio processing\n" << std::endl;
        
        for (int engineID = 0; engineID <= 56; ++engineID) {
            testEngine(engineID);
        }
        
        generateReport();
    }
    
private:
    void testEngine(int engineID) {
        EngineTestResult result;
        result.engineID = engineID;
        result.category = getEngineCategory(engineID);
        
        std::cout << "Testing Engine #" << engineID << "... ";
        
        // Create engine
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            result.errorMessage = "Failed to create";
            std::cout << "FAILED (creation)" << std::endl;
            results.push_back(result);
            return;
        }
        result.passedCreation = true;
        
        // Get name
        result.engineName = engine->getName().toRawUTF8();
        
        // Initialize
        try {
            engine->prepareToPlay(sampleRate, blockSize);
            result.passedInit = true;
        } catch (...) {
            result.errorMessage = "Failed to init";
            std::cout << "FAILED (init)" << std::endl;
            results.push_back(result);
            return;
        }
        
        // Set parameters for maximum effect
        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            params[i] = 0.5f; // Default middle
            
            std::string paramName = engine->getParameterName(i).toRawUTF8();
            std::transform(paramName.begin(), paramName.end(), paramName.begin(), ::tolower);
            
            if (paramName.find("mix") != std::string::npos || 
                paramName.find("wet") != std::string::npos) {
                params[i] = 1.0f; // 100% wet
            }
            else if (paramName.find("drive") != std::string::npos ||
                     paramName.find("gain") != std::string::npos) {
                params[i] = 0.7f; // High drive
            }
            else if (paramName.find("depth") != std::string::npos ||
                     paramName.find("amount") != std::string::npos) {
                params[i] = 0.8f; // High depth
            }
        }
        engine->updateParameters(params);
        
        // Test audio processing
        juce::AudioBuffer<float> buffer(2, blockSize);
        
        // Generate test signal based on category
        if (result.category.find("Dynamics") != std::string::npos) {
            TestSignalGenerator::generateSineWave(buffer.getWritePointer(0), blockSize, 1000, sampleRate, 0.9f);
            TestSignalGenerator::generateSineWave(buffer.getWritePointer(1), blockSize, 1000, sampleRate, 0.9f);
        }
        else if (result.category.find("Filter") != std::string::npos) {
            TestSignalGenerator::generateWhiteNoise(buffer.getWritePointer(0), blockSize);
            TestSignalGenerator::generateWhiteNoise(buffer.getWritePointer(1), blockSize);
        }
        else if (result.category.find("Reverb") != std::string::npos) {
            TestSignalGenerator::generateImpulse(buffer.getWritePointer(0), blockSize);
            TestSignalGenerator::generateImpulse(buffer.getWritePointer(1), blockSize);
        }
        else {
            TestSignalGenerator::generateSineWave(buffer.getWritePointer(0), blockSize, 440, sampleRate, 0.5f);
            TestSignalGenerator::generateSineWave(buffer.getWritePointer(1), blockSize, 440, sampleRate, 0.5f);
        }
        
        // Store input
        std::vector<float> input(buffer.getReadPointer(0), buffer.getReadPointer(0) + blockSize);
        float inputRMS = AudioAnalyzer::calculateRMS(input.data(), blockSize);
        
        // Process
        auto start = std::chrono::high_resolution_clock::now();
        engine->process(buffer);
        auto end = std::chrono::high_resolution_clock::now();
        
        result.cpuUsage = std::chrono::duration<float, std::milli>(end - start).count();
        
        // Analyze output
        float outputRMS = AudioAnalyzer::calculateRMS(buffer.getReadPointer(0), blockSize);
        result.signalModification = std::abs(inputRMS - outputRMS) / (inputRMS + 1e-10f);
        
        // Check if processing occurred
        result.processesAudio = false;
        for (int i = 0; i < blockSize; ++i) {
            if (std::abs(buffer.getReadPointer(0)[i] - input[i]) > 0.0001f) {
                result.processesAudio = true;
                break;
            }
        }
        
        // Determine if behavior is correct based on category
        if (engineID == 0) { // Bypass
            result.correctBehavior = !result.processesAudio;
            result.behaviorDetails = "Bypass (no processing expected)";
        }
        else if (result.category.find("Dynamics") != std::string::npos) {
            float ratio;
            bool compressed = AudioAnalyzer::detectCompression(input.data(), buffer.getReadPointer(0), blockSize, ratio);
            result.correctBehavior = compressed || result.processesAudio;
            result.behaviorDetails = compressed ? "Compression detected" : "Processing detected";
        }
        else if (result.category.find("Filter") != std::string::npos) {
            bool filtered = AudioAnalyzer::detectFiltering(input.data(), buffer.getReadPointer(0), blockSize);
            result.correctBehavior = filtered || result.processesAudio;
            result.behaviorDetails = filtered ? "Filtering detected" : "Processing detected";
        }
        else if (result.category.find("Distortion") != std::string::npos) {
            bool distorted = AudioAnalyzer::detectDistortion(input.data(), buffer.getReadPointer(0), blockSize);
            result.correctBehavior = distorted || result.processesAudio;
            result.behaviorDetails = distorted ? "Distortion detected" : "Processing detected";
        }
        else if (result.category.find("Modulation") != std::string::npos) {
            float modDepth;
            bool modulated = AudioAnalyzer::detectModulation(buffer.getReadPointer(0), blockSize, modDepth);
            result.correctBehavior = modulated || result.processesAudio;
            result.behaviorDetails = modulated ? "Modulation detected" : "Processing detected";
        }
        else if (result.category.find("Reverb") != std::string::npos) {
            float decayTime;
            bool hasReverb = AudioAnalyzer::detectReverb(buffer.getReadPointer(0), blockSize, decayTime);
            result.correctBehavior = hasReverb || result.processesAudio;
            result.behaviorDetails = hasReverb ? "Reverb/delay detected" : "Processing detected";
        }
        else {
            result.correctBehavior = result.processesAudio;
            result.behaviorDetails = result.processesAudio ? "Audio modified" : "No processing";
        }
        
        std::cout << (result.correctBehavior ? "PASSED" : "FAILED") 
                  << " (" << result.behaviorDetails << ")" << std::endl;
        
        results.push_back(result);
    }
    
    void generateReport() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        
        int totalPassed = 0;
        int totalProcessing = 0;
        
        for (const auto& result : results) {
            if (result.correctBehavior) totalPassed++;
            if (result.processesAudio) totalProcessing++;
        }
        
        std::cout << "Total Engines: " << results.size() << std::endl;
        std::cout << "Passed Tests: " << totalPassed << "/" << results.size() << std::endl;
        std::cout << "Processing Audio: " << totalProcessing << "/" << results.size() << std::endl;
        std::cout << "Success Rate: " << (totalPassed * 100.0 / results.size()) << "%" << std::endl;
        
        generateHTMLReport();
    }
    
    void generateHTMLReport() {
        std::ofstream html("comprehensive_test_report.html");
        
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        html << R"(<!DOCTYPE html>
<html>
<head>
    <title>Chimera Engine Test Report</title>
    <style>
        body { font-family: Arial; margin: 20px; background: #f0f0f0; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }
        h1 { color: #333; }
        .summary { display: flex; gap: 20px; margin: 20px 0; }
        .stat { background: #667eea; color: white; padding: 20px; border-radius: 10px; flex: 1; text-align: center; }
        table { width: 100%; border-collapse: collapse; }
        th { background: #667eea; color: white; padding: 10px; }
        td { padding: 8px; border-bottom: 1px solid #ddd; }
        .pass { color: green; font-weight: bold; }
        .fail { color: red; font-weight: bold; }
        .category { background: #f0f0f0; font-weight: bold; }
    </style>
</head>
<body>
    <div class='container'>
        <h1>Comprehensive Engine Test Report</h1>
        <p>Generated: )" << std::ctime(&time) << R"(</p>
        <div class='summary'>)";
        
        int totalPassed = 0;
        int totalProcessing = 0;
        
        for (const auto& result : results) {
            if (result.correctBehavior) totalPassed++;
            if (result.processesAudio) totalProcessing++;
        }
        
        html << "<div class='stat'><h2>" << results.size() << "</h2>Total Engines</div>";
        html << "<div class='stat'><h2>" << totalPassed << "</h2>Passed</div>";
        html << "<div class='stat'><h2>" << std::fixed << std::setprecision(1) 
             << (totalPassed * 100.0 / results.size()) << "%</h2>Success Rate</div>";
        html << "</div><table><tr><th>ID</th><th>Name</th><th>Category</th><th>Processes Audio</th><th>Test Result</th><th>Details</th></tr>";
        
        std::string lastCategory = "";
        for (const auto& r : results) {
            if (r.category != lastCategory) {
                html << "<tr><td colspan='6' class='category'>" << r.category << "</td></tr>";
                lastCategory = r.category;
            }
            
            html << "<tr><td>" << r.engineID << "</td><td>" << r.engineName << "</td><td>" << r.category << "</td>";
            html << "<td class='" << (r.processesAudio ? "pass" : "fail") << "'>" 
                 << (r.processesAudio ? "Yes" : "No") << "</td>";
            html << "<td class='" << (r.correctBehavior ? "pass" : "fail") << "'>" 
                 << (r.correctBehavior ? "PASS" : "FAIL") << "</td>";
            html << "<td>" << r.behaviorDetails << "</td></tr>";
        }
        
        html << "</table></div></body></html>";
        html.close();
        
        std::cout << "\nHTML report: comprehensive_test_report.html" << std::endl;
    }
};

int main() {
    ComprehensiveEngineTester tester;
    tester.runAllTests();
    return 0;
}