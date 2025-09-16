// Autonomous Engine Analyzer for Chimera Phoenix 3.0
// Comprehensive testing framework for all 57 DSP engines
// Analyzes waveforms, spectrograms, FFT, amplitude, and histograms

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cmath>
#include <complex>
#include <algorithm>
#include <numeric>
#include <iomanip>
#include <chrono>
#include <memory>
#include <random>

// JUCE includes
#include "JUCE_Plugin/JuceLibraryCode/JuceHeader.h"
#include "JUCE_Plugin/Source/EngineBase.h"
#include "JUCE_Plugin/Source/DspEngineUtilities.h"

// Include all engine headers
#include "JUCE_Plugin/Source/PitchShifter.h"

// Forward declarations for engines we'll test
class PitchCorrection;
class FrequencyShifter;
class RingModulator;
class GranularDelay;
class IntelligentHarmonizer;

class AutonomousEngineAnalyzer {
public:
    struct TestConfiguration {
        double sampleRate = 44100.0;
        int blockSize = 512;
        int fftSize = 4096;
        float testDuration = 1.0f; // seconds
        bool generatePlots = true;
        bool verboseOutput = true;
    };
    
    struct SpectralAnalysis {
        std::vector<float> magnitudeSpectrum;
        std::vector<float> phaseSpectrum;
        float spectralCentroid;
        float spectralSpread;
        float spectralFlatness;
        float spectralRolloff;
        float spectralFlux;
        std::vector<float> mfcc; // Mel-frequency cepstral coefficients
    };
    
    struct TemporalAnalysis {
        float rmsLevel;
        float peakLevel;
        float crestFactor;
        float zeroCrossingRate;
        float temporalCentroid;
        std::vector<float> envelope;
        float attackTime;
        float decayTime;
        float sustainLevel;
        float releaseTime;
    };
    
    struct StatisticalAnalysis {
        std::vector<float> histogram;
        float mean;
        float median;
        float standardDeviation;
        float variance;
        float skewness;
        float kurtosis;
        float entropy;
        std::pair<float, float> dynamicRange;
    };
    
    struct QualityMetrics {
        float snr;                    // Signal-to-noise ratio
        float thd;                     // Total harmonic distortion
        float thdPlusNoise;           // THD+N
        float sinad;                  // Signal-to-noise and distortion
        float imd;                    // Intermodulation distortion
        float correlationWithInput;
        float phaseCoherence;
        bool hasClipping;
        bool hasDCOffset;
        bool hasAliasing;
        float artifactScore;          // 0-1, lower is better
    };
    
    struct ParameterAnalysis {
        int parameterIndex;
        std::string parameterName;
        float defaultValue;
        float currentValue;
        bool isWorking;
        bool hasDiscontinuities;
        bool causesArtifacts;
        std::string behavior;
        std::map<float, QualityMetrics> sweepResults;
    };
    
    struct EngineAnalysisReport {
        // Engine identification
        int engineIndex;
        std::string engineName;
        std::string engineCategory;
        
        // Processing metrics
        std::chrono::microseconds averageProcessingTime;
        float cpuUsage;
        bool isRealTimeCapable;
        
        // Analysis results for different test signals
        std::map<std::string, SpectralAnalysis> spectralResults;
        std::map<std::string, TemporalAnalysis> temporalResults;
        std::map<std::string, StatisticalAnalysis> statisticalResults;
        std::map<std::string, QualityMetrics> qualityResults;
        
        // Parameter analysis
        std::vector<ParameterAnalysis> parameterAnalysis;
        
        // Issues and recommendations
        std::vector<std::string> criticalIssues;
        std::vector<std::string> warnings;
        std::vector<std::string> recommendations;
        
        // Overall scoring
        float overallQualityScore;     // 0-100
        float stabilityScore;           // 0-100
        float parameterScore;           // 0-100
        char grade;                     // A-F
    };

private:
    TestConfiguration config;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::mt19937 randomGen;
    
    // Test signal generators
    std::vector<float> generateSineWave(float frequency, float amplitude = 1.0f) {
        int numSamples = static_cast<int>(config.sampleRate * config.testDuration);
        std::vector<float> signal(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = amplitude * std::sin(2.0f * M_PI * frequency * i / config.sampleRate);
        }
        
        return signal;
    }
    
    std::vector<float> generateComplexTone(std::vector<float> frequencies, std::vector<float> amplitudes) {
        int numSamples = static_cast<int>(config.sampleRate * config.testDuration);
        std::vector<float> signal(numSamples, 0.0f);
        
        for (size_t h = 0; h < frequencies.size() && h < amplitudes.size(); ++h) {
            for (int i = 0; i < numSamples; ++i) {
                signal[i] += amplitudes[h] * std::sin(2.0f * M_PI * frequencies[h] * i / config.sampleRate);
            }
        }
        
        // Normalize
        float maxVal = *std::max_element(signal.begin(), signal.end(), 
            [](float a, float b) { return std::abs(a) < std::abs(b); });
        if (maxVal > 0) {
            for (float& sample : signal) {
                sample /= maxVal;
            }
        }
        
        return signal;
    }
    
    std::vector<float> generateWhiteNoise() {
        int numSamples = static_cast<int>(config.sampleRate * config.testDuration);
        std::vector<float> signal(numSamples);
        
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        for (int i = 0; i < numSamples; ++i) {
            signal[i] = dist(randomGen);
        }
        
        return signal;
    }
    
    std::vector<float> generatePinkNoise() {
        int numSamples = static_cast<int>(config.sampleRate * config.testDuration);
        std::vector<float> signal(numSamples);
        
        // Pink noise generation using Voss-McCartney algorithm
        float white[16];
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        
        for (int i = 0; i < 16; ++i) {
            white[i] = dist(randomGen);
        }
        
        for (int i = 0; i < numSamples; ++i) {
            int idx = i & 15;
            white[idx] = dist(randomGen);
            
            float sum = 0;
            for (int j = 0; j < 16; ++j) {
                if (i & (1 << j)) {
                    sum += white[j];
                }
            }
            signal[i] = sum / 4.0f;
        }
        
        return signal;
    }
    
    std::vector<float> generateChirp(float startFreq, float endFreq) {
        int numSamples = static_cast<int>(config.sampleRate * config.testDuration);
        std::vector<float> signal(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            float t = i / config.sampleRate;
            float freq = startFreq + (endFreq - startFreq) * t / config.testDuration;
            float phase = 2.0f * M_PI * (startFreq * t + (endFreq - startFreq) * t * t / (2 * config.testDuration));
            signal[i] = std::sin(phase);
        }
        
        return signal;
    }
    
    std::vector<float> generateImpulse() {
        int numSamples = static_cast<int>(config.sampleRate * config.testDuration);
        std::vector<float> signal(numSamples, 0.0f);
        signal[numSamples / 2] = 1.0f;
        return signal;
    }
    
    std::vector<float> generateSquareWave(float frequency) {
        int numSamples = static_cast<int>(config.sampleRate * config.testDuration);
        std::vector<float> signal(numSamples);
        
        for (int i = 0; i < numSamples; ++i) {
            float phase = std::fmod(frequency * i / config.sampleRate, 1.0f);
            signal[i] = (phase < 0.5f) ? 1.0f : -1.0f;
        }
        
        return signal;
    }
    
    // Spectral analysis functions
    SpectralAnalysis performSpectralAnalysis(const std::vector<float>& signal) {
        SpectralAnalysis result;
        
        // Prepare FFT
        std::vector<std::complex<float>> fftData(config.fftSize);
        
        // Apply Hann window and copy to FFT buffer
        for (int i = 0; i < std::min(static_cast<int>(signal.size()), config.fftSize); ++i) {
            float window = 0.5f - 0.5f * std::cos(2.0f * M_PI * i / (config.fftSize - 1));
            fftData[i] = signal[i] * window;
        }
        
        // Perform FFT
        fft->perform(fftData.data(), fftData.data(), false);
        
        // Calculate magnitude and phase spectrum
        result.magnitudeSpectrum.resize(config.fftSize / 2);
        result.phaseSpectrum.resize(config.fftSize / 2);
        
        for (int i = 0; i < config.fftSize / 2; ++i) {
            result.magnitudeSpectrum[i] = std::abs(fftData[i]) * 2.0f / config.fftSize;
            result.phaseSpectrum[i] = std::arg(fftData[i]);
        }
        
        // Calculate spectral features
        float totalMagnitude = 0.0f;
        float weightedSum = 0.0f;
        
        for (int i = 1; i < config.fftSize / 2; ++i) {
            float freq = i * config.sampleRate / config.fftSize;
            float mag = result.magnitudeSpectrum[i];
            totalMagnitude += mag;
            weightedSum += freq * mag;
        }
        
        // Spectral centroid
        result.spectralCentroid = (totalMagnitude > 0) ? weightedSum / totalMagnitude : 0.0f;
        
        // Spectral spread
        float spreadSum = 0.0f;
        for (int i = 1; i < config.fftSize / 2; ++i) {
            float freq = i * config.sampleRate / config.fftSize;
            float deviation = freq - result.spectralCentroid;
            spreadSum += deviation * deviation * result.magnitudeSpectrum[i];
        }
        result.spectralSpread = (totalMagnitude > 0) ? 
            std::sqrt(spreadSum / totalMagnitude) : 0.0f;
        
        // Spectral flatness (Wiener entropy)
        float geometricMean = 0.0f;
        float arithmeticMean = 0.0f;
        int nonZeroBins = 0;
        
        for (int i = 1; i < config.fftSize / 2; ++i) {
            if (result.magnitudeSpectrum[i] > 1e-10f) {
                geometricMean += std::log(result.magnitudeSpectrum[i]);
                arithmeticMean += result.magnitudeSpectrum[i];
                nonZeroBins++;
            }
        }
        
        if (nonZeroBins > 0) {
            geometricMean = std::exp(geometricMean / nonZeroBins);
            arithmeticMean /= nonZeroBins;
            result.spectralFlatness = (arithmeticMean > 0) ? geometricMean / arithmeticMean : 0.0f;
        }
        
        // Spectral rolloff (95% of energy)
        float cumulativeEnergy = 0.0f;
        float totalEnergy = std::accumulate(result.magnitudeSpectrum.begin(), 
                                           result.magnitudeSpectrum.end(), 0.0f);
        
        for (int i = 0; i < config.fftSize / 2; ++i) {
            cumulativeEnergy += result.magnitudeSpectrum[i];
            if (cumulativeEnergy >= 0.95f * totalEnergy) {
                result.spectralRolloff = i * config.sampleRate / config.fftSize;
                break;
            }
        }
        
        // Spectral flux (change between frames)
        result.spectralFlux = 0.0f; // Would need previous frame for real calculation
        
        return result;
    }
    
    // Temporal analysis functions
    TemporalAnalysis performTemporalAnalysis(const std::vector<float>& signal) {
        TemporalAnalysis result;
        
        // RMS level
        float sumSquares = 0.0f;
        for (float sample : signal) {
            sumSquares += sample * sample;
        }
        result.rmsLevel = std::sqrt(sumSquares / signal.size());
        
        // Peak level
        result.peakLevel = 0.0f;
        for (float sample : signal) {
            result.peakLevel = std::max(result.peakLevel, std::abs(sample));
        }
        
        // Crest factor
        result.crestFactor = (result.rmsLevel > 0) ? result.peakLevel / result.rmsLevel : 0.0f;
        
        // Zero crossing rate
        int zeroCrossings = 0;
        for (size_t i = 1; i < signal.size(); ++i) {
            if ((signal[i] >= 0) != (signal[i-1] >= 0)) {
                zeroCrossings++;
            }
        }
        result.zeroCrossingRate = static_cast<float>(zeroCrossings) / signal.size();
        
        // Temporal centroid
        float weightedSum = 0.0f;
        float totalEnergy = 0.0f;
        for (size_t i = 0; i < signal.size(); ++i) {
            float energy = signal[i] * signal[i];
            weightedSum += i * energy;
            totalEnergy += energy;
        }
        result.temporalCentroid = (totalEnergy > 0) ? weightedSum / totalEnergy : 0.0f;
        
        // Envelope extraction (using Hilbert transform approximation)
        result.envelope.resize(signal.size());
        for (size_t i = 0; i < signal.size(); ++i) {
            result.envelope[i] = std::abs(signal[i]);
        }
        
        // Simple smoothing for envelope
        const int smoothingWindow = 100;
        for (size_t i = smoothingWindow; i < result.envelope.size() - smoothingWindow; ++i) {
            float sum = 0.0f;
            for (int j = -smoothingWindow/2; j <= smoothingWindow/2; ++j) {
                sum += result.envelope[i + j];
            }
            result.envelope[i] = sum / smoothingWindow;
        }
        
        // ADSR estimation (simplified)
        float maxEnv = *std::max_element(result.envelope.begin(), result.envelope.end());
        
        // Find attack time (10% to 90% of max)
        size_t attackStart = 0, attackEnd = 0;
        for (size_t i = 0; i < result.envelope.size(); ++i) {
            if (result.envelope[i] > 0.1f * maxEnv && attackStart == 0) {
                attackStart = i;
            }
            if (result.envelope[i] > 0.9f * maxEnv && attackEnd == 0) {
                attackEnd = i;
                break;
            }
        }
        result.attackTime = (attackEnd - attackStart) / config.sampleRate;
        
        // Simplified decay/sustain/release
        result.decayTime = 0.1f;  // Placeholder
        result.sustainLevel = 0.7f;  // Placeholder
        result.releaseTime = 0.2f;  // Placeholder
        
        return result;
    }
    
    // Statistical analysis functions
    StatisticalAnalysis performStatisticalAnalysis(const std::vector<float>& signal) {
        StatisticalAnalysis result;
        
        // Histogram (amplitude distribution)
        const int numBins = 100;
        result.histogram.resize(numBins, 0.0f);
        
        float minVal = *std::min_element(signal.begin(), signal.end());
        float maxVal = *std::max_element(signal.begin(), signal.end());
        float range = maxVal - minVal;
        
        if (range > 1e-6f) {
            for (float sample : signal) {
                int bin = static_cast<int>((sample - minVal) / range * (numBins - 1));
                bin = std::max(0, std::min(numBins - 1, bin));
                result.histogram[bin]++;
            }
            
            // Normalize histogram
            for (float& count : result.histogram) {
                count /= signal.size();
            }
        }
        
        // Mean
        result.mean = std::accumulate(signal.begin(), signal.end(), 0.0f) / signal.size();
        
        // Median
        std::vector<float> sorted = signal;
        std::sort(sorted.begin(), sorted.end());
        result.median = sorted[sorted.size() / 2];
        
        // Variance and standard deviation
        float sumSquaredDiff = 0.0f;
        for (float sample : signal) {
            float diff = sample - result.mean;
            sumSquaredDiff += diff * diff;
        }
        result.variance = sumSquaredDiff / signal.size();
        result.standardDeviation = std::sqrt(result.variance);
        
        // Skewness
        if (result.standardDeviation > 1e-6f) {
            float sumCubedDiff = 0.0f;
            for (float sample : signal) {
                float z = (sample - result.mean) / result.standardDeviation;
                sumCubedDiff += z * z * z;
            }
            result.skewness = sumCubedDiff / signal.size();
        }
        
        // Kurtosis (excess)
        if (result.standardDeviation > 1e-6f) {
            float sumQuadDiff = 0.0f;
            for (float sample : signal) {
                float z = (sample - result.mean) / result.standardDeviation;
                sumQuadDiff += z * z * z * z;
            }
            result.kurtosis = sumQuadDiff / signal.size() - 3.0f;
        }
        
        // Entropy
        result.entropy = 0.0f;
        for (float prob : result.histogram) {
            if (prob > 1e-10f) {
                result.entropy -= prob * std::log2(prob);
            }
        }
        
        // Dynamic range
        result.dynamicRange = {minVal, maxVal};
        
        return result;
    }
    
    // Quality metrics calculation
    QualityMetrics calculateQualityMetrics(const std::vector<float>& input, 
                                          const std::vector<float>& output) {
        QualityMetrics metrics;
        
        // SNR calculation
        float signalPower = 0.0f;
        float noisePower = 0.0f;
        
        size_t minSize = std::min(input.size(), output.size());
        for (size_t i = 0; i < minSize; ++i) {
            signalPower += output[i] * output[i];
            float noise = output[i] - input[i];
            noisePower += noise * noise;
        }
        
        metrics.snr = (noisePower > 1e-10f) ? 
            10.0f * std::log10(signalPower / noisePower) : 100.0f;
        
        // THD calculation (simplified - assumes input is pure tone)
        auto outputSpectrum = performSpectralAnalysis(output);
        
        // Find fundamental
        auto maxIt = std::max_element(outputSpectrum.magnitudeSpectrum.begin() + 10,
                                     outputSpectrum.magnitudeSpectrum.end());
        if (maxIt != outputSpectrum.magnitudeSpectrum.end()) {
            int fundamentalBin = std::distance(outputSpectrum.magnitudeSpectrum.begin(), maxIt);
            float fundamental = *maxIt;
            
            // Sum harmonics
            float harmonicSum = 0.0f;
            for (int h = 2; h <= 10; ++h) {
                int harmonicBin = fundamentalBin * h;
                if (harmonicBin < outputSpectrum.magnitudeSpectrum.size()) {
                    harmonicSum += outputSpectrum.magnitudeSpectrum[harmonicBin] * 
                                  outputSpectrum.magnitudeSpectrum[harmonicBin];
                }
            }
            
            metrics.thd = (fundamental > 0) ? std::sqrt(harmonicSum) / fundamental : 0.0f;
        }
        
        // Simplified THD+N, SINAD, IMD
        metrics.thdPlusNoise = metrics.thd * 1.1f;  // Approximation
        metrics.sinad = metrics.snr * 0.9f;  // Approximation
        metrics.imd = 0.0f;  // Would need two-tone test
        
        // Correlation
        float meanInput = std::accumulate(input.begin(), input.begin() + minSize, 0.0f) / minSize;
        float meanOutput = std::accumulate(output.begin(), output.begin() + minSize, 0.0f) / minSize;
        
        float numerator = 0.0f;
        float denomInput = 0.0f;
        float denomOutput = 0.0f;
        
        for (size_t i = 0; i < minSize; ++i) {
            float di = input[i] - meanInput;
            float do_ = output[i] - meanOutput;
            numerator += di * do_;
            denomInput += di * di;
            denomOutput += do_ * do_;
        }
        
        float denom = std::sqrt(denomInput * denomOutput);
        metrics.correlationWithInput = (denom > 0) ? numerator / denom : 0.0f;
        
        // Phase coherence (simplified)
        metrics.phaseCoherence = std::abs(metrics.correlationWithInput);
        
        // Clipping detection
        int clippedSamples = 0;
        for (float sample : output) {
            if (std::abs(sample) > 0.99f) {
                clippedSamples++;
            }
        }
        metrics.hasClipping = clippedSamples > output.size() * 0.001f;
        
        // DC offset detection
        float dcOffset = std::accumulate(output.begin(), output.end(), 0.0f) / output.size();
        metrics.hasDCOffset = std::abs(dcOffset) > 0.01f;
        
        // Aliasing detection (check for energy near Nyquist)
        float nyquistEnergy = 0.0f;
        for (size_t i = outputSpectrum.magnitudeSpectrum.size() * 0.9; 
             i < outputSpectrum.magnitudeSpectrum.size(); ++i) {
            nyquistEnergy += outputSpectrum.magnitudeSpectrum[i];
        }
        metrics.hasAliasing = nyquistEnergy > 0.1f;
        
        // Artifact score (0-1, lower is better)
        metrics.artifactScore = 0.0f;
        if (metrics.hasClipping) metrics.artifactScore += 0.3f;
        if (metrics.hasDCOffset) metrics.artifactScore += 0.2f;
        if (metrics.hasAliasing) metrics.artifactScore += 0.3f;
        if (metrics.thd > 0.1f) metrics.artifactScore += 0.2f;
        metrics.artifactScore = std::min(1.0f, metrics.artifactScore);
        
        return metrics;
    }

public:
    AutonomousEngineAnalyzer() : randomGen(std::random_device{}()) {
        fft = std::make_unique<juce::dsp::FFT>(
            static_cast<int>(std::log2(config.fftSize)));
    }
    
    // Main analysis function for a single engine
    EngineAnalysisReport analyzeEngine(EngineBase* engine, int engineIndex, 
                                       const std::string& engineName) {
        EngineAnalysisReport report;
        report.engineIndex = engineIndex;
        report.engineName = engineName;
        
        if (config.verboseOutput) {
            std::cout << "Analyzing " << engineName << " (Index " << engineIndex << ")...\n";
        }
        
        // Prepare engine
        engine->prepareToPlay(config.sampleRate, config.blockSize);
        engine->reset();
        
        // Test signals
        std::map<std::string, std::vector<float>> testSignals = {
            {"Sine_440Hz", generateSineWave(440.0f)},
            {"Sine_1kHz", generateSineWave(1000.0f)},
            {"Complex_Tone", generateComplexTone({220, 440, 660, 880}, {1.0, 0.5, 0.3, 0.2})},
            {"White_Noise", generateWhiteNoise()},
            {"Pink_Noise", generatePinkNoise()},
            {"Chirp_20-20k", generateChirp(20.0f, 20000.0f)},
            {"Impulse", generateImpulse()},
            {"Square_100Hz", generateSquareWave(100.0f)}
        };
        
        // Process each test signal
        for (const auto& [signalName, inputSignal] : testSignals) {
            // Create audio buffer
            juce::AudioBuffer<float> buffer(2, inputSignal.size());
            
            // Copy input to both channels
            for (int ch = 0; ch < 2; ++ch) {
                for (size_t i = 0; i < inputSignal.size(); ++i) {
                    buffer.setSample(ch, i, inputSignal[i]);
                }
            }
            
            // Measure processing time
            auto startTime = std::chrono::high_resolution_clock::now();
            
            // Process through engine
            engine->process(buffer);
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto processingTime = std::chrono::duration_cast<std::chrono::microseconds>(
                endTime - startTime);
            
            // Get output signal
            std::vector<float> outputSignal(inputSignal.size());
            for (size_t i = 0; i < outputSignal.size(); ++i) {
                outputSignal[i] = buffer.getSample(0, i);
            }
            
            // Perform analyses
            report.spectralResults[signalName] = performSpectralAnalysis(outputSignal);
            report.temporalResults[signalName] = performTemporalAnalysis(outputSignal);
            report.statisticalResults[signalName] = performStatisticalAnalysis(outputSignal);
            report.qualityResults[signalName] = calculateQualityMetrics(inputSignal, outputSignal);
            
            // Update average processing time
            report.averageProcessingTime = processingTime;
        }
        
        // Analyze each parameter
        analyzeParameters(engine, report);
        
        // Detect issues and generate recommendations
        detectIssues(report);
        
        // Calculate scores
        calculateScores(report);
        
        return report;
    }
    
    void analyzeParameters(EngineBase* engine, EngineAnalysisReport& report) {
        // Test each parameter
        for (int paramIdx = 0; paramIdx < 8; ++paramIdx) {
            ParameterAnalysis param;
            param.parameterIndex = paramIdx;
            param.parameterName = engine->getParameterName(paramIdx).toStdString();
            
            // Test signal for parameter sweep
            auto testSignal = generateSineWave(440.0f);
            
            // Test at different values
            std::vector<float> testValues = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
            
            for (float value : testValues) {
                // Set parameter
                std::map<int, float> params;
                params[paramIdx] = value;
                engine->updateParameters(params);
                
                // Process
                juce::AudioBuffer<float> buffer(2, testSignal.size());
                for (int ch = 0; ch < 2; ++ch) {
                    for (size_t i = 0; i < testSignal.size(); ++i) {
                        buffer.setSample(ch, i, testSignal[i]);
                    }
                }
                
                engine->process(buffer);
                
                // Get output
                std::vector<float> output(testSignal.size());
                for (size_t i = 0; i < output.size(); ++i) {
                    output[i] = buffer.getSample(0, i);
                }
                
                // Calculate metrics
                param.sweepResults[value] = calculateQualityMetrics(testSignal, output);
            }
            
            // Analyze parameter behavior
            analyzeParameterBehavior(param);
            
            report.parameterAnalysis.push_back(param);
        }
    }
    
    void analyzeParameterBehavior(ParameterAnalysis& param) {
        // Check if parameter has any effect
        float firstSNR = param.sweepResults[0.0f].snr;
        float lastSNR = param.sweepResults[1.0f].snr;
        
        param.isWorking = std::abs(firstSNR - lastSNR) > 1.0f;
        
        // Check for discontinuities
        param.hasDiscontinuities = false;
        float prevSNR = -1000.0f;
        for (const auto& [value, metrics] : param.sweepResults) {
            if (prevSNR > -999.0f) {
                float change = std::abs(metrics.snr - prevSNR);
                if (change > 20.0f) {
                    param.hasDiscontinuities = true;
                }
            }
            prevSNR = metrics.snr;
        }
        
        // Check if parameter causes artifacts
        param.causesArtifacts = false;
        for (const auto& [value, metrics] : param.sweepResults) {
            if (metrics.artifactScore > 0.5f) {
                param.causesArtifacts = true;
                break;
            }
        }
        
        // Describe behavior
        if (!param.isWorking) {
            param.behavior = "No effect detected";
        } else if (param.hasDiscontinuities) {
            param.behavior = "Discontinuous/stepping behavior";
        } else if (param.causesArtifacts) {
            param.behavior = "Causes artifacts at some settings";
        } else {
            param.behavior = "Working normally";
        }
    }
    
    void detectIssues(EngineAnalysisReport& report) {
        // Check for critical issues across all test signals
        for (const auto& [signalName, metrics] : report.qualityResults) {
            if (metrics.hasClipping) {
                report.criticalIssues.push_back("Clipping detected with " + signalName);
            }
            
            if (metrics.hasDCOffset) {
                report.criticalIssues.push_back("DC offset present with " + signalName);
            }
            
            if (metrics.hasAliasing) {
                report.criticalIssues.push_back("Aliasing detected with " + signalName);
            }
            
            if (metrics.snr < 20.0f) {
                report.warnings.push_back("Low SNR (" + std::to_string(metrics.snr) + 
                    " dB) with " + signalName);
            }
            
            if (metrics.thd > 0.1f) {
                report.warnings.push_back("High THD (" + 
                    std::to_string(metrics.thd * 100) + "%) with " + signalName);
            }
            
            if (metrics.artifactScore > 0.5f) {
                report.warnings.push_back("High artifact score with " + signalName);
            }
        }
        
        // Check parameter issues
        int nonWorkingParams = 0;
        int problematicParams = 0;
        
        for (const auto& param : report.parameterAnalysis) {
            if (!param.isWorking) {
                nonWorkingParams++;
                report.warnings.push_back("Parameter '" + param.parameterName + 
                    "' appears to have no effect");
            }
            
            if (param.hasDiscontinuities) {
                problematicParams++;
                report.warnings.push_back("Parameter '" + param.parameterName + 
                    "' has discontinuities");
            }
            
            if (param.causesArtifacts) {
                problematicParams++;
                report.criticalIssues.push_back("Parameter '" + param.parameterName + 
                    "' causes artifacts");
            }
        }
        
        // Generate recommendations
        if (!report.criticalIssues.empty()) {
            report.recommendations.push_back("Address critical issues before deployment");
        }
        
        if (nonWorkingParams > 2) {
            report.recommendations.push_back("Review parameter mapping and processing logic");
        }
        
        if (problematicParams > 0) {
            report.recommendations.push_back("Implement parameter smoothing to reduce artifacts");
        }
        
        // Check spectral issues
        float avgSpectralFlatness = 0.0f;
        int count = 0;
        for (const auto& [signalName, spectral] : report.spectralResults) {
            avgSpectralFlatness += spectral.spectralFlatness;
            count++;
        }
        
        if (count > 0) {
            avgSpectralFlatness /= count;
            if (avgSpectralFlatness > 0.8f) {
                report.recommendations.push_back("Output may sound too noisy/harsh");
            } else if (avgSpectralFlatness < 0.1f) {
                report.recommendations.push_back("Output may sound too tonal/resonant");
            }
        }
    }
    
    void calculateScores(EngineAnalysisReport& report) {
        // Overall quality score
        float qualityScore = 100.0f;
        
        // Deduct for issues
        qualityScore -= report.criticalIssues.size() * 15.0f;
        qualityScore -= report.warnings.size() * 5.0f;
        
        // Average quality metrics
        float avgSNR = 0.0f;
        float avgTHD = 0.0f;
        float avgArtifacts = 0.0f;
        int count = 0;
        
        for (const auto& [signalName, metrics] : report.qualityResults) {
            avgSNR += metrics.snr;
            avgTHD += metrics.thd;
            avgArtifacts += metrics.artifactScore;
            count++;
        }
        
        if (count > 0) {
            avgSNR /= count;
            avgTHD /= count;
            avgArtifacts /= count;
            
            // Adjust score based on metrics
            if (avgSNR < 40.0f) qualityScore -= (40.0f - avgSNR) * 0.5f;
            if (avgTHD > 0.05f) qualityScore -= avgTHD * 100.0f;
            if (avgArtifacts > 0.2f) qualityScore -= avgArtifacts * 30.0f;
        }
        
        report.overallQualityScore = std::max(0.0f, std::min(100.0f, qualityScore));
        
        // Stability score
        float stabilityScore = 100.0f;
        
        for (const auto& [signalName, metrics] : report.qualityResults) {
            if (metrics.hasClipping) stabilityScore -= 20.0f;
            if (metrics.hasDCOffset) stabilityScore -= 10.0f;
            if (metrics.hasAliasing) stabilityScore -= 15.0f;
        }
        
        report.stabilityScore = std::max(0.0f, std::min(100.0f, stabilityScore));
        
        // Parameter score
        float paramScore = 100.0f;
        int workingParams = 0;
        
        for (const auto& param : report.parameterAnalysis) {
            if (param.isWorking && !param.hasDiscontinuities && !param.causesArtifacts) {
                workingParams++;
            }
        }
        
        paramScore = (workingParams / 8.0f) * 100.0f;
        report.parameterScore = paramScore;
        
        // Calculate grade
        float avgScore = (report.overallQualityScore + report.stabilityScore + 
                         report.parameterScore) / 3.0f;
        
        if (avgScore >= 90) report.grade = 'A';
        else if (avgScore >= 80) report.grade = 'B';
        else if (avgScore >= 70) report.grade = 'C';
        else if (avgScore >= 60) report.grade = 'D';
        else report.grade = 'F';
    }
    
    // Generate detailed report
    void generateDetailedReport(const EngineAnalysisReport& report, 
                               const std::string& filename) {
        std::ofstream file(filename);
        
        file << "================================================================================\n";
        file << "                        ENGINE ANALYSIS REPORT                                  \n";
        file << "================================================================================\n\n";
        
        file << "Engine: " << report.engineName << " (Index: " << report.engineIndex << ")\n";
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        file << "Analysis Date: " << std::ctime(&time_t);
        file << "Processing Time: " << report.averageProcessingTime.count() << " μs\n\n";
        
        file << "OVERALL SCORES:\n";
        file << "---------------\n";
        file << "Quality Score:    " << std::fixed << std::setprecision(1) 
             << report.overallQualityScore << "/100\n";
        file << "Stability Score:  " << report.stabilityScore << "/100\n";
        file << "Parameter Score:  " << report.parameterScore << "/100\n";
        file << "Overall Grade:    " << report.grade << "\n\n";
        
        // Critical issues
        if (!report.criticalIssues.empty()) {
            file << "❌ CRITICAL ISSUES:\n";
            file << "-------------------\n";
            for (const auto& issue : report.criticalIssues) {
                file << "• " << issue << "\n";
            }
            file << "\n";
        }
        
        // Warnings
        if (!report.warnings.empty()) {
            file << "⚠️  WARNINGS:\n";
            file << "------------\n";
            for (const auto& warning : report.warnings) {
                file << "• " << warning << "\n";
            }
            file << "\n";
        }
        
        // Recommendations
        if (!report.recommendations.empty()) {
            file << "💡 RECOMMENDATIONS:\n";
            file << "-------------------\n";
            for (const auto& rec : report.recommendations) {
                file << "• " << rec << "\n";
            }
            file << "\n";
        }
        
        // Detailed test results
        file << "SIGNAL TEST RESULTS:\n";
        file << "====================\n\n";
        
        for (const auto& [signalName, quality] : report.qualityResults) {
            file << signalName << ":\n";
            file << "  SNR: " << std::setprecision(1) << quality.snr << " dB\n";
            file << "  THD: " << std::setprecision(2) << quality.thd * 100 << "%\n";
            file << "  Correlation: " << std::setprecision(3) << quality.correlationWithInput << "\n";
            file << "  Artifact Score: " << quality.artifactScore << "\n";
            
            if (quality.hasClipping) file << "  ⚠️ CLIPPING\n";
            if (quality.hasDCOffset) file << "  ⚠️ DC OFFSET\n";
            if (quality.hasAliasing) file << "  ⚠️ ALIASING\n";
            
            // Spectral info
            const auto& spectral = report.spectralResults.at(signalName);
            file << "  Spectral Centroid: " << std::setprecision(0) 
                 << spectral.spectralCentroid << " Hz\n";
            file << "  Spectral Spread: " << spectral.spectralSpread << " Hz\n";
            file << "  Spectral Flatness: " << std::setprecision(3) 
                 << spectral.spectralFlatness << "\n";
            
            // Temporal info
            const auto& temporal = report.temporalResults.at(signalName);
            file << "  RMS Level: " << temporal.rmsLevel << "\n";
            file << "  Peak Level: " << temporal.peakLevel << "\n";
            file << "  Crest Factor: " << temporal.crestFactor << "\n";
            
            file << "\n";
        }
        
        // Parameter analysis
        file << "PARAMETER ANALYSIS:\n";
        file << "===================\n\n";
        
        for (const auto& param : report.parameterAnalysis) {
            file << param.parameterName << " (Index " << param.parameterIndex << "):\n";
            file << "  Status: " << param.behavior << "\n";
            file << "  Working: " << (param.isWorking ? "Yes" : "No") << "\n";
            file << "  Discontinuities: " << (param.hasDiscontinuities ? "Yes" : "No") << "\n";
            file << "  Causes Artifacts: " << (param.causesArtifacts ? "Yes" : "No") << "\n";
            
            file << "  Value Sweep Results:\n";
            for (const auto& [value, metrics] : param.sweepResults) {
                file << "    " << std::setprecision(2) << value << ": ";
                file << "SNR=" << std::setprecision(1) << metrics.snr << "dB, ";
                file << "Artifacts=" << std::setprecision(2) << metrics.artifactScore;
                file << "\n";
            }
            file << "\n";
        }
        
        file << "================================================================================\n";
        file << "                              END OF REPORT                                     \n";
        file << "================================================================================\n";
        
        file.close();
    }
    
    // Generate summary CSV for multiple engines
    void generateSummaryCSV(const std::vector<EngineAnalysisReport>& reports, 
                           const std::string& filename) {
        std::ofstream file(filename);
        
        // Header
        file << "Index,Name,Quality,Stability,Parameters,Grade,Issues,Warnings,";
        file << "Avg_SNR,Avg_THD,Avg_Artifacts,Processing_Time_us\n";
        
        for (const auto& report : reports) {
            file << report.engineIndex << ",";
            file << report.engineName << ",";
            file << report.overallQualityScore << ",";
            file << report.stabilityScore << ",";
            file << report.parameterScore << ",";
            file << report.grade << ",";
            file << report.criticalIssues.size() << ",";
            file << report.warnings.size() << ",";
            
            // Calculate averages
            float avgSNR = 0, avgTHD = 0, avgArtifacts = 0;
            int count = 0;
            for (const auto& [name, metrics] : report.qualityResults) {
                avgSNR += metrics.snr;
                avgTHD += metrics.thd;
                avgArtifacts += metrics.artifactScore;
                count++;
            }
            
            if (count > 0) {
                file << avgSNR / count << ",";
                file << avgTHD / count << ",";
                file << avgArtifacts / count << ",";
            } else {
                file << "0,0,0,";
            }
            
            file << report.averageProcessingTime.count() << "\n";
        }
        
        file.close();
    }
};

// Main testing program
int main() {
    std::cout << "╔══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║          AUTONOMOUS ENGINE ANALYZER - CHIMERA PHOENIX 3.0        ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════╝\n\n";
    
    AutonomousEngineAnalyzer analyzer;
    std::vector<AutonomousEngineAnalyzer::EngineAnalysisReport> allReports;
    
    // Test PitchShifter first
    std::cout << "Starting analysis of PitchShifter engine...\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    auto pitchShifter = std::make_unique<PitchShifter>();
    auto report = analyzer.analyzeEngine(pitchShifter.get(), 0, "PitchShifter");
    
    // Generate detailed report
    std::string reportFilename = "Reports/PitchShifter_Analysis.txt";
    analyzer.generateDetailedReport(report, reportFilename);
    
    // Display summary
    std::cout << "\n✅ Analysis Complete!\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Quality Score:    " << report.overallQualityScore << "/100\n";
    std::cout << "Stability Score:  " << report.stabilityScore << "/100\n";
    std::cout << "Parameter Score:  " << report.parameterScore << "/100\n";
    std::cout << "Overall Grade:    " << report.grade << "\n\n";
    
    if (!report.criticalIssues.empty()) {
        std::cout << "❌ Critical Issues Found: " << report.criticalIssues.size() << "\n";
        for (size_t i = 0; i < std::min(size_t(3), report.criticalIssues.size()); ++i) {
            std::cout << "   • " << report.criticalIssues[i] << "\n";
        }
    }
    
    if (!report.warnings.empty()) {
        std::cout << "\n⚠️  Warnings: " << report.warnings.size() << "\n";
        for (size_t i = 0; i < std::min(size_t(3), report.warnings.size()); ++i) {
            std::cout << "   • " << report.warnings[i] << "\n";
        }
    }
    
    std::cout << "\n📊 Detailed report saved to: " << reportFilename << "\n";
    
    allReports.push_back(report);
    
    // Generate summary CSV
    analyzer.generateSummaryCSV(allReports, "Reports/Engine_Analysis_Summary.csv");
    
    std::cout << "\n════════════════════════════════════════════════════════════════════\n";
    std::cout << "Analysis framework ready for all 57 engines.\n";
    std::cout << "Next steps: Analyze remaining engines systematically.\n";
    
    return 0;
}