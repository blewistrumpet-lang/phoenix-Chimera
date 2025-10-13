#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <chrono>
#include <thread>

#if defined(__APPLE__)
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

/**
 * MEMORY LEAK TEST FOR REVERB ENGINES
 *
 * Tests each reverb engine for 5 minutes to detect memory leaks.
 * Reports memory growth rate in MB/min.
 * PASS criteria: < 1 MB/min growth
 *
 * Engines tested:
 * - 39: PlateReverb
 * - 40: SpringReverb
 * - 41: ShimmerReverb
 * - 42: GatedReverb_Platinum
 * - 43: ConvolutionReverb_Platinum
 */

namespace MemoryLeakTest {

//==============================================================================
// Memory Monitoring
//==============================================================================

struct MemorySnapshot {
    size_t rss_bytes = 0;          // Resident Set Size (actual RAM used)
    size_t virtual_bytes = 0;       // Virtual memory size
    double timestamp = 0.0;         // Seconds since test start
};

// Get current memory usage
MemorySnapshot getMemoryUsage() {
    MemorySnapshot snap;

#if defined(__APPLE__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                  (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        snap.rss_bytes = info.resident_size;
        snap.virtual_bytes = info.virtual_size;
    }
#elif defined(__linux__)
    FILE* fp = fopen("/proc/self/status", "r");
    if (fp) {
        char line[128];
        while (fgets(line, 128, fp)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                snap.rss_bytes = atol(line + 6) * 1024; // Convert KB to bytes
            } else if (strncmp(line, "VmSize:", 7) == 0) {
                snap.virtual_bytes = atol(line + 7) * 1024;
            }
        }
        fclose(fp);
    }
#endif

    return snap;
}

// Format bytes to human-readable string
std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unitIndex < 3) {
        size /= 1024.0;
        unitIndex++;
    }

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2) << size << " " << units[unitIndex];
    return ss.str();
}

//==============================================================================
// Test Results
//==============================================================================

struct MemoryLeakTestResult {
    int engineId;
    std::string engineName;
    bool passed = false;

    // Test duration
    double testDurationSeconds = 0.0;
    size_t totalSamplesProcessed = 0;

    // Memory tracking
    std::vector<MemorySnapshot> memorySnapshots;
    size_t initialMemoryBytes = 0;
    size_t finalMemoryBytes = 0;
    size_t peakMemoryBytes = 0;
    double memoryLeakRateMBPerMin = 0.0;

    // Errors
    bool crashed = false;
    std::string errorMessage;
};

//==============================================================================
// Memory Leak Test Runner
//==============================================================================

MemoryLeakTestResult runMemoryLeakTest(int engineId, const std::string& engineName,
                                       double testDurationMinutes = 5.0) {
    MemoryLeakTestResult result;
    result.engineId = engineId;
    result.engineName = engineName;

    std::cout << "\n================================================================\n";
    std::cout << "  Engine " << std::setw(2) << engineId << ": "
              << std::setw(30) << std::left << engineName << "\n";
    std::cout << "================================================================\n";
    std::cout << "Running " << testDurationMinutes << " minute memory leak test...\n\n";

    try {
        // Create engine
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.crashed = true;
            result.errorMessage = "Failed to create engine";
            return result;
        }

        // Setup audio parameters
        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        // Set parameters (moderate settings)
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        if (numParams > 0) params[0] = 0.5f;  // Mix
        if (numParams > 1) params[1] = 0.6f;  // Time/Decay/Size
        if (numParams > 2) params[2] = 0.4f;  // Feedback/Damping
        if (numParams > 3) params[3] = 0.5f;  // Additional params
        if (numParams > 4) params[4] = 0.8f;  // Width/Spread
        engine->updateParameters(params);

        // Calculate test parameters
        const double testDurationSeconds = testDurationMinutes * 60.0;
        const size_t totalBlocks = static_cast<size_t>(
            (testDurationSeconds * sampleRate) / blockSize);
        const size_t memoryCheckInterval = static_cast<size_t>(
            (10.0 * sampleRate) / blockSize); // Every 10 seconds

        // Initialize buffers
        juce::AudioBuffer<float> buffer(2, blockSize);

        // Get initial memory
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        result.initialMemoryBytes = getMemoryUsage().rss_bytes;
        MemorySnapshot initialSnapshot = getMemoryUsage();
        initialSnapshot.timestamp = 0.0;
        result.memorySnapshots.push_back(initialSnapshot);

        std::cout << "Initial Memory: " << formatBytes(result.initialMemoryBytes) << "\n";
        std::cout << "Processing " << totalBlocks << " blocks...\n";
        std::cout << "Progress: ";
        std::cout.flush();

        // Test start time
        auto testStartTime = std::chrono::high_resolution_clock::now();
        auto lastProgressUpdate = testStartTime;
        auto lastMemoryCheck = testStartTime;

        // Main processing loop
        juce::Random random;
        for (size_t blockIdx = 0; blockIdx < totalBlocks; ++blockIdx) {
            // Generate input signal (mixed: sine wave + noise)
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    float phase = 2.0f * M_PI * 440.0f *
                                 (blockIdx * blockSize + i) / sampleRate;
                    float sine = 0.3f * std::sin(phase);
                    float noise = 0.05f * (random.nextFloat() * 2.0f - 1.0f);
                    buffer.setSample(ch, i, sine + noise);
                }
            }

            // Process block
            engine->process(buffer);

            // Check memory usage periodically
            auto now = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(
                now - testStartTime).count();

            if (blockIdx % memoryCheckInterval == 0) {
                MemorySnapshot snap = getMemoryUsage();
                snap.timestamp = elapsed;
                result.memorySnapshots.push_back(snap);

                result.peakMemoryBytes =
                    std::max(result.peakMemoryBytes, snap.rss_bytes);

                lastMemoryCheck = now;
            }

            // Update progress display (every second)
            if (std::chrono::duration<double>(now - lastProgressUpdate).count() >= 1.0) {
                double progressPercent = (blockIdx * 100.0) / totalBlocks;
                size_t currentMemory = getMemoryUsage().rss_bytes;
                double currentGrowthMB = (currentMemory - result.initialMemoryBytes) / (1024.0 * 1024.0);

                std::cout << "\rProgress: " << std::fixed << std::setprecision(1)
                         << progressPercent << "% | Memory: "
                         << formatBytes(currentMemory) << " (+"
                         << std::setprecision(1) << currentGrowthMB << " MB)    ";
                std::cout.flush();
                lastProgressUpdate = now;
            }
        }

        // Calculate final metrics
        auto testEndTime = std::chrono::high_resolution_clock::now();
        result.testDurationSeconds = std::chrono::duration<double>(
            testEndTime - testStartTime).count();
        result.totalSamplesProcessed = totalBlocks * blockSize;

        // Get final memory
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        result.finalMemoryBytes = getMemoryUsage().rss_bytes;

        // Analyze memory leak
        if (result.memorySnapshots.size() >= 2) {
            size_t memoryGrowth = result.finalMemoryBytes - result.initialMemoryBytes;
            double durationMinutes = result.testDurationSeconds / 60.0;
            result.memoryLeakRateMBPerMin =
                (memoryGrowth / (1024.0 * 1024.0)) / durationMinutes;

            // PASS if growing less than 1 MB/min
            result.passed = (result.memoryLeakRateMBPerMin < 1.0);
        }

        std::cout << "\rProgress: 100.0% - COMPLETE                            \n";

    } catch (const std::exception& e) {
        result.crashed = true;
        result.errorMessage = e.what();
        std::cout << "\n\nERROR: Test crashed - " << e.what() << "\n";
    } catch (...) {
        result.crashed = true;
        result.errorMessage = "Unknown exception";
        std::cout << "\n\nERROR: Test crashed - unknown exception\n";
    }

    return result;
}

//==============================================================================
// Report Generation
//==============================================================================

void printTestResult(const MemoryLeakTestResult& result) {
    std::cout << "\n";
    std::cout << "================================================================\n";
    std::cout << "  TEST RESULTS\n";
    std::cout << "================================================================\n\n";

    if (result.crashed) {
        std::cout << "CRASHED: " << result.errorMessage << "\n\n";
        return;
    }

    // Duration
    std::cout << "DURATION:\n";
    std::cout << "  Test Time:       " << std::fixed << std::setprecision(2)
              << result.testDurationSeconds / 60.0 << " minutes\n";
    std::cout << "  Samples:         " << result.totalSamplesProcessed << "\n\n";

    // Memory analysis
    std::cout << "MEMORY ANALYSIS:\n";
    std::cout << "  Initial:         " << formatBytes(result.initialMemoryBytes) << "\n";
    std::cout << "  Final:           " << formatBytes(result.finalMemoryBytes) << "\n";
    std::cout << "  Peak:            " << formatBytes(result.peakMemoryBytes) << "\n";
    std::cout << "  Growth:          "
              << formatBytes(result.finalMemoryBytes - result.initialMemoryBytes);

    if (result.passed) {
        std::cout << " [OK]\n";
    } else {
        std::cout << " [LEAK DETECTED]\n";
    }

    std::cout << "  Growth Rate:     " << std::fixed << std::setprecision(3)
              << result.memoryLeakRateMBPerMin << " MB/min\n\n";

    // Memory growth over time
    if (result.memorySnapshots.size() > 2) {
        std::cout << "MEMORY GROWTH TIMELINE:\n";
        for (size_t i = 0; i < result.memorySnapshots.size(); i++) {
            const auto& snap = result.memorySnapshots[i];
            double growthMB = (snap.rss_bytes - result.initialMemoryBytes) / (1024.0 * 1024.0);

            std::cout << "  " << std::setw(5) << std::fixed << std::setprecision(1)
                     << snap.timestamp << "s: "
                     << formatBytes(snap.rss_bytes)
                     << " (+" << std::setprecision(2) << growthMB << " MB)\n";
        }
        std::cout << "\n";
    }

    // Overall result
    std::cout << "RESULT:  ";
    if (result.passed) {
        std::cout << "PASSED - Memory growth < 1 MB/min\n";
    } else {
        std::cout << "FAILED - Memory leak detected ("
                  << std::fixed << std::setprecision(3)
                  << result.memoryLeakRateMBPerMin << " MB/min)\n";
    }
    std::cout << "\n";
}

} // namespace MemoryLeakTest

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[]) {
    using namespace MemoryLeakTest;

    // Test duration (default 5 minutes, can be overridden)
    double testDurationMinutes = 5.0;
    if (argc > 1) {
        testDurationMinutes = std::atof(argv[1]);
        if (testDurationMinutes < 1.0) testDurationMinutes = 1.0;
    }

    std::cout << "\n================================================================\n";
    std::cout << "       MEMORY LEAK TEST: Reverb Engines (39-43)                \n";
    std::cout << "================================================================\n";
    std::cout << "\nTest Duration: " << testDurationMinutes << " minutes per engine\n";
    std::cout << "Pass Criteria: Memory growth < 1.0 MB/min\n\n";

    // Define engines to test
    std::vector<std::pair<int, std::string>> engines = {
        {39, "PlateReverb"},
        {40, "SpringReverb"},
        {41, "ShimmerReverb"},
        {42, "GatedReverb_Platinum"},
        {43, "ConvolutionReverb_Platinum"}
    };

    std::vector<MemoryLeakTestResult> results;

    // Run tests
    for (const auto& [id, name] : engines) {
        auto result = runMemoryLeakTest(id, name, testDurationMinutes);
        printTestResult(result);
        results.push_back(result);

        // Small delay between tests to let memory settle
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Final summary
    std::cout << "\n================================================================\n";
    std::cout << "  FINAL SUMMARY\n";
    std::cout << "================================================================\n\n";

    int passed = 0, failed = 0, crashed = 0;
    for (const auto& r : results) {
        if (r.crashed) {
            crashed++;
        } else if (r.passed) {
            passed++;
        } else {
            failed++;
        }

        std::cout << "  Engine " << std::setw(2) << r.engineId << " - "
                  << std::setw(30) << std::left << r.engineName << ": ";

        if (r.crashed) {
            std::cout << "CRASHED\n";
        } else if (r.passed) {
            std::cout << "PASSED (" << std::fixed << std::setprecision(3)
                     << r.memoryLeakRateMBPerMin << " MB/min)\n";
        } else {
            std::cout << "FAILED (" << std::fixed << std::setprecision(3)
                     << r.memoryLeakRateMBPerMin << " MB/min)\n";
        }
    }

    std::cout << "\n";
    std::cout << "  Total:   " << engines.size() << " engines\n";
    std::cout << "  Passed:  " << passed << "\n";
    std::cout << "  Failed:  " << failed << "\n";
    std::cout << "  Crashed: " << crashed << "\n";
    std::cout << "================================================================\n\n";

    return (failed == 0 && crashed == 0) ? 0 : 1;
}
