#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>

// JUCE includes
#include "/Users/Branden/JUCE/modules/juce_core/juce_core.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "/Users/Branden/JUCE/modules/juce_dsp/juce_dsp.h"

// Project includes
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

void debugEngineParameters(int engineID, const std::string& expectedName) {
    std::cout << "\n=== Debugging Engine ID " << engineID << " (" << expectedName << ") ===" << std::endl;
    
    try {
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            std::cout << "FAIL: Could not create engine" << std::endl;
            return;
        }

        std::string actualName = engine->getName().toStdString();
        std::cout << "Engine Name: " << actualName << std::endl;
        
        engine->prepareToPlay(44100.0, 512);
        
        int numParams = engine->getNumParameters();
        std::cout << "Number of Parameters: " << numParams << std::endl;
        
        // List all parameters
        for (int i = 0; i < numParams; ++i) {
            std::string paramName = engine->getParameterName(i).toStdString();
            std::cout << "  Parameter " << i << ": " << paramName << std::endl;
        }
        
        // Test with focused parameter settings for each engine type
        std::map<int, float> testParams;
        
        if (actualName.find("Compressor") != std::string::npos || 
            actualName.find("Opto") != std::string::npos) {
            
            std::cout << "\nTesting Compressor Parameters:" << std::endl;
            
            // Try common compressor parameter names and indices
            for (int p = 0; p < numParams; ++p) {
                std::string paramName = engine->getParameterName(p).toStdString();
                std::transform(paramName.begin(), paramName.end(), paramName.begin(), ::tolower);
                
                std::cout << "  Setting parameter " << p << " (" << paramName << "): ";
                
                if (paramName.find("threshold") != std::string::npos) {
                    testParams[p] = 0.2f; // Low threshold
                    std::cout << "0.2 (low threshold)" << std::endl;
                } else if (paramName.find("ratio") != std::string::npos) {
                    testParams[p] = 0.8f; // High ratio
                    std::cout << "0.8 (high ratio)" << std::endl;
                } else if (paramName.find("attack") != std::string::npos) {
                    testParams[p] = 0.1f; // Fast attack
                    std::cout << "0.1 (fast attack)" << std::endl;
                } else if (paramName.find("release") != std::string::npos) {
                    testParams[p] = 0.4f; // Medium release
                    std::cout << "0.4 (medium release)" << std::endl;
                } else if (paramName.find("gain") != std::string::npos && 
                          paramName.find("makeup") == std::string::npos &&
                          paramName.find("output") == std::string::npos) {
                    testParams[p] = 0.7f; // Higher input gain
                    std::cout << "0.7 (higher input gain)" << std::endl;
                } else if (paramName.find("peak") != std::string::npos && 
                          paramName.find("reduction") != std::string::npos) {
                    testParams[p] = 0.8f; // High peak reduction
                    std::cout << "0.8 (high peak reduction)" << std::endl;
                } else if (paramName.find("mix") != std::string::npos) {
                    testParams[p] = 1.0f; // Full wet
                    std::cout << "1.0 (full wet)" << std::endl;
                } else {
                    testParams[p] = 0.5f; // Default middle
                    std::cout << "0.5 (default)" << std::endl;
                }
            }
            
        } else if (actualName.find("Gate") != std::string::npos) {
            
            std::cout << "\nTesting Gate Parameters:" << std::endl;
            
            for (int p = 0; p < numParams; ++p) {
                std::string paramName = engine->getParameterName(p).toStdString();
                std::transform(paramName.begin(), paramName.end(), paramName.begin(), ::tolower);
                
                std::cout << "  Setting parameter " << p << " (" << paramName << "): ";
                
                if (paramName.find("threshold") != std::string::npos) {
                    testParams[p] = 0.4f; // Medium threshold
                    std::cout << "0.4 (medium threshold)" << std::endl;
                } else if (paramName.find("attack") != std::string::npos) {
                    testParams[p] = 0.1f; // Fast attack
                    std::cout << "0.1 (fast attack)" << std::endl;
                } else if (paramName.find("release") != std::string::npos) {
                    testParams[p] = 0.5f; // Medium release
                    std::cout << "0.5 (medium release)" << std::endl;
                } else if (paramName.find("range") != std::string::npos) {
                    testParams[p] = 0.8f; // High range (more attenuation)
                    std::cout << "0.8 (high range)" << std::endl;
                } else {
                    testParams[p] = 0.5f; // Default
                    std::cout << "0.5 (default)" << std::endl;
                }
            }
            
        } else {
            // For other dynamics processors (transient shaper, limiter, dynamic eq)
            std::cout << "\nTesting Generic Dynamic Parameters:" << std::endl;
            for (int p = 0; p < numParams; ++p) {
                testParams[p] = (p % 2 == 0) ? 0.7f : 0.3f;
                std::string paramName = engine->getParameterName(p).toStdString();
                std::cout << "  Parameter " << p << " (" << paramName << "): " << testParams[p] << std::endl;
            }
        }
        
        // Apply parameters
        engine->updateParameters(testParams);
        std::cout << "\nParameters applied successfully" << std::endl;
        
        // Test with loud signal
        juce::AudioBuffer<float> testBuffer(2, 512);
        
        // Generate loud sine wave
        for (int i = 0; i < 512; ++i) {
            float sample = 0.9f * std::sin(2.0f * 3.14159f * 440.0f * i / 44100.0f);
            testBuffer.setSample(0, i, sample);
            testBuffer.setSample(1, i, sample);
        }
        
        // Store original
        std::vector<float> originalSamples;
        for (int i = 0; i < 512; ++i) {
            originalSamples.push_back(testBuffer.getSample(0, i));
        }
        
        // Process
        engine->process(testBuffer);
        
        // Compare
        float totalOriginal = 0.0f, totalProcessed = 0.0f;
        for (int i = 0; i < 512; ++i) {
            totalOriginal += std::abs(originalSamples[i]);
            totalProcessed += std::abs(testBuffer.getSample(0, i));
        }
        
        float avgOriginal = totalOriginal / 512.0f;
        float avgProcessed = totalProcessed / 512.0f;
        float gainChange = 20.0f * std::log10(avgProcessed / std::max(0.001f, avgOriginal));
        
        std::cout << "Average Original Level: " << avgOriginal << std::endl;
        std::cout << "Average Processed Level: " << avgProcessed << std::endl;
        std::cout << "Gain Change: " << gainChange << " dB" << std::endl;
        
        if (std::abs(gainChange) > 0.5f) {
            std::cout << "✓ PROCESSING DETECTED!" << std::endl;
        } else {
            std::cout << "⚠ NO SIGNIFICANT PROCESSING" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== DYNAMICS ENGINES PARAMETER DEBUGGING ===" << std::endl;
    
    // Debug the failing engines
    debugEngineParameters(1, "VintageOptoCompressor_Platinum");
    debugEngineParameters(2, "ClassicCompressor");
    debugEngineParameters(4, "NoiseGate_Platinum");
    
    // Also debug a working one for comparison
    debugEngineParameters(5, "MasteringLimiter_Platinum");
    
    return 0;
}