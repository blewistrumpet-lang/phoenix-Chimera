// Simple test to identify which engines actually hang
#include <iostream>
#include <iomanip>
#include <memory>
#include <chrono>
#include <thread>
#include <atomic>
#include <future>

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define DEBUG 1

#include <JuceHeader.h>
#include "EngineFactory.h"

// Test with a timeout using threads
bool testEngineWithTimeout(int engineId, int timeoutMs = 100) {
    std::atomic<bool> completed(false);
    std::atomic<bool> success(false);
    
    std::thread testThread([&]() {
        try {
            auto engine = EngineFactory::createEngine(engineId);
            if (!engine) {
                success = false;
                completed = true;
                return;
            }
            
            // Initialize
            engine->prepareToPlay(48000.0, 512);
            
            // Create small test buffer
            juce::AudioBuffer<float> buffer(2, 128); // Smaller buffer for faster test
            buffer.clear();
            
            // Process one block
            engine->process(buffer);
            
            success = true;
            completed = true;
        } catch (...) {
            success = false;
            completed = true;
        }
    });
    
    // Wait for completion or timeout
    auto start = std::chrono::steady_clock::now();
    while (!completed) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        
        if (elapsed > timeoutMs) {
            // Timeout - detach thread and report hang
            testThread.detach();
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    testThread.join();
    return success;
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "\n================================================\n";
    std::cout << "  QUICK ENGINE HANG DETECTION TEST\n";
    std::cout << "================================================\n\n";
    
    // Test ALL engines quickly
    std::cout << "Testing all 57 engines with 100ms timeout...\n\n";
    
    std::vector<int> hangingEngines;
    std::vector<int> workingEngines;
    std::vector<int> failedEngines;
    
    for (int id = 0; id <= 56; ++id) {
        std::cout << "[" << std::setw(2) << id << "] ";
        std::cout.flush();
        
        // Get name first
        std::string name = "Unknown";
        try {
            auto engine = EngineFactory::createEngine(id);
            if (engine) {
                name = engine->getName().toStdString();
            }
        } catch (...) {
            name = "Error";
        }
        
        std::cout << std::setw(30) << std::left << name << " - ";
        std::cout.flush();
        
        // Test with timeout
        auto start = std::chrono::steady_clock::now();
        bool success = testEngineWithTimeout(id, 100);
        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        if (!success && elapsed >= 99) {
            std::cout << "❌ HANGS (timeout)\n";
            hangingEngines.push_back(id);
        } else if (success) {
            std::cout << "✅ OK (" << elapsed << "ms)\n";
            workingEngines.push_back(id);
        } else {
            std::cout << "⚠️  FAILED (" << elapsed << "ms)\n";
            failedEngines.push_back(id);
        }
    }
    
    std::cout << "\n================================================\n";
    std::cout << "                 SUMMARY\n";
    std::cout << "================================================\n";
    
    std::cout << "\n✅ Working: " << workingEngines.size() << " engines\n";
    std::cout << "❌ Hanging: " << hangingEngines.size() << " engines\n";
    std::cout << "⚠️  Failed:  " << failedEngines.size() << " engines\n";
    
    if (!hangingEngines.empty()) {
        std::cout << "\nHanging engines:\n";
        for (int id : hangingEngines) {
            auto engine = EngineFactory::createEngine(id);
            if (engine) {
                std::cout << "  #" << std::setw(2) << id << " - " 
                         << engine->getName() << "\n";
            }
        }
    }
    
    if (!failedEngines.empty()) {
        std::cout << "\nFailed engines:\n";
        for (int id : failedEngines) {
            auto engine = EngineFactory::createEngine(id);
            if (engine) {
                std::cout << "  #" << std::setw(2) << id << " - " 
                         << engine->getName() << "\n";
            }
        }
    }
    
    std::cout << "\n";
    return 0;
}