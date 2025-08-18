#include <JuceHeader.h>
#include "EngineFactory.h"
#include <iostream>
#include <memory>
#include <cmath>
#include <chrono>
#include <signal.h>
#include <unistd.h>
#include <atomic>

std::atomic<bool> g_timeout(false);

void timeout_handler(int sig) {
    g_timeout = true;
}

int main() {
    juce::ScopedJuceInitialiser_GUI init;
    
    const double sampleRate = 44100.0;
    const int blockSize = 512;
    const int numChannels = 2;
    
    int passed = 0;
    int failed_nan = 0;
    int failed_timeout = 0;
    
    std::cout << "\n=== Testing All Engines (0-56) ===\n" << std::endl;
    
    for (int id = 0; id <= 56; ++id) {
        auto engine = EngineFactory::createEngine(id);
        if (!engine) {
            std::cout << "Engine #" << id << ": NULL (not implemented)" << std::endl;
            continue;
        }
        
        std::string name = engine->getName().toStdString();
        std::cout << "Engine #" << id << " (" << name << "): ";
        std::cout.flush();
        
        try {
            engine->prepareToPlay(sampleRate, blockSize);
            
            // Set up timeout
            g_timeout = false;
            signal(SIGALRM, timeout_handler);
            alarm(2); // 2 second timeout
            
            bool has_nan = false;
            const int testIterations = 100;
            
            for (int iter = 0; iter < testIterations && !g_timeout; ++iter) {
                juce::AudioBuffer<float> buffer(numChannels, blockSize);
                
                // Fill with test signal
                for (int ch = 0; ch < numChannels; ++ch) {
                    auto* data = buffer.getWritePointer(ch);
                    for (int i = 0; i < blockSize; ++i) {
                        data[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * (iter * blockSize + i) / sampleRate);
                    }
                }
                
                engine->process(buffer);
                
                // Check for NaN/Inf
                for (int ch = 0; ch < numChannels; ++ch) {
                    const auto* data = buffer.getReadPointer(ch);
                    for (int i = 0; i < blockSize; ++i) {
                        if (!std::isfinite(data[i])) {
                            has_nan = true;
                            break;
                        }
                    }
                    if (has_nan) break;
                }
                
                if (has_nan) break;
            }
            
            alarm(0); // Cancel timeout
            
            if (g_timeout) {
                std::cout << "TIMEOUT (hung)" << std::endl;
                failed_timeout++;
            } else if (has_nan) {
                std::cout << "FAILED (NaN/Inf)" << std::endl;
                failed_nan++;
            } else {
                std::cout << "PASSED" << std::endl;
                passed++;
            }
            
        } catch (const std::exception& e) {
            alarm(0);
            std::cout << "EXCEPTION: " << e.what() << std::endl;
            failed_nan++;
        }
    }
    
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed (NaN/Inf): " << failed_nan << std::endl;
    std::cout << "Failed (Timeout): " << failed_timeout << std::endl;
    std::cout << "Total tested: " << (passed + failed_nan + failed_timeout) << std::endl;
    std::cout << "Success rate: " << (100.0 * passed / (passed + failed_nan + failed_timeout)) << "%" << std::endl;
    
    return (failed_nan + failed_timeout) > 0 ? 1 : 0;
}