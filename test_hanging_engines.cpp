// Test for Hanging Engines with Timeout Protection
#include <iostream>
#include <iomanip>
#include <memory>
#include <cmath>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define DEBUG 1

#include <JuceHeader.h>
#include "EngineFactory.h"

// List of engines that hang
std::vector<int> hangingEngines = {
    25,  // Analog Phaser
    26,  // Platinum Ring Modulator  
    42,  // Shimmer Reverb
    48,  // Spectral Gate Platinum
    50,  // Phased Vocoder
    51,  // Granular Cloud
    52   // Feedback Network
};

std::string getEngineName(int id) {
    auto engine = EngineFactory::createEngine(id);
    if (engine) {
        return engine->getName().toStdString();
    }
    return "Unknown";
}

// Test engine in a separate process with timeout
bool testEngineWithTimeout(int engineId, int timeoutSeconds = 2) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process - test the engine
        try {
            juce::ScopedJuceInitialiser_GUI juceInit;
            
            auto engine = EngineFactory::createEngine(engineId);
            if (!engine) {
                exit(1);
            }
            
            const double sampleRate = 48000.0;
            const int blockSize = 512;
            
            // Test 1: Initialization
            engine->prepareToPlay(sampleRate, blockSize);
            
            // Test 2: Parameter setting
            std::map<int, float> params;
            for (int i = 0; i < engine->getNumParameters(); ++i) {
                params[i] = 0.5f;
            }
            engine->updateParameters(params);
            
            // Test 3: Process one block
            juce::AudioBuffer<float> buffer(2, blockSize);
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < blockSize; ++i) {
                    data[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate);
                }
            }
            
            // This is where it likely hangs
            engine->process(buffer);
            
            // If we get here, it didn't hang
            exit(0);
        } catch (...) {
            exit(2);
        }
    } else if (pid > 0) {
        // Parent process - wait with timeout
        int status;
        alarm(timeoutSeconds);
        
        if (waitpid(pid, &status, 0) == -1) {
            // Timeout occurred
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return false;
        }
        
        alarm(0); // Cancel alarm
        
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status) == 0;
        }
        return false;
    } else {
        // Fork failed
        return false;
    }
}

// Test to identify WHERE the hang occurs
void diagnoseHangingEngine(int engineId) {
    std::cout << "\n========================================\n";
    std::cout << "Diagnosing Engine #" << engineId << "\n";
    std::cout << "========================================\n";
    
    // Test 1: Can we create it?
    pid_t pid = fork();
    if (pid == 0) {
        juce::ScopedJuceInitialiser_GUI juceInit;
        auto engine = EngineFactory::createEngine(engineId);
        if (engine) {
            std::cout << "✓ Creation successful: " << engine->getName() << "\n";
            exit(0);
        }
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WEXITSTATUS(status) != 0) {
            std::cout << "✗ Failed to create engine\n";
            return;
        }
    }
    
    // Test 2: Can we initialize it?
    pid = fork();
    if (pid == 0) {
        juce::ScopedJuceInitialiser_GUI juceInit;
        auto engine = EngineFactory::createEngine(engineId);
        engine->prepareToPlay(48000.0, 512);
        std::cout << "✓ Initialization successful\n";
        exit(0);
    } else {
        int status;
        alarm(2);
        if (waitpid(pid, &status, 0) == -1) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            std::cout << "✗ HANGS during initialization!\n";
            return;
        }
        alarm(0);
    }
    
    // Test 3: Can we set parameters?
    pid = fork();
    if (pid == 0) {
        juce::ScopedJuceInitialiser_GUI juceInit;
        auto engine = EngineFactory::createEngine(engineId);
        engine->prepareToPlay(48000.0, 512);
        
        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            params[i] = 0.0f; // Start with all zeros
        }
        engine->updateParameters(params);
        std::cout << "✓ Parameter setting successful\n";
        exit(0);
    } else {
        int status;
        alarm(2);
        if (waitpid(pid, &status, 0) == -1) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            std::cout << "✗ HANGS during parameter setting!\n";
            return;
        }
        alarm(0);
    }
    
    // Test 4: Process with silence
    pid = fork();
    if (pid == 0) {
        juce::ScopedJuceInitialiser_GUI juceInit;
        auto engine = EngineFactory::createEngine(engineId);
        engine->prepareToPlay(48000.0, 512);
        
        // Process silence
        juce::AudioBuffer<float> buffer(2, 512);
        buffer.clear();
        engine->process(buffer);
        std::cout << "✓ Processing silence successful\n";
        exit(0);
    } else {
        int status;
        alarm(2);
        if (waitpid(pid, &status, 0) == -1) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            std::cout << "✗ HANGS when processing silence!\n";
            return;
        }
        alarm(0);
    }
    
    // Test 5: Process with signal
    pid = fork();
    if (pid == 0) {
        juce::ScopedJuceInitialiser_GUI juceInit;
        auto engine = EngineFactory::createEngine(engineId);
        engine->prepareToPlay(48000.0, 512);
        
        juce::AudioBuffer<float> buffer(2, 512);
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < 512; ++i) {
                data[i] = 0.1f * std::sin(2.0f * M_PI * 440.0f * i / 48000.0);
            }
        }
        engine->process(buffer);
        std::cout << "✓ Processing signal successful\n";
        exit(0);
    } else {
        int status;
        alarm(2);
        if (waitpid(pid, &status, 0) == -1) {
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            std::cout << "✗ HANGS when processing audio signal!\n";
            return;
        }
        alarm(0);
    }
    
    std::cout << "✓ All tests passed - no hang detected!\n";
}

int main() {
    std::cout << "\n================================================\n";
    std::cout << "  HANGING ENGINE DIAGNOSTIC TEST\n";
    std::cout << "================================================\n";
    
    std::cout << "\nTesting " << hangingEngines.size() << " potentially hanging engines...\n";
    std::cout << "Each test has a 2-second timeout.\n";
    
    for (int engineId : hangingEngines) {
        diagnoseHangingEngine(engineId);
    }
    
    std::cout << "\n================================================\n";
    std::cout << "  QUICK HANG TEST WITH TIMEOUT\n";
    std::cout << "================================================\n\n";
    
    for (int engineId : hangingEngines) {
        std::cout << "Engine #" << std::setw(2) << engineId << " - ";
        
        bool success = testEngineWithTimeout(engineId, 2);
        if (success) {
            std::cout << "✅ No hang detected\n";
        } else {
            std::cout << "❌ HANGS or crashes\n";
        }
    }
    
    std::cout << "\n================================================\n";
    std::cout << "            DIAGNOSIS COMPLETE\n";
    std::cout << "================================================\n\n";
    
    return 0;
}