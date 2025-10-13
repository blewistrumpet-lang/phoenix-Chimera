/**
 * PITCH ENGINE PERFORMANCE PROFILER
 *
 * Comprehensive CPU and memory profiling for all 8 pitch engines (31-38)
 * Tests real-time capability, efficiency, latency, and resource usage
 *
 * Performance Targets:
 * - CPU: < 5% per engine (48kHz, 512 buffer)
 * - Memory: < 5 MB per engine
 * - Latency: < 100ms total
 * - No memory leaks
 * - Real-time safe (no audio thread allocations)
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <memory>
#include <sys/resource.h>
#include <unistd.h>

// Include JUCE for EngineBase
#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineBase.h"

// Include pitch engines
#include "../JUCE_Plugin/Source/PitchShifter.h"
#include "../JUCE_Plugin/Source/DetuneDoubler.h"
#include "../JUCE_Plugin/Source/IntelligentHarmonizer.h"
#include "../JUCE_Plugin/Source/TapeEcho.h"
#include "../JUCE_Plugin/Source/DigitalDelay.h"
#include "../JUCE_Plugin/Source/MagneticDrumEcho.h"
#include "../JUCE_Plugin/Source/BucketBrigadeDelay.h"
#include "../JUCE_Plugin/Source/BufferRepeat.h"

using namespace std;
using namespace std::chrono;

// ===========================
// MEMORY TRACKING
// ===========================

struct MemorySnapshot {
    size_t rss_kb;           // Resident Set Size (actual RAM usage)
    size_t peak_rss_kb;      // Peak RSS
    size_t heap_kb;          // Heap allocations

    MemorySnapshot() : rss_kb(0), peak_rss_kb(0), heap_kb(0) {}
};

MemorySnapshot getMemoryUsage() {
    MemorySnapshot snapshot;

    // Get RSS from /proc/self/stat (Linux) or task_info (macOS)
#ifdef __APPLE__
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        snapshot.rss_kb = usage.ru_maxrss / 1024; // macOS returns bytes
        snapshot.peak_rss_kb = usage.ru_maxrss / 1024;
    }
#else
    // Linux version
    FILE* file = fopen("/proc/self/statm", "r");
    if (file) {
        long rss;
        if (fscanf(file, "%*s%ld", &rss) == 1) {
            snapshot.rss_kb = rss * (sysconf(_SC_PAGESIZE) / 1024);
        }
        fclose(file);
    }

    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        snapshot.peak_rss_kb = usage.ru_maxrss;
    }
#endif

    return snapshot;
}

// ===========================
// CPU TIMING
// ===========================

struct CPUMeasurement {
    double mean_us;          // Mean processing time (microseconds)
    double std_dev_us;       // Standard deviation
    double min_us;           // Minimum time
    double max_us;           // Maximum time
    double cpu_percent;      // CPU percentage (vs real-time available)
    int samples_processed;   // Total samples processed

    CPUMeasurement() : mean_us(0), std_dev_us(0), min_us(0), max_us(0),
                       cpu_percent(0), samples_processed(0) {}
};

// Measure CPU time for processing a buffer
CPUMeasurement measureCPU(EngineBase* engine, int bufferSize, double sampleRate,
                          int numIterations = 1000) {
    CPUMeasurement result;
    vector<double> times_us;
    times_us.reserve(numIterations);

    // Prepare test signal (sine wave) using JUCE AudioBuffer
    juce::AudioBuffer<float> buffer(2, bufferSize);  // Stereo

    for (int ch = 0; ch < 2; ++ch) {
        float* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < bufferSize; ++i) {
            channelData[i] = 0.5f * sin(2.0 * M_PI * 440.0 * i / sampleRate);
        }
    }

    // Warm-up
    for (int i = 0; i < 10; ++i) {
        engine->process(buffer);
    }

    // Measure
    for (int i = 0; i < numIterations; ++i) {
        auto start = high_resolution_clock::now();
        engine->process(buffer);
        auto end = high_resolution_clock::now();

        double elapsed_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
        times_us.push_back(elapsed_us);
    }

    // Calculate statistics
    result.samples_processed = bufferSize * numIterations;
    result.min_us = *min_element(times_us.begin(), times_us.end());
    result.max_us = *max_element(times_us.begin(), times_us.end());
    result.mean_us = accumulate(times_us.begin(), times_us.end(), 0.0) / times_us.size();

    // Standard deviation
    double sum_squared_diff = 0.0;
    for (double t : times_us) {
        double diff = t - result.mean_us;
        sum_squared_diff += diff * diff;
    }
    result.std_dev_us = sqrt(sum_squared_diff / times_us.size());

    // CPU percentage: (processing_time / real_time) * 100
    double real_time_us = (bufferSize / sampleRate) * 1e6;
    result.cpu_percent = (result.mean_us / real_time_us) * 100.0;

    return result;
}

// ===========================
// LATENCY MEASUREMENT
// ===========================

struct LatencyMeasurement {
    int latency_samples;
    double latency_ms;
    int lookahead_samples;

    LatencyMeasurement() : latency_samples(0), latency_ms(0), lookahead_samples(0) {}
};

LatencyMeasurement measureLatency(EngineBase* engine, double sampleRate, int bufferSize) {
    LatencyMeasurement result;

    // Get reported latency
    result.latency_samples = engine->getLatencySamples();
    result.latency_ms = (result.latency_samples / sampleRate) * 1000.0;

    // Measure actual latency using impulse response
    juce::AudioBuffer<float> impulse(2, bufferSize * 4);
    impulse.clear();

    // Create impulse at start
    impulse.setSample(0, 0, 1.0f);
    impulse.setSample(1, 0, 1.0f);

    // Process
    engine->reset();
    engine->process(impulse);

    // Find first non-zero output
    int actual_latency = 0;
    for (int i = 0; i < impulse.getNumSamples(); ++i) {
        if (fabs(impulse.getSample(0, i)) > 0.001f && i > 0) {
            actual_latency = i;
            break;
        }
    }

    result.lookahead_samples = actual_latency;

    return result;
}

// ===========================
// ENGINE INFO
// ===========================

struct EngineInfo {
    int id;
    string name;
    string category;
};

vector<EngineInfo> getPitchEngines() {
    return {
        {31, "Pitch Shifter", "Pitch"},
        {32, "Detune Doubler", "Pitch"},
        {33, "Intelligent Harmonizer", "Pitch"},
        {34, "Tape Echo", "Delay/Pitch"},
        {35, "Digital Delay", "Delay"},
        {36, "Magnetic Drum Echo", "Delay"},
        {37, "Bucket Brigade Delay", "Delay"},
        {38, "Buffer Repeat", "Delay/Pitch"}
    };
}

unique_ptr<EngineBase> createEngine(int id) {
    switch (id) {
        case 31: return make_unique<PitchShifter>();
        case 32: return make_unique<AudioDSP::DetuneDoubler>();
        case 33: return make_unique<IntelligentHarmonizer>();
        case 34: return make_unique<TapeEcho>();
        case 35: return make_unique<AudioDSP::DigitalDelay>();
        case 36: return make_unique<MagneticDrumEcho>();
        case 37: return make_unique<BucketBrigadeDelay>();
        case 38: return make_unique<BufferRepeat>();
        default: return nullptr;
    }
}

// ===========================
// PERFORMANCE TESTS
// ===========================

struct PerformanceResult {
    EngineInfo info;
    map<pair<double, int>, CPUMeasurement> cpu_results; // (sampleRate, bufferSize) -> measurement
    LatencyMeasurement latency;
    MemorySnapshot memory_before;
    MemorySnapshot memory_after;
    size_t memory_delta_kb;
    bool real_time_capable;
    double worst_case_cpu;
};

void testSingleEngine(int engineID, vector<PerformanceResult>& results) {
    PerformanceResult result;

    auto engines = getPitchEngines();
    auto it = find_if(engines.begin(), engines.end(),
                      [engineID](const EngineInfo& e) { return e.id == engineID; });
    if (it == engines.end()) {
        cout << "ERROR: Unknown engine ID " << engineID << endl;
        return;
    }

    result.info = *it;

    cout << "\n========================================" << endl;
    cout << "Testing Engine " << engineID << ": " << result.info.name << endl;
    cout << "========================================" << endl;

    // Create engine
    auto engine = createEngine(engineID);
    if (!engine) {
        cout << "ERROR: Failed to create engine!" << endl;
        return;
    }

    // Memory snapshot before
    result.memory_before = getMemoryUsage();

    // Test configurations
    vector<double> sampleRates = {44100, 48000, 96000, 192000};
    vector<int> bufferSizes = {64, 128, 256, 512, 1024, 2048};

    double worst_cpu = 0.0;

    for (double sr : sampleRates) {
        for (int bs : bufferSizes) {
            cout << "  Testing " << sr/1000 << "kHz, " << bs << " samples... ";

            // Prepare engine
            engine->prepareToPlay(sr, bs);
            engine->reset();

            // Measure CPU
            auto cpu = measureCPU(engine.get(), bs, sr);
            result.cpu_results[{sr, bs}] = cpu;

            worst_cpu = max(worst_cpu, cpu.cpu_percent);

            cout << fixed << setprecision(2) << cpu.cpu_percent << "% CPU" << endl;
        }
    }

    result.worst_case_cpu = worst_cpu;

    // Measure latency at 48kHz, 512 buffer
    cout << "  Measuring latency... ";
    engine->prepareToPlay(48000, 512);
    engine->reset();
    result.latency = measureLatency(engine.get(), 48000, 512);
    cout << result.latency.latency_ms << " ms" << endl;

    // Memory snapshot after
    result.memory_after = getMemoryUsage();
    result.memory_delta_kb = result.memory_after.rss_kb - result.memory_before.rss_kb;

    // Real-time capability check: < 5% CPU at 48kHz, 512 buffer
    auto key = make_pair(48000.0, 512);
    result.real_time_capable = (result.cpu_results[key].cpu_percent < 5.0);

    results.push_back(result);
}

void testPolyphony(int engineID, int numInstances, double& totalCPU, size_t& totalMemory) {
    cout << "\n  Testing " << numInstances << " simultaneous instances... ";

    vector<unique_ptr<EngineBase>> instances;
    for (int i = 0; i < numInstances; ++i) {
        instances.push_back(createEngine(engineID));
        instances.back()->prepareToPlay(48000, 512);
        instances.back()->reset();
    }

    // Measure memory
    auto mem_start = getMemoryUsage();

    // Measure CPU
    const int bufferSize = 512;
    const int numIterations = 100;

    juce::AudioBuffer<float> buffer(2, bufferSize);

    // Generate test signal
    for (int ch = 0; ch < 2; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < bufferSize; ++i) {
            data[i] = 0.5f * sin(2.0 * M_PI * 440.0 * i / 48000.0);
        }
    }

    // Measure processing time
    auto start = high_resolution_clock::now();

    for (int iter = 0; iter < numIterations; ++iter) {
        for (auto& engine : instances) {
            engine->process(buffer);
        }
    }

    auto end = high_resolution_clock::now();

    double elapsed_us = duration_cast<nanoseconds>(end - start).count() / 1000.0;
    double mean_us = elapsed_us / numIterations;
    double real_time_us = (bufferSize / 48000.0) * 1e6;
    totalCPU = (mean_us / real_time_us) * 100.0;

    auto mem_end = getMemoryUsage();
    totalMemory = mem_end.rss_kb - mem_start.rss_kb;

    cout << fixed << setprecision(2) << totalCPU << "% CPU, "
         << totalMemory << " KB RAM" << endl;
}

// ===========================
// REPORT GENERATION
// ===========================

void generateReport(const vector<PerformanceResult>& results) {
    ofstream report("/Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/PITCH_ENGINE_PERFORMANCE_PROFILING.md");

    report << "# PITCH ENGINE PERFORMANCE PROFILING REPORT\n\n";
    report << "**Generated**: " << __DATE__ << " " << __TIME__ << "\n";
    report << "**Test Suite**: test_pitch_engines_performance.cpp\n";
    report << "**Engines Tested**: 8 pitch/time-based engines (31-38)\n\n";

    report << "---\n\n";

    // Executive Summary
    report << "## EXECUTIVE SUMMARY\n\n";

    int real_time_capable = 0;
    for (const auto& r : results) {
        if (r.real_time_capable) real_time_capable++;
    }

    report << "**Real-Time Capable**: " << real_time_capable << "/8 engines ("
           << fixed << setprecision(1) << (real_time_capable * 100.0 / 8) << "%)\n\n";

    report << "**Performance Targets**:\n";
    report << "- CPU: < 5% per engine (48kHz, 512 buffer)\n";
    report << "- Memory: < 5 MB per engine\n";
    report << "- Latency: < 100ms total\n";
    report << "- Real-time safe: No audio thread allocations\n\n";

    // CPU Usage Summary Table
    report << "## CPU USAGE SUMMARY\n\n";
    report << "**Test Condition**: 48kHz, 512 sample buffer\n\n";
    report << "| Engine | Name | CPU % | Real-Time | Status |\n";
    report << "|--------|------|-------|-----------|--------|\n";

    for (const auto& r : results) {
        auto key = make_pair(48000.0, 512);
        auto cpu = r.cpu_results.at(key);

        string status = cpu.cpu_percent < 2.0 ? "Excellent" :
                       cpu.cpu_percent < 5.0 ? "Good" :
                       cpu.cpu_percent < 10.0 ? "Fair" : "Poor";

        string rt = r.real_time_capable ? "YES" : "NO";

        report << "| " << r.info.id << " | " << r.info.name << " | "
               << fixed << setprecision(2) << cpu.cpu_percent << "% | "
               << rt << " | " << status << " |\n";
    }
    report << "\n";

    // Detailed CPU Analysis
    report << "## DETAILED CPU ANALYSIS\n\n";

    for (const auto& r : results) {
        report << "### Engine " << r.info.id << ": " << r.info.name << "\n\n";

        report << "#### CPU Usage Across Sample Rates and Buffer Sizes\n\n";
        report << "| Sample Rate | Buffer Size | CPU % | Mean (μs) | Max (μs) | Status |\n";
        report << "|-------------|-------------|-------|-----------|----------|--------|\n";

        vector<double> sampleRates = {44100, 48000, 96000, 192000};
        vector<int> bufferSizes = {64, 128, 256, 512, 1024, 2048};

        for (double sr : sampleRates) {
            for (int bs : bufferSizes) {
                auto key = make_pair(sr, bs);
                const auto& cpu = r.cpu_results.at(key);

                string status = cpu.cpu_percent < 5.0 ? "✓ PASS" : "✗ FAIL";

                report << "| " << sr/1000 << " kHz | " << bs << " | "
                       << fixed << setprecision(2) << cpu.cpu_percent << "% | "
                       << setprecision(1) << cpu.mean_us << " | "
                       << cpu.max_us << " | " << status << " |\n";
            }
        }

        report << "\n**Worst Case CPU**: " << fixed << setprecision(2)
               << r.worst_case_cpu << "%\n\n";

        // Real-time capability assessment
        report << "**Real-Time Capability**: ";
        if (r.real_time_capable) {
            report << "✓ YES - Can process in real-time at 48kHz\n\n";
        } else {
            auto key = make_pair(48000.0, 512);
            double cpu = r.cpu_results.at(key).cpu_percent;
            report << "✗ NO - Requires " << fixed << setprecision(1)
                   << cpu << "% CPU (target: < 5%)\n\n";
        }
    }

    // Memory Usage
    report << "## MEMORY USAGE ANALYSIS\n\n";
    report << "| Engine | Name | Memory (KB) | Per-Instance | Status |\n";
    report << "|--------|------|-------------|--------------|--------|\n";

    for (const auto& r : results) {
        string status = r.memory_delta_kb < 5120 ? "✓ PASS" : "✗ FAIL";
        double mb = r.memory_delta_kb / 1024.0;

        report << "| " << r.info.id << " | " << r.info.name << " | "
               << r.memory_delta_kb << " | "
               << fixed << setprecision(2) << mb << " MB | "
               << status << " |\n";
    }
    report << "\n";

    // Latency
    report << "## LATENCY MEASUREMENTS\n\n";
    report << "**Test Condition**: 48kHz sample rate\n\n";
    report << "| Engine | Name | Latency (samples) | Latency (ms) | Status |\n";
    report << "|--------|------|-------------------|--------------|--------|\n";

    for (const auto& r : results) {
        string status = r.latency.latency_ms < 100.0 ? "✓ PASS" : "✗ FAIL";

        report << "| " << r.info.id << " | " << r.info.name << " | "
               << r.latency.latency_samples << " | "
               << fixed << setprecision(2) << r.latency.latency_ms << " ms | "
               << status << " |\n";
    }
    report << "\n";

    // Performance Scaling
    report << "## PERFORMANCE SCALING (POLYPHONY)\n\n";
    report << "Testing simultaneous instances at 48kHz, 512 buffer:\n\n";

    for (const auto& r : results) {
        report << "### Engine " << r.info.id << ": " << r.info.name << "\n\n";
        report << "| Instances | Total CPU % | Total Memory (KB) |\n";
        report << "|-----------|-------------|-------------------|\n";

        for (int instances : {1, 2, 4, 8}) {
            double totalCPU;
            size_t totalMem;
            testPolyphony(r.info.id, instances, totalCPU, totalMem);

            report << "| " << instances << " | "
                   << fixed << setprecision(2) << totalCPU << "% | "
                   << totalMem << " |\n";
        }

        report << "\n";
    }

    // Efficiency Ranking
    report << "## EFFICIENCY RANKING\n\n";
    report << "Engines ranked by CPU efficiency (lower is better):\n\n";

    vector<pair<double, const PerformanceResult*>> ranking;
    for (const auto& r : results) {
        auto key = make_pair(48000.0, 512);
        ranking.push_back({r.cpu_results.at(key).cpu_percent, &r});
    }

    sort(ranking.begin(), ranking.end());

    report << "| Rank | Engine | Name | CPU % | Efficiency |\n";
    report << "|------|--------|------|-------|------------|\n";

    for (size_t i = 0; i < ranking.size(); ++i) {
        auto cpu = ranking[i].first;
        auto info = ranking[i].second->info;

        string efficiency = cpu < 2.0 ? "Excellent" :
                          cpu < 5.0 ? "Good" :
                          cpu < 10.0 ? "Fair" : "Poor";

        report << "| " << (i+1) << " | " << info.id << " | " << info.name << " | "
               << fixed << setprecision(2) << cpu << "% | " << efficiency << " |\n";
    }
    report << "\n";

    // Recommendations
    report << "## OPTIMIZATION RECOMMENDATIONS\n\n";

    for (const auto& r : results) {
        auto key = make_pair(48000.0, 512);
        double cpu = r.cpu_results.at(key).cpu_percent;

        if (cpu >= 5.0) {
            report << "### Engine " << r.info.id << ": " << r.info.name << "\n\n";
            report << "**Current CPU**: " << fixed << setprecision(2) << cpu << "%\n";
            report << "**Target CPU**: < 5%\n";
            report << "**Optimization Needed**: " << setprecision(1)
                   << ((cpu / 5.0) * 100) << "% faster required\n\n";

            report << "**Recommended Actions**:\n";

            if (r.info.category == "Pitch") {
                report << "- Profile FFT operations for optimization opportunities\n";
                report << "- Consider SIMD acceleration for resampling\n";
                report << "- Optimize phase unwrapping algorithms\n";
                report << "- Reduce window function complexity\n";
            } else {
                report << "- Profile delay line operations\n";
                report << "- Optimize interpolation algorithms\n";
                report << "- Consider circular buffer optimizations\n";
            }

            report << "\n";
        }
    }

    // Bottleneck Identification
    report << "## PERFORMANCE BOTTLENECK IDENTIFICATION\n\n";

    report << "### High CPU Usage Patterns\n\n";
    report << "Engines with > 5% CPU at 48kHz, 512 buffer:\n\n";

    for (const auto& r : results) {
        auto key = make_pair(48000.0, 512);
        double cpu = r.cpu_results.at(key).cpu_percent;

        if (cpu >= 5.0) {
            report << "- **Engine " << r.info.id << " (" << r.info.name << ")**: "
                   << fixed << setprecision(2) << cpu << "% CPU\n";
            report << "  - Likely bottlenecks: ";

            if (r.info.category == "Pitch") {
                report << "FFT operations, resampling, phase processing\n";
            } else {
                report << "Delay line access, interpolation, modulation\n";
            }
        }
    }

    report << "\n### Sample Rate Scaling Issues\n\n";

    for (const auto& r : results) {
        auto cpu_44k = r.cpu_results.at({44100.0, 512}).cpu_percent;
        auto cpu_192k = r.cpu_results.at({192000.0, 512}).cpu_percent;
        double scaling_factor = cpu_192k / cpu_44k;

        if (scaling_factor > 4.5) {
            report << "- **Engine " << r.info.id << " (" << r.info.name << ")**: "
                   << fixed << setprecision(1) << scaling_factor
                   << "x CPU increase from 44.1k to 192k\n";
            report << "  - Expected: ~4x, Actual: " << scaling_factor
                   << "x (indicates sample-rate-dependent bottleneck)\n";
        }
    }

    report << "\n---\n\n";

    // Final Summary
    report << "## FINAL ASSESSMENT\n\n";

    int excellent = 0, good = 0, fair = 0, poor = 0;

    for (const auto& r : results) {
        auto key = make_pair(48000.0, 512);
        double cpu = r.cpu_results.at(key).cpu_percent;

        if (cpu < 2.0) excellent++;
        else if (cpu < 5.0) good++;
        else if (cpu < 10.0) fair++;
        else poor++;
    }

    report << "**Performance Distribution**:\n";
    report << "- Excellent (< 2% CPU): " << excellent << "/8 engines\n";
    report << "- Good (2-5% CPU): " << good << "/8 engines\n";
    report << "- Fair (5-10% CPU): " << fair << "/8 engines\n";
    report << "- Poor (> 10% CPU): " << poor << "/8 engines\n\n";

    if (real_time_capable >= 6) {
        report << "✓ **READY FOR PRODUCTION**: Majority of engines meet real-time performance targets.\n\n";
    } else if (real_time_capable >= 4) {
        report << "⚠ **NEEDS OPTIMIZATION**: Some engines require performance improvements.\n\n";
    } else {
        report << "✗ **NOT PRODUCTION READY**: Significant performance optimization required.\n\n";
    }

    report.close();

    cout << "\n\nReport generated: PITCH_ENGINE_PERFORMANCE_PROFILING.md" << endl;
}

// ===========================
// MAIN
// ===========================

int main() {
    cout << "====================================================\n";
    cout << "PITCH ENGINE PERFORMANCE PROFILER\n";
    cout << "====================================================\n";
    cout << "Testing 8 pitch/time engines (31-38)\n";
    cout << "Measuring CPU, memory, latency, and efficiency\n\n";

    vector<PerformanceResult> results;

    auto engines = getPitchEngines();
    for (const auto& engine : engines) {
        testSingleEngine(engine.id, results);
    }

    cout << "\n\nGenerating comprehensive report...\n";
    generateReport(results);

    cout << "\n====================================================\n";
    cout << "PERFORMANCE PROFILING COMPLETE\n";
    cout << "====================================================\n\n";

    // Print summary
    cout << "QUICK SUMMARY:\n\n";

    int real_time = 0;
    for (const auto& r : results) {
        if (r.real_time_capable) real_time++;

        auto key = make_pair(48000.0, 512);
        double cpu = r.cpu_results.at(key).cpu_percent;

        cout << "Engine " << r.info.id << " (" << r.info.name << "): "
             << fixed << setprecision(2) << cpu << "% CPU, "
             << r.latency.latency_ms << " ms latency, "
             << (r.memory_delta_kb / 1024.0) << " MB RAM - "
             << (r.real_time_capable ? "REAL-TIME" : "TOO SLOW") << endl;
    }

    cout << "\nReal-time capable: " << real_time << "/8 engines" << endl;

    return 0;
}
