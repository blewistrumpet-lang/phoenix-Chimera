#pragma once

#include <JuceHeader.h>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>

class EngineTestRunner {
public:
    struct TestResult {
        int id;
        juce::String name;
        bool created;
        bool initialized;
        bool processed;
        bool modifiesAudio;
        float processingTimeMs;
        juce::String error;
    };
    
    static void runAllTests() {
        std::cout << "\n========================================\n";
        std::cout << "   CHIMERA ENGINE TEST SUITE\n";
        std::cout << "========================================\n\n";
        
        std::vector<TestResult> results;
        const double sampleRate = 48000.0;
        const int blockSize = 512;
        
        for (int engineId = 0; engineId <= 56; ++engineId) {
            TestResult result = testEngine(engineId, sampleRate, blockSize);
            results.push_back(result);
            printResult(result);
        }
        
        printSummary(results);
    }
    
private:
    static TestResult testEngine(int engineId, double sampleRate, int blockSize) {
        TestResult result;
        result.id = engineId;
        result.created = false;
        result.initialized = false;
        result.processed = false;
        result.modifiesAudio = false;
        result.processingTimeMs = 0;
        
        std::cout << "[" << std::setw(2) << std::setfill('0') << engineId << "] ";
        
        try {
            auto engine = EngineFactory::createEngine(engineId);
            if (!engine) {
                result.error = "Failed to create";
                return result;
            }
            result.created = true;
            result.name = engine->getName();
            
            std::cout << std::setw(30) << std::setfill(' ') << std::left 
                     << result.name.toStdString() << " ";
            
            engine->prepareToPlay(sampleRate, blockSize);
            result.initialized = true;
            
            // Set test parameters
            std::map<int, float> params;
            for (int i = 0; i < engine->getNumParameters(); ++i) {
                params[i] = 0.7f;
            }
            engine->updateParameters(params);
            
            // Create test buffer
            juce::AudioBuffer<float> buffer(2, blockSize);
            juce::AudioBuffer<float> original(2, blockSize);
            
            // Fill with test signal
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
                }
            }
            original.makeCopyOf(buffer);
            
            // Process and time it
            auto start = std::chrono::high_resolution_clock::now();
            engine->process(buffer);
            auto end = std::chrono::high_resolution_clock::now();
            
            result.processed = true;
            result.processingTimeMs = std::chrono::duration<float, std::milli>(end - start).count();
            
            // Check for modification
            for (int i = 0; i < blockSize; ++i) {
                if (std::abs(buffer.getReadPointer(0)[i] - original.getReadPointer(0)[i]) > 0.0001f) {
                    result.modifiesAudio = true;
                    break;
                }
            }
            
        } catch (const std::exception& e) {
            result.error = juce::String(e.what());
        }
        
        return result;
    }
    
    static void printResult(const TestResult& result) {
        if (!result.created) {
            std::cout << "❌ CREATE FAILED: " << result.error << "\n";
        } else if (!result.initialized) {
            std::cout << "❌ INIT FAILED: " << result.error << "\n";
        } else if (!result.processed) {
            std::cout << "❌ PROCESS FAILED: " << result.error << "\n";
        } else if (result.id == 0) { // Bypass
            if (!result.modifiesAudio) {
                std::cout << "✅ PASS (bypass)\n";
            } else {
                std::cout << "❌ FAIL (bypass modified)\n";
            }
        } else {
            if (result.modifiesAudio) {
                std::cout << "✅ PASS [" << std::fixed << std::setprecision(2) 
                         << result.processingTimeMs << "ms]\n";
            } else {
                std::cout << "⚠️  NO MODIFICATION\n";
            }
        }
    }
    
    static void printSummary(const std::vector<TestResult>& results) {
        int passed = 0, failed = 0, warnings = 0;
        
        for (const auto& r : results) {
            if (!r.created || !r.initialized || !r.processed) {
                failed++;
            } else if (r.id == 0) {
                if (!r.modifiesAudio) passed++;
                else failed++;
            } else {
                if (r.modifiesAudio) passed++;
                else warnings++;
            }
        }
        
        std::cout << "\n========================================\n";
        std::cout << "SUMMARY: ";
        std::cout << passed << " passed, ";
        std::cout << failed << " failed, ";
        std::cout << warnings << " warnings\n";
        std::cout << "Success Rate: " << (passed * 100.0 / 57) << "%\n";
        std::cout << "========================================\n\n";
    }
};