// Detailed Engine Test - Provides diagnostic info for failed engines
#include <iostream>
#include <iomanip>
#include <memory>
#include <cmath>
#include <vector>
#include <chrono>
#include <map>

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define DEBUG 1

#include <JuceHeader.h>
#include "EngineFactory.h"

struct DetailedTestResult {
    int id;
    std::string name;
    bool created = false;
    bool initialized = false;
    bool processed = false;
    bool hasNaN = false;
    bool hasInf = false;
    bool hangs = false;
    int nanCount = 0;
    int infCount = 0;
    int firstNaNSample = -1;
    int firstInfSample = -1;
    float inputRMS = 0.0f;
    float outputRMS = 0.0f;
    float processingTimeMs = 0.0f;
    std::vector<std::string> parameterNames;
    std::map<int, float> parameterValues;
    std::string errorDetails;
};

float calculateRMS(const float* data, int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        sum += data[i] * data[i];
    }
    return std::sqrt(sum / numSamples);
}

DetailedTestResult testEngineDetailed(int id, double sampleRate, int blockSize) {
    DetailedTestResult result;
    result.id = id;
    
    try {
        auto engine = EngineFactory::createEngine(id);
        if (!engine) {
            result.errorDetails = "Failed to create engine";
            return result;
        }
        result.created = true;
        result.name = engine->getName().toStdString();
        
        // Get parameter info
        int numParams = engine->getNumParameters();
        for (int i = 0; i < numParams; ++i) {
            result.parameterNames.push_back(engine->getParameterName(i).toStdString());
        }
        
        // Initialize
        engine->prepareToPlay(sampleRate, blockSize);
        result.initialized = true;
        
        // Set parameters to safe middle values
        std::map<int, float> params;
        for (int i = 0; i < numParams; ++i) {
            float value = 0.5f;
            
            // Special handling for certain parameter types
            std::string paramName = result.parameterNames[i];
            std::transform(paramName.begin(), paramName.end(), paramName.begin(), ::tolower);
            
            if (paramName.find("mix") != std::string::npos || 
                paramName.find("wet") != std::string::npos) {
                value = 0.5f; // 50% mix
            } else if (paramName.find("gain") != std::string::npos || 
                      paramName.find("volume") != std::string::npos) {
                value = 0.5f; // Moderate gain
            } else if (paramName.find("feedback") != std::string::npos) {
                value = 0.0f; // No feedback initially
            }
            
            params[i] = value;
            result.parameterValues[i] = value;
        }
        engine->updateParameters(params);
        
        // Create test buffer with sine wave
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        for (int ch = 0; ch < 2; ++ch) {
            float* data = inputBuffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                data[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
            }
        }
        
        // Calculate input RMS
        result.inputRMS = calculateRMS(inputBuffer.getReadPointer(0), blockSize);
        
        // Copy for processing
        juce::AudioBuffer<float> buffer(inputBuffer);
        
        // Process with timing
        auto start = std::chrono::high_resolution_clock::now();
        
        // Process directly (remove async for now as it requires additional headers)
        engine->process(buffer);
        
        auto end = std::chrono::high_resolution_clock::now();
        result.processingTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
        result.processed = true;
        
        // Calculate output RMS
        result.outputRMS = calculateRMS(buffer.getReadPointer(0), blockSize);
        
        // Detailed NaN/Inf analysis
        for (int ch = 0; ch < 2; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                if (std::isnan(data[i])) {
                    result.hasNaN = true;
                    result.nanCount++;
                    if (result.firstNaNSample == -1) {
                        result.firstNaNSample = i;
                    }
                }
                if (std::isinf(data[i])) {
                    result.hasInf = true;
                    result.infCount++;
                    if (result.firstInfSample == -1) {
                        result.firstInfSample = i;
                    }
                }
            }
        }
        
        if (result.hasNaN || result.hasInf) {
            result.errorDetails = "Output contains ";
            if (result.hasNaN) {
                result.errorDetails += std::to_string(result.nanCount) + " NaN values (first at sample " + 
                                      std::to_string(result.firstNaNSample) + ")";
            }
            if (result.hasInf) {
                if (result.hasNaN) result.errorDetails += " and ";
                result.errorDetails += std::to_string(result.infCount) + " Inf values (first at sample " + 
                                      std::to_string(result.firstInfSample) + ")";
            }
        }
        
    } catch (const std::exception& e) {
        result.errorDetails = std::string("Exception: ") + e.what();
    } catch (...) {
        result.errorDetails = "Unknown exception";
    }
    
    return result;
}

void printDetailedResult(const DetailedTestResult& r) {
    std::cout << "\n========================================\n";
    std::cout << "Engine #" << r.id << ": " << r.name << "\n";
    std::cout << "========================================\n";
    
    std::cout << "Status:\n";
    std::cout << "  Created:     " << (r.created ? "✓" : "✗") << "\n";
    std::cout << "  Initialized: " << (r.initialized ? "✓" : "✗") << "\n";
    std::cout << "  Processed:   " << (r.processed ? "✓" : "✗") << "\n";
    
    if (r.processed) {
        std::cout << "\nProcessing Info:\n";
        std::cout << "  Time:        " << std::fixed << std::setprecision(2) << r.processingTimeMs << " ms\n";
        std::cout << "  Input RMS:   " << std::fixed << std::setprecision(4) << r.inputRMS << "\n";
        std::cout << "  Output RMS:  " << std::fixed << std::setprecision(4) << r.outputRMS << "\n";
        std::cout << "  Gain change: " << std::fixed << std::setprecision(2) 
                  << 20.0f * std::log10(r.outputRMS / (r.inputRMS + 0.0001f)) << " dB\n";
    }
    
    if (r.hasNaN || r.hasInf) {
        std::cout << "\n❌ NUMERICAL ERRORS:\n";
        if (r.hasNaN) {
            std::cout << "  NaN values:  " << r.nanCount << " (first at sample " << r.firstNaNSample << ")\n";
        }
        if (r.hasInf) {
            std::cout << "  Inf values:  " << r.infCount << " (first at sample " << r.firstInfSample << ")\n";
        }
    }
    
    if (r.hangs) {
        std::cout << "\n❌ HANGING: Processing took longer than 100ms timeout\n";
    }
    
    if (!r.parameterNames.empty()) {
        std::cout << "\nParameters (" << r.parameterNames.size() << "):\n";
        for (size_t i = 0; i < std::min(size_t(5), r.parameterNames.size()); ++i) {
            std::cout << "  [" << i << "] " << std::setw(20) << std::left << r.parameterNames[i];
            if (r.parameterValues.count(i) > 0) {
                std::cout << " = " << std::fixed << std::setprecision(2) << r.parameterValues.at(i);
            }
            std::cout << "\n";
        }
        if (r.parameterNames.size() > 5) {
            std::cout << "  ... and " << (r.parameterNames.size() - 5) << " more\n";
        }
    }
    
    if (!r.errorDetails.empty()) {
        std::cout << "\nError Details: " << r.errorDetails << "\n";
    }
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "\n================================================\n";
    std::cout << "  DETAILED ANALYSIS OF PROBLEMATIC ENGINES\n";
    std::cout << "================================================\n";
    
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    
    // Test the failed engines
    std::vector<int> problematicEngines = {
        1,   // Vintage Opto Platinum
        22,  // K-Style Overdrive
        40,  // Spring Reverb Platinum
        46,  // Dimension Expander
        56   // Phase Align Platinum
        // Skip the hanging ones for now
    };
    
    std::cout << "\nTesting " << problematicEngines.size() << " problematic engines...\n";
    
    for (int id : problematicEngines) {
        std::cout << "\nTesting Engine #" << id << "...\n";
        DetailedTestResult result = testEngineDetailed(id, sampleRate, blockSize);
        printDetailedResult(result);
    }
    
    // Also run a quick test on some known working engines for comparison
    std::cout << "\n\n================================================\n";
    std::cout << "  COMPARISON: KNOWN WORKING ENGINES\n";
    std::cout << "================================================\n";
    
    std::vector<int> workingEngines = {0, 9, 18, 20};  // Bypass, Ladder Filter, Bit Crusher, Muff Fuzz
    
    for (int id : workingEngines) {
        DetailedTestResult result = testEngineDetailed(id, sampleRate, blockSize);
        std::cout << "\nEngine #" << std::setw(2) << id << " (" << std::setw(25) << std::left << result.name << "): ";
        if (result.processed && !result.hasNaN && !result.hasInf) {
            std::cout << "✅ WORKING (RMS: " << std::fixed << std::setprecision(4) << result.outputRMS 
                     << ", Time: " << std::setprecision(2) << result.processingTimeMs << "ms)\n";
        } else {
            std::cout << "❌ ISSUES\n";
        }
    }
    
    std::cout << "\n================================================\n";
    std::cout << "            ANALYSIS COMPLETE\n";
    std::cout << "================================================\n\n";
    
    return 0;
}