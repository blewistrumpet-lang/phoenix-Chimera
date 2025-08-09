#include <JuceHeader.h>
#include "EngineFactory.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>

bool testEngine(int id, const std::string& name) {
    try {
        auto engine = EngineFactory::createEngine(id);
        if (!engine) {
            std::cout << "[" << std::setw(2) << id << "] " << std::setw(30) << std::left << name << " NULL ENGINE\n";
            return false;
        }
        
        const double sampleRate = 44100.0;
        const int blockSize = 512;
        
        engine->prepareToPlay(sampleRate, blockSize);
        
        juce::AudioBuffer<float> buffer(2, blockSize);
        
        // Fill with test signal
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                data[i] = 0.1f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * i / sampleRate);
            }
        }
        
        // Process with timeout
        std::atomic<bool> done(false);
        std::atomic<bool> hasNanInf(false);
        
        std::thread processThread([&]() {
            engine->process(buffer);
            
            // Check for NaN/Inf
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    if (std::isnan(data[i]) || std::isinf(data[i])) {
                        hasNanInf.store(true);
                        break;
                    }
                }
                if (hasNanInf.load()) break;
            }
            done.store(true);
        });
        
        // Wait up to 1 second
        auto start = std::chrono::steady_clock::now();
        while (!done.load() && 
               std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        if (!done.load()) {
            std::cout << "[" << std::setw(2) << id << "] " << std::setw(30) << std::left << name << " TIMEOUT\n";
            processThread.detach();
            return false;
        }
        
        processThread.join();
        
        if (hasNanInf.load()) {
            std::cout << "[" << std::setw(2) << id << "] " << std::setw(30) << std::left << name << " NaN/Inf\n";
            return false;
        }
        
        std::cout << "[" << std::setw(2) << id << "] " << std::setw(30) << std::left << name << " PASS\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cout << "[" << std::setw(2) << id << "] " << std::setw(30) << std::left << name << " EXCEPTION: " << e.what() << "\n";
        return false;
    }
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "\nCHIMERA PHOENIX - ENGINE RETEST\n";
    std::cout << "Testing replaced engines #1, #22, #40, #46, #56\n";
    std::cout << "=====================================\n\n";
    
    // Test the 5 replaced engines
    int passed = 0;
    passed += testEngine(1, "Vintage Opto Platinum");
    passed += testEngine(22, "K-Style Overdrive");
    passed += testEngine(40, "Spring Reverb Platinum");
    passed += testEngine(46, "Dimension Expander");
    passed += testEngine(56, "Phase Align Platinum");
    
    std::cout << "\n=====================================\n";
    std::cout << "RESULTS: " << passed << "/5 engines passed\n\n";
    
    return (passed == 5) ? 0 : 1;
}