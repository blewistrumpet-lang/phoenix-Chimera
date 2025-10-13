// Quick 1-minute test for all reverbs to verify memory leak fixes
#include "../JUCE_Plugin/Source/PlateReverb.h"
#include "../JUCE_Plugin/Source/SpringReverb.h"
#include "../JUCE_Plugin/Source/ShimmerReverb.h"
#include "../JUCE_Plugin/Source/GatedReverb.h"
#include "../JUCE_Plugin/Source/ConvolutionReverb.h"
#include <JuceHeader.h>
#include <iostream>
#include <iomanip>

#if __has_include(<mach/mach.h>)
#include <mach/mach.h>
#endif

size_t getCurrentMemoryUsage() {
#if __has_include(<mach/mach.h>)
    struct task_basic_info info;
    mach_msg_type_number_t size = TASK_BASIC_INFO_COUNT;
    kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO,
                                  (task_info_t)&info, &size);
    if (kerr == KERN_SUCCESS) return info.resident_size;
#endif
    return 0;
}

double MB(size_t bytes) { return bytes / (1024.0 * 1024.0); }

struct Result {
    std::string name;
    double initialMB, finalMB, growthMB, rateMBPerMin;
    bool passed;
};

Result testReverb(EngineBase* reverb, const std::string& name) {
    const double sampleRate = 48000.0;
    const int samplesPerBlock = 512;
    const double durationMin = 1.0; // 1 minute
    const int totalBlocks = static_cast<int>((durationMin * 60.0 * sampleRate) / samplesPerBlock);

    std::cout << "\nTesting " << name << "..." << std::flush;

    reverb->prepareToPlay(sampleRate, samplesPerBlock);
    juce::AudioBuffer<float> buffer(2, samplesPerBlock);

    // Warmup
    for (int i = 0; i < 100; i++) {
        buffer.clear();
        reverb->process(buffer);
    }

    size_t initialMemory = getCurrentMemoryUsage();

    // Process with parameter automation
    for (int block = 0; block < totalBlocks; block++) {
        // Fill with test signal
        for (int ch = 0; ch < 2; ch++) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < samplesPerBlock; i++) {
                float t = (block * samplesPerBlock + i) / (float)sampleRate;
                data[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * t);
            }
        }

        // Automate parameters
        std::map<int, float> params;
        for (int p = 0; p < 10; p++) {
            float phase = (block * 2.0f * M_PI) / (totalBlocks / (p + 1));
            params[p] = 0.5f + 0.5f * std::sin(phase);
        }
        reverb->updateParameters(params);

        reverb->process(buffer);

        if (block % 500 == 0) {
            std::cout << "." << std::flush;
        }
    }

    size_t finalMemory = getCurrentMemoryUsage();

    Result r;
    r.name = name;
    r.initialMB = MB(initialMemory);
    r.finalMB = MB(finalMemory);
    r.growthMB = r.finalMB - r.initialMB;
    r.rateMBPerMin = r.growthMB / durationMin;
    r.passed = r.rateMBPerMin < 1.0;

    std::cout << " " << std::fixed << std::setprecision(2)
              << r.growthMB << " MB growth, "
              << r.rateMBPerMin << " MB/min - "
              << (r.passed ? "PASS" : "FAIL") << std::endl;

    return r;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  QUICK REVERB MEMORY TEST (1 min)" << std::endl;
    std::cout << "========================================" << std::endl;

    std::vector<Result> results;

    {
        auto engine = std::make_unique<PlateReverb>();
        results.push_back(testReverb(engine.get(), "PlateReverb"));
    }

    {
        auto engine = std::make_unique<SpringReverb>();
        results.push_back(testReverb(engine.get(), "SpringReverb"));
    }

    {
        auto engine = std::make_unique<ShimmerReverb>();
        results.push_back(testReverb(engine.get(), "ShimmerReverb"));
    }

    {
        auto engine = std::make_unique<GatedReverb>();
        results.push_back(testReverb(engine.get(), "GatedReverb"));
    }

    {
        auto engine = std::make_unique<ConvolutionReverb>();
        results.push_back(testReverb(engine.get(), "ConvolutionReverb"));
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "  SUMMARY" << std::endl;
    std::cout << "========================================" << std::endl;

    int passed = 0, failed = 0;
    for (const auto& r : results) {
        std::cout << std::left << std::setw(20) << r.name
                  << std::right << std::fixed << std::setprecision(2)
                  << std::setw(10) << r.growthMB << " MB"
                  << std::setw(15) << r.rateMBPerMin << " MB/min"
                  << "  " << (r.passed ? "PASS" : "FAIL") << std::endl;
        if (r.passed) passed++; else failed++;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "Passed: " << passed << " / " << results.size() << std::endl;

    if (failed > 0) {
        std::cout << "\nFAILED - Memory leaks detected" << std::endl;
        return 1;
    }

    std::cout << "\nPASSED - All reverbs stable" << std::endl;
    return 0;
}
