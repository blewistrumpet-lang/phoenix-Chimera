#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <complex>
#include <chrono>
#include <sstream>

/**
 * DEEP VALIDATION MISSION - Distortion Engines (15-22)
 *
 * Comprehensive validation of all distortion/saturation parameters
 * Testing drive, tone controls, mix, saturation curves, harmonic content
 */

namespace DeepValidation {

// Fast FFT for harmonic analysis
class FFTAnalyzer {
    static constexpr int FFT_SIZE = 16384;
    std::vector<std::complex<float>> fftBuffer;

public:
    FFTAnalyzer() : fftBuffer(FFT_SIZE) {}

    struct HarmonicSpectrum {
        float fundamental_Hz = 0.0f;
        float fundamental_dB = 0.0f;
        std::vector<float> harmonics_dB; // Relative to fundamental
        float thd_percent = 0.0f;
        float even_harmonic_ratio = 0.0f;
        float odd_harmonic_ratio = 0.0f;
    };

    HarmonicSpectrum analyze(const std::vector<float>& signal, float sampleRate, float targetFreq) {
        HarmonicSpectrum result;
        result.harmonics_dB.resize(10, -120.0f);

        int N = std::min((int)signal.size(), FFT_SIZE);

        // Simple DFT for target frequency and harmonics
        for (int k = 0; k < N/2; ++k) {
            std::complex<float> sum(0.0f, 0.0f);
            for (int n = 0; n < N; ++n) {
                float angle = -2.0f * M_PI * k * n / N;
                sum += signal[n] * std::complex<float>(std::cos(angle), std::sin(angle));
            }
            fftBuffer[k] = sum / float(N);
        }

        // Find fundamental bin
        int fundamentalBin = std::round(targetFreq * N / sampleRate);
        result.fundamental_Hz = targetFreq;

        float fundamentalMag = std::abs(fftBuffer[fundamentalBin]);
        result.fundamental_dB = 20.0f * std::log10(std::max(1e-10f, fundamentalMag));

        // Analyze harmonics
        float harmonicPowerSum = 0.0f;
        float evenPowerSum = 0.0f;
        float oddPowerSum = 0.0f;

        for (int h = 2; h <= 11; ++h) {
            int harmonicBin = fundamentalBin * h;
            if (harmonicBin < N/2) {
                float harmonicMag = std::abs(fftBuffer[harmonicBin]);
                float harmonicDB = 20.0f * std::log10(std::max(1e-10f, harmonicMag / fundamentalMag));
                result.harmonics_dB[h-2] = harmonicDB;

                float power = harmonicMag * harmonicMag;
                harmonicPowerSum += power;

                if (h % 2 == 0) {
                    evenPowerSum += power;
                } else {
                    oddPowerSum += power;
                }
            }
        }

        // THD calculation
        float fundamentalPower = fundamentalMag * fundamentalMag;
        result.thd_percent = 100.0f * std::sqrt(harmonicPowerSum / std::max(1e-20f, fundamentalPower));

        // Even/odd ratio
        float totalHarmonicPower = evenPowerSum + oddPowerSum;
        if (totalHarmonicPower > 1e-20f) {
            result.even_harmonic_ratio = evenPowerSum / totalHarmonicPower;
            result.odd_harmonic_ratio = oddPowerSum / totalHarmonicPower;
        }

        return result;
    }
};

struct ParameterInfo {
    int index;
    std::string name;
    std::vector<float> testValues;
};

struct DistortionMetrics {
    std::string engineName;
    int engineID;

    // Parameter sweep results
    struct ParameterSweepResult {
        std::string paramName;
        std::vector<float> values;
        std::vector<float> thd_values;
        std::vector<float> output_rms;
        std::vector<float> peak_values;
        bool clipping_detected = false;
        std::string behavior; // "linear", "exponential", "logarithmic"
    };

    std::vector<ParameterSweepResult> paramSweeps;

    // Tone control analysis
    struct ToneControlAnalysis {
        std::vector<float> frequencies; // Test frequencies
        std::vector<std::vector<float>> responses; // Response at each tone setting
        float low_freq_Hz = 100.0f;
        float mid_freq_Hz = 1000.0f;
        float high_freq_Hz = 8000.0f;
    };

    ToneControlAnalysis toneAnalysis;

    // Saturation curve
    struct SaturationCurve {
        std::vector<float> input_dB;
        std::vector<float> output_dB;
        std::string curve_type; // "soft_clip", "hard_clip", "asymmetric"
        float compression_ratio = 1.0f;
        float knee_threshold_dB = 0.0f;
    };

    SaturationCurve saturationCurve;

    // Harmonic content at different drive levels
    struct HarmonicProfile {
        std::vector<float> drive_levels;
        std::vector<FFTAnalyzer::HarmonicSpectrum> spectra;
    };

    HarmonicProfile harmonicProfile;

    // Mix control validation
    struct MixBehavior {
        std::vector<float> mix_values;
        std::vector<float> dry_wet_ratios;
        bool power_preserving = false;
        bool linear = true;
    };

    MixBehavior mixBehavior;

    // Oversampling analysis
    struct OversamplingAnalysis {
        bool oversampling_detected = false;
        float aliasing_level_dB = -120.0f;
        std::string quality; // "excellent", "good", "fair", "poor"
    };

    OversamplingAnalysis oversamplingQuality;

    // Transient response
    struct TransientResponse {
        float attack_time_ms = 0.0f;
        float overshoot_percent = 0.0f;
        float settling_time_ms = 0.0f;
    };

    TransientResponse transientResponse;

    // Noise floor
    float noise_floor_dB = -120.0f;

    // Overall assessment
    int quality_score = 0; // 0-100
    std::vector<std::string> warnings;
    std::vector<std::string> strengths;
    bool passed = false;
};

class DistortionValidator {
    FFTAnalyzer fftAnalyzer;
    float sampleRate = 48000.0f;
    int blockSize = 512;

public:

    DistortionMetrics validateEngine(int engineID, const std::string& name) {
        DistortionMetrics metrics;
        metrics.engineID = engineID;
        metrics.engineName = name;

        std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║ DEEP VALIDATION: Engine " << std::setw(2) << engineID << " - "
                  << std::setw(38) << std::left << name << "║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";

        auto engine = EngineFactory::createEngine(engineID);
        if (!engine) {
            std::cerr << "ERROR: Could not create engine " << engineID << "\n";
            return metrics;
        }

        engine->prepareToPlay(sampleRate, blockSize);

        int numParams = engine->getNumParameters();
        std::cout << "  Parameters: " << numParams << "\n";

        // Identify parameters by name
        std::vector<ParameterInfo> paramInfos;
        for (int i = 0; i < numParams; ++i) {
            ParameterInfo info;
            info.index = i;
            info.name = engine->getParameterName(i).toStdString();

            // Create test sweep values
            for (int j = 0; j <= 10; ++j) {
                info.testValues.push_back(j / 10.0f);
            }

            paramInfos.push_back(info);
            std::cout << "    [" << i << "] " << info.name << "\n";
        }

        // Test 1: Parameter sweeps
        std::cout << "\n  [1/8] Parameter sweep tests...\n";
        testParameterSweeps(engine.get(), paramInfos, metrics);

        // Test 2: Tone control analysis
        std::cout << "  [2/8] Tone control analysis...\n";
        testToneControls(engine.get(), paramInfos, metrics);

        // Test 3: Saturation curve
        std::cout << "  [3/8] Saturation curve measurement...\n";
        testSaturationCurve(engine.get(), paramInfos, metrics);

        // Test 4: Harmonic content vs drive
        std::cout << "  [4/8] Harmonic profile analysis...\n";
        testHarmonicProfile(engine.get(), paramInfos, metrics);

        // Test 5: Mix control behavior
        std::cout << "  [5/8] Mix control validation...\n";
        testMixBehavior(engine.get(), paramInfos, metrics);

        // Test 6: Oversampling quality
        std::cout << "  [6/8] Oversampling analysis...\n";
        testOversamplingQuality(engine.get(), paramInfos, metrics);

        // Test 7: Transient response
        std::cout << "  [7/8] Transient response...\n";
        testTransientResponse(engine.get(), paramInfos, metrics);

        // Test 8: Noise floor
        std::cout << "  [8/8] Noise floor measurement...\n";
        testNoiseFloor(engine.get(), metrics);

        // Final assessment
        assessQuality(metrics);

        return metrics;
    }

private:

    void testParameterSweeps(EngineBase* engine, const std::vector<ParameterInfo>& params,
                            DistortionMetrics& metrics) {
        const float testFreq = 1000.0f;
        const int testSamples = 8192;

        for (const auto& param : params) {
            DistortionMetrics::ParameterSweepResult sweep;
            sweep.paramName = param.name;

            for (float value : param.testValues) {
                // Set this parameter, defaults for others
                std::map<int, float> paramMap;
                for (const auto& p : params) {
                    paramMap[p.index] = (p.index == param.index) ? value : 0.5f;
                }
                engine->updateParameters(paramMap);

                // Generate test signal
                juce::AudioBuffer<float> buffer(2, testSamples);
                for (int i = 0; i < testSamples; ++i) {
                    float phase = 2.0f * M_PI * testFreq * i / sampleRate;
                    float sample = 0.316f * std::sin(phase); // -10dB
                    buffer.setSample(0, i, sample);
                    buffer.setSample(1, i, sample);
                }

                engine->process(buffer);

                // Measure output
                float rms = 0.0f;
                float peak = 0.0f;
                for (int i = 0; i < testSamples; ++i) {
                    float sample = buffer.getSample(0, i);
                    rms += sample * sample;
                    peak = std::max(peak, std::abs(sample));
                }
                rms = std::sqrt(rms / testSamples);

                // Check for clipping
                if (peak > 0.999f) {
                    sweep.clipping_detected = true;
                }

                // Measure THD
                std::vector<float> signal(testSamples);
                for (int i = 0; i < testSamples; ++i) {
                    signal[i] = buffer.getSample(0, i);
                }
                auto spectrum = fftAnalyzer.analyze(signal, sampleRate, testFreq);

                sweep.values.push_back(value);
                sweep.thd_values.push_back(spectrum.thd_percent);
                sweep.output_rms.push_back(rms);
                sweep.peak_values.push_back(peak);
            }

            // Determine behavior (linear, exponential, logarithmic)
            analyzeBehavior(sweep);

            metrics.paramSweeps.push_back(sweep);
        }
    }

    void analyzeBehavior(DistortionMetrics::ParameterSweepResult& sweep) {
        if (sweep.output_rms.size() < 3) {
            sweep.behavior = "unknown";
            return;
        }

        // Calculate differences
        float diff1 = sweep.output_rms[5] - sweep.output_rms[0]; // First half
        float diff2 = sweep.output_rms[10] - sweep.output_rms[5]; // Second half

        if (std::abs(diff1 - diff2) < 0.1f * std::max(diff1, diff2)) {
            sweep.behavior = "linear";
        } else if (diff2 > diff1 * 1.5f) {
            sweep.behavior = "exponential";
        } else if (diff1 > diff2 * 1.5f) {
            sweep.behavior = "logarithmic";
        } else {
            sweep.behavior = "nonlinear";
        }
    }

    void testToneControls(EngineBase* engine, const std::vector<ParameterInfo>& params,
                         DistortionMetrics& metrics) {
        // Find tone-related parameters
        int toneParamIdx = -1;
        for (const auto& p : params) {
            std::string lower = p.name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find("tone") != std::string::npos ||
                lower.find("treble") != std::string::npos ||
                lower.find("bass") != std::string::npos) {
                toneParamIdx = p.index;
                break;
            }
        }

        if (toneParamIdx < 0) {
            return; // No tone control found
        }

        std::vector<float> testFreqs = {100.0f, 500.0f, 1000.0f, 3000.0f, 8000.0f};
        metrics.toneAnalysis.frequencies = testFreqs;

        for (float toneValue : {0.0f, 0.5f, 1.0f}) {
            std::vector<float> responses;

            for (float freq : testFreqs) {
                std::map<int, float> paramMap;
                for (const auto& p : params) {
                    paramMap[p.index] = (p.index == toneParamIdx) ? toneValue : 0.5f;
                }
                engine->updateParameters(paramMap);

                // Generate sweep
                const int testSamples = 4096;
                juce::AudioBuffer<float> buffer(2, testSamples);
                for (int i = 0; i < testSamples; ++i) {
                    float phase = 2.0f * M_PI * freq * i / sampleRate;
                    float sample = 0.1f * std::sin(phase);
                    buffer.setSample(0, i, sample);
                    buffer.setSample(1, i, sample);
                }

                engine->process(buffer);

                // Measure output level
                float rms = 0.0f;
                for (int i = testSamples/2; i < testSamples; ++i) { // Second half to avoid transients
                    float sample = buffer.getSample(0, i);
                    rms += sample * sample;
                }
                rms = std::sqrt(rms / (testSamples/2));
                float dB = 20.0f * std::log10(std::max(1e-10f, rms / 0.1f));

                responses.push_back(dB);
            }

            metrics.toneAnalysis.responses.push_back(responses);
        }
    }

    void testSaturationCurve(EngineBase* engine, const std::vector<ParameterInfo>& params,
                            DistortionMetrics& metrics) {
        const float testFreq = 1000.0f;
        const int testSamples = 4096;

        // Set moderate drive
        std::map<int, float> paramMap;
        for (const auto& p : params) {
            paramMap[p.index] = 0.7f; // High drive for saturation
        }
        engine->updateParameters(paramMap);

        // Test different input levels
        std::vector<float> inputLevels_dB = {-40.0f, -30.0f, -20.0f, -10.0f, 0.0f, 6.0f, 12.0f};

        for (float inputDB : inputLevels_dB) {
            float amplitude = std::pow(10.0f, inputDB / 20.0f);

            juce::AudioBuffer<float> buffer(2, testSamples);
            for (int i = 0; i < testSamples; ++i) {
                float phase = 2.0f * M_PI * testFreq * i / sampleRate;
                float sample = amplitude * std::sin(phase);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            engine->process(buffer);

            // Measure output RMS
            float rms = 0.0f;
            for (int i = testSamples/2; i < testSamples; ++i) {
                float sample = buffer.getSample(0, i);
                rms += sample * sample;
            }
            rms = std::sqrt(rms / (testSamples/2));
            float outputDB = 20.0f * std::log10(std::max(1e-10f, rms));

            metrics.saturationCurve.input_dB.push_back(inputDB);
            metrics.saturationCurve.output_dB.push_back(outputDB);
        }

        // Analyze curve type
        if (metrics.saturationCurve.output_dB.size() >= 3) {
            float gain1 = metrics.saturationCurve.output_dB[1] - metrics.saturationCurve.input_dB[1];
            float gain2 = metrics.saturationCurve.output_dB.back() - metrics.saturationCurve.input_dB.back();
            float gainReduction = gain1 - gain2;

            if (gainReduction > 10.0f) {
                metrics.saturationCurve.curve_type = "hard_clip";
            } else if (gainReduction > 3.0f) {
                metrics.saturationCurve.curve_type = "soft_clip";
            } else {
                metrics.saturationCurve.curve_type = "linear";
            }

            metrics.saturationCurve.compression_ratio =
                std::abs((metrics.saturationCurve.input_dB.back() - metrics.saturationCurve.input_dB[1]) /
                        (metrics.saturationCurve.output_dB.back() - metrics.saturationCurve.output_dB[1]));
        }
    }

    void testHarmonicProfile(EngineBase* engine, const std::vector<ParameterInfo>& params,
                            DistortionMetrics& metrics) {
        const float testFreq = 1000.0f;
        const int testSamples = 16384;

        // Find drive parameter
        int driveParamIdx = -1;
        for (const auto& p : params) {
            std::string lower = p.name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find("drive") != std::string::npos ||
                lower.find("gain") != std::string::npos ||
                lower.find("distortion") != std::string::npos) {
                driveParamIdx = p.index;
                break;
            }
        }

        if (driveParamIdx < 0 && params.size() > 0) {
            driveParamIdx = 0; // Default to first parameter
        }

        for (float drive : {0.0f, 0.3f, 0.5f, 0.7f, 1.0f}) {
            std::map<int, float> paramMap;
            for (const auto& p : params) {
                if (p.index == driveParamIdx) {
                    paramMap[p.index] = drive;
                } else {
                    paramMap[p.index] = 0.5f;
                }
            }
            engine->updateParameters(paramMap);

            juce::AudioBuffer<float> buffer(2, testSamples);
            for (int i = 0; i < testSamples; ++i) {
                float phase = 2.0f * M_PI * testFreq * i / sampleRate;
                float sample = 0.1f * std::sin(phase); // -20dB
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            engine->process(buffer);

            std::vector<float> signal(testSamples);
            for (int i = 0; i < testSamples; ++i) {
                signal[i] = buffer.getSample(0, i);
            }

            auto spectrum = fftAnalyzer.analyze(signal, sampleRate, testFreq);

            metrics.harmonicProfile.drive_levels.push_back(drive);
            metrics.harmonicProfile.spectra.push_back(spectrum);
        }
    }

    void testMixBehavior(EngineBase* engine, const std::vector<ParameterInfo>& params,
                        DistortionMetrics& metrics) {
        // Find mix parameter
        int mixParamIdx = -1;
        for (const auto& p : params) {
            std::string lower = p.name;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            if (lower.find("mix") != std::string::npos ||
                lower.find("blend") != std::string::npos ||
                lower.find("wet") != std::string::npos) {
                mixParamIdx = p.index;
                break;
            }
        }

        if (mixParamIdx < 0) {
            return; // No mix control
        }

        const int testSamples = 4096;
        for (float mixValue : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f}) {
            std::map<int, float> paramMap;
            for (const auto& p : params) {
                if (p.index == mixParamIdx) {
                    paramMap[p.index] = mixValue;
                } else {
                    paramMap[p.index] = 0.5f;
                }
            }
            engine->updateParameters(paramMap);

            juce::AudioBuffer<float> buffer(2, testSamples);
            for (int i = 0; i < testSamples; ++i) {
                float sample = 0.1f * std::sin(2.0f * M_PI * 1000.0f * i / sampleRate);
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            engine->process(buffer);

            float rms = 0.0f;
            for (int i = 0; i < testSamples; ++i) {
                float sample = buffer.getSample(0, i);
                rms += sample * sample;
            }
            rms = std::sqrt(rms / testSamples);

            metrics.mixBehavior.mix_values.push_back(mixValue);
            metrics.mixBehavior.dry_wet_ratios.push_back(rms);
        }
    }

    void testOversamplingQuality(EngineBase* engine, const std::vector<ParameterInfo>& params,
                                DistortionMetrics& metrics) {
        // Generate high-frequency content and look for aliasing
        const float testFreq = sampleRate * 0.4f; // Near Nyquist
        const int testSamples = 16384;

        std::map<int, float> paramMap;
        for (const auto& p : params) {
            paramMap[p.index] = 0.8f; // High drive
        }
        engine->updateParameters(paramMap);

        juce::AudioBuffer<float> buffer(2, testSamples);
        for (int i = 0; i < testSamples; ++i) {
            float phase = 2.0f * M_PI * testFreq * i / sampleRate;
            float sample = 0.5f * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        engine->process(buffer);

        std::vector<float> signal(testSamples);
        for (int i = 0; i < testSamples; ++i) {
            signal[i] = buffer.getSample(0, i);
        }

        // Look for energy above the expected harmonics
        int N = testSamples;
        int fundamentalBin = std::round(testFreq * N / sampleRate);

        float fundamentalPower = 0.0f;
        float aliasingPower = 0.0f;

        for (int k = 0; k < N/2; ++k) {
            std::complex<float> sum(0.0f, 0.0f);
            for (int n = 0; n < N; ++n) {
                float angle = -2.0f * M_PI * k * n / N;
                sum += signal[n] * std::complex<float>(std::cos(angle), std::sin(angle));
            }
            float power = std::norm(sum);

            // Check if this is near the fundamental or its harmonics
            bool isHarmonic = false;
            for (int h = 1; h <= 5; ++h) {
                if (std::abs(k - fundamentalBin * h) < 5) {
                    isHarmonic = true;
                    if (h == 1) fundamentalPower += power;
                    break;
                }
            }

            // Look for unexpected energy (aliasing)
            if (!isHarmonic && k > fundamentalBin/2 && power > 1e-10f) {
                aliasingPower += power;
            }
        }

        if (aliasingPower > 0.0f && fundamentalPower > 0.0f) {
            metrics.oversamplingQuality.aliasing_level_dB =
                10.0f * std::log10(aliasingPower / fundamentalPower);
            metrics.oversamplingQuality.oversampling_detected =
                metrics.oversamplingQuality.aliasing_level_dB < -60.0f;

            if (metrics.oversamplingQuality.aliasing_level_dB < -80.0f) {
                metrics.oversamplingQuality.quality = "excellent";
            } else if (metrics.oversamplingQuality.aliasing_level_dB < -60.0f) {
                metrics.oversamplingQuality.quality = "good";
            } else if (metrics.oversamplingQuality.aliasing_level_dB < -40.0f) {
                metrics.oversamplingQuality.quality = "fair";
            } else {
                metrics.oversamplingQuality.quality = "poor";
            }
        }
    }

    void testTransientResponse(EngineBase* engine, const std::vector<ParameterInfo>& params,
                              DistortionMetrics& metrics) {
        const int testSamples = 4096;

        std::map<int, float> paramMap;
        for (const auto& p : params) {
            paramMap[p.index] = 0.5f;
        }
        engine->updateParameters(paramMap);

        // Generate impulse
        juce::AudioBuffer<float> buffer(2, testSamples);
        buffer.clear();
        buffer.setSample(0, 100, 1.0f); // Impulse at sample 100
        buffer.setSample(1, 100, 1.0f);

        engine->process(buffer);

        // Measure response
        float peak = 0.0f;
        int peakSample = 0;
        for (int i = 100; i < 200; ++i) {
            float sample = std::abs(buffer.getSample(0, i));
            if (sample > peak) {
                peak = sample;
                peakSample = i;
            }
        }

        metrics.transientResponse.attack_time_ms = (peakSample - 100) * 1000.0f / sampleRate;

        // Check for overshoot/ringing
        float settledLevel = peak * 0.1f;
        int settledSample = peakSample;
        for (int i = peakSample; i < testSamples; ++i) {
            if (std::abs(buffer.getSample(0, i)) < settledLevel) {
                settledSample = i;
                break;
            }
        }

        metrics.transientResponse.settling_time_ms = (settledSample - 100) * 1000.0f / sampleRate;
    }

    void testNoiseFloor(EngineBase* engine, DistortionMetrics& metrics) {
        const int testSamples = 48000; // 1 second of silence

        juce::AudioBuffer<float> buffer(2, testSamples);
        buffer.clear();

        engine->process(buffer);

        float rms = 0.0f;
        for (int i = 0; i < testSamples; ++i) {
            float sample = buffer.getSample(0, i);
            rms += sample * sample;
        }
        rms = std::sqrt(rms / testSamples);

        metrics.noise_floor_dB = 20.0f * std::log10(std::max(1e-10f, rms));
    }

    void assessQuality(DistortionMetrics& metrics) {
        int score = 100;

        // Check for clipping
        for (const auto& sweep : metrics.paramSweeps) {
            if (sweep.clipping_detected) {
                metrics.warnings.push_back("Hard clipping detected in " + sweep.paramName);
                score -= 10;
            }
        }

        // Check oversampling
        if (metrics.oversamplingQuality.quality == "excellent") {
            metrics.strengths.push_back("Excellent oversampling (no aliasing)");
            score += 10;
        } else if (metrics.oversamplingQuality.quality == "poor") {
            metrics.warnings.push_back("Poor oversampling quality - aliasing detected");
            score -= 15;
        }

        // Check noise floor
        if (metrics.noise_floor_dB < -100.0f) {
            metrics.strengths.push_back("Excellent noise floor: " +
                std::to_string((int)metrics.noise_floor_dB) + " dB");
        } else if (metrics.noise_floor_dB > -60.0f) {
            metrics.warnings.push_back("High noise floor: " +
                std::to_string((int)metrics.noise_floor_dB) + " dB");
            score -= 20;
        }

        // Check for musical harmonic content
        if (!metrics.harmonicProfile.spectra.empty()) {
            auto& highDrive = metrics.harmonicProfile.spectra.back();
            if (highDrive.thd_percent > 0.1f && highDrive.thd_percent < 50.0f) {
                metrics.strengths.push_back("Good harmonic distortion characteristics");
            }

            if (highDrive.even_harmonic_ratio > 0.6f || highDrive.odd_harmonic_ratio > 0.6f) {
                metrics.strengths.push_back("Distinct harmonic character");
            }
        }

        metrics.quality_score = std::max(0, std::min(100, score));
        metrics.passed = (score >= 60);
    }
};

void saveDetailedReport(const std::vector<DistortionMetrics>& allMetrics) {
    std::ofstream report("DISTORTION_PARAMETER_VALIDATION_REPORT.md");

    report << "# DISTORTION ENGINES DEEP VALIDATION REPORT\n\n";
    report << "**Generated:** " << std::chrono::system_clock::now().time_since_epoch().count() << "\n";
    report << "**Test Suite:** Comprehensive Parameter & Harmonic Analysis\n\n";

    report << "## Executive Summary\n\n";

    int passed = 0;
    int totalScore = 0;
    for (const auto& m : allMetrics) {
        if (m.passed) passed++;
        totalScore += m.quality_score;
    }

    report << "- **Engines Tested:** " << allMetrics.size() << "\n";
    report << "- **Passed:** " << passed << "/" << allMetrics.size() << "\n";
    report << "- **Average Quality Score:** " << (totalScore / (int)allMetrics.size()) << "/100\n\n";

    report << "---\n\n";

    // Detailed results for each engine
    for (const auto& m : allMetrics) {
        report << "## Engine " << m.engineID << ": " << m.engineName << "\n\n";
        report << "**Status:** " << (m.passed ? "✅ PASSED" : "❌ FAILED") << "\n";
        report << "**Quality Score:** " << m.quality_score << "/100\n\n";

        // Strengths
        if (!m.strengths.empty()) {
            report << "### Strengths\n\n";
            for (const auto& s : m.strengths) {
                report << "- ✨ " << s << "\n";
            }
            report << "\n";
        }

        // Warnings
        if (!m.warnings.empty()) {
            report << "### Warnings\n\n";
            for (const auto& w : m.warnings) {
                report << "- ⚠️ " << w << "\n";
            }
            report << "\n";
        }

        // Parameter Analysis
        report << "### Parameter Analysis\n\n";
        report << "| Parameter | Behavior | THD Range | RMS Range | Clipping |\n";
        report << "|-----------|----------|-----------|-----------|----------|\n";
        for (const auto& sweep : m.paramSweeps) {
            report << "| " << sweep.paramName << " | " << sweep.behavior << " | ";
            if (!sweep.thd_values.empty()) {
                float minTHD = *std::min_element(sweep.thd_values.begin(), sweep.thd_values.end());
                float maxTHD = *std::max_element(sweep.thd_values.begin(), sweep.thd_values.end());
                report << std::fixed << std::setprecision(2) << minTHD << "-" << maxTHD << "% | ";
            } else {
                report << "N/A | ";
            }
            if (!sweep.output_rms.empty()) {
                float minRMS = *std::min_element(sweep.output_rms.begin(), sweep.output_rms.end());
                float maxRMS = *std::max_element(sweep.output_rms.begin(), sweep.output_rms.end());
                report << std::fixed << std::setprecision(3) << minRMS << "-" << maxRMS << " | ";
            } else {
                report << "N/A | ";
            }
            report << (sweep.clipping_detected ? "⚠️ Yes" : "✅ No") << " |\n";
        }
        report << "\n";

        // Saturation Curve
        report << "### Saturation Curve\n\n";
        report << "- **Type:** " << m.saturationCurve.curve_type << "\n";
        report << "- **Compression Ratio:** " << std::fixed << std::setprecision(2)
               << m.saturationCurve.compression_ratio << ":1\n\n";

        // Harmonic Profile
        if (!m.harmonicProfile.spectra.empty()) {
            report << "### Harmonic Profile\n\n";
            report << "| Drive | THD% | Even% | Odd% |\n";
            report << "|-------|------|-------|------|\n";
            for (size_t i = 0; i < m.harmonicProfile.drive_levels.size(); ++i) {
                auto& spec = m.harmonicProfile.spectra[i];
                report << "| " << std::fixed << std::setprecision(2)
                       << m.harmonicProfile.drive_levels[i] << " | "
                       << spec.thd_percent << "% | "
                       << (spec.even_harmonic_ratio * 100.0f) << "% | "
                       << (spec.odd_harmonic_ratio * 100.0f) << "% |\n";
            }
            report << "\n";
        }

        // Oversampling Quality
        report << "### Anti-Aliasing\n\n";
        report << "- **Quality:** " << m.oversamplingQuality.quality << "\n";
        report << "- **Aliasing Level:** " << std::fixed << std::setprecision(1)
               << m.oversamplingQuality.aliasing_level_dB << " dB\n\n";

        // Noise Floor
        report << "### Noise Performance\n\n";
        report << "- **Noise Floor:** " << std::fixed << std::setprecision(1)
               << m.noise_floor_dB << " dB\n\n";

        report << "---\n\n";
    }

    report.close();
    std::cout << "\n✅ Detailed report saved to: DISTORTION_PARAMETER_VALIDATION_REPORT.md\n";
}

} // namespace DeepValidation

int main() {
    using namespace DeepValidation;

    std::vector<std::pair<int, std::string>> distortionEngines = {
        {15, "Vintage Tube Preamp Studio"},
        {16, "Wave Folder"},
        {17, "Harmonic Exciter Platinum"},
        {18, "Bit Crusher"},
        {19, "Multiband Saturator"},
        {20, "Muff Fuzz"},
        {21, "Rodent Distortion"},
        {22, "K-Style Overdrive"}
    };

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  DISTORTION ENGINES DEEP VALIDATION                        ║\n";
    std::cout << "║  Comprehensive Parameter & Harmonic Analysis               ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";

    DistortionValidator validator;
    std::vector<DistortionMetrics> allMetrics;

    for (const auto& [id, name] : distortionEngines) {
        auto metrics = validator.validateEngine(id, name);
        allMetrics.push_back(metrics);
    }

    // Save comprehensive report
    saveDetailedReport(allMetrics);

    // Summary
    int passed = 0;
    int totalScore = 0;
    for (const auto& m : allMetrics) {
        if (m.passed) passed++;
        totalScore += m.quality_score;
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  VALIDATION SUMMARY                                        ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n  Total Engines:     " << allMetrics.size() << "\n";
    std::cout << "  Passed:            " << passed << "\n";
    std::cout << "  Failed:            " << (allMetrics.size() - passed) << "\n";
    std::cout << "  Average Score:     " << (totalScore / (int)allMetrics.size()) << "/100\n\n";

    return (passed == (int)allMetrics.size()) ? 0 : 1;
}
