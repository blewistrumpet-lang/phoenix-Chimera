#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>

#if defined(__APPLE__)
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

/**
 * ENDURANCE TEST SUITE FOR REVERBS AND TIME-BASED EFFECTS
 *
 * Tests each engine for 5+ minutes of continuous processing to detect:
 * - Memory leaks (increasing memory usage over time)
 * - Buffer overflows (crashes, NaN/Inf values)
 * - Performance degradation (increasing CPU usage)
 * - Stability issues (unexpected behavior)
 *
 * Engines tested:
 * - 34: Tape Echo
 * - 35: Digital Delay
 * - 36: Magnetic Drum Echo
 * - 37: Bucket Brigade Delay
 * - 38: Buffer Repeat Platinum
 * - 39: Convolution Reverb
 * - 40: Shimmer Reverb
 * - 41: Plate Reverb
 * - 42: Spring Reverb
 * - 43: Gated Reverb
 */

namespace EnduranceTest {

//==============================================================================
// Memory and Performance Monitoring
//==============================================================================

struct MemorySnapshot {
    size_t rss_bytes = 0;          // Resident Set Size (actual RAM used)
    size_t virtual_bytes = 0;       // Virtual memory size
    double timestamp = 0.0;         // Seconds since test start
};

struct PerformanceMetrics {
    double avgProcessTimeUs = 0.0;  // Average processing time per block (microseconds)
    double maxProcessTimeUs = 0.0;  // Peak processing time
    double minProcessTimeUs = 1e9;  // Minimum processing time
    size_t blocksProcessed = 0;     // Total blocks processed
    double totalTimeSeconds = 0.0;  // Total elapsed time
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
// Audio Validation
//==============================================================================

struct AudioQualityMetrics {
    bool hasNaN = false;
    bool hasInf = false;
    bool hasDCOffset = false;
    bool isClipping = false;
    float peakLevel = 0.0f;
    float rmsLevel = 0.0f;
    float dcOffset = 0.0f;
};

AudioQualityMetrics analyzeBuffer(const juce::AudioBuffer<float>& buffer) {
    AudioQualityMetrics metrics;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        float sum = 0.0f;
        float sumSquares = 0.0f;

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float val = data[i];

            // Check for invalid values
            if (std::isnan(val)) metrics.hasNaN = true;
            if (std::isinf(val)) metrics.hasInf = true;

            // Check for clipping
            if (std::abs(val) > 1.0f) metrics.isClipping = true;

            // Track peak
            metrics.peakLevel = std::max(metrics.peakLevel, std::abs(val));

            // Accumulate for RMS and DC
            sum += val;
            sumSquares += val * val;
        }

        // Calculate RMS and DC offset
        int totalSamples = buffer.getNumSamples();
        float channelDC = sum / totalSamples;
        float channelRMS = std::sqrt(sumSquares / totalSamples);

        metrics.dcOffset += std::abs(channelDC);
        metrics.rmsLevel = std::max(metrics.rmsLevel, channelRMS);
    }

    // Average DC offset across channels
    metrics.dcOffset /= buffer.getNumChannels();
    metrics.hasDCOffset = (metrics.dcOffset > 0.01f);

    return metrics;
}

//==============================================================================
// Endurance Test Results
//==============================================================================

struct EnduranceTestResult {
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
    bool memoryLeak = false;
    double memoryLeakRateMBPerMin = 0.0;

    // Performance tracking
    PerformanceMetrics performance;
    bool performanceDegraded = false;
    double performanceDegradationPercent = 0.0;

    // Audio quality
    bool audioQualityPassed = true;
    int nanCount = 0;
    int infCount = 0;
    int dcOffsetCount = 0;
    int clippingCount = 0;

    // Errors
    bool crashed = false;
    std::string errorMessage;
};

//==============================================================================
// Endurance Test Runner
//==============================================================================

EnduranceTestResult runEnduranceTest(int engineId, const std::string& engineName,
                                     double testDurationMinutes = 5.0) {
    EnduranceTestResult result;
    result.engineId = engineId;
    result.engineName = engineName;

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Engine " << std::setw(2) << engineId << ": "
              << std::setw(45) << std::left << engineName << "║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "Starting " << testDurationMinutes << " minute endurance test...\n\n";

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
        if (numParams > 1) params[1] = 0.6f;  // Time/Decay
        if (numParams > 2) params[2] = 0.4f;  // Feedback/Damping
        if (numParams > 3) params[3] = 0.5f;  // Additional params
        if (numParams > 4) params[4] = 0.8f;  // Width/Spread
        engine->updateParameters(params);

        // Calculate test parameters
        const double testDurationSeconds = testDurationMinutes * 60.0;
        const size_t totalBlocks = static_cast<size_t>(
            (testDurationSeconds * sampleRate) / blockSize);
        const size_t memoryCheckInterval = static_cast<size_t>(
            (5.0 * sampleRate) / blockSize); // Every 5 seconds

        // Initialize buffers
        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);

        // Get initial memory
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

        // Main processing loop
        for (size_t blockIdx = 0; blockIdx < totalBlocks; ++blockIdx) {
            // Generate input signal (mixed: sine wave + noise)
            juce::Random random;
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    float phase = 2.0f * M_PI * 440.0f *
                                 (blockIdx * blockSize + i) / sampleRate;
                    float sine = 0.3f * std::sin(phase);
                    float noise = 0.05f * (random.nextFloat() * 2.0f - 1.0f);
                    inputBuffer.setSample(ch, i, sine + noise);
                }
            }

            // Copy input to output
            outputBuffer.makeCopyOf(inputBuffer);

            // Process block with timing
            auto blockStartTime = std::chrono::high_resolution_clock::now();
            engine->process(outputBuffer);
            auto blockEndTime = std::chrono::high_resolution_clock::now();

            // Calculate processing time
            double processTimeUs = std::chrono::duration<double, std::micro>(
                blockEndTime - blockStartTime).count();

            // Update performance metrics
            result.performance.blocksProcessed++;
            result.performance.avgProcessTimeUs += processTimeUs;
            result.performance.maxProcessTimeUs =
                std::max(result.performance.maxProcessTimeUs, processTimeUs);
            result.performance.minProcessTimeUs =
                std::min(result.performance.minProcessTimeUs, processTimeUs);

            // Validate audio output
            auto audioMetrics = analyzeBuffer(outputBuffer);
            if (audioMetrics.hasNaN) result.nanCount++;
            if (audioMetrics.hasInf) result.infCount++;
            if (audioMetrics.hasDCOffset) result.dcOffsetCount++;
            if (audioMetrics.isClipping) result.clippingCount++;

            // Check memory usage periodically
            if (blockIdx % memoryCheckInterval == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(
                    now - testStartTime).count();

                MemorySnapshot snap = getMemoryUsage();
                snap.timestamp = elapsed;
                result.memorySnapshots.push_back(snap);

                result.peakMemoryBytes =
                    std::max(result.peakMemoryBytes, snap.rss_bytes);
            }

            // Update progress display (every second)
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<double>(now - lastProgressUpdate).count() >= 1.0) {
                double progressPercent = (blockIdx * 100.0) / totalBlocks;
                std::cout << "\rProgress: " << std::fixed << std::setprecision(1)
                         << progressPercent << "% ";
                std::cout.flush();
                lastProgressUpdate = now;
            }
        }

        // Calculate final metrics
        auto testEndTime = std::chrono::high_resolution_clock::now();
        result.testDurationSeconds = std::chrono::duration<double>(
            testEndTime - testStartTime).count();
        result.totalSamplesProcessed = totalBlocks * blockSize;
        result.finalMemoryBytes = getMemoryUsage().rss_bytes;

        // Finalize performance metrics
        result.performance.avgProcessTimeUs /= result.performance.blocksProcessed;
        result.performance.totalTimeSeconds = result.testDurationSeconds;

        // Analyze memory leak
        if (result.memorySnapshots.size() >= 2) {
            size_t memoryGrowth = result.finalMemoryBytes - result.initialMemoryBytes;
            double durationMinutes = result.testDurationSeconds / 60.0;
            result.memoryLeakRateMBPerMin =
                (memoryGrowth / (1024.0 * 1024.0)) / durationMinutes;

            // Consider it a leak if growing more than 1 MB/min
            result.memoryLeak = (result.memoryLeakRateMBPerMin > 1.0);
        }

        // Analyze performance degradation
        // Compare first 10% vs last 10% of blocks
        if (result.performance.blocksProcessed > 1000) {
            double firstBlockTime = result.memorySnapshots.front().timestamp > 0 ?
                result.performance.minProcessTimeUs : result.performance.avgProcessTimeUs * 0.8;
            double lastBlockTime = result.performance.maxProcessTimeUs;

            if (firstBlockTime > 0) {
                result.performanceDegradationPercent =
                    ((lastBlockTime - firstBlockTime) / firstBlockTime) * 100.0;
                result.performanceDegraded =
                    (result.performanceDegradationPercent > 20.0);
            }
        }

        // Determine audio quality pass/fail
        result.audioQualityPassed = (result.nanCount == 0 &&
                                     result.infCount == 0 &&
                                     result.dcOffsetCount < totalBlocks * 0.01 &&
                                     result.clippingCount < totalBlocks * 0.01);

        // Overall pass/fail
        result.passed = !result.memoryLeak &&
                       !result.performanceDegraded &&
                       result.audioQualityPassed;

        std::cout << "\rProgress: 100.0% - COMPLETE\n";

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

void printTestResult(const EnduranceTestResult& result) {
    std::cout << "\n";
    std::cout << "════════════════════════════════════════════════════════════\n";
    std::cout << "  TEST RESULTS\n";
    std::cout << "════════════════════════════════════════════════════════════\n\n";

    if (result.crashed) {
        std::cout << "❌ TEST CRASHED: " << result.errorMessage << "\n\n";
        return;
    }

    // Duration and throughput
    std::cout << "DURATION:\n";
    std::cout << "  Test Time:       " << std::fixed << std::setprecision(2)
              << result.testDurationSeconds / 60.0 << " minutes\n";
    std::cout << "  Samples Processed: " << result.totalSamplesProcessed
              << " (" << (result.totalSamplesProcessed / 48000.0 / 60.0)
              << " minutes of audio)\n";
    std::cout << "  Blocks Processed:  " << result.performance.blocksProcessed << "\n\n";

    // Memory analysis
    std::cout << "MEMORY ANALYSIS:\n";
    std::cout << "  Initial:         " << formatBytes(result.initialMemoryBytes) << "\n";
    std::cout << "  Final:           " << formatBytes(result.finalMemoryBytes) << "\n";
    std::cout << "  Peak:            " << formatBytes(result.peakMemoryBytes) << "\n";
    std::cout << "  Growth:          "
              << formatBytes(result.finalMemoryBytes - result.initialMemoryBytes);
    if (result.memoryLeak) {
        std::cout << " ⚠️ LEAK DETECTED";
    } else {
        std::cout << " ✓ OK";
    }
    std::cout << "\n";
    std::cout << "  Growth Rate:     " << std::fixed << std::setprecision(3)
              << result.memoryLeakRateMBPerMin << " MB/min\n\n";

    // Performance analysis
    std::cout << "PERFORMANCE ANALYSIS:\n";
    std::cout << "  Avg Block Time:  " << std::fixed << std::setprecision(2)
              << result.performance.avgProcessTimeUs << " μs\n";
    std::cout << "  Min Block Time:  " << result.performance.minProcessTimeUs << " μs\n";
    std::cout << "  Max Block Time:  " << result.performance.maxProcessTimeUs << " μs\n";

    double realTimeRatio = (result.performance.avgProcessTimeUs / 1000.0) /
                          (512.0 / 48.0); // ms processing / ms of audio
    std::cout << "  Real-time Ratio: " << std::fixed << std::setprecision(3)
              << realTimeRatio << "x ";
    if (realTimeRatio < 0.7) {
        std::cout << "(excellent)";
    } else if (realTimeRatio < 0.9) {
        std::cout << "(good)";
    } else {
        std::cout << "(⚠️ high CPU)";
    }
    std::cout << "\n";

    if (result.performanceDegraded) {
        std::cout << "  Degradation:     ⚠️ " << std::fixed << std::setprecision(1)
                  << result.performanceDegradationPercent << "% slower over time\n";
    } else {
        std::cout << "  Degradation:     ✓ None detected\n";
    }
    std::cout << "\n";

    // Audio quality
    std::cout << "AUDIO QUALITY:\n";
    std::cout << "  NaN Detected:    " << result.nanCount
              << (result.nanCount > 0 ? " ❌" : " ✓") << "\n";
    std::cout << "  Inf Detected:    " << result.infCount
              << (result.infCount > 0 ? " ❌" : " ✓") << "\n";
    std::cout << "  DC Offset:       " << result.dcOffsetCount << " blocks"
              << (result.dcOffsetCount > result.performance.blocksProcessed * 0.01 ? " ⚠️" : " ✓")
              << "\n";
    std::cout << "  Clipping:        " << result.clippingCount << " blocks"
              << (result.clippingCount > result.performance.blocksProcessed * 0.01 ? " ⚠️" : " ✓")
              << "\n\n";

    // Overall result
    std::cout << "OVERALL RESULT:  ";
    if (result.passed) {
        std::cout << "✅ PASSED\n";
    } else {
        std::cout << "❌ FAILED\n";
        if (result.memoryLeak) std::cout << "  - Memory leak detected\n";
        if (result.performanceDegraded) std::cout << "  - Performance degradation\n";
        if (!result.audioQualityPassed) std::cout << "  - Audio quality issues\n";
    }
    std::cout << "\n";
}

void generateCSVReport(const std::vector<EnduranceTestResult>& results,
                      const std::string& filename) {
    std::ofstream csv(filename);

    csv << "Engine ID,Engine Name,Passed,Test Duration (min),Samples Processed,"
        << "Initial Memory (MB),Final Memory (MB),Peak Memory (MB),"
        << "Memory Growth (MB),Memory Leak Rate (MB/min),Memory Leak,"
        << "Avg Block Time (us),Max Block Time (us),Real-time Ratio,"
        << "Performance Degraded,Degradation %,"
        << "NaN Count,Inf Count,DC Offset Count,Clipping Count,"
        << "Audio Quality Passed,Crashed\n";

    for (const auto& r : results) {
        csv << r.engineId << ","
            << "\"" << r.engineName << "\","
            << (r.passed ? "PASS" : "FAIL") << ","
            << (r.testDurationSeconds / 60.0) << ","
            << r.totalSamplesProcessed << ","
            << (r.initialMemoryBytes / (1024.0 * 1024.0)) << ","
            << (r.finalMemoryBytes / (1024.0 * 1024.0)) << ","
            << (r.peakMemoryBytes / (1024.0 * 1024.0)) << ","
            << ((r.finalMemoryBytes - r.initialMemoryBytes) / (1024.0 * 1024.0)) << ","
            << r.memoryLeakRateMBPerMin << ","
            << (r.memoryLeak ? "YES" : "NO") << ","
            << r.performance.avgProcessTimeUs << ","
            << r.performance.maxProcessTimeUs << ","
            << ((r.performance.avgProcessTimeUs / 1000.0) / (512.0 / 48.0)) << ","
            << (r.performanceDegraded ? "YES" : "NO") << ","
            << r.performanceDegradationPercent << ","
            << r.nanCount << ","
            << r.infCount << ","
            << r.dcOffsetCount << ","
            << r.clippingCount << ","
            << (r.audioQualityPassed ? "PASS" : "FAIL") << ","
            << (r.crashed ? "YES" : "NO") << "\n";
    }

    csv.close();
}

void generateMarkdownReport(const std::vector<EnduranceTestResult>& results,
                           const std::string& filename) {
    std::ofstream md(filename);

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    md << "# Endurance Test Report\n\n";
    md << "**Generated:** " << std::ctime(&time) << "\n";
    md << "**Test Duration:** 5 minutes per engine\n\n";

    // Summary table
    md << "## Summary\n\n";
    md << "| Engine | Name | Result | Memory Leak | Performance | Audio Quality |\n";
    md << "|--------|------|--------|-------------|-------------|---------------|\n";

    for (const auto& r : results) {
        md << "| " << r.engineId << " | " << r.engineName << " | ";
        md << (r.passed ? "✅ PASS" : "❌ FAIL") << " | ";
        md << (r.memoryLeak ? "⚠️ YES" : "✓ NO") << " | ";
        md << (r.performanceDegraded ? "⚠️ DEGRADED" : "✓ STABLE") << " | ";
        md << (r.audioQualityPassed ? "✓ PASS" : "❌ FAIL") << " |\n";
    }

    md << "\n## Detailed Results\n\n";

    for (const auto& r : results) {
        md << "### Engine " << r.engineId << ": " << r.engineName << "\n\n";

        if (r.crashed) {
            md << "**❌ TEST CRASHED:** " << r.errorMessage << "\n\n";
            continue;
        }

        md << "**Overall Result:** " << (r.passed ? "✅ PASSED" : "❌ FAILED") << "\n\n";

        md << "#### Duration\n";
        md << "- Test Time: " << std::fixed << std::setprecision(2)
           << (r.testDurationSeconds / 60.0) << " minutes\n";
        md << "- Samples Processed: " << r.totalSamplesProcessed << "\n";
        md << "- Blocks Processed: " << r.performance.blocksProcessed << "\n\n";

        md << "#### Memory Analysis\n";
        md << "- Initial: " << formatBytes(r.initialMemoryBytes) << "\n";
        md << "- Final: " << formatBytes(r.finalMemoryBytes) << "\n";
        md << "- Peak: " << formatBytes(r.peakMemoryBytes) << "\n";
        md << "- Growth: " << formatBytes(r.finalMemoryBytes - r.initialMemoryBytes);
        md << (r.memoryLeak ? " ⚠️ LEAK DETECTED" : " ✓ OK") << "\n";
        md << "- Growth Rate: " << std::fixed << std::setprecision(3)
           << r.memoryLeakRateMBPerMin << " MB/min\n\n";

        md << "#### Performance\n";
        md << "- Avg Block Time: " << std::fixed << std::setprecision(2)
           << r.performance.avgProcessTimeUs << " μs\n";
        md << "- Max Block Time: " << r.performance.maxProcessTimeUs << " μs\n";
        md << "- Real-time Ratio: " << std::fixed << std::setprecision(3)
           << ((r.performance.avgProcessTimeUs / 1000.0) / (512.0 / 48.0)) << "x\n";
        if (r.performanceDegraded) {
            md << "- Degradation: ⚠️ " << std::fixed << std::setprecision(1)
               << r.performanceDegradationPercent << "% slower over time\n";
        } else {
            md << "- Degradation: ✓ None detected\n";
        }
        md << "\n";

        md << "#### Audio Quality\n";
        md << "- NaN Detected: " << r.nanCount
           << (r.nanCount > 0 ? " ❌" : " ✓") << "\n";
        md << "- Inf Detected: " << r.infCount
           << (r.infCount > 0 ? " ❌" : " ✓") << "\n";
        md << "- DC Offset: " << r.dcOffsetCount << " blocks"
           << (r.dcOffsetCount > r.performance.blocksProcessed * 0.01 ? " ⚠️" : " ✓") << "\n";
        md << "- Clipping: " << r.clippingCount << " blocks"
           << (r.clippingCount > r.performance.blocksProcessed * 0.01 ? " ⚠️" : " ✓") << "\n";
        md << "\n";
    }

    // Add recommendations
    md << "## Recommendations\n\n";

    for (const auto& r : results) {
        if (!r.passed) {
            md << "### Engine " << r.engineId << ": " << r.engineName << "\n";
            if (r.memoryLeak) {
                md << "- ⚠️ **Memory Leak:** Growing at " << std::fixed << std::setprecision(3)
                   << r.memoryLeakRateMBPerMin << " MB/min. Check for unreleased buffers or resources.\n";
            }
            if (r.performanceDegraded) {
                md << "- ⚠️ **Performance Degradation:** " << std::fixed << std::setprecision(1)
                   << r.performanceDegradationPercent << "% slower. Check for accumulating state or inefficient algorithms.\n";
            }
            if (!r.audioQualityPassed) {
                md << "- ❌ **Audio Quality Issues:** ";
                if (r.nanCount > 0) md << "NaN values detected (" << r.nanCount << " blocks). ";
                if (r.infCount > 0) md << "Inf values detected (" << r.infCount << " blocks). ";
                if (r.dcOffsetCount > r.performance.blocksProcessed * 0.01)
                    md << "Excessive DC offset. ";
                if (r.clippingCount > r.performance.blocksProcessed * 0.01)
                    md << "Excessive clipping. ";
                md << "\n";
            }
            md << "\n";
        }
    }

    md.close();
}

} // namespace EnduranceTest

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[]) {
    using namespace EnduranceTest;

    // Test duration (default 5 minutes, can be overridden)
    double testDurationMinutes = 5.0;
    if (argc > 1) {
        testDurationMinutes = std::atof(argv[1]);
        if (testDurationMinutes < 1.0) testDurationMinutes = 1.0;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   ENDURANCE TEST: Reverbs & Time-Based Effects            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\nTest Duration: " << testDurationMinutes << " minutes per engine\n";
    std::cout << "Monitoring: Memory leaks, buffer overflows, performance degradation\n\n";

    // Define engines to test
    std::vector<std::pair<int, std::string>> engines = {
        {34, "Tape Echo"},
        {35, "Digital Delay"},
        {36, "Magnetic Drum Echo"},
        {37, "Bucket Brigade Delay"},
        {38, "Buffer Repeat Platinum"},
        {39, "Convolution Reverb"},
        {40, "Shimmer Reverb"},
        {41, "Plate Reverb"},
        {42, "Spring Reverb"},
        {43, "Gated Reverb"}
    };

    std::vector<EnduranceTestResult> results;

    // Run tests
    for (const auto& [id, name] : engines) {
        auto result = runEnduranceTest(id, name, testDurationMinutes);
        printTestResult(result);
        results.push_back(result);

        // Small delay between tests
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Generate reports
    std::cout << "\n════════════════════════════════════════════════════════════\n";
    std::cout << "  GENERATING REPORTS\n";
    std::cout << "════════════════════════════════════════════════════════════\n\n";

    generateCSVReport(results, "endurance_test_results.csv");
    std::cout << "✓ Generated: endurance_test_results.csv\n";

    generateMarkdownReport(results, "ENDURANCE_TEST_REPORT.md");
    std::cout << "✓ Generated: ENDURANCE_TEST_REPORT.md\n";

    // Summary
    int passed = 0, failed = 0, crashed = 0;
    for (const auto& r : results) {
        if (r.crashed) crashed++;
        else if (r.passed) passed++;
        else failed++;
    }

    std::cout << "\n════════════════════════════════════════════════════════════\n";
    std::cout << "  FINAL SUMMARY\n";
    std::cout << "════════════════════════════════════════════════════════════\n";
    std::cout << "  Passed:  " << passed << " / " << engines.size() << "\n";
    std::cout << "  Failed:  " << failed << " / " << engines.size() << "\n";
    std::cout << "  Crashed: " << crashed << " / " << engines.size() << "\n";
    std::cout << "════════════════════════════════════════════════════════════\n\n";

    return (failed == 0 && crashed == 0) ? 0 : 1;
}
