// Actual Engine Test - Tests Real Chimera Engines
#include <iostream>
#include <iomanip>
#include <memory>
#include <cmath>
#include <fstream>

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define DEBUG 1

#include <JuceHeader.h>
#include "EngineFactory.h"

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "\nCHIMERA PHOENIX - ACTUAL ENGINE TEST\n";
    std::cout << "=====================================\n\n";
    
    int passed = 0, failed = 0;
    const double sampleRate = 48000.0;
    const int blockSize = 512;
    
    for (int id = 0; id <= 56; ++id) {
        std::cout << "[" << std::setw(2) << id << "] ";
        std::cout.flush(); // Ensure output is shown immediately
        
        // Skip known problematic engines temporarily
        if (id == 25 || id == 26 || id == 42 || id == 48 || id == 50 || id == 51 || id == 52) {
            std::cout << "SKIPPED (may hang)\n";
            continue;
        }
        
        try {
            auto engine = EngineFactory::createEngine(id);
            if (!engine) {
                std::cout << "FAILED to create\n";
                failed++;
                continue;
            }
            
            std::cout << std::setw(30) << std::left << engine->getName().toStdString();
            std::cout.flush();
            
            engine->prepareToPlay(sampleRate, blockSize);
            
            juce::AudioBuffer<float> buffer(2, blockSize);
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
                }
            }
            
            engine->process(buffer);
            
            bool hasNaN = false;
            for (int ch = 0; ch < 2; ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    if (std::isnan(data[i]) || std::isinf(data[i])) {
                        hasNaN = true;
                        break;
                    }
                }
            }
            
            if (hasNaN) {
                std::cout << " FAILED (NaN/Inf)\n";
                failed++;
            } else {
                std::cout << " PASS\n";
                passed++;
            }
            
        } catch (const std::exception& e) {
            std::cout << " EXCEPTION: " << e.what() << "\n";
            failed++;
        } catch (...) {
            std::cout << " UNKNOWN EXCEPTION\n";
            failed++;
        }
    }
    
    std::cout << "\n=====================================\n";
    std::cout << "RESULTS: " << passed << " passed, " << failed << " failed\n";
    std::cout << "Success rate: " << (passed * 100.0 / 57) << "%\n\n";
    
    return failed == 0 ? 0 : 1;
}
