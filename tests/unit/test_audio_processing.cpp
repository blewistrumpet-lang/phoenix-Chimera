/**
 * Audio Processing Tester
 * Tests if each engine actually processes/modifies audio
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <map>
#include <memory>
#include "../JuceLibraryCode/JuceHeader.h"
#include "EngineFactory.h"
#include "EngineTypes.h"

// Simple test to see if engine modifies audio at all
bool testEngineProcessing(int engineID) {
    auto engine = EngineFactory::createEngine(engineID);
    if (!engine) {
        std::cout << "  ✗ Failed to create" << std::endl;
        return false;
    }
    
    std::string name = engine->getName().toStdString();
    std::cout << std::setw(30) << std::left << name;
    
    // Prepare engine
    engine->prepareToPlay(48000.0, 512);
    
    // Create test buffer with sine wave
    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < 512; ++i) {
            data[i] = 0.5f * std::sin(2.0f * M_PI * 1000.0f * i / 48000.0f);
        }
    }
    
    // Store original RMS
    float originalRMS = 0;
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            originalRMS += data[i] * data[i];
        }
    }
    originalRMS = std::sqrt(originalRMS / (2 * 512));
    
    // Copy for comparison
    juce::AudioBuffer<float> original(buffer);
    
    // Set parameters to make effect obvious
    std::map<int, float> params;
    
    // Try to set parameters that should cause processing
    // Different engines need different params to be active
    for (int i = 0; i < 15; ++i) {
        params[i] = 0.7f; // Set most params to 70%
    }
    
    // Special cases for known parameter mappings
    switch (engineID) {
        case 7: // ParametricEQ_Studio - set gain params
            params[1] = 0.9f; // Band 1 gain high
            params[4] = 0.9f; // Band 2 gain high
            params[7] = 0.9f; // Band 3 gain high
            break;
        case 15: // VintageTubePreamp_Studio
            params[0] = 0.9f; // Drive high
            break;
        case 2: // ClassicCompressor
            params[0] = 0.1f; // Threshold low to trigger compression
            params[1] = 0.9f; // Ratio high
            break;
    }
    
    engine->updateParameters(params);
    
    // Process
    engine->process(buffer);
    
    // Calculate processed RMS
    float processedRMS = 0;
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            processedRMS += data[i] * data[i];
        }
    }
    processedRMS = std::sqrt(processedRMS / (2 * 512));
    
    // Check if audio was modified
    float maxDiff = 0;
    int diffCount = 0;
    for (int ch = 0; ch < 2; ++ch) {
        const float* proc = buffer.getReadPointer(ch);
        const float* orig = original.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            float diff = std::abs(proc[i] - orig[i]);
            maxDiff = std::max(maxDiff, diff);
            if (diff > 0.0001f) diffCount++;
        }
    }
    
    // Check for NaN/Inf
    bool hasInvalid = false;
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            if (!std::isfinite(data[i])) {
                hasInvalid = true;
                break;
            }
        }
    }
    
    // Report results
    if (hasInvalid) {
        std::cout << "✗ PRODUCES NaN/Inf!" << std::endl;
        return false;
    }
    
    float rmsChange = std::abs(processedRMS - originalRMS);
    float percentDiff = (float)diffCount / (2 * 512) * 100.0f;
    
    if (maxDiff < 0.0001f) {
        std::cout << "✗ NO PROCESSING (no change detected)" << std::endl;
        return false;
    } else if (maxDiff < 0.001f) {
        std::cout << "⚠ MINIMAL change (max diff: " << maxDiff << ")" << std::endl;
        return false;
    } else {
        std::cout << "✓ Processing (" 
                  << std::fixed << std::setprecision(1) << percentDiff << "% samples changed, "
                  << "RMS Δ: " << std::setprecision(3) << rmsChange << ")" << std::endl;
        return true;
    }
}

int main() {
    std::cout << "=== Testing Audio Processing for All Engines ===" << std::endl;
    std::cout << "Testing with 1kHz sine wave at 0.5 amplitude" << std::endl;
    std::cout << std::endl;
    
    const char* categories[] = {
        "DYNAMICS & COMPRESSION (1-6)",
        "FILTERS & EQ (7-14)",
        "DISTORTION & SATURATION (15-22)",
        "MODULATION (23-33)",
        "REVERB & DELAY (34-43)",
        "SPATIAL & SPECIAL (44-52)",
        "UTILITY (53-56)"
    };
    
    int categoryStarts[] = {1, 7, 15, 23, 34, 44, 53, 57};
    
    int totalEngines = 0;
    int workingEngines = 0;
    int notProcessing = 0;
    
    // Test Engine 0 (None) separately
    std::cout << "Engine 0: ";
    testEngineProcessing(0);
    std::cout << std::endl;
    
    // Test all other engines by category
    for (int cat = 0; cat < 7; ++cat) {
        std::cout << categories[cat] << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        
        for (int id = categoryStarts[cat]; id < categoryStarts[cat + 1] && id <= 56; ++id) {
            std::cout << "Engine " << std::setw(2) << id << ": ";
            totalEngines++;
            
            if (testEngineProcessing(id)) {
                workingEngines++;
            } else {
                notProcessing++;
            }
        }
        std::cout << std::endl;
    }
    
    // Summary
    std::cout << "=== SUMMARY ===" << std::endl;
    std::cout << "Total engines tested: " << totalEngines << std::endl;
    std::cout << "Working (modifying audio): " << workingEngines << std::endl;
    std::cout << "Not processing: " << notProcessing << std::endl;
    std::cout << "Success rate: " << (workingEngines * 100 / totalEngines) << "%" << std::endl;
    
    return 0;
}