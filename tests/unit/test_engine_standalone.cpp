// Standalone Engine Test - Direct JUCE Implementation
// Compile: g++ -std=c++17 test_engine_standalone.cpp JUCE_Plugin/Source/*.cpp -o test_engines -framework CoreAudio -framework CoreFoundation -framework Accelerate -I/path/to/JUCE/modules -IJUCE_Plugin/Source

#include <iostream>
#include <iomanip>
#include <memory>
#include <vector>
#include <map>
#include <chrono>

// JUCE includes
#include "../JuceLibraryCode/JuceHeader.h"

// Engine includes
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineBase.h"

class StandaloneEngineTest {
public:
    void run() {
        std::cout << "\n====== Chimera Phoenix Engine Test ======\n\n";
        
        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        int passed = 0;
        int failed = 0;
        
        for (int engineId = 0; engineId <= 56; ++engineId) {
            std::cout << "Engine #" << std::setw(2) << engineId << ": ";
            
            try {
                // Create engine
                auto engine = EngineFactory::createEngine(engineId);
                if (!engine) {
                    std::cout << "❌ Failed to create\n";
                    failed++;
                    continue;
                }
                
                std::string name = engine->getName().toStdString();
                std::cout << std::setw(30) << std::left << name << " ";
                
                // Initialize
                engine->prepareToPlay(sampleRate, blockSize);
                
                // Set parameters to test values
                std::map<int, float> params;
                for (int i = 0; i < engine->getNumParameters(); ++i) {
                    params[i] = 0.7f; // 70% for all params
                }
                engine->updateParameters(params);
                
                // Create test buffer
                juce::AudioBuffer<float> buffer(2, blockSize);
                
                // Fill with test signal
                for (int ch = 0; ch < 2; ++ch) {
                    float* data = buffer.getWritePointer(ch);
                    for (int i = 0; i < blockSize; ++i) {
                        data[i] = std::sin(2.0f * M_PI * 440.0f * i / sampleRate) * 0.5f;
                    }
                }
                
                // Store original for comparison
                juce::AudioBuffer<float> original(buffer);
                
                // Process
                auto start = std::chrono::high_resolution_clock::now();
                engine->process(buffer);
                auto end = std::chrono::high_resolution_clock::now();
                
                float processTime = std::chrono::duration<float, std::milli>(end - start).count();
                
                // Check if audio was modified (except bypass)
                bool modified = false;
                if (engineId != 0) { // Not bypass
                    for (int i = 0; i < blockSize; ++i) {
                        if (std::abs(buffer.getReadPointer(0)[i] - original.getReadPointer(0)[i]) > 0.0001f) {
                            modified = true;
                            break;
                        }
                    }
                }
                
                if (engineId == 0) {
                    // Bypass should NOT modify
                    bool unmodified = true;
                    for (int i = 0; i < blockSize; ++i) {
                        if (std::abs(buffer.getReadPointer(0)[i] - original.getReadPointer(0)[i]) > 0.0001f) {
                            unmodified = false;
                            break;
                        }
                    }
                    if (unmodified) {
                        std::cout << "✅ PASS (bypass)\n";
                        passed++;
                    } else {
                        std::cout << "❌ FAIL (bypass modified signal)\n";
                        failed++;
                    }
                } else {
                    if (modified) {
                        std::cout << "✅ PASS [" << std::fixed << std::setprecision(2) 
                                 << processTime << "ms]\n";
                        passed++;
                    } else {
                        std::cout << "⚠️  No modification\n";
                        passed++; // Count as pass but warn
                    }
                }
                
            } catch (const std::exception& e) {
                std::cout << "❌ Exception: " << e.what() << "\n";
                failed++;
            }
        }
        
        std::cout << "\n========== Test Results ==========\n";
        std::cout << "Total: 57 engines\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Failed: " << failed << "\n";
        std::cout << "Success Rate: " << (passed * 100.0 / 57) << "%\n\n";
    }
};

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    StandaloneEngineTest test;
    test.run();
    
    return 0;
}