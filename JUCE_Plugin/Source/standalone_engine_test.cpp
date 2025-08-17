/**
 * Standalone Engine Test - Check which engines actually process audio
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "EngineBase.h"

// Forward declare all engines we want to test
#include "ParametricEQ_Studio.h"
#include "VintageConsoleEQ_Studio.h"
#include "VintageTubePreamp_Studio.h"
#include "ClassicCompressor.h"

struct TestResult {
    std::string name;
    bool processes;
    float changePercent;
    std::string status;
};

TestResult testEngine(std::unique_ptr<EngineBase> engine, const std::string& name) {
    TestResult result;
    result.name = name;
    
    // Prepare
    engine->prepareToPlay(48000.0, 512);
    
    // Create test buffer
    juce::AudioBuffer<float> buffer(2, 512);
    juce::AudioBuffer<float> original(2, 512);
    
    // Fill with test signal
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 512; ++i) {
            float sample = 0.5f * std::sin(2.0f * M_PI * 1000.0f * i / 48000.0f);
            buffer.setSample(ch, i, sample);
            original.setSample(ch, i, sample);
        }
    }
    
    // Set parameters to trigger processing
    std::map<int, float> params;
    
    // Set specific params based on engine type
    if (name.find("EQ") != std::string::npos) {
        // For EQ engines, boost some bands
        params[1] = 0.9f;  // Gain high
        params[4] = 0.9f;  // Gain high
        params[7] = 0.9f;  // Gain high
    } else if (name.find("Tube") != std::string::npos || name.find("Distortion") != std::string::npos) {
        params[0] = 0.9f;  // Drive high
    } else if (name.find("Compressor") != std::string::npos) {
        params[0] = 0.1f;  // Threshold low
        params[1] = 0.9f;  // Ratio high
    } else {
        // Generic params
        for (int i = 0; i < 15; ++i) {
            params[i] = 0.7f;
        }
    }
    
    engine->updateParameters(params);
    
    // Process
    engine->process(buffer);
    
    // Compare
    int changedSamples = 0;
    float maxDiff = 0;
    
    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < 512; ++i) {
            float orig = original.getSample(ch, i);
            float proc = buffer.getSample(ch, i);
            
            if (!std::isfinite(proc)) {
                result.status = "ERROR: NaN/Inf!";
                result.processes = false;
                return result;
            }
            
            float diff = std::abs(proc - orig);
            maxDiff = std::max(maxDiff, diff);
            if (diff > 0.0001f) {
                changedSamples++;
            }
        }
    }
    
    result.changePercent = (float)changedSamples / (2 * 512) * 100.0f;
    
    if (maxDiff < 0.0001f) {
        result.status = "NO CHANGE";
        result.processes = false;
    } else if (maxDiff < 0.001f) {
        result.status = "MINIMAL";
        result.processes = false;
    } else {
        result.status = "PROCESSING";
        result.processes = true;
    }
    
    return result;
}

int main() {
    std::cout << "=== Standalone Engine Processing Test ===" << std::endl;
    std::cout << std::endl;
    
    std::vector<TestResult> results;
    
    // Test the Studio engines
    std::cout << "Testing Studio Engines:" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    {
        auto engine = std::make_unique<ParametricEQ_Studio>();
        TestResult r = testEngine(std::move(engine), "ParametricEQ_Studio");
        std::cout << std::setw(25) << std::left << r.name 
                  << " | " << std::setw(12) << r.status 
                  << " | " << std::fixed << std::setprecision(1) 
                  << r.changePercent << "% changed" << std::endl;
        results.push_back(r);
    }
    
    {
        auto engine = std::make_unique<VintageConsoleEQ_Studio>();
        TestResult r = testEngine(std::move(engine), "VintageConsoleEQ_Studio");
        std::cout << std::setw(25) << std::left << r.name 
                  << " | " << std::setw(12) << r.status 
                  << " | " << std::fixed << std::setprecision(1) 
                  << r.changePercent << "% changed" << std::endl;
        results.push_back(r);
    }
    
    {
        auto engine = std::make_unique<VintageTubePreamp_Studio>();
        TestResult r = testEngine(std::move(engine), "VintageTubePreamp_Studio");
        std::cout << std::setw(25) << std::left << r.name 
                  << " | " << std::setw(12) << r.status 
                  << " | " << std::fixed << std::setprecision(1) 
                  << r.changePercent << "% changed" << std::endl;
        results.push_back(r);
    }
    
    std::cout << std::endl;
    std::cout << "Testing Other Engines:" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    
    {
        auto engine = std::make_unique<ClassicCompressor>();
        TestResult r = testEngine(std::move(engine), "ClassicCompressor");
        std::cout << std::setw(25) << std::left << r.name 
                  << " | " << std::setw(12) << r.status 
                  << " | " << std::fixed << std::setprecision(1) 
                  << r.changePercent << "% changed" << std::endl;
        results.push_back(r);
    }
    
    // Summary
    std::cout << std::endl;
    std::cout << "=== SUMMARY ===" << std::endl;
    
    int working = 0;
    for (const auto& r : results) {
        if (r.processes) working++;
    }
    
    std::cout << "Working: " << working << "/" << results.size() << std::endl;
    
    std::cout << std::endl;
    std::cout << "Non-working engines:" << std::endl;
    for (const auto& r : results) {
        if (!r.processes) {
            std::cout << "  - " << r.name << ": " << r.status << std::endl;
        }
    }
    
    return 0;
}