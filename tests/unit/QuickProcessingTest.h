#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "EngineFactory.h"
#include <iostream>
#include <iomanip>

class QuickProcessingTest {
public:
    struct TestResult {
        int engineID;
        juce::String engineName;
        bool processes = false;
        float maxDifference = 0.0f;
        float rmsChange = 0.0f;
        bool hasNaN = false;
        juce::String status;
    };
    
    static TestResult testEngine(int engineID) {
        TestResult result;
        result.engineID = engineID;
        
        // Create engine
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            result.status = "Failed to create";
            return result;
        }
        
        result.engineName = engine->getName();
        
        // Prepare
        engine->prepareToPlay(48000.0, 512);
        
        // Create test signal
        juce::AudioBuffer<float> buffer(2, 512);
        juce::AudioBuffer<float> original(2, 512);
        
        // Fill with 1kHz sine
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            float* orig = original.getWritePointer(ch);
            for (int i = 0; i < 512; ++i) {
                float sample = 0.5f * std::sin(2.0f * M_PI * 1000.0f * i / 48000.0f);
                data[i] = sample;
                orig[i] = sample;
            }
        }
        
        // Set aggressive parameters to trigger processing
        std::map<int, float> params;
        
        // Set all params to various values
        for (int i = 0; i < 15; ++i) {
            // Use different values to trigger different behaviors
            if (i % 3 == 0) params[i] = 0.8f;  // High values
            else if (i % 3 == 1) params[i] = 0.2f;  // Low values  
            else params[i] = 0.5f;  // Mid values
        }
        
        // Override specific params for known engines
        switch (engineID) {
            case 7: // ParametricEQ - boost gains
                params[1] = 1.0f;  // Band 1 gain max
                params[4] = 1.0f;  // Band 2 gain max
                break;
            case 8: // VintageConsoleEQ - boost bands
                params[1] = 1.0f;  // Low gain
                params[3] = 1.0f;  // LM gain
                break;
            case 15: // VintageTubePreamp
                params[0] = 1.0f;  // Drive max
                break;
            case 1: // Opto Compressor
            case 2: // Classic Compressor
                params[0] = 0.0f;  // Threshold min (to trigger)
                params[1] = 1.0f;  // Ratio max
                params[2] = 0.0f;  // Attack fast
                break;
        }
        
        engine->updateParameters(params);
        
        // Process
        engine->process(buffer);
        
        // Analyze results
        float originalRMS = 0, processedRMS = 0;
        result.maxDifference = 0;
        int changedSamples = 0;
        
        for (int ch = 0; ch < 2; ++ch) {
            const float* proc = buffer.getReadPointer(ch);
            const float* orig = original.getReadPointer(ch);
            
            for (int i = 0; i < 512; ++i) {
                // Check for NaN/Inf
                if (!std::isfinite(proc[i])) {
                    result.hasNaN = true;
                }
                
                originalRMS += orig[i] * orig[i];
                processedRMS += proc[i] * proc[i];
                
                float diff = std::abs(proc[i] - orig[i]);
                result.maxDifference = std::max(result.maxDifference, diff);
                
                if (diff > 0.0001f) {
                    changedSamples++;
                }
            }
        }
        
        originalRMS = std::sqrt(originalRMS / (2 * 512));
        processedRMS = std::sqrt(processedRMS / (2 * 512));
        result.rmsChange = std::abs(processedRMS - originalRMS);
        
        // Determine status
        if (result.hasNaN) {
            result.status = "ERROR: NaN/Inf produced!";
            result.processes = false;
        } else if (result.maxDifference < 0.0001f) {
            result.status = "NO PROCESSING";
            result.processes = false;
        } else if (result.maxDifference < 0.001f) {
            result.status = "MINIMAL";
            result.processes = false;
        } else {
            result.status = juce::String("OK (") + juce::String(changedSamples * 100 / 1024) + "% changed)";
            result.processes = true;
        }
        
        return result;
    }
    
    static void runAllTests() {
        DBG("=== AUDIO PROCESSING TEST ===");
        
        int working = 0, notWorking = 0, errors = 0;
        
        for (int id = 0; id <= 56; ++id) {
            TestResult result = testEngine(id);
            
            juce::String output = "Engine " + juce::String(id) + " [" + result.engineName + "]: " + result.status;
            
            if (result.hasNaN) {
                errors++;
                DBG("❌ " + output);
            } else if (result.processes) {
                working++;
                DBG("✅ " + output);
            } else {
                notWorking++;
                DBG("⚠️  " + output);
            }
        }
        
        DBG("");
        DBG("=== SUMMARY ===");
        DBG("Working: " + juce::String(working));
        DBG("Not Processing: " + juce::String(notWorking));
        DBG("Errors: " + juce::String(errors));
        DBG("Total: 57 engines");
    }
};