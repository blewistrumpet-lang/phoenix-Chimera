#include <iostream>
#include <memory>
#include <chrono>
#include <vector>
#include <iomanip>
#include "JUCE_Plugin/JuceLibraryCode/JuceHeader.h"
#include "JUCE_Plugin/Source/EngineFactory.h"

#define GREEN "\033[32m"
#define RED "\033[31m"
#define YELLOW "\033[33m"
#define CYAN "\033[36m"
#define RESET "\033[0m"

struct EnginePerformance {
    std::string name;
    double avgTimeMs;
    double maxTimeMs;
    double cpuPercent;
};

void testEnginePerformance(int engineId, const std::string& name, EnginePerformance& result) {
    const int sampleRate = 44100;
    const int blockSize = 512;
    const int numChannels = 2;
    const int numIterations = 1000;
    
    auto engine = EngineFactory::createEngine(engineId);
    if (!engine) {
        result.name = name;
        result.avgTimeMs = -1;
        result.maxTimeMs = -1;
        result.cpuPercent = -1;
        return;
    }
    
    engine->prepareToPlay(sampleRate, blockSize);
    
    // Set typical parameters
    std::map<int, float> params;
    params[0] = 0.7f;
    params[1] = 0.5f;
    params[2] = 0.3f;
    engine->updateParameters(params);
    
    // Create test buffer
    juce::AudioBuffer<float> buffer(numChannels, blockSize);
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < blockSize; ++i) {
            buffer.setSample(ch, i, 0.3f * std::sin(2.0f * M_PI * 440.0f * i / sampleRate));
        }
    }
    
    // Warm up
    for (int i = 0; i < 100; ++i) {
        juce::AudioBuffer<float> tempBuffer(buffer);
        engine->process(tempBuffer);
    }
    
    // Measure performance
    double totalTime = 0;
    double maxTime = 0;
    
    for (int i = 0; i < numIterations; ++i) {
        juce::AudioBuffer<float> tempBuffer(buffer);
        
        auto start = std::chrono::high_resolution_clock::now();
        engine->process(tempBuffer);
        auto end = std::chrono::high_resolution_clock::now();
        
        double elapsed = std::chrono::duration<double, std::milli>(end - start).count();
        totalTime += elapsed;
        maxTime = std::max(maxTime, elapsed);
    }
    
    result.name = name;
    result.avgTimeMs = totalTime / numIterations;
    result.maxTimeMs = maxTime;
    
    // Calculate CPU percentage
    // Time available per block at 44100Hz with 512 samples
    double blockDuration = (blockSize * 1000.0) / sampleRate; // ~11.6ms
    result.cpuPercent = (result.avgTimeMs / blockDuration) * 100.0;
}

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Chimera Phoenix 3.0 - Performance Test" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    std::vector<std::pair<int, std::string>> engines = {
        // Test key engines from each category
        {2, "ClassicCompressor"},
        {4, "NoiseGate_Platinum"},
        {8, "MultibandCompressor"},
        {11, "ParametricEQ"},
        {15, "LinearPhaseEQ"},
        {18, "BitCrusher"},
        {26, "AnalogRingModulator"},
        {27, "Chorus"},
        {35, "PitchShifter"},
        {36, "DigitalDelay"},
        {40, "MultitapDelay"},
        {42, "RoomReverb"},
        {46, "ConvolutionReverb"},
        {47, "Stereoizer"},
        {52, "FeedbackNetwork"}
    };
    
    std::cout << "Testing " << engines.size() << " engines..." << std::endl;
    std::cout << "Block size: 512 samples @ 44100 Hz (11.6ms)" << std::endl;
    std::cout << std::endl;
    
    std::vector<EnginePerformance> results;
    
    for (const auto& [id, name] : engines) {
        std::cout << "Testing " << std::setw(25) << std::left << name << "... ";
        std::cout.flush();
        
        EnginePerformance perf;
        testEnginePerformance(id, name, perf);
        results.push_back(perf);
        
        if (perf.cpuPercent < 0) {
            std::cout << RED << "FAILED" << RESET << std::endl;
        } else if (perf.cpuPercent > 10) {
            std::cout << RED << std::fixed << std::setprecision(2) 
                     << perf.cpuPercent << "%" << RESET << std::endl;
        } else if (perf.cpuPercent > 5) {
            std::cout << YELLOW << std::fixed << std::setprecision(2) 
                     << perf.cpuPercent << "%" << RESET << std::endl;
        } else {
            std::cout << GREEN << std::fixed << std::setprecision(2) 
                     << perf.cpuPercent << "%" << RESET << std::endl;
        }
    }
    
    // Print summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "Performance Summary" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << std::setw(25) << std::left << "Engine" 
              << std::setw(12) << "Avg (ms)" 
              << std::setw(12) << "Max (ms)" 
              << std::setw(10) << "CPU %" << std::endl;
    std::cout << std::string(59, '-') << std::endl;
    
    double totalCpu = 0;
    int validCount = 0;
    
    for (const auto& perf : results) {
        std::cout << std::setw(25) << std::left << perf.name;
        
        if (perf.cpuPercent < 0) {
            std::cout << std::setw(12) << "N/A" 
                     << std::setw(12) << "N/A" 
                     << std::setw(10) << "N/A" << std::endl;
        } else {
            std::cout << std::setw(12) << std::fixed << std::setprecision(4) << perf.avgTimeMs
                     << std::setw(12) << std::fixed << std::setprecision(4) << perf.maxTimeMs;
            
            if (perf.cpuPercent > 10) {
                std::cout << RED;
            } else if (perf.cpuPercent > 5) {
                std::cout << YELLOW;
            } else {
                std::cout << GREEN;
            }
            
            std::cout << std::setw(10) << std::fixed << std::setprecision(2) 
                     << perf.cpuPercent << "%" << RESET << std::endl;
            
            totalCpu += perf.cpuPercent;
            validCount++;
        }
    }
    
    if (validCount > 0) {
        std::cout << std::string(59, '-') << std::endl;
        double avgCpu = totalCpu / validCount;
        std::cout << "Average CPU usage: ";
        
        if (avgCpu > 5) {
            std::cout << YELLOW;
        } else {
            std::cout << GREEN;
        }
        
        std::cout << std::fixed << std::setprecision(2) << avgCpu << "%" << RESET << std::endl;
        
        // Estimate how many engines can run simultaneously
        int maxSimultaneous = static_cast<int>(100.0 / avgCpu);
        std::cout << "Estimated simultaneous engines @ 100% CPU: " << CYAN 
                 << maxSimultaneous << RESET << std::endl;
    }
    
    std::cout << "\nLegend:" << std::endl;
    std::cout << GREEN << "  Green" << RESET << ": < 5% CPU (Excellent)" << std::endl;
    std::cout << YELLOW << "  Yellow" << RESET << ": 5-10% CPU (Good)" << std::endl;
    std::cout << RED << "  Red" << RESET << ": > 10% CPU (Heavy)" << std::endl;
    
    return 0;
}