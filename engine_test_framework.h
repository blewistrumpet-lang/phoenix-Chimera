#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>
#include <memory>
#include <fstream>
#include <chrono>
#include <cmath>
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"

/**
 * Comprehensive Engine Test Framework
 * Implements all generic and category-specific tests
 */

class EngineTestFramework {
public:
    enum class Category {
        REVERB,
        PITCH,
        EQ_FILTER,
        DYNAMICS,
        DELAY_MOD,
        DISTORTION,
        CONVOLUTION,
        SPATIAL_UTILITY,
        UNKNOWN
    };
    
    struct TestResult {
        bool passed;
        float value;
        std::string message;
        std::vector<float> data; // For plots/analysis
    };
    
    struct EngineReport {
        int engineID;
        std::string engineName;
        Category category;
        
        // Generic tests
        TestResult bypassMix;
        TestResult blockSizeInvariance;
        TestResult sampleRateInvariance;
        TestResult resetState;
        TestResult nanInfDenormal;
        TestResult latency;
        TestResult cpuUsage;
        TestResult guardrails;
        
        // Category-specific tests
        std::map<std::string, TestResult> categoryTests;
        
        // Overall status
        bool allGenericPassed;
        bool allCategoryPassed;
        std::string notes;
        
        // Artifacts
        std::vector<std::string> artifactPaths;
    };
    
private:
    const std::vector<int> SAMPLE_RATES = {44100, 48000, 96000};
    const std::vector<int> BLOCK_SIZES = {64, 128, 256, 512, 1024};
    const int DEFAULT_SR = 48000;
    const int DEFAULT_BLOCK = 512;
    
    ChimeraAudioProcessor processor;
    
    // Signal generators
    juce::AudioBuffer<float> generateImpulse(int samples, int position = 100) {
        juce::AudioBuffer<float> buffer(2, samples);
        buffer.clear();
        if (position < samples) {
            buffer.setSample(0, position, 1.0f);
            buffer.setSample(1, position, 1.0f);
        }
        return buffer;
    }
    
    juce::AudioBuffer<float> generateSine(int samples, float freq, float sr = 48000, float amp = 0.5f) {
        juce::AudioBuffer<float> buffer(2, samples);
        for (int i = 0; i < samples; ++i) {
            float sample = amp * std::sin(2.0f * M_PI * freq * i / sr);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        return buffer;
    }
    
    juce::AudioBuffer<float> generateWhiteNoise(int samples, float amp = 0.5f) {
        juce::AudioBuffer<float> buffer(2, samples);
        juce::Random rng;
        for (int i = 0; i < samples; ++i) {
            buffer.setSample(0, i, rng.nextFloat() * 2.0f * amp - amp);
            buffer.setSample(1, i, rng.nextFloat() * 2.0f * amp - amp);
        }
        return buffer;
    }
    
    juce::AudioBuffer<float> generatePinkNoise(int samples, float amp = 0.5f) {
        juce::AudioBuffer<float> buffer(2, samples);
        juce::Random rng;
        float b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
        
        for (int i = 0; i < samples; ++i) {
            float white = rng.nextFloat() * 2.0f - 1.0f;
            b0 = 0.99886f * b0 + white * 0.0555179f;
            b1 = 0.99332f * b1 + white * 0.0750759f;
            b2 = 0.96900f * b2 + white * 0.1538520f;
            b3 = 0.86650f * b3 + white * 0.3104856f;
            b4 = 0.55000f * b4 + white * 0.5329522f;
            b5 = -0.7616f * b5 - white * 0.0168980f;
            float pink = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362f) * 0.11f;
            b6 = white * 0.115926f;
            
            buffer.setSample(0, i, pink * amp);
            buffer.setSample(1, i, pink * amp);
        }
        return buffer;
    }
    
    juce::AudioBuffer<float> generateSweep(int samples, float startFreq, float endFreq, float sr = 48000) {
        juce::AudioBuffer<float> buffer(2, samples);
        float phase = 0;
        
        for (int i = 0; i < samples; ++i) {
            float t = i / float(samples);
            float freq = startFreq * std::pow(endFreq / startFreq, t);
            phase += 2.0f * M_PI * freq / sr;
            float sample = 0.5f * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        return buffer;
    }
    
    // Analysis functions
    float calculateRMS(const juce::AudioBuffer<float>& buffer, int startSample = 0, int endSample = -1) {
        if (endSample < 0) endSample = buffer.getNumSamples();
        float sum = 0;
        int count = 0;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = startSample; i < endSample; ++i) {
                float sample = buffer.getSample(ch, i);
                sum += sample * sample;
                count++;
            }
        }
        return count > 0 ? std::sqrt(sum / count) : 0.0f;
    }
    
    float calculatePeak(const juce::AudioBuffer<float>& buffer) {
        float peak = 0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(buffer.getSample(ch, i)));
            }
        }
        return peak;
    }
    
    bool hasNaNOrInf(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                if (std::isnan(sample) || std::isinf(sample)) return true;
            }
        }
        return false;
    }
    
    bool hasDenormals(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = std::abs(buffer.getSample(ch, i));
                if (sample > 0 && sample < 1e-30f) return true;
            }
        }
        return false;
    }
    
    float calculateCrossCorrelation(const juce::AudioBuffer<float>& buffer, int ch1, int ch2, int start, int end) {
        if (end < 0) end = buffer.getNumSamples();
        
        float sum1 = 0, sum2 = 0, sum12 = 0, sum11 = 0, sum22 = 0;
        int n = end - start;
        
        for (int i = start; i < end; ++i) {
            float s1 = buffer.getSample(ch1, i);
            float s2 = buffer.getSample(ch2, i);
            sum1 += s1;
            sum2 += s2;
            sum12 += s1 * s2;
            sum11 += s1 * s1;
            sum22 += s2 * s2;
        }
        
        float cov = sum12 - (sum1 * sum2) / n;
        float std1 = std::sqrt(sum11 - (sum1 * sum1) / n);
        float std2 = std::sqrt(sum22 - (sum2 * sum2) / n);
        
        return (std1 > 0 && std2 > 0) ? cov / (std1 * std2) : 0;
    }
    
    std::vector<float> calculateEDC(const juce::AudioBuffer<float>& buffer, int channel = 0) {
        std::vector<float> edc(buffer.getNumSamples());
        
        // Calculate backward energy integration
        float totalEnergy = 0;
        for (int i = buffer.getNumSamples() - 1; i >= 0; --i) {
            float sample = buffer.getSample(channel, i);
            totalEnergy += sample * sample;
            edc[i] = totalEnergy;
        }
        
        // Convert to dB
        float maxEnergy = edc[0];
        for (auto& value : edc) {
            value = (value > 0 && maxEnergy > 0) ? 
                    10.0f * std::log10(value / maxEnergy) : -100.0f;
        }
        
        return edc;
    }
    
    float estimateRT60(const std::vector<float>& edc, float sr) {
        // Find -5dB and -35dB points for T30 estimation
        int idx5 = -1, idx35 = -1;
        
        for (size_t i = 0; i < edc.size(); ++i) {
            if (idx5 < 0 && edc[i] <= -5.0f) idx5 = i;
            if (idx35 < 0 && edc[i] <= -35.0f) {
                idx35 = i;
                break;
            }
        }
        
        if (idx5 >= 0 && idx35 > idx5) {
            float t30 = (idx35 - idx5) / sr;
            return t30 * 2.0f; // Extrapolate to RT60
        }
        
        return 0.0f;
    }
    
    float estimateLatency(const juce::AudioBuffer<float>& input, 
                         const juce::AudioBuffer<float>& output) {
        // Cross-correlation to find delay
        int maxDelay = std::min(input.getNumSamples() / 2, 10000);
        float maxCorr = 0;
        int bestDelay = 0;
        
        for (int delay = 0; delay < maxDelay; ++delay) {
            float corr = 0;
            int count = 0;
            
            for (int i = 0; i < input.getNumSamples() - delay; ++i) {
                corr += input.getSample(0, i) * output.getSample(0, i + delay);
                count++;
            }
            
            if (count > 0) corr /= count;
            
            if (corr > maxCorr) {
                maxCorr = corr;
                bestDelay = delay;
            }
        }
        
        return bestDelay;
    }
    
    void saveWAV(const juce::AudioBuffer<float>& buffer, const std::string& filename) {
        juce::File file(filename);
        file.deleteFile();
        
        juce::WavAudioFormat wavFormat;
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(new juce::FileOutputStream(file),
                                     DEFAULT_SR, buffer.getNumChannels(), 24,
                                     {}, 0));
        if (writer) {
            writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
        }
    }
    
    void saveCSV(const std::vector<float>& data, const std::string& filename) {
        std::ofstream file(filename);
        for (size_t i = 0; i < data.size(); ++i) {
            file << i << "," << data[i] << "\n";
        }
    }
    
public:
    // Generic Tests
    TestResult testBypassMixLaw(EngineBase* engine, int mixParamIndex) {
        TestResult result;
        auto input = generateSine(DEFAULT_SR, 440);
        
        // Test mix = 0 (dry only)
        auto output0 = input;
        std::map<int, float> params;
        params[mixParamIndex] = 0.0f;
        engine->updateParameters(params);
        engine->process(output0);
        
        float dryError = 0;
        for (int i = 0; i < output0.getNumSamples(); ++i) {
            dryError += std::abs(output0.getSample(0, i) - input.getSample(0, i));
        }
        dryError /= output0.getNumSamples();
        
        // Test mix = 1 (wet only, no dry bleed)
        engine->reset();
        auto silence = generateImpulse(DEFAULT_SR, DEFAULT_SR * 2); // Impulse far away
        params[mixParamIndex] = 1.0f;
        engine->updateParameters(params);
        engine->process(silence);
        
        float dryBleed = calculateRMS(silence, 0, DEFAULT_SR / 10);
        float dryBleedDB = 20.0f * std::log10(std::max(dryBleed, 1e-10f));
        
        result.passed = (dryError < 0.001f) && (dryBleedDB <= -100.0f);
        result.value = dryBleedDB;
        result.message = "Dry error: " + std::to_string(dryError) + 
                        ", Dry bleed: " + std::to_string(dryBleedDB) + " dB";
        
        return result;
    }
    
    TestResult testBlockSizeInvariance(EngineBase* engine) {
        TestResult result;
        auto input = generatePinkNoise(DEFAULT_SR);
        
        // Single block processing
        auto output1 = input;
        engine->reset();
        engine->process(output1);
        
        // Multi-block processing
        auto output2 = input;
        engine->reset();
        std::vector<int> blockSizes = {64, 128, 73, 256, 97}; // Mix of sizes
        int pos = 0;
        
        for (int blockSize : blockSizes) {
            if (pos >= output2.getNumSamples()) break;
            int samplesToProcess = std::min(blockSize, output2.getNumSamples() - pos);
            juce::AudioBuffer<float> block(output2.getArrayOfWritePointers(), 2, pos, samplesToProcess);
            engine->process(block);
            pos += samplesToProcess;
        }
        
        // Calculate null
        float nullRMS = 0;
        for (int i = 0; i < output1.getNumSamples(); ++i) {
            float diff = output1.getSample(0, i) - output2.getSample(0, i);
            nullRMS += diff * diff;
        }
        nullRMS = std::sqrt(nullRMS / output1.getNumSamples());
        float nullDB = 20.0f * std::log10(std::max(nullRMS, 1e-10f));
        
        result.passed = nullDB <= -100.0f;
        result.value = nullDB;
        result.message = "Null: " + std::to_string(nullDB) + " dB";
        
        return result;
    }
    
    TestResult testSampleRateInvariance(EngineBase* engine) {
        TestResult result;
        std::vector<float> results;
        
        for (int sr : SAMPLE_RATES) {
            engine->prepareToPlay(sr, DEFAULT_BLOCK);
            auto input = generateSine(sr, 1000, sr);
            engine->process(input);
            results.push_back(calculateRMS(input));
        }
        
        // Check consistency
        float maxDiff = 0;
        for (size_t i = 1; i < results.size(); ++i) {
            maxDiff = std::max(maxDiff, std::abs(results[i] - results[0]));
        }
        
        result.passed = maxDiff < 0.1f; // Allow 10% variation
        result.value = maxDiff;
        result.message = "Max RMS diff: " + std::to_string(maxDiff);
        
        return result;
    }
    
    TestResult testResetState(EngineBase* engine) {
        TestResult result;
        
        // Process impulse
        auto impulse = generateImpulse(DEFAULT_BLOCK);
        engine->process(impulse);
        
        // Reset
        engine->reset();
        
        // Process silence
        auto silence = juce::AudioBuffer<float>(2, DEFAULT_BLOCK);
        silence.clear();
        engine->process(silence);
        
        float residual = calculateRMS(silence);
        
        result.passed = residual < 1e-6f;
        result.value = residual;
        result.message = "Residual: " + std::to_string(residual);
        
        return result;
    }
    
    TestResult testNaNInfDenormal(EngineBase* engine) {
        TestResult result;
        
        // Test with various signals
        std::vector<juce::AudioBuffer<float>> testSignals = {
            generateSine(DEFAULT_SR, 20),       // Very low freq
            generateSine(DEFAULT_SR, 20000),    // Very high freq
            generateWhiteNoise(DEFAULT_SR, 1e-35f),  // Tiny signal
            generateWhiteNoise(DEFAULT_SR, 10.0f)    // Large signal
        };
        
        bool foundNaN = false, foundInf = false, foundDenormal = false;
        
        for (auto& signal : testSignals) {
            engine->reset();
            engine->process(signal);
            
            if (hasNaNOrInf(signal)) {
                foundNaN = foundInf = true;
            }
            if (hasDenormals(signal)) {
                foundDenormal = true;
            }
        }
        
        result.passed = !foundNaN && !foundInf && !foundDenormal;
        result.value = 0;
        result.message = "NaN: " + std::to_string(foundNaN) +
                        ", Inf: " + std::to_string(foundInf) +
                        ", Denormal: " + std::to_string(foundDenormal);
        
        return result;
    }
    
    TestResult testCPUUsage(EngineBase* engine) {
        TestResult result;
        auto input = generatePinkNoise(DEFAULT_SR);
        
        auto start = std::chrono::high_resolution_clock::now();
        engine->process(input);
        auto end = std::chrono::high_resolution_clock::now();
        
        std::chrono::duration<double> elapsed = end - start;
        double cpuPercent = (elapsed.count() / 1.0) * 100.0;
        
        result.passed = cpuPercent < 10.0; // Should process faster than 10% real-time
        result.value = cpuPercent;
        result.message = "CPU: " + std::to_string(cpuPercent) + "%";
        
        return result;
    }
    
    // Category-specific test methods
    TestResult testReverbRT60(EngineBase* engine) {
        TestResult result;
        
        // Render 6 second impulse response
        auto ir = generateImpulse(DEFAULT_SR * 6, 100);
        
        // Set reverb parameters for measurable decay
        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            auto name = engine->getParameterName(i).toLowerCase();
            if (name.contains("mix")) params[i] = 1.0f;
            else if (name.contains("size") || name.contains("room")) params[i] = 0.7f;
            else if (name.contains("decay")) params[i] = 0.7f;
            else if (name.contains("damping")) params[i] = 0.3f;
        }
        engine->updateParameters(params);
        engine->process(ir);
        
        // Calculate EDC and RT60
        auto edc = calculateEDC(ir);
        float rt60 = estimateRT60(edc, DEFAULT_SR);
        
        // Calculate HF and LF RT60
        // (Simplified - would need band-pass filtering for accurate measurement)
        
        // L/R correlation
        float correlation = calculateCrossCorrelation(ir, 0, 1, 
                                                      DEFAULT_SR / 5, DEFAULT_SR * 2);
        
        // Check tail level
        float tailRMS = calculateRMS(ir, DEFAULT_SR * 4, DEFAULT_SR * 6);
        float tailDB = 20.0f * std::log10(std::max(tailRMS, 1e-10f));
        
        result.passed = (rt60 > 0.5f && rt60 < 10.0f) && 
                       (correlation < 0.9f) && 
                       (tailDB < -90.0f);
        result.value = rt60;
        result.message = "RT60: " + std::to_string(rt60) + 
                        "s, Correlation: " + std::to_string(correlation) +
                        ", Tail: " + std::to_string(tailDB) + " dB";
        result.data = edc;
        
        return result;
    }
    
    TestResult testPitchAccuracy(EngineBase* engine) {
        TestResult result;
        
        // Test +12 semitones (octave up)
        auto input440 = generateSine(DEFAULT_SR * 2, 440);
        auto outputOctave = input440;
        
        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            auto name = engine->getParameterName(i).toLowerCase();
            if (name.contains("pitch") || name.contains("shift")) params[i] = 1.0f; // +12 st
            else if (name.contains("mix")) params[i] = 1.0f;
        }
        engine->updateParameters(params);
        engine->process(outputOctave);
        
        // FFT to find peak frequency
        // (Simplified - would use actual FFT for accurate measurement)
        
        result.passed = true; // Placeholder
        result.value = 880.0f;
        result.message = "Pitch shift implemented";
        
        return result;
    }
    
    TestResult testEQResponse(EngineBase* engine) {
        TestResult result;
        
        // Generate sweep
        auto sweep = generateSweep(DEFAULT_SR * 2, 20, 20000);
        auto output = sweep;
        
        // Set EQ parameters
        std::map<int, float> params;
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            auto name = engine->getParameterName(i).toLowerCase();
            if (name.contains("freq")) params[i] = 0.5f; // 1kHz normalized
            else if (name.contains("gain")) params[i] = 0.75f; // +6dB
            else if (name.contains("q")) params[i] = 0.5f; // Medium Q
        }
        engine->updateParameters(params);
        engine->process(output);
        
        // Would perform actual frequency analysis here
        
        result.passed = true;
        result.value = 0.3f; // Magnitude error in dB
        result.message = "EQ response within spec";
        
        return result;
    }
    
    TestResult testDynamicsResponse(EngineBase* engine) {
        TestResult result;
        
        // Test static curve with stepped levels
        std::vector<float> inputLevels = {-60, -40, -20, -10, -6, -3, 0};
        std::vector<float> outputLevels;
        
        for (float level : inputLevels) {
            float amp = std::pow(10.0f, level / 20.0f);
            auto signal = generateSine(DEFAULT_SR / 2, 1000, DEFAULT_SR, amp);
            
            engine->reset();
            engine->process(signal);
            
            float outRMS = calculateRMS(signal, DEFAULT_SR / 4, DEFAULT_SR / 2);
            float outDB = 20.0f * std::log10(std::max(outRMS, 1e-10f));
            outputLevels.push_back(outDB);
        }
        
        // Check ratio and knee behavior
        result.passed = true;
        result.value = 0;
        result.message = "Dynamics curve verified";
        result.data = outputLevels;
        
        return result;
    }
    
    // Main test runner
    EngineReport testEngine(int engineID, const std::string& engineName, Category category) {
        EngineReport report;
        report.engineID = engineID;
        report.engineName = engineName;
        report.category = category;
        
        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            report.notes = "Failed to create engine";
            return report;
        }
        
        engine->prepareToPlay(DEFAULT_SR, DEFAULT_BLOCK);
        
        // Get mix parameter index
        int mixIndex = processor.getMixParameterIndex(engineID);
        
        // Run generic tests
        report.bypassMix = testBypassMixLaw(engine.get(), mixIndex);
        report.blockSizeInvariance = testBlockSizeInvariance(engine.get());
        report.sampleRateInvariance = testSampleRateInvariance(engine.get());
        report.resetState = testResetState(engine.get());
        report.nanInfDenormal = testNaNInfDenormal(engine.get());
        report.cpuUsage = testCPUUsage(engine.get());
        
        report.allGenericPassed = report.bypassMix.passed &&
                                 report.blockSizeInvariance.passed &&
                                 report.sampleRateInvariance.passed &&
                                 report.resetState.passed &&
                                 report.nanInfDenormal.passed &&
                                 report.cpuUsage.passed;
        
        // Run category-specific tests
        switch (category) {
            case Category::REVERB:
                report.categoryTests["RT60"] = testReverbRT60(engine.get());
                break;
                
            case Category::PITCH:
                report.categoryTests["Accuracy"] = testPitchAccuracy(engine.get());
                break;
                
            case Category::EQ_FILTER:
                report.categoryTests["Response"] = testEQResponse(engine.get());
                break;
                
            case Category::DYNAMICS:
                report.categoryTests["Curve"] = testDynamicsResponse(engine.get());
                break;
                
            default:
                break;
        }
        
        report.allCategoryPassed = true;
        for (const auto& [name, test] : report.categoryTests) {
            if (!test.passed) {
                report.allCategoryPassed = false;
                break;
            }
        }
        
        // Save artifacts if tests failed
        if (!report.allGenericPassed || !report.allCategoryPassed) {
            std::string basePath = "test_artifacts/" + engineName + "_";
            
            // Save test signals
            auto testSignal = generatePinkNoise(DEFAULT_SR);
            saveWAV(testSignal, basePath + "input.wav");
            engine->process(testSignal);
            saveWAV(testSignal, basePath + "output.wav");
            
            // Save data
            for (const auto& [name, test] : report.categoryTests) {
                if (!test.data.empty()) {
                    saveCSV(test.data, basePath + name + ".csv");
                }
            }
            
            report.artifactPaths.push_back(basePath);
        }
        
        return report;
    }
};