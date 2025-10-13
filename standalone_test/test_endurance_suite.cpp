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
#include <map>
#include <algorithm>

#if defined(__APPLE__)
#include <mach/mach.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <unistd.h>
#endif

/**
 * COMPREHENSIVE ENDURANCE & STRESS TEST SUITE
 *
 * Test Scenarios:
 * 1. Memory Stability (30 minutes) - All engines
 * 2. CPU Stability (30 minutes) - All engines
 * 3. Parameter Stability (10 minutes) - Continuous automation
 * 4. Buffer Overflow Testing (5 minutes) - Extreme buffer sizes
 * 5. Sample Rate Testing (5 minutes) - 44.1k to 192k
 *
 * Failure Criteria:
 * - Memory growth > 1 MB/min
 * - CPU usage drift > 20%
 * - Any NaN/Inf/crash
 * - Sample rate incompatibility
 */

namespace EnduranceTestSuite {

//==============================================================================
// Memory and Performance Monitoring
//==============================================================================

struct MemorySnapshot {
    size_t rss_bytes = 0;
    size_t virtual_bytes = 0;
    double timestamp = 0.0;
};

struct PerformanceMetrics {
    double avgProcessTimeUs = 0.0;
    double maxProcessTimeUs = 0.0;
    double minProcessTimeUs = 1e9;
    size_t blocksProcessed = 0;
    double totalTimeSeconds = 0.0;
    std::vector<double> blockTimes;  // For trend analysis
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
                snap.rss_bytes = atol(line + 6) * 1024;
            } else if (strncmp(line, "VmSize:", 7) == 0) {
                snap.virtual_bytes = atol(line + 7) * 1024;
            }
        }
        fclose(fp);
    }
#endif
    return snap;
}

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
            if (std::isnan(val)) metrics.hasNaN = true;
            if (std::isinf(val)) metrics.hasInf = true;
            if (std::abs(val) > 1.0f) metrics.isClipping = true;
            metrics.peakLevel = std::max(metrics.peakLevel, std::abs(val));
            sum += val;
            sumSquares += val * val;
        }
        int totalSamples = buffer.getNumSamples();
        float channelDC = sum / totalSamples;
        float channelRMS = std::sqrt(sumSquares / totalSamples);
        metrics.dcOffset += std::abs(channelDC);
        metrics.rmsLevel = std::max(metrics.rmsLevel, channelRMS);
    }
    metrics.dcOffset /= buffer.getNumChannels();
    metrics.hasDCOffset = (metrics.dcOffset > 0.01f);
    return metrics;
}

//==============================================================================
// Test Results
//==============================================================================

struct TestResult {
    int engineId;
    std::string engineName;
    std::string testName;
    bool passed = false;

    double testDurationSeconds = 0.0;
    size_t totalSamplesProcessed = 0;

    // Memory tracking
    std::vector<MemorySnapshot> memorySnapshots;
    size_t initialMemoryBytes = 0;
    size_t finalMemoryBytes = 0;
    size_t peakMemoryBytes = 0;
    double memoryLeakRateMBPerMin = 0.0;

    // Performance tracking
    PerformanceMetrics performance;
    double cpuDriftPercent = 0.0;

    // Audio quality
    int nanCount = 0;
    int infCount = 0;
    int dcOffsetCount = 0;
    int clippingCount = 0;

    bool crashed = false;
    std::string errorMessage;
};

//==============================================================================
// Test 1: Memory Stability Test (30 minutes)
//==============================================================================

TestResult testMemoryStability(int engineId, const std::string& engineName) {
    TestResult result;
    result.engineId = engineId;
    result.engineName = engineName;
    result.testName = "Memory Stability (30 min)";

    std::cout << "\n[Test 1: Memory Stability] Engine " << engineId << ": " << engineName << "\n";
    std::cout << "Testing for 30 minutes...\n";

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.crashed = true;
            result.errorMessage = "Failed to create engine";
            return result;
        }

        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        // Set moderate parameters
        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        if (numParams > 0) params[0] = 0.5f;
        if (numParams > 1) params[1] = 0.6f;
        if (numParams > 2) params[2] = 0.4f;
        if (numParams > 3) params[3] = 0.5f;
        if (numParams > 4) params[4] = 0.7f;
        engine->updateParameters(params);

        const double testDurationSeconds = 30.0 * 60.0;  // 30 minutes
        const size_t totalBlocks = static_cast<size_t>((testDurationSeconds * sampleRate) / blockSize);
        const size_t memoryCheckInterval = static_cast<size_t>((10.0 * sampleRate) / blockSize);  // Every 10 seconds

        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);

        result.initialMemoryBytes = getMemoryUsage().rss_bytes;
        MemorySnapshot initialSnapshot = getMemoryUsage();
        initialSnapshot.timestamp = 0.0;
        result.memorySnapshots.push_back(initialSnapshot);

        auto testStartTime = std::chrono::high_resolution_clock::now();
        auto lastProgressUpdate = testStartTime;

        juce::Random random;
        for (size_t blockIdx = 0; blockIdx < totalBlocks; ++blockIdx) {
            // Generate mixed test signal
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    float phase = 2.0f * M_PI * 440.0f * (blockIdx * blockSize + i) / sampleRate;
                    float sine = 0.3f * std::sin(phase);
                    float noise = 0.05f * (random.nextFloat() * 2.0f - 1.0f);
                    inputBuffer.setSample(ch, i, sine + noise);
                }
            }

            outputBuffer.makeCopyOf(inputBuffer);

            auto blockStartTime = std::chrono::high_resolution_clock::now();
            engine->process(outputBuffer);
            auto blockEndTime = std::chrono::high_resolution_clock::now();

            double processTimeUs = std::chrono::duration<double, std::micro>(blockEndTime - blockStartTime).count();
            result.performance.blocksProcessed++;
            result.performance.avgProcessTimeUs += processTimeUs;
            result.performance.maxProcessTimeUs = std::max(result.performance.maxProcessTimeUs, processTimeUs);
            result.performance.minProcessTimeUs = std::min(result.performance.minProcessTimeUs, processTimeUs);
            result.performance.blockTimes.push_back(processTimeUs);

            // Validate audio
            auto audioMetrics = analyzeBuffer(outputBuffer);
            if (audioMetrics.hasNaN) result.nanCount++;
            if (audioMetrics.hasInf) result.infCount++;
            if (audioMetrics.hasDCOffset) result.dcOffsetCount++;
            if (audioMetrics.isClipping) result.clippingCount++;

            // Check memory periodically
            if (blockIdx % memoryCheckInterval == 0) {
                auto now = std::chrono::high_resolution_clock::now();
                double elapsed = std::chrono::duration<double>(now - testStartTime).count();
                MemorySnapshot snap = getMemoryUsage();
                snap.timestamp = elapsed;
                result.memorySnapshots.push_back(snap);
                result.peakMemoryBytes = std::max(result.peakMemoryBytes, snap.rss_bytes);
            }

            // Progress update every 10 seconds
            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<double>(now - lastProgressUpdate).count() >= 10.0) {
                double progressPercent = (blockIdx * 100.0) / totalBlocks;
                double elapsedMin = std::chrono::duration<double>(now - testStartTime).count() / 60.0;
                std::cout << "  Progress: " << std::fixed << std::setprecision(1)
                         << progressPercent << "% (" << elapsedMin << " min)\n";
                lastProgressUpdate = now;
            }
        }

        auto testEndTime = std::chrono::high_resolution_clock::now();
        result.testDurationSeconds = std::chrono::duration<double>(testEndTime - testStartTime).count();
        result.totalSamplesProcessed = totalBlocks * blockSize;
        result.finalMemoryBytes = getMemoryUsage().rss_bytes;

        result.performance.avgProcessTimeUs /= result.performance.blocksProcessed;
        result.performance.totalTimeSeconds = result.testDurationSeconds;

        // Analyze memory leak
        if (result.memorySnapshots.size() >= 2) {
            size_t memoryGrowth = result.finalMemoryBytes - result.initialMemoryBytes;
            double durationMinutes = result.testDurationSeconds / 60.0;
            result.memoryLeakRateMBPerMin = (memoryGrowth / (1024.0 * 1024.0)) / durationMinutes;
        }

        // Calculate CPU drift (compare first 10% vs last 10% of blocks)
        size_t windowSize = result.performance.blockTimes.size() / 10;
        double firstAvg = 0.0, lastAvg = 0.0;
        for (size_t i = 0; i < windowSize; ++i) {
            firstAvg += result.performance.blockTimes[i];
            lastAvg += result.performance.blockTimes[result.performance.blockTimes.size() - 1 - i];
        }
        firstAvg /= windowSize;
        lastAvg /= windowSize;
        if (firstAvg > 0) {
            result.cpuDriftPercent = ((lastAvg - firstAvg) / firstAvg) * 100.0;
        }

        // Determine pass/fail
        result.passed = (result.memoryLeakRateMBPerMin <= 1.0) &&
                       (result.nanCount == 0) &&
                       (result.infCount == 0) &&
                       (!result.crashed);

    } catch (const std::exception& e) {
        result.crashed = true;
        result.errorMessage = e.what();
    }

    return result;
}

//==============================================================================
// Test 2: CPU Stability Test (30 minutes)
//==============================================================================

TestResult testCPUStability(int engineId, const std::string& engineName) {
    TestResult result;
    result.engineId = engineId;
    result.engineName = engineName;
    result.testName = "CPU Stability (30 min)";

    std::cout << "\n[Test 2: CPU Stability] Engine " << engineId << ": " << engineName << "\n";
    std::cout << "Testing CPU usage for 30 minutes...\n";

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.crashed = true;
            result.errorMessage = "Failed to create engine";
            return result;
        }

        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        std::map<int, float> params;
        int numParams = engine->getNumParameters();
        if (numParams > 0) params[0] = 0.7f;  // Higher mix for more CPU load
        if (numParams > 1) params[1] = 0.8f;
        if (numParams > 2) params[2] = 0.6f;
        if (numParams > 3) params[3] = 0.7f;
        if (numParams > 4) params[4] = 0.8f;
        engine->updateParameters(params);

        const double testDurationSeconds = 30.0 * 60.0;
        const size_t totalBlocks = static_cast<size_t>((testDurationSeconds * sampleRate) / blockSize);

        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);

        result.initialMemoryBytes = getMemoryUsage().rss_bytes;
        auto testStartTime = std::chrono::high_resolution_clock::now();
        auto lastProgressUpdate = testStartTime;

        juce::Random random;
        for (size_t blockIdx = 0; blockIdx < totalBlocks; ++blockIdx) {
            // Generate test signal
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    float phase = 2.0f * M_PI * 440.0f * (blockIdx * blockSize + i) / sampleRate;
                    float sine = 0.4f * std::sin(phase);
                    float noise = 0.1f * (random.nextFloat() * 2.0f - 1.0f);
                    inputBuffer.setSample(ch, i, sine + noise);
                }
            }

            outputBuffer.makeCopyOf(inputBuffer);

            auto blockStartTime = std::chrono::high_resolution_clock::now();
            engine->process(outputBuffer);
            auto blockEndTime = std::chrono::high_resolution_clock::now();

            double processTimeUs = std::chrono::duration<double, std::micro>(blockEndTime - blockStartTime).count();
            result.performance.blocksProcessed++;
            result.performance.avgProcessTimeUs += processTimeUs;
            result.performance.maxProcessTimeUs = std::max(result.performance.maxProcessTimeUs, processTimeUs);
            result.performance.minProcessTimeUs = std::min(result.performance.minProcessTimeUs, processTimeUs);
            result.performance.blockTimes.push_back(processTimeUs);

            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<double>(now - lastProgressUpdate).count() >= 10.0) {
                double progressPercent = (blockIdx * 100.0) / totalBlocks;
                double elapsedMin = std::chrono::duration<double>(now - testStartTime).count() / 60.0;
                std::cout << "  Progress: " << std::fixed << std::setprecision(1)
                         << progressPercent << "% (" << elapsedMin << " min)\n";
                lastProgressUpdate = now;
            }
        }

        auto testEndTime = std::chrono::high_resolution_clock::now();
        result.testDurationSeconds = std::chrono::duration<double>(testEndTime - testStartTime).count();
        result.totalSamplesProcessed = totalBlocks * blockSize;
        result.finalMemoryBytes = getMemoryUsage().rss_bytes;

        result.performance.avgProcessTimeUs /= result.performance.blocksProcessed;
        result.performance.totalTimeSeconds = result.testDurationSeconds;

        // Calculate CPU drift
        size_t windowSize = result.performance.blockTimes.size() / 10;
        double firstAvg = 0.0, lastAvg = 0.0;
        for (size_t i = 0; i < windowSize; ++i) {
            firstAvg += result.performance.blockTimes[i];
            lastAvg += result.performance.blockTimes[result.performance.blockTimes.size() - 1 - i];
        }
        firstAvg /= windowSize;
        lastAvg /= windowSize;
        if (firstAvg > 0) {
            result.cpuDriftPercent = ((lastAvg - firstAvg) / firstAvg) * 100.0;
        }

        result.passed = (result.cpuDriftPercent <= 20.0) && (!result.crashed);

    } catch (const std::exception& e) {
        result.crashed = true;
        result.errorMessage = e.what();
    }

    return result;
}

//==============================================================================
// Test 3: Parameter Stability Test (10 minutes with automation)
//==============================================================================

TestResult testParameterStability(int engineId, const std::string& engineName) {
    TestResult result;
    result.engineId = engineId;
    result.engineName = engineName;
    result.testName = "Parameter Stability (10 min)";

    std::cout << "\n[Test 3: Parameter Stability] Engine " << engineId << ": " << engineName << "\n";
    std::cout << "Testing with continuous parameter automation...\n";

    try {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.crashed = true;
            result.errorMessage = "Failed to create engine";
            return result;
        }

        const float sampleRate = 48000.0f;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);

        const double testDurationSeconds = 10.0 * 60.0;  // 10 minutes
        const size_t totalBlocks = static_cast<size_t>((testDurationSeconds * sampleRate) / blockSize);

        juce::AudioBuffer<float> inputBuffer(2, blockSize);
        juce::AudioBuffer<float> outputBuffer(2, blockSize);

        result.initialMemoryBytes = getMemoryUsage().rss_bytes;
        auto testStartTime = std::chrono::high_resolution_clock::now();
        auto lastProgressUpdate = testStartTime;

        int numParams = engine->getNumParameters();
        juce::Random random;

        for (size_t blockIdx = 0; blockIdx < totalBlocks; ++blockIdx) {
            // Modulate parameters with LFOs
            double time = (blockIdx * blockSize) / sampleRate;
            std::map<int, float> params;

            for (int p = 0; p < numParams; ++p) {
                // Different LFO rates for each parameter
                float lfo = 0.5f + 0.5f * std::sin(2.0 * M_PI * (0.05 + p * 0.01) * time);
                params[p] = lfo;
            }
            engine->updateParameters(params);

            // Generate test signal
            for (int ch = 0; ch < 2; ++ch) {
                for (int i = 0; i < blockSize; ++i) {
                    float phase = 2.0f * M_PI * 440.0f * (blockIdx * blockSize + i) / sampleRate;
                    float sine = 0.3f * std::sin(phase);
                    float noise = 0.05f * (random.nextFloat() * 2.0f - 1.0f);
                    inputBuffer.setSample(ch, i, sine + noise);
                }
            }

            outputBuffer.makeCopyOf(inputBuffer);

            auto blockStartTime = std::chrono::high_resolution_clock::now();
            engine->process(outputBuffer);
            auto blockEndTime = std::chrono::high_resolution_clock::now();

            double processTimeUs = std::chrono::duration<double, std::micro>(blockEndTime - blockStartTime).count();
            result.performance.blocksProcessed++;
            result.performance.avgProcessTimeUs += processTimeUs;
            result.performance.maxProcessTimeUs = std::max(result.performance.maxProcessTimeUs, processTimeUs);
            result.performance.minProcessTimeUs = std::min(result.performance.minProcessTimeUs, processTimeUs);

            auto audioMetrics = analyzeBuffer(outputBuffer);
            if (audioMetrics.hasNaN) result.nanCount++;
            if (audioMetrics.hasInf) result.infCount++;

            auto now = std::chrono::high_resolution_clock::now();
            if (std::chrono::duration<double>(now - lastProgressUpdate).count() >= 10.0) {
                double progressPercent = (blockIdx * 100.0) / totalBlocks;
                double elapsedMin = std::chrono::duration<double>(now - testStartTime).count() / 60.0;
                std::cout << "  Progress: " << std::fixed << std::setprecision(1)
                         << progressPercent << "% (" << elapsedMin << " min)\n";
                lastProgressUpdate = now;
            }
        }

        auto testEndTime = std::chrono::high_resolution_clock::now();
        result.testDurationSeconds = std::chrono::duration<double>(testEndTime - testStartTime).count();
        result.totalSamplesProcessed = totalBlocks * blockSize;
        result.finalMemoryBytes = getMemoryUsage().rss_bytes;

        result.performance.avgProcessTimeUs /= result.performance.blocksProcessed;
        result.passed = (result.nanCount == 0) && (result.infCount == 0) && (!result.crashed);

    } catch (const std::exception& e) {
        result.crashed = true;
        result.errorMessage = e.what();
    }

    return result;
}

//==============================================================================
// Test 4: Buffer Overflow Test (5 minutes with extreme buffer sizes)
//==============================================================================

TestResult testBufferOverflow(int engineId, const std::string& engineName) {
    TestResult result;
    result.engineId = engineId;
    result.engineName = engineName;
    result.testName = "Buffer Overflow (5 min)";

    std::cout << "\n[Test 4: Buffer Overflow] Engine " << engineId << ": " << engineName << "\n";
    std::cout << "Testing with extreme buffer sizes (64-8192)...\n";

    try {
        const float sampleRate = 48000.0f;
        const double testDurationSeconds = 5.0 * 60.0;  // 5 minutes
        std::vector<int> bufferSizes = {64, 128, 256, 512, 1024, 2048, 4096, 8192};

        auto testStartTime = std::chrono::high_resolution_clock::now();
        double timePerSize = testDurationSeconds / bufferSizes.size();

        for (int blockSize : bufferSizes) {
            std::cout << "  Testing buffer size: " << blockSize << "\n";

            auto engine = EngineFactory::createEngine(engineId);
            if (!engine) {
                result.crashed = true;
                result.errorMessage = "Failed to create engine";
                return result;
            }

            engine->prepareToPlay(sampleRate, blockSize);

            std::map<int, float> params;
            int numParams = engine->getNumParameters();
            if (numParams > 0) params[0] = 0.5f;
            if (numParams > 1) params[1] = 0.6f;
            engine->updateParameters(params);

            size_t blocksForThisSize = static_cast<size_t>((timePerSize * sampleRate) / blockSize);
            juce::AudioBuffer<float> inputBuffer(2, blockSize);
            juce::AudioBuffer<float> outputBuffer(2, blockSize);
            juce::Random random;

            for (size_t blockIdx = 0; blockIdx < blocksForThisSize; ++blockIdx) {
                for (int ch = 0; ch < 2; ++ch) {
                    for (int i = 0; i < blockSize; ++i) {
                        float phase = 2.0f * M_PI * 440.0f * (blockIdx * blockSize + i) / sampleRate;
                        inputBuffer.setSample(ch, i, 0.3f * std::sin(phase));
                    }
                }

                outputBuffer.makeCopyOf(inputBuffer);
                engine->process(outputBuffer);

                auto audioMetrics = analyzeBuffer(outputBuffer);
                if (audioMetrics.hasNaN) result.nanCount++;
                if (audioMetrics.hasInf) result.infCount++;

                result.performance.blocksProcessed++;
                result.totalSamplesProcessed += blockSize;
            }
        }

        auto testEndTime = std::chrono::high_resolution_clock::now();
        result.testDurationSeconds = std::chrono::duration<double>(testEndTime - testStartTime).count();
        result.passed = (result.nanCount == 0) && (result.infCount == 0) && (!result.crashed);

    } catch (const std::exception& e) {
        result.crashed = true;
        result.errorMessage = e.what();
    }

    return result;
}

//==============================================================================
// Test 5: Sample Rate Test (5 minutes across multiple rates)
//==============================================================================

TestResult testSampleRates(int engineId, const std::string& engineName) {
    TestResult result;
    result.engineId = engineId;
    result.engineName = engineName;
    result.testName = "Sample Rate (5 min)";

    std::cout << "\n[Test 5: Sample Rate] Engine " << engineId << ": " << engineName << "\n";
    std::cout << "Testing at 44.1k, 48k, 88.2k, 96k, 192k...\n";

    try {
        const double testDurationSeconds = 5.0 * 60.0;
        std::vector<float> sampleRates = {44100.0f, 48000.0f, 88200.0f, 96000.0f, 192000.0f};
        const int blockSize = 512;

        auto testStartTime = std::chrono::high_resolution_clock::now();
        double timePerRate = testDurationSeconds / sampleRates.size();

        for (float sampleRate : sampleRates) {
            std::cout << "  Testing sample rate: " << sampleRate << " Hz\n";

            auto engine = EngineFactory::createEngine(engineId);
            if (!engine) {
                result.crashed = true;
                result.errorMessage = "Failed to create engine";
                return result;
            }

            engine->prepareToPlay(sampleRate, blockSize);

            std::map<int, float> params;
            int numParams = engine->getNumParameters();
            if (numParams > 0) params[0] = 0.5f;
            if (numParams > 1) params[1] = 0.6f;
            engine->updateParameters(params);

            size_t blocksForThisRate = static_cast<size_t>((timePerRate * sampleRate) / blockSize);
            juce::AudioBuffer<float> inputBuffer(2, blockSize);
            juce::AudioBuffer<float> outputBuffer(2, blockSize);
            juce::Random random;

            for (size_t blockIdx = 0; blockIdx < blocksForThisRate; ++blockIdx) {
                for (int ch = 0; ch < 2; ++ch) {
                    for (int i = 0; i < blockSize; ++i) {
                        float phase = 2.0f * M_PI * 440.0f * (blockIdx * blockSize + i) / sampleRate;
                        inputBuffer.setSample(ch, i, 0.3f * std::sin(phase));
                    }
                }

                outputBuffer.makeCopyOf(inputBuffer);
                engine->process(outputBuffer);

                auto audioMetrics = analyzeBuffer(outputBuffer);
                if (audioMetrics.hasNaN) result.nanCount++;
                if (audioMetrics.hasInf) result.infCount++;

                result.performance.blocksProcessed++;
                result.totalSamplesProcessed += blockSize;
            }
        }

        auto testEndTime = std::chrono::high_resolution_clock::now();
        result.testDurationSeconds = std::chrono::duration<double>(testEndTime - testStartTime).count();
        result.passed = (result.nanCount == 0) && (result.infCount == 0) && (!result.crashed);

    } catch (const std::exception& e) {
        result.crashed = true;
        result.errorMessage = e.what();
    }

    return result;
}

//==============================================================================
// Engine Categories
//==============================================================================

struct EngineInfo {
    int id;
    std::string name;
    std::string category;
};

std::vector<EngineInfo> getAllEngines() {
    return {
        // Utility (0-1)
        {0, "Clean Signal Pass", "Utility"},
        {1, "Mute", "Utility"},

        // Dynamics (2-7)
        {2, "Compressor", "Dynamics"},
        {3, "Limiter", "Dynamics"},
        {4, "Gate", "Dynamics"},
        {5, "Expander", "Dynamics"},
        {6, "Transient Shaper", "Dynamics"},
        {7, "Dynamic EQ", "Dynamics"},

        // Filters/EQ (8-14)
        {8, "Low Pass Filter", "Filter"},
        {9, "Ladder Filter", "Filter"},
        {10, "Comb Filter", "Filter"},
        {11, "Resonant Low Pass", "Filter"},
        {12, "State Variable Filter", "Filter"},
        {13, "Formant Filter", "Filter"},
        {14, "Parametric EQ", "Filter"},

        // Distortion (15-22)
        {15, "Muff Fuzz", "Distortion"},
        {16, "Rodent Distortion", "Distortion"},
        {17, "TS9 Overdrive", "Distortion"},
        {18, "Soft Clip", "Distortion"},
        {19, "Hard Clip", "Distortion"},
        {20, "Bit Crusher", "Distortion"},
        {21, "Wave Folder", "Distortion"},
        {22, "Tube Saturator", "Distortion"},

        // Modulation (23-31)
        {23, "Tremolo", "Modulation"},
        {24, "Ring Modulator", "Modulation"},
        {25, "Phaser", "Modulation"},
        {26, "Flanger", "Modulation"},
        {27, "Chorus", "Modulation"},
        {28, "Vibrato", "Modulation"},
        {29, "Auto-Wah", "Modulation"},
        {30, "Envelope Follower", "Modulation"},
        {31, "Rotary Speaker", "Modulation"},

        // Pitch/Time (32-38)
        {32, "Pitch Shifter", "Pitch"},
        {33, "Intelligent Harmonizer", "Pitch"},
        {34, "Tape Echo", "Delay"},
        {35, "Digital Delay", "Delay"},
        {36, "Magnetic Drum Echo", "Delay"},
        {37, "Bucket Brigade Delay", "Delay"},
        {38, "Buffer Repeat Platinum", "Delay"},

        // Reverbs (39-43)
        {39, "Convolution Reverb", "Reverb"},
        {40, "Shimmer Reverb", "Reverb"},
        {41, "Plate Reverb", "Reverb"},
        {42, "Spring Reverb", "Reverb"},
        {43, "Gated Reverb", "Reverb"},

        // Spatial (44-48)
        {44, "Stereo Widener", "Spatial"},
        {45, "Detune Doubler", "Spatial"},
        {46, "Haas Effect", "Spatial"},
        {47, "Mid-Side Processor", "Spatial"},
        {48, "Binaural Panner", "Spatial"},

        // Spectral (49-52)
        {49, "Phased Vocoder", "Spectral"},
        {50, "Spectral Freeze", "Spectral"},
        {51, "Spectral Blur", "Spectral"},
        {52, "Spectral Gate", "Spectral"},

        // Special (53-55)
        {53, "Granular Engine", "Special"},
        {54, "Texture Synthesizer", "Special"},
        {55, "Field Recording Sim", "Special"}
    };
}

//==============================================================================
// Main Test Runner
//==============================================================================

void printTestResult(const TestResult& result) {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";
    std::cout << " ENGINE " << result.engineId << ": " << result.engineName << "\n";
    std::cout << " TEST: " << result.testName << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";

    if (result.crashed) {
        std::cout << "❌ CRASHED: " << result.errorMessage << "\n";
        return;
    }

    std::cout << "Duration: " << std::fixed << std::setprecision(2)
             << result.testDurationSeconds / 60.0 << " minutes\n";
    std::cout << "Samples Processed: " << result.totalSamplesProcessed << "\n";

    if (result.testName.find("Memory") != std::string::npos) {
        std::cout << "\nMEMORY ANALYSIS:\n";
        std::cout << "  Initial: " << formatBytes(result.initialMemoryBytes) << "\n";
        std::cout << "  Final:   " << formatBytes(result.finalMemoryBytes) << "\n";
        std::cout << "  Peak:    " << formatBytes(result.peakMemoryBytes) << "\n";
        std::cout << "  Growth:  " << formatBytes(result.finalMemoryBytes - result.initialMemoryBytes);
        std::cout << " (" << std::fixed << std::setprecision(3) << result.memoryLeakRateMBPerMin << " MB/min)\n";

        if (result.memoryLeakRateMBPerMin > 1.0) {
            std::cout << "  ❌ MEMORY LEAK DETECTED\n";
        } else {
            std::cout << "  ✅ No significant memory leak\n";
        }
    }

    if (result.testName.find("CPU") != std::string::npos || result.testName.find("Memory") != std::string::npos) {
        std::cout << "\nCPU ANALYSIS:\n";
        std::cout << "  Avg Block Time: " << std::fixed << std::setprecision(2)
                 << result.performance.avgProcessTimeUs << " μs\n";
        std::cout << "  Min Block Time: " << result.performance.minProcessTimeUs << " μs\n";
        std::cout << "  Max Block Time: " << result.performance.maxProcessTimeUs << " μs\n";
        std::cout << "  CPU Drift:      " << std::fixed << std::setprecision(1)
                 << result.cpuDriftPercent << "%\n";

        if (result.cpuDriftPercent > 20.0) {
            std::cout << "  ❌ SIGNIFICANT CPU DRIFT\n";
        } else {
            std::cout << "  ✅ CPU usage stable\n";
        }
    }

    std::cout << "\nAUDIO QUALITY:\n";
    std::cout << "  NaN:      " << result.nanCount << (result.nanCount == 0 ? " ✅" : " ❌") << "\n";
    std::cout << "  Inf:      " << result.infCount << (result.infCount == 0 ? " ✅" : " ❌") << "\n";
    std::cout << "  DC Offset:" << result.dcOffsetCount << "\n";
    std::cout << "  Clipping: " << result.clippingCount << "\n";

    std::cout << "\nRESULT: " << (result.passed ? "✅ PASSED" : "❌ FAILED") << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════\n";
}

} // namespace EnduranceTestSuite

//==============================================================================
// Main
//==============================================================================

int main(int argc, char* argv[]) {
    using namespace EnduranceTestSuite;

    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   COMPREHENSIVE ENDURANCE & STRESS TEST SUITE                 ║\n";
    std::cout << "║   Project Chimera v3.0 Phoenix                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "Test Suite:\n";
    std::cout << "  1. Memory Stability (30 minutes per engine)\n";
    std::cout << "  2. CPU Stability (30 minutes per engine)\n";
    std::cout << "  3. Parameter Stability (10 minutes with automation)\n";
    std::cout << "  4. Buffer Overflow (5 minutes, sizes 64-8192)\n";
    std::cout << "  5. Sample Rate Test (5 minutes, 44.1k-192k)\n";
    std::cout << "\n";

    // Parse command line
    int testMode = 0;  // 0 = all tests, 1-5 = specific test
    int engineFilter = -1;  // -1 = all engines, specific ID = one engine

    if (argc > 1) {
        testMode = std::atoi(argv[1]);
    }
    if (argc > 2) {
        engineFilter = std::atoi(argv[2]);
    }

    auto allEngines = getAllEngines();
    std::vector<TestResult> allResults;

    // Filter engines if requested
    std::vector<EngineInfo> enginesToTest;
    if (engineFilter >= 0) {
        for (const auto& engine : allEngines) {
            if (engine.id == engineFilter) {
                enginesToTest.push_back(engine);
                break;
            }
        }
    } else {
        enginesToTest = allEngines;
    }

    std::cout << "Testing " << enginesToTest.size() << " engine(s)...\n";
    std::cout << "\n";

    // Run tests
    for (const auto& engine : enginesToTest) {
        std::cout << "\n";
        std::cout << "═══════════════════════════════════════════════════════════════\n";
        std::cout << " ENGINE " << engine.id << ": " << engine.name << " (" << engine.category << ")\n";
        std::cout << "═══════════════════════════════════════════════════════════════\n";

        if (testMode == 0 || testMode == 1) {
            auto result = testMemoryStability(engine.id, engine.name);
            allResults.push_back(result);
            printTestResult(result);
        }

        if (testMode == 0 || testMode == 2) {
            auto result = testCPUStability(engine.id, engine.name);
            allResults.push_back(result);
            printTestResult(result);
        }

        if (testMode == 0 || testMode == 3) {
            auto result = testParameterStability(engine.id, engine.name);
            allResults.push_back(result);
            printTestResult(result);
        }

        if (testMode == 0 || testMode == 4) {
            auto result = testBufferOverflow(engine.id, engine.name);
            allResults.push_back(result);
            printTestResult(result);
        }

        if (testMode == 0 || testMode == 5) {
            auto result = testSampleRates(engine.id, engine.name);
            allResults.push_back(result);
            printTestResult(result);
        }
    }

    // Generate summary report
    std::cout << "\n\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   FINAL SUMMARY                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";

    int totalTests = allResults.size();
    int passed = 0;
    int failed = 0;
    int crashed = 0;

    for (const auto& result : allResults) {
        if (result.crashed) crashed++;
        else if (result.passed) passed++;
        else failed++;
    }

    std::cout << "\nTotal Tests: " << totalTests << "\n";
    std::cout << "  ✅ Passed:  " << passed << "\n";
    std::cout << "  ❌ Failed:  " << failed << "\n";
    std::cout << "  💥 Crashed: " << crashed << "\n";

    std::cout << "\n";

    // Write CSV report
    std::ofstream csv("endurance_suite_results.csv");
    csv << "EngineID,EngineName,TestName,Passed,Crashed,MemoryLeakMBperMin,CPUDriftPercent,NaNCount,InfCount\n";
    for (const auto& result : allResults) {
        csv << result.engineId << ","
            << result.engineName << ","
            << result.testName << ","
            << (result.passed ? "YES" : "NO") << ","
            << (result.crashed ? "YES" : "NO") << ","
            << std::fixed << std::setprecision(3) << result.memoryLeakRateMBPerMin << ","
            << std::fixed << std::setprecision(1) << result.cpuDriftPercent << ","
            << result.nanCount << ","
            << result.infCount << "\n";
    }
    csv.close();

    std::cout << "✅ Report saved: endurance_suite_results.csv\n\n";

    return (failed > 0 || crashed > 0) ? 1 : 0;
}
