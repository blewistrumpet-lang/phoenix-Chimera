// Comprehensive memory leak test for all reverb engines
// Tests each reverb for 5 minutes with parameter automation
// Fails if any reverb shows > 1 MB/min growth

#include "../JUCE_Plugin/Source/PlateReverb.h"
#include "../JUCE_Plugin/Source/SpringReverb.h"
#include "../JUCE_Plugin/Source/ShimmerReverb.h"
#include "../JUCE_Plugin/Source/GatedReverb.h"
#include "../JUCE_Plugin/Source/ConvolutionReverb.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>
#include <memory>
#include <chrono>
#include <cmath>

#if __has_include(<mach/mach.h>)
#include <mach/mach.h>
#endif

class MemoryMonitor {
public:
    static size_t getCurrentMemoryUsage() {
#if __has_include(<mach/mach.h>)
        struct task_basic_info info;
        mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
        kern_return_t kerr = task_info(mach_task_self(),
                                      TASK_BASIC_INFO,
                                      (task_info_t)&info,
                                      &size);
        if (kerr == KERN_SUCCESS) {
            return info.resident_size;
        }
#endif
        return 0;
    }

    static double bytesToMB(size_t bytes) {
        return bytes / (1024.0 * 1024.0);
    }
};

struct TestResult {
    std::string engineName;
    double initialMemoryMB;
    double finalMemoryMB;
    double peakMemoryMB;
    double growthMB;
    double growthRateMBPerMin;
    double durationMin;
    bool passed;
    std::string failureReason;
};

class ReverbMemoryTest {
public:
    static TestResult testReverb(EngineBase* reverb, const std::string& name, double durationMin) {
        TestResult result;
        result.engineName = name;
        result.durationMin = durationMin;
        result.passed = true;

        const double sampleRate = 48000.0;
        const int samplesPerBlock = 512;
        const int totalBlocks = static_cast<int>((durationMin * 60.0 * sampleRate) / samplesPerBlock);

        std::cout << "\n========================================" << std::endl;
        std::cout << "Testing: " << name << std::endl;
        std::cout << "Duration: " << durationMin << " minutes" << std::endl;
        std::cout << "Blocks: " << totalBlocks << std::endl;
        std::cout << "========================================" << std::endl;

        // Prepare engine
        reverb->prepareToPlay(sampleRate, samplesPerBlock);

        // Create test buffer
        juce::AudioBuffer<float> buffer(2, samplesPerBlock);

        // Initial memory measurement (after warmup)
        // Run a few blocks first to let everything initialize
        for (int i = 0; i < 100; i++) {
            fillBufferWithTestSignal(buffer, i);
            reverb->process(buffer);
        }

        size_t initialMemory = MemoryMonitor::getCurrentMemoryUsage();
        result.initialMemoryMB = MemoryMonitor::bytesToMB(initialMemory);
        size_t peakMemory = initialMemory;

        std::cout << "Initial Memory: " << std::fixed << std::setprecision(2)
                  << result.initialMemoryMB << " MB" << std::endl;

        auto startTime = std::chrono::steady_clock::now();

        // Process blocks with parameter automation
        int measurementInterval = totalBlocks / 30; // 30 measurements
        if (measurementInterval < 1) measurementInterval = 1;

        for (int block = 0; block < totalBlocks; block++) {
            // Fill buffer with test signal
            fillBufferWithTestSignal(buffer, block);

            // Automate parameters (all 10 parameters)
            std::map<int, float> params;
            for (int p = 0; p < 10; p++) {
                // Create different modulation patterns for different parameters
                float phase = (block * 2.0f * M_PI) / (totalBlocks / (p + 1));
                float value = 0.5f + 0.5f * std::sin(phase);
                params[p] = value;
            }
            reverb->updateParameters(params);

            // Process audio
            reverb->process(buffer);

            // Measure memory periodically
            if (block % measurementInterval == 0) {
                size_t currentMemory = MemoryMonitor::getCurrentMemoryUsage();
                if (currentMemory > peakMemory) {
                    peakMemory = currentMemory;
                }

                double currentMB = MemoryMonitor::bytesToMB(currentMemory);
                double growthMB = currentMB - result.initialMemoryMB;
                double progress = (100.0 * block) / totalBlocks;

                std::cout << "\rProgress: " << std::fixed << std::setprecision(1)
                          << progress << "% - Memory: " << std::setprecision(2)
                          << currentMB << " MB (+";
                if (growthMB >= 0) {
                    std::cout << growthMB;
                } else {
                    std::cout << growthMB;
                }
                std::cout << " MB)" << std::flush;
            }
        }

        std::cout << std::endl;

        auto endTime = std::chrono::steady_clock::now();
        double actualDuration = std::chrono::duration<double>(endTime - startTime).count() / 60.0;

        // Final memory measurement
        size_t finalMemory = MemoryMonitor::getCurrentMemoryUsage();
        result.finalMemoryMB = MemoryMonitor::bytesToMB(finalMemory);
        result.peakMemoryMB = MemoryMonitor::bytesToMB(peakMemory);
        result.growthMB = result.finalMemoryMB - result.initialMemoryMB;
        result.growthRateMBPerMin = result.growthMB / durationMin;

        // Check for memory leak (threshold: 1 MB/min)
        const double threshold = 1.0; // MB/min
        if (result.growthRateMBPerMin > threshold) {
            result.passed = false;
            result.failureReason = "Memory leak detected: " +
                                  std::to_string(result.growthRateMBPerMin) +
                                  " MB/min > " + std::to_string(threshold) + " MB/min";
        }

        // Print results
        std::cout << "\n--- Results ---" << std::endl;
        std::cout << "Initial:  " << std::fixed << std::setprecision(2) << result.initialMemoryMB << " MB" << std::endl;
        std::cout << "Final:    " << result.finalMemoryMB << " MB" << std::endl;
        std::cout << "Peak:     " << result.peakMemoryMB << " MB" << std::endl;
        std::cout << "Growth:   " << std::showpos << result.growthMB << " MB" << std::noshowpos << std::endl;
        std::cout << "Rate:     " << result.growthRateMBPerMin << " MB/min" << std::endl;
        std::cout << "Status:   " << (result.passed ? "PASSED" : "FAILED") << std::endl;

        if (!result.passed) {
            std::cout << "Reason:   " << result.failureReason << std::endl;
        }

        return result;
    }

private:
    static void fillBufferWithTestSignal(juce::AudioBuffer<float>& buffer, int blockIndex) {
        // Generate complex test signal with multiple frequencies
        const float sampleRate = 48000.0f;
        const int samplesPerBlock = buffer.getNumSamples();
        const int numChannels = buffer.getNumChannels();

        for (int ch = 0; ch < numChannels; ch++) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < samplesPerBlock; i++) {
                int globalSample = blockIndex * samplesPerBlock + i;
                float t = globalSample / sampleRate;

                // Mix of multiple frequencies
                float signal = 0.0f;
                signal += 0.3f * std::sin(2.0f * M_PI * 220.0f * t);  // A3
                signal += 0.2f * std::sin(2.0f * M_PI * 440.0f * t);  // A4
                signal += 0.1f * std::sin(2.0f * M_PI * 880.0f * t);  // A5

                // Add some transients every second
                if ((globalSample % 48000) < 100) {
                    signal += 0.5f * std::sin(2.0f * M_PI * 1000.0f * t);
                }

                data[i] = signal * 0.5f;
            }
        }
    }
};

int main() {
    std::cout << "==============================================================" << std::endl;
    std::cout << "  REVERB MEMORY LEAK TEST - COMPREHENSIVE" << std::endl;
    std::cout << "==============================================================" << std::endl;
    std::cout << "Test Duration: 5 minutes per reverb" << std::endl;
    std::cout << "Pass Threshold: < 1.0 MB/min growth" << std::endl;
    std::cout << "Parameter Automation: All 10 parameters modulated" << std::endl;
    std::cout << "==============================================================" << std::endl;

    std::vector<TestResult> results;

    // Test all reverbs
    const double testDuration = 5.0; // 5 minutes

    {
        std::cout << "\n[1/5] PlateReverb" << std::endl;
        auto engine = std::make_unique<PlateReverb>();
        results.push_back(ReverbMemoryTest::testReverb(engine.get(), "PlateReverb", testDuration));
    }

    {
        std::cout << "\n[2/5] SpringReverb" << std::endl;
        auto engine = std::make_unique<SpringReverb>();
        results.push_back(ReverbMemoryTest::testReverb(engine.get(), "SpringReverb", testDuration));
    }

    {
        std::cout << "\n[3/5] ShimmerReverb" << std::endl;
        auto engine = std::make_unique<ShimmerReverb>();
        results.push_back(ReverbMemoryTest::testReverb(engine.get(), "ShimmerReverb", testDuration));
    }

    {
        std::cout << "\n[4/5] GatedReverb" << std::endl;
        auto engine = std::make_unique<GatedReverb>();
        results.push_back(ReverbMemoryTest::testReverb(engine.get(), "GatedReverb", testDuration));
    }

    {
        std::cout << "\n[5/5] ConvolutionReverb" << std::endl;
        auto engine = std::make_unique<ConvolutionReverb>();
        results.push_back(ReverbMemoryTest::testReverb(engine.get(), "ConvolutionReverb", testDuration));
    }

    // Summary
    std::cout << "\n==============================================================" << std::endl;
    std::cout << "  SUMMARY" << std::endl;
    std::cout << "==============================================================" << std::endl;

    std::cout << std::left << std::setw(25) << "Engine"
              << std::right << std::setw(12) << "Initial"
              << std::setw(12) << "Final"
              << std::setw(12) << "Growth"
              << std::setw(15) << "Rate (MB/min)"
              << std::setw(10) << "Status" << std::endl;
    std::cout << std::string(86, '-') << std::endl;

    int passed = 0;
    int failed = 0;

    for (const auto& result : results) {
        std::cout << std::left << std::setw(25) << result.engineName
                  << std::right << std::fixed << std::setprecision(2)
                  << std::setw(12) << result.initialMemoryMB
                  << std::setw(12) << result.finalMemoryMB
                  << std::setw(12) << std::showpos << result.growthMB << std::noshowpos
                  << std::setw(15) << result.growthRateMBPerMin
                  << std::setw(10) << (result.passed ? "PASS" : "FAIL") << std::endl;

        if (result.passed) passed++;
        else failed++;
    }

    std::cout << "==============================================================" << std::endl;
    std::cout << "Total: " << results.size() << " | Passed: " << passed << " | Failed: " << failed << std::endl;
    std::cout << "==============================================================" << std::endl;

    if (failed > 0) {
        std::cout << "\nFAILURES DETECTED:" << std::endl;
        for (const auto& result : results) {
            if (!result.passed) {
                std::cout << "  - " << result.engineName << ": " << result.failureReason << std::endl;
            }
        }
        std::cout << "\nTEST FAILED - Memory leaks detected" << std::endl;
        return 1;
    }

    std::cout << "\nALL TESTS PASSED - No memory leaks detected" << std::endl;
    std::cout << "All reverb engines are production-ready!" << std::endl;

    return 0;
}
