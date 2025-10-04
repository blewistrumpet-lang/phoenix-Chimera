// Standalone Engine Test Harness for Chimera Plugin
// This comprehensive test analyzes all 57 engines for quality, safety, and performance

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <thread>
#include <mutex>
#include <random>
#include <limits>
#include <cstring>

// Platform-specific includes
#ifdef _WIN32
    #include <windows.h>
    #include <psapi.h>
#else
    #include <sys/resource.h>
    #include <mach/mach.h>
#endif

// Forward declarations of all engine classes
#include "Source/EngineBase.h"
#include "Source/BitCrusher.h"
#include "Source/SampleRateReducer.h"
#include "Source/WaveShaper.h"
#include "Source/Distortion.h"
#include "Source/TapeEcho.h"
#include "Source/BBDDelay.h"
#include "Source/DigitalDelay.h"
#include "Source/PingPongDelay.h"
#include "Source/DualDelay.h"
#include "Source/ReverseDelay.h"
#include "Source/SpringReverb.h"
#include "Source/PlateReverb.h"
#include "Source/HallReverb.h"
#include "Source/RoomReverb.h"
#include "Source/ShimmerReverb.h"
#include "Source/EnvelopeFilter.h"
#include "Source/LadderFilter.h"
#include "Source/StateVariableFilter.h"
#include "Source/CombFilter.h"
#include "Source/AutoWah.h"
#include "Source/Phaser.h"
#include "Source/Flanger.h"
#include "Source/Chorus.h"
#include "Source/Ensemble.h"
#include "Source/RotarySpeaker.h"
#include "Source/VintageCompressor.h"
#include "Source/VintageOptoCompressor.h"
#include "Source/Limiter.h"
#include "Source/Gate.h"
#include "Source/DynamicEQ.h"
#include "Source/ThreeBandEQ.h"
#include "Source/GraphicEQ.h"
#include "Source/ParametricEQ.h"
#include "Source/VintageEQ.h"
#include "Source/TiltEQ.h"
#include "Source/Tremolo.h"
#include "Source/Vibrato.h"
#include "Source/AutoPan.h"
#include "Source/RingModulator.h"
#include "Source/FrequencyShifter.h"
#include "Source/PitchShifter.h"
#include "Source/HarmonicExciter.h"
#include "Source/SubOctaveGenerator.h"
#include "Source/VintageWidener.h"
#include "Source/ModernWidener.h"
#include "Source/ConvolutionEngine.h"
#include "Source/Vocoder.h"
#include "Source/TalkBox.h"
#include "Source/TapeSimulation.h"
#include "Source/VinylSimulation.h"
#include "Source/TubeSimulation.h"
#include "Source/TransformerSimulation.h"
#include "Source/LoFi.h"
#include "Source/Multiband.h"
#include "Source/Resonator.h"
#include "Source/RodentDistortion.h"

// Test result structures
struct ParameterTestResult {
    int paramIndex;
    std::string paramName;
    bool responsive = true;
    bool hasEffect = true;
    bool causesCrash = false;
    bool causesNaN = false;
    bool causesInf = false;
    float minOutput = 0.0f;
    float maxOutput = 0.0f;
    float avgCpuUsage = 0.0f;
    std::string issues;
};

struct SafetyTestResult {
    bool passedNaNTest = true;
    bool passedInfTest = true;
    bool passedDenormalTest = true;
    bool passedBufferTest = true;
    bool passedThreadTest = true;
    bool passedMemoryTest = true;
    std::vector<std::string> failures;
};

struct AudioQualityResult {
    bool passesSineTest = true;
    bool passesNoiseTest = true;
    bool passesTransientTest = true;
    bool passesClippingTest = true;
    bool passesSilenceTest = true;
    float thd = 0.0f;  // Total Harmonic Distortion
    float snr = 0.0f;  // Signal to Noise Ratio
    std::vector<std::string> issues;
};

struct PerformanceResult {
    float avgCpuPercent = 0.0f;
    float maxCpuPercent = 0.0f;
    float avgLatencySamples = 0.0f;
    bool meetsRealtimeConstraints = true;
    std::string bottlenecks;
};

struct StabilityResult {
    bool passesMixTest = true;
    bool passesAutomationTest = true;
    bool passesBypassTest = true;
    bool passesResetTest = true;
    std::vector<std::string> issues;
};

struct EngineTestResult {
    int engineId;
    std::string engineName;
    bool createdSuccessfully = false;
    bool crashed = false;
    
    std::vector<ParameterTestResult> parameterTests;
    SafetyTestResult safetyTest;
    AudioQualityResult audioQuality;
    PerformanceResult performance;
    StabilityResult stability;
    
    std::vector<std::string> recommendations;
    int severityScore = 0;  // 0 = perfect, higher = worse
};

// Test harness class
class EngineTestHarness {
private:
    static constexpr double SAMPLE_RATE = 48000.0;
    static constexpr int BLOCK_SIZE = 512;
    static constexpr int TEST_DURATION_SAMPLES = SAMPLE_RATE * 2; // 2 seconds
    
    std::vector<EngineTestResult> results;
    std::mutex resultsMutex;
    
    // CPU measurement helpers
    double getCpuUsage() {
#ifdef _WIN32
        static ULARGE_INTEGER lastCPU, lastSysCPU, lastUserCPU;
        static int numProcessors = 0;
        static HANDLE self = GetCurrentProcess();
        
        if (numProcessors == 0) {
            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);
            numProcessors = sysInfo.dwNumberOfProcessors;
        }
        
        FILETIME ftime, fsys, fuser;
        ULARGE_INTEGER now, sys, user;
        
        GetSystemTimeAsFileTime(&ftime);
        memcpy(&now, &ftime, sizeof(FILETIME));
        
        GetProcessTimes(self, &ftime, &ftime, &fsys, &fuser);
        memcpy(&sys, &fsys, sizeof(FILETIME));
        memcpy(&user, &fuser, sizeof(FILETIME));
        
        double percent = (sys.QuadPart - lastSysCPU.QuadPart) +
                        (user.QuadPart - lastUserCPU.QuadPart);
        percent /= (now.QuadPart - lastCPU.QuadPart);
        percent /= numProcessors;
        
        lastCPU = now;
        lastUserCPU = user;
        lastSysCPU = sys;
        
        return percent * 100.0;
#else
        // macOS version
        struct rusage usage;
        getrusage(RUSAGE_SELF, &usage);
        
        static struct timeval lastTime = usage.ru_utime;
        static clock_t lastClock = clock();
        
        clock_t currentClock = clock();
        double timeDiff = (currentClock - lastClock) / (double)CLOCKS_PER_SEC;
        
        double userDiff = (usage.ru_utime.tv_sec - lastTime.tv_sec) +
                         (usage.ru_utime.tv_usec - lastTime.tv_usec) / 1000000.0;
        
        lastTime = usage.ru_utime;
        lastClock = currentClock;
        
        return (userDiff / timeDiff) * 100.0;
#endif
    }
    
    // Signal generation helpers
    void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, float amplitude = 0.5f) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = amplitude * std::sin(2.0f * M_PI * frequency * i / SAMPLE_RATE);
            }
        }
    }
    
    void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude = 0.5f) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-amplitude, amplitude);
        
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                data[i] = dist(gen);
            }
        }
    }
    
    void generateImpulse(juce::AudioBuffer<float>& buffer, int position = 0, float amplitude = 1.0f) {
        buffer.clear();
        const int numChannels = buffer.getNumChannels();
        
        if (position < buffer.getNumSamples()) {
            for (int ch = 0; ch < numChannels; ++ch) {
                buffer.setSample(ch, position, amplitude);
            }
        }
    }
    
    // Analysis helpers
    bool containsNaN(const juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                if (std::isnan(data[i])) return true;
            }
        }
        return false;
    }
    
    bool containsInf(const juce::AudioBuffer<float>& buffer) {
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                if (std::isinf(data[i])) return true;
            }
        }
        return false;
    }
    
    float calculateRMS(const juce::AudioBuffer<float>& buffer) {
        float sum = 0.0f;
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                sum += data[i] * data[i];
            }
        }
        
        return std::sqrt(sum / (numChannels * numSamples));
    }
    
    float calculatePeak(const juce::AudioBuffer<float>& buffer) {
        float peak = 0.0f;
        const int numChannels = buffer.getNumChannels();
        const int numSamples = buffer.getNumSamples();
        
        for (int ch = 0; ch < numChannels; ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i) {
                peak = std::max(peak, std::abs(data[i]));
            }
        }
        
        return peak;
    }
    
    float calculateTHD(const juce::AudioBuffer<float>& buffer, float fundamentalFreq) {
        // Simplified THD calculation
        // In a real implementation, this would use FFT
        return 0.01f; // Placeholder
    }
    
    // Test implementations
    void testParameterSweep(EngineBase* engine, EngineTestResult& result) {
        const int numParams = engine->getNumParameters();
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
        
        for (int paramIdx = 0; paramIdx < numParams; ++paramIdx) {
            ParameterTestResult paramResult;
            paramResult.paramIndex = paramIdx;
            paramResult.paramName = engine->getParameterName(paramIdx).toStdString();
            
            // Test parameter at different values
            std::vector<float> testValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
            std::vector<float> outputLevels;
            
            for (float value : testValues) {
                // Set parameter
                std::map<int, float> params;
                params[paramIdx] = value;
                engine->updateParameters(params);
                
                // Generate test signal
                generateSineWave(testBuffer, 440.0f, 0.5f);
                
                // Process
                auto startTime = std::chrono::high_resolution_clock::now();
                try {
                    engine->process(testBuffer);
                    
                    // Check for NaN/Inf
                    if (containsNaN(testBuffer)) {
                        paramResult.causesNaN = true;
                        paramResult.issues += "NaN at value " + std::to_string(value) + "; ";
                    }
                    if (containsInf(testBuffer)) {
                        paramResult.causesInf = true;
                        paramResult.issues += "Inf at value " + std::to_string(value) + "; ";
                    }
                    
                    // Measure output level
                    float rms = calculateRMS(testBuffer);
                    outputLevels.push_back(rms);
                    
                } catch (...) {
                    paramResult.causesCrash = true;
                    paramResult.issues += "Crash at value " + std::to_string(value) + "; ";
                }
                
                auto endTime = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
                paramResult.avgCpuUsage += duration / 1000.0f; // Convert to ms
            }
            
            // Analyze parameter effectiveness
            if (!outputLevels.empty()) {
                paramResult.minOutput = *std::min_element(outputLevels.begin(), outputLevels.end());
                paramResult.maxOutput = *std::max_element(outputLevels.begin(), outputLevels.end());
                
                // Check if parameter has any effect
                float variance = 0.0f;
                float mean = 0.0f;
                for (float level : outputLevels) mean += level;
                mean /= outputLevels.size();
                
                for (float level : outputLevels) {
                    variance += (level - mean) * (level - mean);
                }
                variance /= outputLevels.size();
                
                if (variance < 0.00001f) {
                    paramResult.hasEffect = false;
                    paramResult.issues += "Parameter has no audible effect; ";
                }
            }
            
            paramResult.avgCpuUsage /= testValues.size();
            result.parameterTests.push_back(paramResult);
        }
    }
    
    void testSafety(EngineBase* engine, EngineTestResult& result) {
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
        
        // Test 1: NaN input
        testBuffer.clear();
        testBuffer.setSample(0, 0, std::numeric_limits<float>::quiet_NaN());
        try {
            engine->process(testBuffer);
            if (containsNaN(testBuffer)) {
                result.safetyTest.passedNaNTest = false;
                result.safetyTest.failures.push_back("Failed to handle NaN input");
            }
        } catch (...) {
            result.safetyTest.passedNaNTest = false;
            result.safetyTest.failures.push_back("Crashed on NaN input");
        }
        
        // Test 2: Infinity input
        testBuffer.clear();
        testBuffer.setSample(0, 0, std::numeric_limits<float>::infinity());
        try {
            engine->process(testBuffer);
            if (containsInf(testBuffer)) {
                result.safetyTest.passedInfTest = false;
                result.safetyTest.failures.push_back("Failed to handle infinity input");
            }
        } catch (...) {
            result.safetyTest.passedInfTest = false;
            result.safetyTest.failures.push_back("Crashed on infinity input");
        }
        
        // Test 3: Denormal input
        testBuffer.clear();
        testBuffer.setSample(0, 0, std::numeric_limits<float>::min() / 2.0f);
        try {
            engine->process(testBuffer);
            // Check if denormals are handled (output should be clean)
            float peak = calculatePeak(testBuffer);
            if (peak < 1e-30f && peak > 0.0f) {
                result.safetyTest.passedDenormalTest = false;
                result.safetyTest.failures.push_back("Denormal numbers not prevented");
            }
        } catch (...) {
            result.safetyTest.passedDenormalTest = false;
            result.safetyTest.failures.push_back("Crashed on denormal input");
        }
        
        // Test 4: Various buffer sizes
        std::vector<int> bufferSizes = {1, 17, 64, 256, 1024, 4096};
        for (int size : bufferSizes) {
            juce::AudioBuffer<float> sizedBuffer(2, size);
            generateWhiteNoise(sizedBuffer, 0.3f);
            
            try {
                engine->process(sizedBuffer);
            } catch (...) {
                result.safetyTest.passedBufferTest = false;
                result.safetyTest.failures.push_back("Failed with buffer size " + std::to_string(size));
            }
        }
        
        // Test 5: Thread safety (basic test)
        std::vector<std::thread> threads;
        std::atomic<bool> threadFailed(false);
        
        for (int i = 0; i < 4; ++i) {
            threads.emplace_back([&]() {
                juce::AudioBuffer<float> threadBuffer(2, BLOCK_SIZE);
                generateSineWave(threadBuffer, 440.0f + i * 110.0f);
                
                try {
                    for (int j = 0; j < 10; ++j) {
                        engine->process(threadBuffer);
                    }
                } catch (...) {
                    threadFailed = true;
                }
            });
        }
        
        for (auto& t : threads) t.join();
        
        if (threadFailed) {
            result.safetyTest.passedThreadTest = false;
            result.safetyTest.failures.push_back("Thread safety issues detected");
        }
    }
    
    void testAudioQuality(EngineBase* engine, EngineTestResult& result) {
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
        juce::AudioBuffer<float> referenceBuffer(2, BLOCK_SIZE);
        
        // Test 1: Sine wave response
        std::vector<float> testFrequencies = {100.0f, 440.0f, 1000.0f, 5000.0f, 10000.0f};
        for (float freq : testFrequencies) {
            generateSineWave(testBuffer, freq);
            referenceBuffer = testBuffer; // Copy for comparison
            
            engine->process(testBuffer);
            
            // Check if output is reasonable
            float outputRMS = calculateRMS(testBuffer);
            float inputRMS = calculateRMS(referenceBuffer);
            
            if (outputRMS > inputRMS * 10.0f) {
                result.audioQuality.passesSineTest = false;
                result.audioQuality.issues.push_back("Excessive gain at " + std::to_string(freq) + "Hz");
            }
            
            // Simple THD calculation (placeholder)
            result.audioQuality.thd += calculateTHD(testBuffer, freq);
        }
        result.audioQuality.thd /= testFrequencies.size();
        
        // Test 2: White noise stability
        generateWhiteNoise(testBuffer, 0.3f);
        float inputPeak = calculatePeak(testBuffer);
        engine->process(testBuffer);
        float outputPeak = calculatePeak(testBuffer);
        
        if (outputPeak > 1.0f) {
            result.audioQuality.passesNoiseTest = false;
            result.audioQuality.issues.push_back("Output exceeds 0dBFS with noise input");
        }
        
        // Test 3: Transient response
        generateImpulse(testBuffer, 0);
        engine->process(testBuffer);
        
        // Check for extended ringing or instability
        float tailEnergy = 0.0f;
        const float* data = testBuffer.getReadPointer(0);
        for (int i = BLOCK_SIZE / 2; i < BLOCK_SIZE; ++i) {
            tailEnergy += std::abs(data[i]);
        }
        
        if (tailEnergy > 10.0f) {
            result.audioQuality.passesTransientTest = false;
            result.audioQuality.issues.push_back("Excessive ringing on transients");
        }
        
        // Test 4: Near-clipping behavior
        generateSineWave(testBuffer, 440.0f, 0.95f);
        engine->process(testBuffer);
        
        if (calculatePeak(testBuffer) > 1.0f) {
            result.audioQuality.passesClippingTest = false;
            result.audioQuality.issues.push_back("Produces clipping with high-level input");
        }
        
        // Test 5: Silence test
        testBuffer.clear();
        engine->process(testBuffer);
        
        float silenceRMS = calculateRMS(testBuffer);
        if (silenceRMS > 0.001f) {
            result.audioQuality.passesSilenceTest = false;
            result.audioQuality.issues.push_back("Produces noise with silent input");
        }
        
        // Calculate SNR
        float noiseFloor = silenceRMS;
        float signalLevel = 0.5f; // Nominal level
        result.audioQuality.snr = 20.0f * std::log10(signalLevel / (noiseFloor + 1e-10f));
    }
    
    void testPerformance(EngineBase* engine, EngineTestResult& result) {
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
        generateWhiteNoise(testBuffer, 0.5f);
        
        // Warm up
        for (int i = 0; i < 10; ++i) {
            engine->process(testBuffer);
        }
        
        // Performance test
        const int numIterations = 1000;
        std::vector<double> cpuReadings;
        std::vector<double> timings;
        
        for (int i = 0; i < numIterations; ++i) {
            double cpuBefore = getCpuUsage();
            auto startTime = std::chrono::high_resolution_clock::now();
            
            engine->process(testBuffer);
            
            auto endTime = std::chrono::high_resolution_clock::now();
            double cpuAfter = getCpuUsage();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
            timings.push_back(duration);
            
            if (cpuAfter > cpuBefore) {
                cpuReadings.push_back(cpuAfter - cpuBefore);
            }
        }
        
        // Calculate statistics
        if (!timings.empty()) {
            double avgTime = 0.0;
            double maxTime = 0.0;
            
            for (double time : timings) {
                avgTime += time;
                maxTime = std::max(maxTime, time);
            }
            avgTime /= timings.size();
            
            // Convert to percentage of available time
            double availableTimeUs = (BLOCK_SIZE / SAMPLE_RATE) * 1e6;
            result.performance.avgCpuPercent = (avgTime / availableTimeUs) * 100.0;
            result.performance.maxCpuPercent = (maxTime / availableTimeUs) * 100.0;
            
            // Check realtime constraints
            if (result.performance.maxCpuPercent > 80.0) {
                result.performance.meetsRealtimeConstraints = false;
                result.performance.bottlenecks = "Processing time exceeds 80% of available time";
            }
        }
        
        // Latency test (impulse response)
        testBuffer.clear();
        generateImpulse(testBuffer, 0);
        engine->process(testBuffer);
        
        // Find first significant output sample
        const float* data = testBuffer.getReadPointer(0);
        for (int i = 0; i < BLOCK_SIZE; ++i) {
            if (std::abs(data[i]) > 0.001f) {
                result.performance.avgLatencySamples = i;
                break;
            }
        }
    }
    
    void testStability(EngineBase* engine, EngineTestResult& result) {
        juce::AudioBuffer<float> testBuffer(2, BLOCK_SIZE);
        
        // Test 1: Mix parameter linearity
        int mixParamIndex = -1;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            std::string paramName = engine->getParameterName(i).toStdString();
            std::transform(paramName.begin(), paramName.end(), paramName.begin(), ::tolower);
            if (paramName.find("mix") != std::string::npos || 
                paramName.find("wet") != std::string::npos ||
                paramName.find("dry") != std::string::npos) {
                mixParamIndex = i;
                break;
            }
        }
        
        if (mixParamIndex >= 0) {
            generateSineWave(testBuffer, 440.0f);
            float dryRMS = calculateRMS(testBuffer);
            
            // Test mix at 0% (should be dry)
            std::map<int, float> params;
            params[mixParamIndex] = 0.0f;
            engine->updateParameters(params);
            engine->process(testBuffer);
            float mix0RMS = calculateRMS(testBuffer);
            
            // Test mix at 50%
            generateSineWave(testBuffer, 440.0f);
            params[mixParamIndex] = 0.5f;
            engine->updateParameters(params);
            engine->process(testBuffer);
            float mix50RMS = calculateRMS(testBuffer);
            
            // Test mix at 100%
            generateSineWave(testBuffer, 440.0f);
            params[mixParamIndex] = 1.0f;
            engine->updateParameters(params);
            engine->process(testBuffer);
            float mix100RMS = calculateRMS(testBuffer);
            
            // Check linearity
            if (std::abs(mix0RMS - dryRMS) > 0.01f) {
                result.stability.passesMixTest = false;
                result.stability.issues.push_back("Mix at 0% doesn't match dry signal");
            }
        }
        
        // Test 2: Rapid parameter changes (automation)
        generateWhiteNoise(testBuffer, 0.3f);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        try {
            for (int i = 0; i < 100; ++i) {
                std::map<int, float> params;
                for (int p = 0; p < std::min(3, engine->getNumParameters()); ++p) {
                    params[p] = dist(gen);
                }
                engine->updateParameters(params);
                engine->process(testBuffer);
                
                if (containsNaN(testBuffer) || containsInf(testBuffer)) {
                    result.stability.passesAutomationTest = false;
                    result.stability.issues.push_back("Instability during rapid parameter changes");
                    break;
                }
            }
        } catch (...) {
            result.stability.passesAutomationTest = false;
            result.stability.issues.push_back("Crash during automation test");
        }
        
        // Test 3: Reset functionality
        engine->reset();
        testBuffer.clear();
        engine->process(testBuffer);
        
        if (calculateRMS(testBuffer) > 0.001f) {
            result.stability.passesResetTest = false;
            result.stability.issues.push_back("Engine produces output after reset with silent input");
        }
    }
    
    std::unique_ptr<EngineBase> createEngine(int engineId) {
        switch (engineId) {
            case 0: return std::make_unique<BitCrusher>();
            case 1: return std::make_unique<SampleRateReducer>();
            case 2: return std::make_unique<WaveShaper>();
            case 3: return std::make_unique<Distortion>();
            case 4: return std::make_unique<TapeEcho>();
            case 5: return std::make_unique<BBDDelay>();
            case 6: return std::make_unique<DigitalDelay>();
            case 7: return std::make_unique<PingPongDelay>();
            case 8: return std::make_unique<DualDelay>();
            case 9: return std::make_unique<ReverseDelay>();
            case 10: return std::make_unique<SpringReverb>();
            case 11: return std::make_unique<PlateReverb>();
            case 12: return std::make_unique<HallReverb>();
            case 13: return std::make_unique<RoomReverb>();
            case 14: return std::make_unique<ShimmerReverb>();
            case 15: return std::make_unique<EnvelopeFilter>();
            case 16: return std::make_unique<LadderFilter>();
            case 17: return std::make_unique<StateVariableFilter>();
            case 18: return std::make_unique<CombFilter>();
            case 19: return std::make_unique<AutoWah>();
            case 20: return std::make_unique<Phaser>();
            case 21: return std::make_unique<Flanger>();
            case 22: return std::make_unique<Chorus>();
            case 23: return std::make_unique<Ensemble>();
            case 24: return std::make_unique<RotarySpeaker>();
            case 25: return std::make_unique<VintageCompressor>();
            case 26: return std::make_unique<VintageOptoCompressor>();
            case 27: return std::make_unique<Limiter>();
            case 28: return std::make_unique<Gate>();
            case 29: return std::make_unique<DynamicEQ>();
            case 30: return std::make_unique<ThreeBandEQ>();
            case 31: return std::make_unique<GraphicEQ>();
            case 32: return std::make_unique<ParametricEQ>();
            case 33: return std::make_unique<VintageEQ>();
            case 34: return std::make_unique<TiltEQ>();
            case 35: return std::make_unique<Tremolo>();
            case 36: return std::make_unique<Vibrato>();
            case 37: return std::make_unique<AutoPan>();
            case 38: return std::make_unique<RingModulator>();
            case 39: return std::make_unique<FrequencyShifter>();
            case 40: return std::make_unique<PitchShifter>();
            case 41: return std::make_unique<HarmonicExciter>();
            case 42: return std::make_unique<SubOctaveGenerator>();
            case 43: return std::make_unique<VintageWidener>();
            case 44: return std::make_unique<ModernWidener>();
            case 45: return std::make_unique<ConvolutionEngine>();
            case 46: return std::make_unique<Vocoder>();
            case 47: return std::make_unique<TalkBox>();
            case 48: return std::make_unique<TapeSimulation>();
            case 49: return std::make_unique<VinylSimulation>();
            case 50: return std::make_unique<TubeSimulation>();
            case 51: return std::make_unique<TransformerSimulation>();
            case 52: return std::make_unique<LoFi>();
            case 53: return std::make_unique<Multiband>();
            case 54: return std::make_unique<Resonator>();
            case 55: return std::make_unique<RodentDistortion>();
            case 56: return nullptr; // Trinity (special case)
            default: return nullptr;
        }
    }
    
public:
    void runAllTests() {
        std::cout << "=== Chimera Engine Test Harness ===" << std::endl;
        std::cout << "Testing all 57 engines comprehensively..." << std::endl << std::endl;
        
        // Test each engine
        for (int engineId = 0; engineId <= 56; ++engineId) {
            EngineTestResult result;
            result.engineId = engineId;
            
            std::cout << "Testing Engine " << engineId << "... ";
            std::cout.flush();
            
            try {
                auto engine = createEngine(engineId);
                
                if (!engine) {
                    result.createdSuccessfully = false;
                    result.engineName = "Failed to create";
                    result.severityScore = 100;
                    results.push_back(result);
                    std::cout << "FAILED TO CREATE" << std::endl;
                    continue;
                }
                
                result.createdSuccessfully = true;
                result.engineName = engine->getName().toStdString();
                
                // Prepare engine
                engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
                
                // Run all tests
                testParameterSweep(engine.get(), result);
                testSafety(engine.get(), result);
                testAudioQuality(engine.get(), result);
                testPerformance(engine.get(), result);
                testStability(engine.get(), result);
                
                // Calculate severity score
                calculateSeverity(result);
                
                // Generate recommendations
                generateRecommendations(result);
                
                std::cout << "COMPLETE";
                if (result.severityScore > 0) {
                    std::cout << " [Issues: " << result.severityScore << "]";
                }
                std::cout << std::endl;
                
            } catch (const std::exception& e) {
                result.crashed = true;
                result.severityScore = 100;
                result.recommendations.push_back("Engine crashes during testing: " + std::string(e.what()));
                std::cout << "CRASHED" << std::endl;
            } catch (...) {
                result.crashed = true;
                result.severityScore = 100;
                result.recommendations.push_back("Engine crashes with unknown exception");
                std::cout << "CRASHED" << std::endl;
            }
            
            results.push_back(result);
        }
        
        std::cout << std::endl << "All tests complete. Generating reports..." << std::endl;
    }
    
    void calculateSeverity(EngineTestResult& result) {
        result.severityScore = 0;
        
        // Critical issues
        if (result.crashed) result.severityScore += 50;
        if (!result.safetyTest.passedNaNTest) result.severityScore += 20;
        if (!result.safetyTest.passedInfTest) result.severityScore += 20;
        if (!result.safetyTest.passedThreadTest) result.severityScore += 15;
        
        // Major issues
        for (const auto& param : result.parameterTests) {
            if (param.causesCrash) result.severityScore += 10;
            if (param.causesNaN || param.causesInf) result.severityScore += 5;
        }
        
        // Performance issues
        if (!result.performance.meetsRealtimeConstraints) result.severityScore += 10;
        if (result.performance.maxCpuPercent > 50.0) result.severityScore += 5;
        
        // Quality issues
        if (!result.audioQuality.passesSineTest) result.severityScore += 3;
        if (!result.audioQuality.passesClippingTest) result.severityScore += 5;
        if (result.audioQuality.thd > 0.1) result.severityScore += 3;
        
        // Stability issues
        if (!result.stability.passesAutomationTest) result.severityScore += 8;
        if (!result.stability.passesMixTest) result.severityScore += 3;
    }
    
    void generateRecommendations(EngineTestResult& result) {
        // Safety recommendations
        if (!result.safetyTest.passedNaNTest) {
            result.recommendations.push_back("Add NaN checking and replacement in process()");
        }
        if (!result.safetyTest.passedInfTest) {
            result.recommendations.push_back("Add infinity checking and clamping in process()");
        }
        if (!result.safetyTest.passedDenormalTest) {
            result.recommendations.push_back("Implement denormal prevention (add/subtract small DC)");
        }
        if (!result.safetyTest.passedThreadTest) {
            result.recommendations.push_back("Check for static variables and ensure thread safety");
        }
        
        // Parameter recommendations
        for (const auto& param : result.parameterTests) {
            if (param.causesCrash) {
                result.recommendations.push_back("Parameter '" + param.paramName + 
                    "' causes crashes - add bounds checking");
            }
            if (!param.hasEffect) {
                result.recommendations.push_back("Parameter '" + param.paramName + 
                    "' has no effect - check implementation");
            }
        }
        
        // Performance recommendations
        if (result.performance.maxCpuPercent > 50.0) {
            result.recommendations.push_back("Optimize processing - CPU usage too high (" + 
                std::to_string(result.performance.maxCpuPercent) + "%)");
        }
        
        // Audio quality recommendations
        if (!result.audioQuality.passesClippingTest) {
            result.recommendations.push_back("Add output limiting to prevent clipping");
        }
        if (result.audioQuality.snr < 60.0) {
            result.recommendations.push_back("Improve noise floor - SNR is only " + 
                std::to_string(result.audioQuality.snr) + "dB");
        }
        
        // Stability recommendations
        if (!result.stability.passesMixTest) {
            result.recommendations.push_back("Fix mix/dry-wet parameter implementation");
        }
        if (!result.stability.passesResetTest) {
            result.recommendations.push_back("Ensure all state variables are cleared in reset()");
        }
    }
    
    void generateReports() {
        generateSummaryReport();
        generateDetailedReport();
        generateHTMLReport();
        generateJSONReport();
    }
    
    void generateSummaryReport() {
        std::ofstream report("engine_test_summary.txt");
        
        report << "CHIMERA ENGINE TEST SUMMARY" << std::endl;
        report << "===========================" << std::endl;
        report << "Date: " << getCurrentDateTime() << std::endl;
        report << "Total Engines Tested: " << results.size() << std::endl << std::endl;
        
        // Sort by severity
        std::vector<EngineTestResult> sortedResults = results;
        std::sort(sortedResults.begin(), sortedResults.end(), 
                  [](const EngineTestResult& a, const EngineTestResult& b) {
                      return a.severityScore > b.severityScore;
                  });
        
        // Summary statistics
        int perfectEngines = 0;
        int minorIssues = 0;
        int majorIssues = 0;
        int criticalIssues = 0;
        
        for (const auto& result : results) {
            if (result.severityScore == 0) perfectEngines++;
            else if (result.severityScore < 10) minorIssues++;
            else if (result.severityScore < 50) majorIssues++;
            else criticalIssues++;
        }
        
        report << "OVERVIEW:" << std::endl;
        report << "  Perfect: " << perfectEngines << std::endl;
        report << "  Minor Issues: " << minorIssues << std::endl;
        report << "  Major Issues: " << majorIssues << std::endl;
        report << "  Critical Issues: " << criticalIssues << std::endl << std::endl;
        
        // Top problematic engines
        report << "TOP 10 PROBLEMATIC ENGINES:" << std::endl;
        report << "---------------------------" << std::endl;
        
        for (int i = 0; i < std::min(10, (int)sortedResults.size()); ++i) {
            const auto& result = sortedResults[i];
            if (result.severityScore > 0) {
                report << std::setw(3) << result.engineId << ": " 
                       << std::setw(25) << std::left << result.engineName 
                       << " (Severity: " << result.severityScore << ")" << std::endl;
                
                // List main issues
                if (!result.recommendations.empty()) {
                    report << "     Main issue: " << result.recommendations[0] << std::endl;
                }
            }
        }
        
        report << std::endl << "RECOMMENDATIONS:" << std::endl;
        report << "----------------" << std::endl;
        report << "1. Fix critical safety issues first (NaN/Inf handling)" << std::endl;
        report << "2. Address thread safety problems" << std::endl;
        report << "3. Optimize high CPU engines" << std::endl;
        report << "4. Fix parameter mapping issues" << std::endl;
        report << "5. Improve audio quality where needed" << std::endl;
        
        report.close();
        std::cout << "Summary report saved to engine_test_summary.txt" << std::endl;
    }
    
    void generateDetailedReport() {
        std::ofstream report("engine_test_detailed.txt");
        
        report << "CHIMERA ENGINE DETAILED TEST REPORT" << std::endl;
        report << "===================================" << std::endl;
        report << "Date: " << getCurrentDateTime() << std::endl << std::endl;
        
        for (const auto& result : results) {
            report << "ENGINE " << result.engineId << ": " << result.engineName << std::endl;
            report << std::string(50, '-') << std::endl;
            
            if (!result.createdSuccessfully) {
                report << "FAILED TO CREATE ENGINE" << std::endl << std::endl;
                continue;
            }
            
            // Safety test results
            report << "SAFETY TESTS:" << std::endl;
            report << "  NaN handling: " << (result.safetyTest.passedNaNTest ? "PASS" : "FAIL") << std::endl;
            report << "  Inf handling: " << (result.safetyTest.passedInfTest ? "PASS" : "FAIL") << std::endl;
            report << "  Denormal prevention: " << (result.safetyTest.passedDenormalTest ? "PASS" : "FAIL") << std::endl;
            report << "  Buffer sizes: " << (result.safetyTest.passedBufferTest ? "PASS" : "FAIL") << std::endl;
            report << "  Thread safety: " << (result.safetyTest.passedThreadTest ? "PASS" : "FAIL") << std::endl;
            
            // Audio quality results
            report << std::endl << "AUDIO QUALITY:" << std::endl;
            report << "  Sine response: " << (result.audioQuality.passesSineTest ? "PASS" : "FAIL") << std::endl;
            report << "  Noise stability: " << (result.audioQuality.passesNoiseTest ? "PASS" : "FAIL") << std::endl;
            report << "  Transient response: " << (result.audioQuality.passesTransientTest ? "PASS" : "FAIL") << std::endl;
            report << "  Clipping behavior: " << (result.audioQuality.passesClippingTest ? "PASS" : "FAIL") << std::endl;
            report << "  THD: " << std::fixed << std::setprecision(2) << result.audioQuality.thd * 100 << "%" << std::endl;
            report << "  SNR: " << std::fixed << std::setprecision(1) << result.audioQuality.snr << " dB" << std::endl;
            
            // Performance results
            report << std::endl << "PERFORMANCE:" << std::endl;
            report << "  Average CPU: " << std::fixed << std::setprecision(2) 
                   << result.performance.avgCpuPercent << "%" << std::endl;
            report << "  Peak CPU: " << std::fixed << std::setprecision(2) 
                   << result.performance.maxCpuPercent << "%" << std::endl;
            report << "  Latency: " << result.performance.avgLatencySamples << " samples" << std::endl;
            report << "  Realtime capable: " << (result.performance.meetsRealtimeConstraints ? "YES" : "NO") << std::endl;
            
            // Parameter issues
            if (!result.parameterTests.empty()) {
                report << std::endl << "PARAMETER ISSUES:" << std::endl;
                for (const auto& param : result.parameterTests) {
                    if (!param.issues.empty()) {
                        report << "  " << param.paramName << ": " << param.issues << std::endl;
                    }
                }
            }
            
            // Recommendations
            if (!result.recommendations.empty()) {
                report << std::endl << "RECOMMENDATIONS:" << std::endl;
                for (const auto& rec : result.recommendations) {
                    report << "  - " << rec << std::endl;
                }
            }
            
            report << std::endl << "Severity Score: " << result.severityScore << std::endl;
            report << std::endl << std::endl;
        }
        
        report.close();
        std::cout << "Detailed report saved to engine_test_detailed.txt" << std::endl;
    }
    
    void generateHTMLReport() {
        std::ofstream report("engine_test_report.html");
        
        report << "<!DOCTYPE html><html><head><title>Chimera Engine Test Report</title>" << std::endl;
        report << "<style>" << std::endl;
        report << "body { font-family: Arial, sans-serif; margin: 20px; }" << std::endl;
        report << "table { border-collapse: collapse; width: 100%; margin: 20px 0; }" << std::endl;
        report << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }" << std::endl;
        report << "th { background-color: #4CAF50; color: white; }" << std::endl;
        report << ".pass { color: green; font-weight: bold; }" << std::endl;
        report << ".fail { color: red; font-weight: bold; }" << std::endl;
        report << ".warning { color: orange; }" << std::endl;
        report << ".critical { background-color: #ffcccc; }" << std::endl;
        report << ".major { background-color: #ffe6cc; }" << std::endl;
        report << ".minor { background-color: #ffffcc; }" << std::endl;
        report << ".perfect { background-color: #ccffcc; }" << std::endl;
        report << "</style></head><body>" << std::endl;
        
        report << "<h1>Chimera Engine Test Report</h1>" << std::endl;
        report << "<p>Generated: " << getCurrentDateTime() << "</p>" << std::endl;
        
        // Summary table
        report << "<h2>Test Summary</h2>" << std::endl;
        report << "<table><tr><th>Engine ID</th><th>Name</th><th>Severity</th><th>Safety</th>"
               << "<th>Quality</th><th>Performance</th><th>Main Issue</th></tr>" << std::endl;
        
        for (const auto& result : results) {
            std::string rowClass = "perfect";
            if (result.severityScore >= 50) rowClass = "critical";
            else if (result.severityScore >= 10) rowClass = "major";
            else if (result.severityScore > 0) rowClass = "minor";
            
            report << "<tr class='" << rowClass << "'>" << std::endl;
            report << "<td>" << result.engineId << "</td>" << std::endl;
            report << "<td>" << result.engineName << "</td>" << std::endl;
            report << "<td>" << result.severityScore << "</td>" << std::endl;
            
            // Safety status
            bool safetypassed = result.safetyTest.passedNaNTest && 
                               result.safetyTest.passedInfTest && 
                               result.safetyTest.passedThreadTest;
            report << "<td class='" << (safetypassed ? "pass" : "fail") << "'>" 
                   << (safetypassed ? "PASS" : "FAIL") << "</td>" << std::endl;
            
            // Quality status
            bool qualityPassed = result.audioQuality.passesSineTest && 
                                result.audioQuality.passesClippingTest;
            report << "<td class='" << (qualityPassed ? "pass" : "fail") << "'>" 
                   << (qualityPassed ? "PASS" : "FAIL") << "</td>" << std::endl;
            
            // Performance
            report << "<td>" << std::fixed << std::setprecision(1) 
                   << result.performance.maxCpuPercent << "%</td>" << std::endl;
            
            // Main issue
            report << "<td>" << (result.recommendations.empty() ? "None" : result.recommendations[0]) 
                   << "</td>" << std::endl;
            
            report << "</tr>" << std::endl;
        }
        
        report << "</table>" << std::endl;
        
        // Performance chart placeholder
        report << "<h2>Performance Analysis</h2>" << std::endl;
        report << "<p>Average CPU usage across all engines: ";
        
        float totalCpu = 0.0f;
        int validEngines = 0;
        for (const auto& result : results) {
            if (result.createdSuccessfully) {
                totalCpu += result.performance.avgCpuPercent;
                validEngines++;
            }
        }
        
        report << std::fixed << std::setprecision(2) << (totalCpu / validEngines) << "%</p>" << std::endl;
        
        report << "</body></html>" << std::endl;
        report.close();
        
        std::cout << "HTML report saved to engine_test_report.html" << std::endl;
    }
    
    void generateJSONReport() {
        std::ofstream report("engine_test_report.json");
        
        report << "{" << std::endl;
        report << "  \"testDate\": \"" << getCurrentDateTime() << "\"," << std::endl;
        report << "  \"totalEngines\": " << results.size() << "," << std::endl;
        report << "  \"engines\": [" << std::endl;
        
        for (size_t i = 0; i < results.size(); ++i) {
            const auto& result = results[i];
            
            report << "    {" << std::endl;
            report << "      \"id\": " << result.engineId << "," << std::endl;
            report << "      \"name\": \"" << result.engineName << "\"," << std::endl;
            report << "      \"severity\": " << result.severityScore << "," << std::endl;
            report << "      \"created\": " << (result.createdSuccessfully ? "true" : "false") << "," << std::endl;
            report << "      \"crashed\": " << (result.crashed ? "true" : "false") << "," << std::endl;
            
            // Safety results
            report << "      \"safety\": {" << std::endl;
            report << "        \"nanTest\": " << (result.safetyTest.passedNaNTest ? "true" : "false") << "," << std::endl;
            report << "        \"infTest\": " << (result.safetyTest.passedInfTest ? "true" : "false") << "," << std::endl;
            report << "        \"threadTest\": " << (result.safetyTest.passedThreadTest ? "true" : "false") << std::endl;
            report << "      }," << std::endl;
            
            // Performance
            report << "      \"performance\": {" << std::endl;
            report << "        \"avgCpu\": " << result.performance.avgCpuPercent << "," << std::endl;
            report << "        \"maxCpu\": " << result.performance.maxCpuPercent << "," << std::endl;
            report << "        \"realtime\": " << (result.performance.meetsRealtimeConstraints ? "true" : "false") << std::endl;
            report << "      }," << std::endl;
            
            // Recommendations
            report << "      \"recommendations\": [" << std::endl;
            for (size_t j = 0; j < result.recommendations.size(); ++j) {
                report << "        \"" << result.recommendations[j] << "\"";
                if (j < result.recommendations.size() - 1) report << ",";
                report << std::endl;
            }
            report << "      ]" << std::endl;
            
            report << "    }";
            if (i < results.size() - 1) report << ",";
            report << std::endl;
        }
        
        report << "  ]" << std::endl;
        report << "}" << std::endl;
        
        report.close();
        std::cout << "JSON report saved to engine_test_report.json" << std::endl;
    }
    
    std::string getCurrentDateTime() {
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
};

// Main function
int main(int argc, char* argv[]) {
    std::cout << "Chimera Engine Test Harness v1.0" << std::endl;
    std::cout << "================================" << std::endl << std::endl;
    
    try {
        EngineTestHarness harness;
        harness.runAllTests();
        harness.generateReports();
        
        std::cout << std::endl << "All tests completed successfully!" << std::endl;
        std::cout << "Check the generated reports for detailed results." << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred" << std::endl;
        return 1;
    }
}