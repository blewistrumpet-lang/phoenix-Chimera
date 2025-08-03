#include "ParameterSweepTest.h"
#include "EngineFactory.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace ParameterSweepTest {

// =============================================================================
// SweepResult Implementation
// =============================================================================

void SweepResult::analyzeResults() {
    if (measurements.empty()) return;
    
    std::vector<float> values;
    std::vector<float> params;
    
    for (const auto& point : measurements) {
        if (point.isValid) {
            values.push_back(point.measuredValue);
            params.push_back(point.parameterValue);
        }
    }
    
    if (values.size() < 2) {
        isEffective = false;
        return;
    }
    
    // Calculate basic statistics
    auto minMax = std::minmax_element(values.begin(), values.end());
    totalRange = *minMax.second - *minMax.first;
    
    // Calculate average change per step
    float totalChange = 0.0f;
    for (size_t i = 1; i < values.size(); ++i) {
        totalChange += std::abs(values[i] - values[i-1]);
    }
    averageChange = totalChange / (values.size() - 1);
    
    // Find maximum single step change
    maxChange = 0.0f;
    for (size_t i = 1; i < values.size(); ++i) {
        float change = std::abs(values[i] - values[i-1]);
        maxChange = std::max(maxChange, change);
    }
    
    // Calculate monotonicity using correlation
    monotonicity = ParameterAnalysis::calculateCorrelationCoefficient(params, values);
    
    // Calculate sensitivity (normalized total range)
    float valueRange = *minMax.second - *minMax.first;
    float avgValue = std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
    sensitivity = (avgValue > 0) ? valueRange / avgValue : valueRange;
    
    // Determine if parameter is effective
    isEffective = (totalRange > 0.01f && averageChange > 0.001f);
    
    // Analyze curve properties
    smoothness = ParameterAnalysis::calculateSmoothness(values);
    linearity = std::abs(monotonicity); // Higher correlation = more linear
    consistency = 1.0f - ParameterAnalysis::calculateStandardDeviation(values) / (avgValue + 1e-6f);
    
    // Generate analysis notes
    std::ostringstream notes;
    notes << "Range: " << totalRange << ", ";
    notes << "Sensitivity: " << sensitivity << ", ";
    notes << "Monotonicity: " << monotonicity << ", ";
    notes << "Effective: " << (isEffective ? "Yes" : "No");
    analysisNotes = notes.str();
}

float SweepResult::calculateCorrelation() const {
    std::vector<float> params, values;
    for (const auto& point : measurements) {
        if (point.isValid) {
            params.push_back(point.parameterValue);
            values.push_back(point.measuredValue);
        }
    }
    return ParameterAnalysis::calculateCorrelationCoefficient(params, values);
}

std::string SweepResult::generateSummary() const {
    std::ostringstream summary;
    summary << "Parameter: " << config.parameterName << "\n";
    summary << "Effective: " << (isEffective ? "Yes" : "No") << "\n";
    summary << "Range: " << std::fixed << std::setprecision(3) << totalRange << "\n";
    summary << "Sensitivity: " << sensitivity << "\n";
    summary << "Monotonicity: " << monotonicity << "\n";
    summary << "Smoothness: " << smoothness << "\n";
    summary << "Linearity: " << linearity << "\n";
    summary << "Quality: " << ParameterAnalysis::assessParameterQuality(*this) << "\n";
    return summary.str();
}

// =============================================================================
// EngineSweepResults Implementation
// =============================================================================

void EngineSweepResults::calculateOverallMetrics() {
    if (parameterResults.empty()) return;
    
    effectiveParameterCount = 0;
    float totalSensitivity = 0.0f;
    int validParams = 0;
    
    for (const auto& result : parameterResults) {
        if (result.isEffective) {
            effectiveParameterCount++;
        }
        if (!result.measurements.empty()) {
            totalSensitivity += result.sensitivity;
            validParams++;
        }
    }
    
    averageSensitivity = (validParams > 0) ? totalSensitivity / validParams : 0.0f;
    allParametersWorking = (effectiveParameterCount == static_cast<int>(parameterResults.size()));
    
    // Calculate overall quality score
    float qualitySum = 0.0f;
    for (const auto& result : parameterResults) {
        qualitySum += ParameterAnalysis::assessParameterQuality(result);
    }
    overallQuality = (parameterResults.size() > 0) ? qualitySum / parameterResults.size() : 0.0f;
}

std::string EngineSweepResults::generateReport() const {
    std::ostringstream report;
    report << "=== Engine Parameter Sweep Report ===\n";
    report << "Engine: " << engineName << "\n";
    report << "Engine Type: " << engineType << "\n";
    report << "Total Parameters: " << parameterResults.size() << "\n";
    report << "Effective Parameters: " << effectiveParameterCount << "\n";
    report << "Average Sensitivity: " << std::fixed << std::setprecision(3) << averageSensitivity << "\n";
    report << "Overall Quality: " << overallQuality << "\n";
    report << "All Parameters Working: " << (allParametersWorking ? "Yes" : "No") << "\n";
    report << "Test Duration: " << testDurationMs << " ms\n\n";
    
    report << "=== Parameter Details ===\n";
    for (size_t i = 0; i < parameterResults.size(); ++i) {
        const auto& result = parameterResults[i];
        report << "Parameter " << i << ": " << result.config.parameterName << "\n";
        report << "  Effective: " << (result.isEffective ? "Yes" : "No") << "\n";
        report << "  Range: " << result.totalRange << "\n";
        report << "  Sensitivity: " << result.sensitivity << "\n";
        report << "  Quality: " << ParameterAnalysis::assessParameterQuality(result) << "\n";
        if (!result.isEffective) {
            report << "  Issue: Parameter may not be functioning correctly\n";
        }
        report << "\n";
    }
    
    return report.str();
}

// =============================================================================
// ParameterSweeper Implementation
// =============================================================================

ParameterSweeper::ParameterSweeper() {
    // Register built-in measurement functions
    registerCustomMeasurement("peak_frequency", [](const juce::AudioBuffer<float>& original, const juce::AudioBuffer<float>& processed) {
        auto spectrum = AudioMeasurements::computeFrequencyResponse(processed, 44100.0);
        return AudioMeasurements::findPeakFrequency(spectrum.magnitudes, 44100.0);
    });
    
    registerCustomMeasurement("spectral_centroid", [](const juce::AudioBuffer<float>& original, const juce::AudioBuffer<float>& processed) {
        auto spectrum = AudioMeasurements::computeFrequencyResponse(processed, 44100.0);
        float centroid = 0.0f;
        float totalMagnitude = 0.0f;
        
        for (size_t i = 0; i < spectrum.magnitudes.size(); ++i) {
            float freq = (float)i * 44100.0f / (2.0f * spectrum.magnitudes.size());
            centroid += freq * spectrum.magnitudes[i];
            totalMagnitude += spectrum.magnitudes[i];
        }
        
        return (totalMagnitude > 0) ? centroid / totalMagnitude : 0.0f;
    });
}

EngineSweepResults ParameterSweeper::testAllParameters(std::unique_ptr<EngineBase> engine, double sampleRate) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    EngineSweepResults results;
    results.engineName = engine->getName().toStdString();
    results.engineType = -1; // Will be set by caller
    
    // Generate configurations for all parameters
    auto configs = generateConfigsForEngine(engine.get(), results.engineType);
    
    // Test each parameter
    for (const auto& config : configs) {
        auto sweepResult = testSingleParameter(engine.get(), config, sampleRate);
        results.parameterResults.push_back(sweepResult);
    }
    
    // Calculate overall metrics
    results.calculateOverallMetrics();
    
    auto endTime = std::chrono::high_resolution_clock::now();
    results.testDurationMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    
    return results;
}

SweepResult ParameterSweeper::testSingleParameter(EngineBase* engine, const SweepConfig& config, double sampleRate) {
    SweepResult result;
    result.config = config;
    
    // Generate test signal
    auto testSignal = generateTestSignal(config, sampleRate);
    
    // Sweep through parameter values
    for (int step = 0; step < config.numSteps; ++step) {
        float t = static_cast<float>(step) / (config.numSteps - 1);
        float paramValue = config.minValue + t * (config.maxValue - config.minValue);
        
        MeasurementPoint point;
        point.parameterValue = paramValue;
        
        try {
            // Reset engine and set parameter
            engine->reset();
            std::map<int, float> params;
            params[config.parameterIndex] = paramValue;
            engine->updateParameters(params);
            
            // Process test signal
            auto processedSignal = testSignal;
            engine->process(processedSignal);
            
            // Measure the result
            point.measuredValue = measureParameter(testSignal, processedSignal, config.measurementType, config);
            point.isValid = !std::isnan(point.measuredValue) && !std::isinf(point.measuredValue);
            
        } catch (const std::exception& e) {
            point.isValid = false;
            point.notes = "Exception: " + std::string(e.what());
        }
        
        result.measurements.push_back(point);
    }
    
    // Analyze results
    result.analyzeResults();
    
    return result;
}

void ParameterSweeper::registerCustomMeasurement(const std::string& name, 
                                                std::function<float(const juce::AudioBuffer<float>&, const juce::AudioBuffer<float>&)> func) {
    m_customMeasurements[name] = func;
}

std::vector<SweepConfig> ParameterSweeper::generateConfigsForEngine(EngineBase* engine, int engineType) {
    std::vector<SweepConfig> configs;
    
    // Determine engine category and generate appropriate configs
    if (engineType >= ENGINE_OPTO_COMPRESSOR && engineType <= ENGINE_MASTERING_LIMITER) {
        configs = getDynamicsConfigs(engine);
    } else if (engineType >= ENGINE_PARAMETRIC_EQ && engineType <= ENGINE_FORMANT_FILTER) {
        configs = getFilterConfigs(engine);
    } else if (engineType >= ENGINE_TAPE_ECHO && engineType <= ENGINE_GATED_REVERB) {
        configs = getTimeBasedConfigs(engine);
    } else if (engineType >= ENGINE_DIGITAL_CHORUS && engineType <= ENGINE_STEREO_IMAGER) {
        configs = getModulationConfigs(engine);
    } else if (engineType >= ENGINE_VINTAGE_TUBE && engineType <= ENGINE_K_STYLE) {
        configs = getDistortionConfigs(engine);
    } else if (engineType >= ENGINE_PITCH_SHIFTER && engineType <= ENGINE_DETUNE_DOUBLER) {
        configs = getSpectralConfigs(engine);
    } else {
        // Default configuration for unknown engine types
        for (int i = 0; i < engine->getNumParameters(); ++i) {
            SweepConfig config;
            config.parameterIndex = i;
            config.parameterName = engine->getParameterName(i).toStdString();
            config.numSteps = m_defaultSteps;
            config.testSignalType = SweepConfig::SINE_WAVE;
            config.measurementType = MeasurementType::RMS_LEVEL;
            configs.push_back(config);
        }
    }
    
    return configs;
}

float ParameterSweeper::measureParameter(const juce::AudioBuffer<float>& original, const juce::AudioBuffer<float>& processed, 
                                       MeasurementType type, const SweepConfig& config) {
    switch (type) {
        case MeasurementType::RMS_LEVEL:
            return AudioMeasurements::measureRMS(processed);
            
        case MeasurementType::PEAK_LEVEL:
            return AudioMeasurements::measurePeak(processed);
            
        case MeasurementType::FREQUENCY_CONTENT: {
            auto spectrum = AudioMeasurements::computeFrequencyResponse(processed, 44100.0);
            return std::accumulate(spectrum.magnitudes.begin(), spectrum.magnitudes.end(), 0.0f);
        }
        
        case MeasurementType::HARMONIC_CONTENT:
            return AudioMeasurements::measureTHD(processed, config.testFrequency, 44100.0);
            
        case MeasurementType::PHASE_RESPONSE: {
            auto originalSpectrum = AudioMeasurements::computeFrequencyResponse(original, 44100.0);
            auto processedSpectrum = AudioMeasurements::computeFrequencyResponse(processed, 44100.0);
            
            float avgPhaseDiff = 0.0f;
            size_t validBins = 0;
            for (size_t i = 0; i < std::min(originalSpectrum.phases.size(), processedSpectrum.phases.size()); ++i) {
                if (originalSpectrum.magnitudes[i] > 0.01f) {
                    avgPhaseDiff += std::abs(processedSpectrum.phases[i] - originalSpectrum.phases[i]);
                    validBins++;
                }
            }
            return (validBins > 0) ? avgPhaseDiff / validBins : 0.0f;
        }
        
        case MeasurementType::TRANSIENT_RESPONSE: {
            float originalCrest = AudioMeasurements::measurePeak(original) / (AudioMeasurements::measureRMS(original) + 1e-6f);
            float processedCrest = AudioMeasurements::measurePeak(processed) / (AudioMeasurements::measureRMS(processed) + 1e-6f);
            return processedCrest / (originalCrest + 1e-6f);
        }
        
        case MeasurementType::NOISE_FLOOR:
            return AudioMeasurements::measureNoiseFloor(processed);
            
        case MeasurementType::CORRELATION:
            if (processed.getNumChannels() >= 2) {
                return AudioMeasurements::correlate(processed.getReadPointer(0), processed.getReadPointer(1), processed.getNumSamples());
            }
            return 1.0f;
            
        case MeasurementType::DELAY_TIME:
            return AudioMeasurements::measureDelayTime(original, processed, 44100.0);
            
        case MeasurementType::MODULATION_DEPTH: {
            auto modProfile = AudioMeasurements::extractModulationProfile(processed, 44100.0);
            return modProfile.depth;
        }
        
        case MeasurementType::CUSTOM:
            if (config.customMeasurement) {
                return config.customMeasurement(original, processed);
            }
            return 0.0f;
            
        default:
            return AudioMeasurements::measureRMS(processed);
    }
}

juce::AudioBuffer<float> ParameterSweeper::generateTestSignal(const SweepConfig& config, double sampleRate) {
    switch (config.testSignalType) {
        case SweepConfig::SINE_WAVE:
            return TestSignalGenerator::generateSineWave(config.testFrequency, config.testDuration, sampleRate, config.testAmplitude);
            
        case SweepConfig::WHITE_NOISE:
            return TestSignalGenerator::generateWhiteNoise(config.testDuration, sampleRate, config.testAmplitude);
            
        case SweepConfig::PINK_NOISE:
            return TestSignalGenerator::generatePinkNoise(config.testDuration, sampleRate, config.testAmplitude);
            
        case SweepConfig::IMPULSE:
            return TestSignalGenerator::generateImpulse(sampleRate, config.testAmplitude);
            
        case SweepConfig::SWEEP:
            return TestSignalGenerator::generateSweep(20.0f, 20000.0f, config.testDuration, sampleRate, config.testAmplitude);
            
        case SweepConfig::CHORD:
            return TestSignalGenerator::generateChord(config.testFrequency, config.testDuration, sampleRate);
            
        case SweepConfig::DRUM_HIT:
            return TestSignalGenerator::generateDrumHit(sampleRate);
            
        case SweepConfig::TWO_TONE:
            return TestSignalGenerator::generateTwoTone(config.testFrequency, config.testFrequency * 1.2f, config.testDuration, sampleRate);
            
        case SweepConfig::CUSTOM_SIGNAL:
            return config.customTestSignal;
            
        default:
            return TestSignalGenerator::generateSineWave(config.testFrequency, config.testDuration, sampleRate, config.testAmplitude);
    }
}

std::vector<SweepConfig> ParameterSweeper::getDynamicsConfigs(EngineBase* engine) {
    std::vector<SweepConfig> configs;
    
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        SweepConfig config;
        config.parameterIndex = i;
        config.parameterName = engine->getParameterName(i).toStdString();
        config.numSteps = m_defaultSteps;
        config.testSignalType = SweepConfig::SINE_WAVE;
        config.testFrequency = 1000.0f;
        config.testAmplitude = 0.7f; // Loud enough to trigger compression
        
        // Choose measurement type based on parameter name/index
        if (i == 0) { // Usually threshold
            config.measurementType = MeasurementType::RMS_LEVEL;
        } else if (i == 1) { // Usually ratio
            config.measurementType = MeasurementType::RMS_LEVEL;
        } else if (i == 2 || i == 3) { // Attack/Release
            config.measurementType = MeasurementType::TRANSIENT_RESPONSE;
            config.testSignalType = SweepConfig::DRUM_HIT;
        } else {
            config.measurementType = MeasurementType::RMS_LEVEL;
        }
        
        configs.push_back(config);
    }
    
    return configs;
}

std::vector<SweepConfig> ParameterSweeper::getFilterConfigs(EngineBase* engine) {
    std::vector<SweepConfig> configs;
    
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        SweepConfig config;
        config.parameterIndex = i;
        config.parameterName = engine->getParameterName(i).toStdString();
        config.numSteps = m_defaultSteps;
        config.testSignalType = SweepConfig::PINK_NOISE; // Good for frequency response
        config.measurementType = MeasurementType::FREQUENCY_CONTENT;
        
        // Special cases for different parameter types
        if (i == 0) { // Usually frequency
            config.measurementType = MeasurementType::FREQUENCY_CONTENT;
        } else if (i == 1) { // Usually Q/resonance
            config.measurementType = MeasurementType::PEAK_LEVEL;
        } else { // Gain parameters
            config.measurementType = MeasurementType::RMS_LEVEL;
        }
        
        configs.push_back(config);
    }
    
    return configs;
}

std::vector<SweepConfig> ParameterSweeper::getTimeBasedConfigs(EngineBase* engine) {
    std::vector<SweepConfig> configs;
    
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        SweepConfig config;
        config.parameterIndex = i;
        config.parameterName = engine->getParameterName(i).toStdString();
        config.numSteps = m_defaultSteps;
        config.testSignalType = SweepConfig::IMPULSE; // Good for time-based effects
        config.measurementType = MeasurementType::DELAY_TIME;
        config.testDuration = 3.0f; // Longer for decay measurement
        
        // Parameter-specific configurations
        if (i == 0) { // Usually time/size
            config.measurementType = MeasurementType::DELAY_TIME;
        } else if (i == 1) { // Usually feedback/decay
            config.measurementType = MeasurementType::RMS_LEVEL;
        } else { // Mix parameters
            config.measurementType = MeasurementType::RMS_LEVEL;
            config.testSignalType = SweepConfig::SINE_WAVE;
        }
        
        configs.push_back(config);
    }
    
    return configs;
}

std::vector<SweepConfig> ParameterSweeper::getModulationConfigs(EngineBase* engine) {
    std::vector<SweepConfig> configs;
    
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        SweepConfig config;
        config.parameterIndex = i;
        config.parameterName = engine->getParameterName(i).toStdString();
        config.numSteps = m_defaultSteps;
        config.testSignalType = SweepConfig::SINE_WAVE;
        config.testFrequency = 440.0f;
        config.testDuration = 2.0f; // Longer to capture modulation
        
        // Parameter-specific measurements
        if (i == 0) { // Usually rate
            config.measurementType = MeasurementType::MODULATION_DEPTH;
        } else if (i == 1) { // Usually depth
            config.measurementType = MeasurementType::MODULATION_DEPTH;
        } else if (i == 2) { // Usually stereo width
            config.measurementType = MeasurementType::CORRELATION;
        } else {
            config.measurementType = MeasurementType::PHASE_RESPONSE;
        }
        
        configs.push_back(config);
    }
    
    return configs;
}

std::vector<SweepConfig> ParameterSweeper::getDistortionConfigs(EngineBase* engine) {
    std::vector<SweepConfig> configs;
    
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        SweepConfig config;
        config.parameterIndex = i;
        config.parameterName = engine->getParameterName(i).toStdString();
        config.numSteps = m_defaultSteps;
        config.testSignalType = SweepConfig::SINE_WAVE;
        config.testFrequency = 220.0f; // Lower frequency for harmonic analysis
        
        // Parameter-specific measurements
        if (i == 0) { // Usually drive/gain
            config.measurementType = MeasurementType::HARMONIC_CONTENT;
        } else if (i == 1) { // Usually tone/filter
            config.measurementType = MeasurementType::FREQUENCY_CONTENT;
            config.testSignalType = SweepConfig::PINK_NOISE;
        } else {
            config.measurementType = MeasurementType::RMS_LEVEL;
        }
        
        configs.push_back(config);
    }
    
    return configs;
}

std::vector<SweepConfig> ParameterSweeper::getSpectralConfigs(EngineBase* engine) {
    std::vector<SweepConfig> configs;
    
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        SweepConfig config;
        config.parameterIndex = i;
        config.parameterName = engine->getParameterName(i).toStdString();
        config.numSteps = m_defaultSteps;
        config.testSignalType = SweepConfig::CHORD; // Complex harmonic content
        
        // Parameter-specific measurements
        if (i == 0) { // Usually pitch/frequency shift
            config.measurementType = MeasurementType::FREQUENCY_CONTENT;
            config.customMeasurement = m_customMeasurements["peak_frequency"];
            config.measurementType = MeasurementType::CUSTOM;
        } else if (i == 1) { // Usually formant preservation
            config.measurementType = MeasurementType::FREQUENCY_CONTENT;
        } else {
            config.measurementType = MeasurementType::FREQUENCY_CONTENT;
        }
        
        configs.push_back(config);
    }
    
    return configs;
}

// =============================================================================
// ParameterAnalysis Implementation
// =============================================================================

namespace ParameterAnalysis {

float calculateMean(const std::vector<float>& values) {
    if (values.empty()) return 0.0f;
    return std::accumulate(values.begin(), values.end(), 0.0f) / values.size();
}

float calculateStandardDeviation(const std::vector<float>& values) {
    if (values.size() < 2) return 0.0f;
    
    float mean = calculateMean(values);
    float variance = 0.0f;
    
    for (float value : values) {
        float diff = value - mean;
        variance += diff * diff;
    }
    
    variance /= (values.size() - 1);
    return std::sqrt(variance);
}

float calculateCorrelationCoefficient(const std::vector<float>& x, const std::vector<float>& y) {
    if (x.size() != y.size() || x.size() < 2) return 0.0f;
    
    float meanX = calculateMean(x);
    float meanY = calculateMean(y);
    
    float numerator = 0.0f;
    float denomX = 0.0f;
    float denomY = 0.0f;
    
    for (size_t i = 0; i < x.size(); ++i) {
        float diffX = x[i] - meanX;
        float diffY = y[i] - meanY;
        
        numerator += diffX * diffY;
        denomX += diffX * diffX;
        denomY += diffY * diffY;
    }
    
    float denominator = std::sqrt(denomX * denomY);
    return (denominator > 1e-6f) ? numerator / denominator : 0.0f;
}

float calculateMonotonicity(const std::vector<float>& values) {
    if (values.size() < 2) return 0.0f;
    
    int increasing = 0;
    int decreasing = 0;
    
    for (size_t i = 1; i < values.size(); ++i) {
        if (values[i] > values[i-1]) increasing++;
        else if (values[i] < values[i-1]) decreasing++;
    }
    
    int total = increasing + decreasing;
    if (total == 0) return 0.0f;
    
    float maxDirection = std::max(increasing, decreasing);
    return (2.0f * maxDirection / total) - 1.0f; // -1 to 1 scale
}

LinearFit fitLinear(const std::vector<float>& x, const std::vector<float>& y) {
    LinearFit fit;
    
    if (x.size() != y.size() || x.size() < 2) return fit;
    
    float meanX = calculateMean(x);
    float meanY = calculateMean(y);
    
    float numerator = 0.0f;
    float denominator = 0.0f;
    
    for (size_t i = 0; i < x.size(); ++i) {
        float diffX = x[i] - meanX;
        numerator += diffX * (y[i] - meanY);
        denominator += diffX * diffX;
    }
    
    if (denominator > 1e-6f) {
        fit.slope = numerator / denominator;
        fit.intercept = meanY - fit.slope * meanX;
        
        // Calculate R-squared
        float ssRes = 0.0f;
        float ssTot = 0.0f;
        
        for (size_t i = 0; i < x.size(); ++i) {
            float predicted = fit.predict(x[i]);
            float residual = y[i] - predicted;
            float total = y[i] - meanY;
            
            ssRes += residual * residual;
            ssTot += total * total;
        }
        
        fit.rSquared = (ssTot > 1e-6f) ? 1.0f - (ssRes / ssTot) : 0.0f;
    }
    
    return fit;
}

float calculateCurvature(const std::vector<float>& values) {
    if (values.size() < 3) return 0.0f;
    
    float totalCurvature = 0.0f;
    
    for (size_t i = 1; i < values.size() - 1; ++i) {
        float secondDerivative = values[i+1] - 2*values[i] + values[i-1];
        totalCurvature += std::abs(secondDerivative);
    }
    
    return totalCurvature / (values.size() - 2);
}

float calculateSmoothness(const std::vector<float>& values) {
    if (values.size() < 2) return 1.0f;
    
    float curvature = calculateCurvature(values);
    float range = *std::max_element(values.begin(), values.end()) - *std::min_element(values.begin(), values.end());
    
    // Normalize curvature by range
    float normalizedCurvature = (range > 1e-6f) ? curvature / range : 0.0f;
    
    // Return smoothness (inverse of curvature)
    return 1.0f / (1.0f + normalizedCurvature);
}

bool isParameterEffective(const SweepResult& result, float threshold) {
    return result.totalRange > threshold && result.averageChange > threshold * 0.1f;
}

float assessParameterQuality(const SweepResult& result) {
    if (!result.isEffective) return 0.0f;
    
    float qualityScore = 0.0f;
    
    // Effectiveness (40% of score)
    qualityScore += 0.4f * std::min(1.0f, result.sensitivity);
    
    // Monotonicity (30% of score)
    qualityScore += 0.3f * std::abs(result.monotonicity);
    
    // Smoothness (20% of score)
    qualityScore += 0.2f * result.smoothness;
    
    // Consistency (10% of score)
    qualityScore += 0.1f * result.consistency;
    
    return std::min(1.0f, qualityScore);
}

std::string classifyParameterBehavior(const SweepResult& result) {
    if (!result.isEffective) {
        return "Ineffective";
    }
    
    if (std::abs(result.monotonicity) > 0.8f) {
        return "Monotonic";
    } else if (result.smoothness > 0.8f) {
        return "Smooth Non-monotonic";
    } else if (result.sensitivity > 0.5f) {
        return "Highly Sensitive";
    } else {
        return "Complex Response";
    }
}

std::vector<std::pair<float, float>> normalizeForPlotting(const SweepResult& result) {
    std::vector<std::pair<float, float>> points;
    
    if (result.measurements.empty()) return points;
    
    // Find value range for normalization
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();
    
    for (const auto& point : result.measurements) {
        if (point.isValid) {
            minVal = std::min(minVal, point.measuredValue);
            maxVal = std::max(maxVal, point.measuredValue);
        }
    }
    
    float range = maxVal - minVal;
    if (range < 1e-6f) range = 1.0f;
    
    // Normalize and create point pairs
    for (const auto& point : result.measurements) {
        if (point.isValid) {
            float normalizedValue = (point.measuredValue - minVal) / range;
            points.emplace_back(point.parameterValue, normalizedValue);
        }
    }
    
    return points;
}

std::string generateDataForPlot(const SweepResult& result, const std::string& format) {
    auto points = normalizeForPlotting(result);
    
    if (format == "json") {
        std::ostringstream json;
        json << "{\n";
        json << "  \"parameterName\": \"" << result.config.parameterName << "\",\n";
        json << "  \"data\": [\n";
        
        for (size_t i = 0; i < points.size(); ++i) {
            json << "    {\"x\": " << points[i].first << ", \"y\": " << points[i].second << "}";
            if (i < points.size() - 1) json << ",";
            json << "\n";
        }
        
        json << "  ],\n";
        json << "  \"metadata\": {\n";
        json << "    \"effective\": " << (result.isEffective ? "true" : "false") << ",\n";
        json << "    \"sensitivity\": " << result.sensitivity << ",\n";
        json << "    \"monotonicity\": " << result.monotonicity << ",\n";
        json << "    \"quality\": " << assessParameterQuality(result) << "\n";
        json << "  }\n";
        json << "}";
        
        return json.str();
    } else if (format == "csv") {
        std::ostringstream csv;
        csv << "Parameter Value,Measured Value,Normalized Value\n";
        
        for (const auto& point : points) {
            csv << point.first << "," << point.second << "," << point.second << "\n";
        }
        
        return csv.str();
    }
    
    return "";
}

} // namespace ParameterAnalysis

// =============================================================================
// VisualProofGenerator Implementation
// =============================================================================

VisualProofGenerator::VisualProof VisualProofGenerator::generateProofForParameter(const SweepResult& result, EngineBase* engine, double sampleRate) {
    VisualProof proof;
    proof.parameterName = result.config.parameterName;
    proof.effectivenessScore = ParameterAnalysis::assessParameterQuality(result);
    
    // Generate plot data
    proof.plotData = ParameterAnalysis::generateDataForPlot(result, "json");
    
    // Generate analysis text
    proof.analysisText = formatAnalysisText(result);
    
    return proof;
}

std::string VisualProofGenerator::generateHTMLReport(const EngineSweepResults& results) {
    std::ostringstream html;
    
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<title>Parameter Sweep Report - " << results.engineName << "</title>\n";
    html << "<script src=\"https://cdn.plot.ly/plotly-latest.min.js\"></script>\n";
    html << "<style>\n";
    html << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    html << ".parameter-section { margin: 20px 0; border: 1px solid #ddd; padding: 15px; }\n";
    html << ".effective { border-left: 5px solid #4CAF50; }\n";
    html << ".ineffective { border-left: 5px solid #f44336; }\n";
    html << ".plot-container { width: 600px; height: 400px; margin: 10px 0; }\n";
    html << "</style>\n";
    html << "</head>\n<body>\n";
    
    // Header
    html << "<h1>Parameter Sweep Report</h1>\n";
    html << "<h2>" << results.engineName << "</h2>\n";
    html << "<p><strong>Overall Quality:</strong> " << std::fixed << std::setprecision(2) << results.overallQuality << "</p>\n";
    html << "<p><strong>Effective Parameters:</strong> " << results.effectiveParameterCount << "/" << results.parameterResults.size() << "</p>\n";
    html << "<p><strong>Test Duration:</strong> " << results.testDurationMs << " ms</p>\n";
    
    // Parameter sections
    for (size_t i = 0; i < results.parameterResults.size(); ++i) {
        const auto& result = results.parameterResults[i];
        
        html << "<div class=\"parameter-section " << (result.isEffective ? "effective" : "ineffective") << "\">\n";
        html << "<h3>Parameter " << i << ": " << result.config.parameterName << "</h3>\n";
        html << "<p><strong>Status:</strong> " << (result.isEffective ? "Effective" : "Not Effective") << "</p>\n";
        html << "<p><strong>Quality Score:</strong> " << ParameterAnalysis::assessParameterQuality(result) << "</p>\n";
        html << "<p><strong>Behavior:</strong> " << ParameterAnalysis::classifyParameterBehavior(result) << "</p>\n";
        
        // Plot
        html << "<div id=\"plot" << i << "\" class=\"plot-container\"></div>\n";
        html << "<script>\n";
        html << "var data" << i << " = " << ParameterAnalysis::generateDataForPlot(result, "json") << ";\n";
        html << "var trace" << i << " = {\n";
        html << "  x: data" << i << ".data.map(d => d.x),\n";
        html << "  y: data" << i << ".data.map(d => d.y),\n";
        html << "  type: 'scatter',\n";
        html << "  mode: 'lines+markers',\n";
        html << "  name: '" << result.config.parameterName << "'\n";
        html << "};\n";
        html << "var layout" << i << " = {\n";
        html << "  title: 'Parameter Response Curve',\n";
        html << "  xaxis: { title: 'Parameter Value (0-1)' },\n";
        html << "  yaxis: { title: 'Normalized Response' }\n";
        html << "};\n";
        html << "Plotly.newPlot('plot" << i << "', [trace" << i << "], layout" << i << ");\n";
        html << "</script>\n";
        
        html << "</div>\n";
    }
    
    html << "</body>\n</html>";
    return html.str();
}

std::string VisualProofGenerator::formatAnalysisText(const SweepResult& result) {
    std::ostringstream text;
    
    text << "Parameter Analysis:\n";
    text << "- Effectiveness: " << (result.isEffective ? "YES" : "NO") << "\n";
    text << "- Total Range: " << std::fixed << std::setprecision(3) << result.totalRange << "\n";
    text << "- Sensitivity: " << result.sensitivity << "\n";
    text << "- Monotonicity: " << result.monotonicity << "\n";
    text << "- Smoothness: " << result.smoothness << "\n";
    text << "- Quality Score: " << ParameterAnalysis::assessParameterQuality(result) << "\n";
    text << "- Behavior: " << ParameterAnalysis::classifyParameterBehavior(result) << "\n";
    
    if (!result.isEffective) {
        text << "\nISSUE: This parameter may not be functioning correctly.\n";
        text << "The measured changes are too small to be significant.\n";
    }
    
    return text.str();
}

} // namespace ParameterSweepTest