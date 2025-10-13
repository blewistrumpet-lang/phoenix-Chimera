# ChimeraPhoenix Testing Methodology
## Comprehensive Guide to DSP Engine Quality Assessment

**Document Version**: 2.0
**Last Updated**: October 11, 2025
**Author**: ChimeraPhoenix QA Team

---

## Table of Contents

1. [Overview](#overview)
2. [Test Signal Generation](#test-signal-generation)
3. [Impulse Response Testing](#impulse-response-testing)
4. [Sine Wave Testing](#sine-wave-testing)
5. [THD Measurement](#thd-measurement)
6. [CPU Benchmarking](#cpu-benchmarking)
7. [Stereo Analysis](#stereo-analysis)
8. [Interpretation Guidelines](#interpretation-guidelines)
9. [Pass/Fail Criteria](#passfail-criteria)
10. [Engine-Specific Testing](#engine-specific-testing)
11. [Regression Testing](#regression-testing)
12. [Troubleshooting Guide](#troubleshooting-guide)

---

## Overview

### Purpose

This document provides comprehensive testing methodologies for validating the quality and functionality of ChimeraPhoenix's 56 DSP engines. It covers test signal generation, analysis techniques, interpretation guidelines, and pass/fail criteria for production readiness.

### Testing Philosophy

1. **Multi-faceted Testing**: Use multiple test types to reveal different aspects of quality
2. **Objective Metrics**: Rely on measurable criteria with clear thresholds
3. **Domain-Specific Tests**: Apply specialized tests to specific engine categories
4. **Regression Protection**: Maintain test history to prevent quality degradation
5. **Real-World Scenarios**: Test with realistic parameters and use cases

### Quality Thresholds Summary

| Metric | Clean Engines | Musical Effects | Distortion | Notes |
|--------|---------------|-----------------|------------|-------|
| **THD** | < 0.1% | < 1.0% | Any | Intentional for distortion |
| **CPU Usage** | < 5.0% | < 10.0% | < 5.0% | Per engine at 48kHz/512 |
| **Latency** | < 10ms | < 50ms | < 10ms | Real-time requirement |
| **Stereo Correlation** | 0.9-1.0 (mono) | < 0.5 | N/A | < 0.3 ideal for spatial |
| **Frequency Response** | ±0.5dB | ±1.0dB | N/A | At passband |

---

## Test Signal Generation

### Overview

Quality test signal generation is fundamental to accurate testing. Poor test signals produce misleading results.

### 1. Impulse Signal

**Purpose**: Reveals time-domain response, reverb tails, echoes, latency

**Generation**:
```cpp
void generateImpulse(juce::AudioBuffer<float>& buffer) {
    buffer.clear();

    // Single sample impulse at start
    buffer.setSample(0, 0, 1.0f);  // Left channel
    buffer.setSample(1, 0, 1.0f);  // Right channel

    // Rest of buffer remains at zero
}
```

**Characteristics**:
- Single sample at maximum amplitude (1.0)
- All other samples at zero
- Contains all frequencies equally (flat spectrum)
- Ideal for impulse response analysis

**Use Cases**:
- Reverb RT60 measurement
- Echo density analysis
- Latency detection
- Pre-delay measurement
- Stereo width analysis

---

### 2. Sine Wave Signal

**Purpose**: Frequency response, THD, phase measurement, filter testing

**Generation**:
```cpp
void generateSineWave(juce::AudioBuffer<float>& buffer,
                      float frequency,
                      float sampleRate,
                      float amplitude = 0.5f) {
    int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        float phase = 2.0f * M_PI * frequency * i / sampleRate;
        float sample = amplitude * std::sin(phase);

        // Write to both channels
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
}
```

**Characteristics**:
- Pure single frequency
- Amplitude typically -6dBFS (0.5) for headroom
- Phase-coherent across channels
- Duration: minimum 1 second for accurate FFT

**Common Test Frequencies**:
```cpp
// Standard test frequencies (Hz)
std::vector<float> testFrequencies = {
    100.0f,    // Low frequency
    440.0f,    // Musical A4
    1000.0f,   // Standard reference (1kHz)
    5000.0f,   // Upper midrange
    10000.0f,  // High frequency
    15000.0f   // Near Nyquist (at 48kHz)
};
```

**Use Cases**:
- THD measurement (use 1kHz)
- Frequency response curves
- Filter cutoff verification
- Phase response
- Harmonic analysis

---

### 3. Swept Sine (Chirp)

**Purpose**: Automated frequency response measurement

**Generation**:
```cpp
void generateSweptSine(juce::AudioBuffer<float>& buffer,
                       float startFreq,
                       float endFreq,
                       float sampleRate) {
    int numSamples = buffer.getNumSamples();
    float duration = numSamples / sampleRate;

    for (int i = 0; i < numSamples; ++i) {
        float t = i / sampleRate;

        // Log sweep: frequency increases exponentially
        float k = std::log(endFreq / startFreq) / duration;
        float instantFreq = startFreq * std::exp(k * t);
        float phase = 2.0f * M_PI * startFreq * (std::exp(k * t) - 1.0f) / k;

        float sample = 0.5f * std::sin(phase);
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
}
```

**Characteristics**:
- Sweeps from low to high frequency
- Log sweep recommended (equal time per octave)
- Typical range: 20Hz to 20kHz
- Duration: 5-10 seconds

**Use Cases**:
- Full frequency response in single test
- Filter characterization
- Crossover measurement
- Automated testing

---

### 4. White Noise

**Purpose**: Broadband testing, reverb excitation

**Generation**:
```cpp
void generateWhiteNoise(juce::AudioBuffer<float>& buffer,
                        float amplitude = 0.25f) {
    juce::Random random;
    int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i) {
        // Uniform random [-1, +1]
        float sample = (random.nextFloat() * 2.0f - 1.0f) * amplitude;

        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
}
```

**Characteristics**:
- Equal energy at all frequencies
- Random phase
- Amplitude typically -12dBFS (0.25)
- Gaussian distribution

**Use Cases**:
- Reverb natural sound testing
- Noise gate testing
- Spectral analysis
- Real-world signal simulation

---

### 5. Pink Noise

**Purpose**: Perceptually flat spectrum testing

**Generation**:
```cpp
class PinkNoiseGenerator {
public:
    PinkNoiseGenerator() {
        for (int i = 0; i < 7; ++i) {
            rows[i] = 0.0f;
        }
        runningSum = 0.0f;
    }

    float getNextSample() {
        juce::Random random;
        float white = random.nextFloat() * 2.0f - 1.0f;

        // Voss-McCartney algorithm
        rows[0] = 0.99886f * rows[0] + white * 0.0555179f;
        rows[1] = 0.99332f * rows[1] + white * 0.0750759f;
        rows[2] = 0.96900f * rows[2] + white * 0.1538520f;
        rows[3] = 0.86650f * rows[3] + white * 0.3104856f;
        rows[4] = 0.55000f * rows[4] + white * 0.5329522f;
        rows[5] = -0.7616f * rows[5] - white * 0.0168980f;

        float pink = rows[0] + rows[1] + rows[2] + rows[3] + rows[4] + rows[5] + white * 0.5362f;

        return pink * 0.11f; // Scale down
    }

private:
    float rows[7];
    float runningSum;
};
```

**Characteristics**:
- -3dB per octave rolloff
- Equal energy per octave
- More realistic than white noise
- Better for listening tests

**Use Cases**:
- Reverb subjective testing
- EQ audibility testing
- Natural sound testing

---

## Impulse Response Testing

### Purpose

Impulse response testing is the **gold standard** for time-domain analysis. An impulse contains all frequencies and reveals:

1. **Time Domain Behavior**: Echoes, reverb tails, transient response
2. **Latency**: Processing delay
3. **Stereo Characteristics**: Channel decorrelation, width
4. **Reverb Quality**: RT60, early reflections, echo density
5. **Pre-delay**: Initial gap before reverb onset

### Complete Implementation

```cpp
struct ImpulseResponseTest {
    // Configuration
    double sampleRate = 48000.0;
    int blockSize = 512;
    int durationSeconds = 10;

    // Perform impulse response test
    juce::AudioBuffer<float> testEngine(EngineBase* engine) {
        // 1. Prepare engine
        engine->prepareToPlay(sampleRate, blockSize);
        engine->reset();

        // 2. Set to 100% wet mix for pure analysis
        if (engine->getNumParameters() > 0) {
            engine->setParameter(0, 1.0f);  // MIX = 100% wet
        }

        // 3. Create impulse signal
        int totalSamples = (int)(sampleRate * durationSeconds);
        juce::AudioBuffer<float> buffer(2, totalSamples);
        buffer.clear();

        // Impulse at first sample
        buffer.setSample(0, 0, 1.0f);  // Left
        buffer.setSample(1, 0, 1.0f);  // Right

        // 4. Process in blocks (simulate real-time processing)
        int samplesProcessed = 0;
        while (samplesProcessed < totalSamples) {
            int samplesToProcess = std::min(blockSize, totalSamples - samplesProcessed);

            float* channels[2] = {
                buffer.getWritePointer(0) + samplesProcessed,
                buffer.getWritePointer(1) + samplesProcessed
            };

            engine->process(channels, samplesToProcess);
            samplesProcessed += samplesToProcess;
        }

        return buffer;
    }

    // Save results to CSV
    void saveToCSV(const juce::AudioBuffer<float>& buffer, const std::string& filename) {
        std::ofstream file(filename);
        file << "Sample,Time_s,Left,Right\n";

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float time = i / (float)sampleRate;
            file << i << "," << time << ","
                 << buffer.getSample(0, i) << ","
                 << buffer.getSample(1, i) << "\n";
        }
    }
};
```

### Analysis Metrics

#### 1. RT60 (Reverb Time)

**Definition**: Time for reverb to decay by 60dB (to 1/1000 of peak amplitude)

**Implementation**:
```cpp
float measureRT60(const juce::AudioBuffer<float>& impulseResponse, float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Find peak amplitude
    float peak = 0.0f;
    int peakIndex = 0;
    for (int i = 0; i < numSamples; ++i) {
        float absValue = std::abs(data[i]);
        if (absValue > peak) {
            peak = absValue;
            peakIndex = i;
        }
    }

    if (peak < 1e-6f) {
        return 0.0f;  // No significant output
    }

    // Threshold for -60dB: peak / 1000
    float threshold60dB = peak / 1000.0f;

    // Find where signal drops below threshold
    for (int i = peakIndex; i < numSamples; ++i) {
        if (std::abs(data[i]) < threshold60dB) {
            // Found decay point
            return (i - peakIndex) / sampleRate;
        }
    }

    // Still above threshold at end
    return (numSamples - peakIndex) / sampleRate;
}
```

**Interpretation**:
- **Small Room**: 0.3-0.8 seconds
- **Medium Room**: 0.8-1.5 seconds
- **Large Hall**: 1.5-3.0 seconds
- **Cathedral**: 3.0-6.0+ seconds

**Pass Criteria**:
- RT60 should match reverb type specification
- RT60 should be consistent across multiple runs (±10%)
- No sudden cutoffs (gating artifacts)

---

#### 2. Stereo Width (Inter-channel Correlation)

**Definition**: Pearson correlation coefficient between left and right channels

**Implementation**:
```cpp
float measureStereoCorrelation(const juce::AudioBuffer<float>& buffer) {
    const float* left = buffer.getReadPointer(0);
    const float* right = buffer.getReadPointer(1);
    int numSamples = buffer.getNumSamples();

    // Calculate means
    float meanL = 0.0f, meanR = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        meanL += left[i];
        meanR += right[i];
    }
    meanL /= numSamples;
    meanR /= numSamples;

    // Calculate correlation
    float sumLR = 0.0f, sumLL = 0.0f, sumRR = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        float l = left[i] - meanL;
        float r = right[i] - meanR;
        sumLR += l * r;
        sumLL += l * l;
        sumRR += r * r;
    }

    float denominator = std::sqrt(sumLL * sumRR);
    if (denominator < 1e-10f) {
        return 1.0f;  // Both channels silent or identical
    }

    return sumLR / denominator;
}
```

**Interpretation**:
- **1.0**: Perfect correlation (mono)
- **0.7-0.9**: Narrow stereo
- **0.3-0.7**: Medium stereo width
- **0.0-0.3**: Wide stereo (excellent for reverbs)
- **< 0.0**: Out of phase (potential mono compatibility issues)

**Pass Criteria**:
- Reverbs: correlation < 0.5 (wide stereo)
- Stereo delays: correlation < 0.7
- Mono effects: correlation > 0.95
- No negative correlation (phase reversal issues)

---

#### 3. Echo Density

**Definition**: Number of reflections per second (complexity of reverb)

**Implementation**:
```cpp
float measureEchoDensity(const juce::AudioBuffer<float>& impulseResponse,
                         float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Find threshold (1% of peak)
    float peak = 0.0f;
    for (int i = 0; i < numSamples; ++i) {
        peak = std::max(peak, std::abs(data[i]));
    }
    float threshold = peak * 0.01f;

    // Count zero crossings above threshold
    int crossings = 0;
    bool wasPositive = (data[0] > 0);

    for (int i = 1; i < numSamples; ++i) {
        if (std::abs(data[i]) < threshold) continue;

        bool isPositive = (data[i] > 0);
        if (isPositive != wasPositive) {
            crossings++;
            wasPositive = isPositive;
        }
    }

    // Density = crossings per second
    float duration = numSamples / sampleRate;
    return crossings / duration;
}
```

**Interpretation**:
- **< 100 echoes/sec**: Sparse (early reflections, discrete echoes)
- **100-1000**: Medium density (small room reverb)
- **1000-5000**: Dense (smooth reverb tail)
- **> 5000**: Very dense (large hall, plate reverb)

**Pass Criteria**:
- Early reflections: 10-100 echoes/sec
- Room reverb: 500-2000 echoes/sec
- Hall reverb: 1000-5000 echoes/sec
- Plate reverb: 2000-8000 echoes/sec

---

#### 4. Pre-delay Measurement

**Definition**: Time before reverb onset (initial silence)

**Implementation**:
```cpp
float measurePreDelay(const juce::AudioBuffer<float>& impulseResponse,
                      float sampleRate) {
    const float* data = impulseResponse.getReadPointer(0);
    int numSamples = impulseResponse.getNumSamples();

    // Threshold: -60dB (0.001)
    float threshold = 0.001f;

    // Skip initial impulse (first 10 samples)
    for (int i = 10; i < numSamples; ++i) {
        if (std::abs(data[i]) > threshold) {
            // Found first significant sample
            return i * 1000.0f / sampleRate;  // Return in milliseconds
        }
    }

    return 0.0f;  // No pre-delay detected
}
```

**Interpretation**:
- **0-5ms**: No pre-delay
- **5-20ms**: Small room
- **20-50ms**: Medium room
- **50-100ms**: Large hall
- **> 100ms**: Cathedral or intentional delay

**Pass Criteria**:
- Should match parameter setting (if adjustable)
- Should be consistent across runs (±1ms)

---

### Complete Impulse Response Analysis Example

```cpp
void analyzeImpulseResponse(const std::string& engineName, int engineId) {
    // Create and configure engine
    auto engine = EngineFactory::createEngine(engineId);
    engine->prepareToPlay(48000.0, 512);

    // Generate impulse response
    ImpulseResponseTest test;
    auto ir = test.testEngine(engine.get());

    // Analyze
    float rt60 = measureRT60(ir, 48000.0f);
    float correlation = measureStereoCorrelation(ir);
    float density = measureEchoDensity(ir, 48000.0f);
    float preDelay = measurePreDelay(ir, 48000.0f);

    // Report
    std::cout << "\n=== Impulse Response Analysis ===" << std::endl;
    std::cout << "Engine: " << engineName << " (ID: " << engineId << ")" << std::endl;
    std::cout << "RT60:           " << rt60 << " seconds" << std::endl;
    std::cout << "Stereo Corr:    " << correlation << std::endl;
    std::cout << "Echo Density:   " << density << " echoes/sec" << std::endl;
    std::cout << "Pre-delay:      " << preDelay << " ms" << std::endl;

    // Save to file
    test.saveToCSV(ir, "impulse_engine_" + std::to_string(engineId) + ".csv");
}
```

---

## Sine Wave Testing

### Purpose

Sine wave testing is essential for:

1. **Frequency Response**: How engine affects different frequencies
2. **THD Measurement**: Harmonic distortion quantification
3. **Phase Response**: Frequency-dependent phase shift
4. **Filter Characterization**: Cutoff frequency, Q factor, slope
5. **Pitch Accuracy**: For pitch shifters and harmonizers

### Frequency Response Measurement

**Complete Implementation**:

```cpp
class FrequencyResponseTest {
public:
    struct FrequencyPoint {
        float frequency;
        float gainDB;
        float phaseShift;
        float outputLevel;
    };

    std::vector<FrequencyPoint> measureResponse(EngineBase* engine,
                                                 double sampleRate = 48000.0) {
        engine->prepareToPlay(sampleRate, 512);

        // Test frequencies (logarithmically spaced)
        std::vector<float> testFreqs = generateLogFrequencies(20.0f, 20000.0f, 50);

        std::vector<FrequencyPoint> results;

        for (float freq : testFreqs) {
            FrequencyPoint point;
            point.frequency = freq;

            // Generate sine wave
            int numSamples = (int)(sampleRate * 1.0);  // 1 second
            juce::AudioBuffer<float> buffer(2, numSamples);

            for (int i = 0; i < numSamples; ++i) {
                float phase = 2.0f * M_PI * freq * i / sampleRate;
                float sample = 0.5f * std::sin(phase);  // -6dBFS
                buffer.setSample(0, i, sample);
                buffer.setSample(1, i, sample);
            }

            // Process
            engine->reset();
            engine->process(buffer.getArrayOfWritePointers(), numSamples);

            // Measure output (skip first 100ms for settling)
            int skipSamples = (int)(sampleRate * 0.1);
            float rms = 0.0f;

            for (int i = skipSamples; i < numSamples; ++i) {
                float sample = buffer.getSample(0, i);
                rms += sample * sample;
            }
            rms = std::sqrt(rms / (numSamples - skipSamples));

            // Calculate gain (reference is 0.5 input)
            point.outputLevel = rms;
            point.gainDB = 20.0f * std::log10(rms / 0.5f);

            // Phase measurement (optional, requires FFT)
            point.phaseShift = measurePhase(buffer, freq, sampleRate, skipSamples);

            results.push_back(point);
        }

        return results;
    }

private:
    std::vector<float> generateLogFrequencies(float start, float end, int count) {
        std::vector<float> freqs;
        float logStart = std::log10(start);
        float logEnd = std::log10(end);
        float step = (logEnd - logStart) / (count - 1);

        for (int i = 0; i < count; ++i) {
            float logFreq = logStart + i * step;
            freqs.push_back(std::pow(10.0f, logFreq));
        }

        return freqs;
    }

    float measurePhase(const juce::AudioBuffer<float>& buffer,
                       float frequency,
                       float sampleRate,
                       int startSample) {
        // Simplified phase measurement using zero-crossing
        const float* data = buffer.getReadPointer(0) + startSample;
        int numSamples = buffer.getNumSamples() - startSample;

        // Find first positive zero-crossing
        for (int i = 1; i < numSamples; ++i) {
            if (data[i-1] < 0.0f && data[i] >= 0.0f) {
                // Calculate phase offset
                float sampleOffset = startSample + i;
                float expectedZeroCrossing = sampleRate / frequency;
                float phaseShift = (sampleOffset / expectedZeroCrossing) * 360.0f;
                return std::fmod(phaseShift, 360.0f);
            }
        }

        return 0.0f;
    }
};
```

### Interpretation

**Gain Response**:
```
Frequency (Hz) | Gain (dB) | Interpretation
---------------|-----------|---------------
100            | -0.1      | Flat bass response
1000           | +0.0      | Reference (0dB)
10000          | -3.0      | -3dB at 10kHz (lowpass behavior)
15000          | -12.0     | Strong rolloff
```

**Filter Types**:
- **Lowpass**: Gain decreases above cutoff (-6dB, -12dB, or -24dB per octave)
- **Highpass**: Gain decreases below cutoff
- **Bandpass**: Peak at center frequency, rolloff on both sides
- **Notch**: Deep cut (-40dB+) at center frequency
- **Allpass**: Flat gain, but phase shifts

### Pass/Fail Criteria

**Clean Signal Path (EQ, Filters)**:
- Passband flatness: ±0.5dB
- Cutoff accuracy: ±5% of specified frequency
- Stopband rejection: > 40dB

**Musical Effects**:
- Passband flatness: ±1.0dB
- No unexpected resonant peaks (unless intentional)
- Smooth response (no jagged curves)

---

## THD Measurement

### Purpose

Total Harmonic Distortion (THD) quantifies **unwanted** harmonic content added by signal processing. It's critical for clean signal paths.

### Complete Implementation

```cpp
class THDAnalyzer {
public:
    static constexpr int FFT_ORDER = 14;  // 16384 samples
    static constexpr int FFT_SIZE = 1 << FFT_ORDER;

    struct HarmonicAnalysis {
        float fundamentalDB = -200.0f;
        float secondHarmonicDB = -200.0f;
        float thirdHarmonicDB = -200.0f;
        float fourthHarmonicDB = -200.0f;
        float fifthHarmonicDB = -200.0f;
        float thdPercent = 0.0f;
        float thdDB = -200.0f;
        float snrDB = 0.0f;
        bool measurementValid = false;
    };

    static HarmonicAnalysis measureTHD(EngineBase* engine,
                                       float testFreq = 1000.0f,
                                       float sampleRate = 48000.0f) {
        HarmonicAnalysis result;

        // Prepare engine
        engine->prepareToPlay(sampleRate, 512);
        engine->reset();

        // Generate pure sine wave
        int numSamples = (int)(sampleRate * 1.0);  // 1 second
        juce::AudioBuffer<float> buffer(2, numSamples);

        for (int i = 0; i < numSamples; ++i) {
            float phase = 2.0f * M_PI * testFreq * i / sampleRate;
            float sample = 0.5f * std::sin(phase);  // -6dBFS
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        // Process through engine
        engine->process(buffer.getArrayOfWritePointers(), numSamples);

        // Perform FFT analysis
        if (numSamples < FFT_SIZE) {
            return result;  // Buffer too short
        }

        juce::dsp::FFT fft(FFT_ORDER);
        std::vector<float> fftData(FFT_SIZE * 2, 0.0f);

        // Skip first 100ms for settling
        int startOffset = (int)(sampleRate * 0.1);
        const float* data = buffer.getReadPointer(0);

        // Apply Blackman-Harris window
        for (int i = 0; i < FFT_SIZE; ++i) {
            float w = i / (float)(FFT_SIZE - 1);
            float window = 0.35875f
                         - 0.48829f * std::cos(2.0f * M_PI * w)
                         + 0.14128f * std::cos(4.0f * M_PI * w)
                         - 0.01168f * std::cos(6.0f * M_PI * w);

            fftData[i * 2] = data[startOffset + i] * window;
            fftData[i * 2 + 1] = 0.0f;
        }

        // Perform FFT
        fft.performRealOnlyForwardTransform(fftData.data(), true);

        // Calculate magnitude spectrum
        std::vector<float> magnitude(FFT_SIZE / 2);
        for (int i = 0; i < FFT_SIZE / 2; ++i) {
            float real = fftData[i * 2];
            float imag = fftData[i * 2 + 1];
            magnitude[i] = std::sqrt(real * real + imag * imag);
        }

        // Find fundamental and harmonics
        auto findPeak = [&](float frequency) -> float {
            int bin = (int)(frequency * FFT_SIZE / sampleRate);

            // Search ±2 bins for peak
            float maxMag = 0.0f;
            for (int offset = -2; offset <= 2; ++offset) {
                int idx = bin + offset;
                if (idx >= 0 && idx < FFT_SIZE / 2) {
                    maxMag = std::max(maxMag, magnitude[idx]);
                }
            }

            return maxMag;
        };

        float fundamental = findPeak(testFreq);
        float harmonic2 = findPeak(testFreq * 2.0f);
        float harmonic3 = findPeak(testFreq * 3.0f);
        float harmonic4 = findPeak(testFreq * 4.0f);
        float harmonic5 = findPeak(testFreq * 5.0f);

        // Convert to dB
        result.fundamentalDB = 20.0f * std::log10(fundamental + 1e-10f);
        result.secondHarmonicDB = 20.0f * std::log10(harmonic2 + 1e-10f);
        result.thirdHarmonicDB = 20.0f * std::log10(harmonic3 + 1e-10f);
        result.fourthHarmonicDB = 20.0f * std::log10(harmonic4 + 1e-10f);
        result.fifthHarmonicDB = 20.0f * std::log10(harmonic5 + 1e-10f);

        // Calculate THD
        float harmonicsPower = harmonic2*harmonic2 + harmonic3*harmonic3 +
                              harmonic4*harmonic4 + harmonic5*harmonic5;
        float fundamentalPower = fundamental * fundamental;

        if (fundamentalPower > 1e-10f) {
            result.thdPercent = std::sqrt(harmonicsPower / fundamentalPower) * 100.0f;
            result.thdDB = 20.0f * std::log10(result.thdPercent / 100.0f);
            result.measurementValid = true;
        }

        return result;
    }
};
```

### Interpretation

**THD Values**:
- **< 0.001%** (-100dB): Bit-perfect, no measurable distortion
- **0.001-0.01%** (-80 to -100dB): Excellent, transparent
- **0.01-0.1%** (-60 to -80dB): Very good, high quality
- **0.1-1.0%** (-40 to -60dB): Good, acceptable for effects
- **1.0-5.0%** (-26 to -40dB): Noticeable, intentional character
- **> 5.0%** (> -26dB): Significant distortion

**Harmonic Pattern Interpretation**:
```
2nd Harmonic Dominant    → Tube-like, warm character
3rd Harmonic Dominant    → Transistor-like, harder edge
2nd + 3rd Equal         → Balanced analog character
Higher harmonics        → Digital clipping, harsh distortion
All harmonics low       → Clean, transparent processing
```

### Pass/Fail Criteria by Engine Type

```cpp
bool passTHDTest(float thdPercent, EngineType type) {
    switch (type) {
        case UTILITY:           return thdPercent < 0.01%;   // Bit-perfect
        case CLEAN_DYNAMICS:    return thdPercent < 0.05%;   // Transparent
        case CLEAN_FILTER:      return thdPercent < 0.1%;    // High quality
        case MODULATION:        return thdPercent < 1.0%;    // Acceptable
        case ANALOG_EMULATION:  return thdPercent < 5.0%;    // Character OK
        case DISTORTION:        return true;                  // Any is fine
    }
}
```

---

## CPU Benchmarking

### Purpose

Ensure engines meet real-time performance requirements across different buffer sizes and sample rates.

### Complete Implementation

```cpp
class CPUBenchmark {
public:
    struct BenchmarkResult {
        int engineId;
        std::string engineName;
        double sampleRate;
        int blockSize;
        float cpuPercent;
        float avgProcessingTimeUs;
        float maxProcessingTimeUs;
        float minProcessingTimeUs;
        int iterations;
        bool passedRealtimeTest;
    };

    static BenchmarkResult benchmarkEngine(EngineBase* engine,
                                          int engineId,
                                          const std::string& engineName,
                                          double sampleRate = 48000.0,
                                          int blockSize = 512,
                                          int iterations = 10000) {
        BenchmarkResult result;
        result.engineId = engineId;
        result.engineName = engineName;
        result.sampleRate = sampleRate;
        result.blockSize = blockSize;
        result.iterations = iterations;

        // Prepare engine
        engine->prepareToPlay(sampleRate, blockSize);
        engine->reset();

        // Create test buffer
        juce::AudioBuffer<float> buffer(2, blockSize);

        // Fill with test signal (sine wave)
        for (int i = 0; i < blockSize; ++i) {
            float phase = 2.0f * M_PI * 1000.0f * i / sampleRate;
            float sample = 0.5f * std::sin(phase);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }

        // Warm-up (10 iterations)
        for (int i = 0; i < 10; ++i) {
            engine->process(buffer.getArrayOfWritePointers(), blockSize);
        }

        // Benchmark
        std::vector<double> processingTimes;
        processingTimes.reserve(iterations);

        for (int i = 0; i < iterations; ++i) {
            auto start = std::chrono::high_resolution_clock::now();

            engine->process(buffer.getArrayOfWritePointers(), blockSize);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            processingTimes.push_back(duration.count());
        }

        // Calculate statistics
        double totalTime = 0.0;
        double maxTime = 0.0;
        double minTime = 1e9;

        for (double time : processingTimes) {
            totalTime += time;
            maxTime = std::max(maxTime, time);
            minTime = std::min(minTime, time);
        }

        double avgTime = totalTime / iterations;

        // Calculate CPU percentage
        // Available time per block = blockSize / sampleRate (in seconds)
        double availableTimeUs = (blockSize / sampleRate) * 1000000.0;
        double cpuPercent = (avgTime / availableTimeUs) * 100.0;

        result.avgProcessingTimeUs = (float)avgTime;
        result.maxProcessingTimeUs = (float)maxTime;
        result.minProcessingTimeUs = (float)minTime;
        result.cpuPercent = (float)cpuPercent;
        result.passedRealtimeTest = (cpuPercent < 100.0f);  // Must be real-time capable

        return result;
    }

    // Benchmark across multiple configurations
    static std::vector<BenchmarkResult> comprehensiveBenchmark(EngineBase* engine,
                                                               int engineId,
                                                               const std::string& engineName) {
        std::vector<BenchmarkResult> results;

        // Test configurations
        struct Config {
            double sampleRate;
            int blockSize;
        };

        std::vector<Config> configs = {
            {44100.0, 64},
            {44100.0, 512},
            {48000.0, 64},
            {48000.0, 512},
            {48000.0, 1024},
            {96000.0, 512},
            {96000.0, 1024}
        };

        for (const auto& config : configs) {
            auto result = benchmarkEngine(engine, engineId, engineName,
                                         config.sampleRate, config.blockSize, 5000);
            results.push_back(result);
        }

        return results;
    }
};
```

### Interpretation

**CPU Usage Levels**:
- **< 1%**: Minimal (utility, simple gain)
- **1-5%**: Low (filters, simple modulation)
- **5-15%**: Moderate (dynamics, complex filters)
- **15-30%**: High (pitch shifting, spectral processing)
- **30-50%**: Very high (convolution, granular)
- **> 50%**: Extreme (may not be usable in multi-engine chains)

**Real-time Capability**:
```cpp
void interpretCPUResult(const CPUBenchmark::BenchmarkResult& result) {
    std::cout << "\n=== CPU Benchmark Results ===" << std::endl;
    std::cout << "Engine: " << result.engineName << std::endl;
    std::cout << "Sample Rate: " << result.sampleRate << " Hz" << std::endl;
    std::cout << "Block Size: " << result.blockSize << " samples" << std::endl;
    std::cout << "CPU Usage: " << result.cpuPercent << "%" << std::endl;
    std::cout << "Avg Time: " << result.avgProcessingTimeUs << " μs" << std::endl;
    std::cout << "Max Time: " << result.maxProcessingTimeUs << " μs" << std::endl;

    // Interpret
    if (!result.passedRealtimeTest) {
        std::cout << "❌ FAIL: Not real-time capable!" << std::endl;
    } else if (result.cpuPercent < 1.0f) {
        std::cout << "✅ EXCELLENT: Minimal CPU usage" << std::endl;
    } else if (result.cpuPercent < 5.0f) {
        std::cout << "✅ GOOD: Low CPU usage" << std::endl;
    } else if (result.cpuPercent < 15.0f) {
        std::cout << "⚠ MODERATE: Acceptable CPU usage" << std::endl;
    } else if (result.cpuPercent < 30.0f) {
        std::cout << "⚠ HIGH: Use sparingly in chains" << std::endl;
    } else {
        std::cout << "⚠ VERY HIGH: Heavy CPU load" << std::endl;
    }
}
```

### Pass/Fail Criteria

**Standard Configuration** (48kHz, 512 samples):
- Utility engines: < 1%
- Clean effects: < 5%
- Musical effects: < 10%
- Complex effects: < 20%
- Extreme effects: < 50%

**Buffer Size Independence**:
- CPU usage should scale linearly with block size
- No sudden jumps or anomalies
- Smaller blocks may show higher % (overhead)

---

## Stereo Analysis

### Purpose

Comprehensive stereo field analysis to ensure proper spatial imaging, channel independence, and mono compatibility.

### Complete Implementation

```cpp
class StereoAnalyzer {
public:
    struct StereoAnalysisResult {
        // Correlation metrics
        float lrCorrelation;          // -1 to +1
        float stereoWidth;            // 0 to 2+
        float phaseCoherence;         // -1 to +1

        // Mid/Side analysis
        float midLevelRMS;
        float sideLevelRMS;
        float midSideRatioDB;

        // Mono compatibility
        float monoCompatibility;      // 0 to 1
        float phaseCancellation;      // 0 to 1

        // Channel balance
        float peakL, peakR;
        float rmsL, rmsR;
        float balanceRatio;           // L/R ratio

        // Quality flags
        bool hasMonoCollapse;         // Corr > 0.95 when should be stereo
        bool hasPhaseIssues;          // Phase coherence < -0.5
        bool hasImbalance;            // L/R ratio outside 0.7-1.43 (±3dB)
        bool hasStereoContent;        // Width > 0.1
    };

    static StereoAnalysisResult analyze(const juce::AudioBuffer<float>& buffer) {
        StereoAnalysisResult result = {};

        if (buffer.getNumChannels() < 2) {
            return result;  // Not stereo
        }

        const float* left = buffer.getReadPointer(0);
        const float* right = buffer.getReadPointer(1);
        int numSamples = buffer.getNumSamples();

        // 1. Calculate L/R Correlation (Pearson)
        result.lrCorrelation = calculateCorrelation(left, right, numSamples);

        // 2. Calculate Stereo Width
        result.stereoWidth = calculateStereoWidth(left, right, numSamples);

        // 3. Calculate Phase Coherence
        result.phaseCoherence = calculatePhaseCoherence(left, right, numSamples);

        // 4. Mid/Side Analysis
        std::tie(result.midLevelRMS, result.sideLevelRMS, result.midSideRatioDB) =
            calculateMidSideAnalysis(left, right, numSamples);

        // 5. Mono Compatibility
        result.monoCompatibility = calculateMonoCompatibility(left, right, numSamples);

        // 6. Channel Balance
        calculateChannelBalance(left, right, numSamples,
                               result.peakL, result.peakR,
                               result.rmsL, result.rmsR,
                               result.balanceRatio);

        // 7. Quality Flags
        result.hasMonoCollapse = (result.lrCorrelation > 0.95f);
        result.hasPhaseIssues = (result.phaseCoherence < -0.5f);
        result.hasImbalance = (result.balanceRatio < 0.7f || result.balanceRatio > 1.43f);
        result.hasStereoContent = (result.stereoWidth > 0.1f);

        return result;
    }

private:
    static float calculateCorrelation(const float* left, const float* right, int numSamples) {
        // Calculate means
        float meanL = 0.0f, meanR = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            meanL += left[i];
            meanR += right[i];
        }
        meanL /= numSamples;
        meanR /= numSamples;

        // Calculate Pearson correlation
        float sumLR = 0.0f, sumLL = 0.0f, sumRR = 0.0f;
        for (int i = 0; i < numSamples; ++i) {
            float l = left[i] - meanL;
            float r = right[i] - meanR;
            sumLR += l * r;
            sumLL += l * l;
            sumRR += r * r;
        }

        float denom = std::sqrt(sumLL * sumRR);
        return (denom > 1e-10f) ? (sumLR / denom) : 1.0f;
    }

    static float calculateStereoWidth(const float* left, const float* right, int numSamples) {
        // Calculate mid and side signals
        float midRMS = 0.0f, sideRMS = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            float mid = (left[i] + right[i]) * 0.5f;
            float side = (left[i] - right[i]) * 0.5f;
            midRMS += mid * mid;
            sideRMS += side * side;
        }

        midRMS = std::sqrt(midRMS / numSamples);
        sideRMS = std::sqrt(sideRMS / numSamples);

        // Width = side/mid ratio
        return (midRMS > 1e-10f) ? (sideRMS / midRMS) : 0.0f;
    }

    static float calculatePhaseCoherence(const float* left, const float* right, int numSamples) {
        // Cross-correlation at zero lag
        float crossCorr = 0.0f;
        float autoL = 0.0f;
        float autoR = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            crossCorr += left[i] * right[i];
            autoL += left[i] * left[i];
            autoR += right[i] * right[i];
        }

        float denom = std::sqrt(autoL * autoR);
        return (denom > 1e-10f) ? (crossCorr / denom) : 1.0f;
    }

    static std::tuple<float, float, float> calculateMidSideAnalysis(
        const float* left, const float* right, int numSamples) {

        float midRMS = 0.0f, sideRMS = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            float mid = (left[i] + right[i]) * 0.5f;
            float side = (left[i] - right[i]) * 0.5f;
            midRMS += mid * mid;
            sideRMS += side * side;
        }

        midRMS = std::sqrt(midRMS / numSamples);
        sideRMS = std::sqrt(sideRMS / numSamples);

        float ratioDB = (midRMS > 1e-10f && sideRMS > 1e-10f) ?
                       (20.0f * std::log10(sideRMS / midRMS)) : -100.0f;

        return std::make_tuple(midRMS, sideRMS, ratioDB);
    }

    static float calculateMonoCompatibility(const float* left, const float* right, int numSamples) {
        // Mono sum
        float monoRMS = 0.0f;
        float stereoRMS = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            float mono = left[i] + right[i];
            float stereo = std::abs(left[i]) + std::abs(right[i]);
            monoRMS += mono * mono;
            stereoRMS += stereo * stereo;
        }

        monoRMS = std::sqrt(monoRMS / numSamples);
        stereoRMS = std::sqrt(stereoRMS / numSamples);

        // Compatibility = how much signal survives mono summing
        return (stereoRMS > 1e-10f) ? (monoRMS / stereoRMS) : 1.0f;
    }

    static void calculateChannelBalance(const float* left, const float* right, int numSamples,
                                       float& peakL, float& peakR,
                                       float& rmsL, float& rmsR,
                                       float& ratio) {
        peakL = peakR = 0.0f;
        rmsL = rmsR = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            peakL = std::max(peakL, std::abs(left[i]));
            peakR = std::max(peakR, std::abs(right[i]));
            rmsL += left[i] * left[i];
            rmsR += right[i] * right[i];
        }

        rmsL = std::sqrt(rmsL / numSamples);
        rmsR = std::sqrt(rmsR / numSamples);

        ratio = (rmsR > 1e-10f) ? (rmsL / rmsR) : 1.0f;
    }
};
```

### Interpretation

**L/R Correlation**:
- **0.95-1.0**: Mono or nearly mono
- **0.7-0.95**: Narrow stereo
- **0.3-0.7**: Medium stereo width
- **0.0-0.3**: Wide stereo (excellent for spatial effects)
- **< 0.0**: Phase correlation issues (check for reversal)

**Stereo Width**:
- **0.0-0.1**: Essentially mono
- **0.1-0.5**: Narrow stereo
- **0.5-1.0**: Medium width
- **1.0-2.0**: Wide stereo
- **> 2.0**: Extremely wide (may have mono compatibility issues)

**Mid/Side Ratio**:
- **-20dB**: Very mono (side much quieter than mid)
- **-10dB**: Narrow stereo
- **-6dB**: Balanced stereo
- **0dB**: Wide stereo (mid and side equal)
- **+6dB**: Very wide (side louder than mid)

**Mono Compatibility**:
- **0.9-1.0**: Excellent (no phase cancellation)
- **0.7-0.9**: Good
- **0.5-0.7**: Fair (some cancellation)
- **< 0.5**: Poor (significant phase issues)

### Pass/Fail Criteria by Engine Type

```cpp
struct StereoTestCriteria {
    std::string engineType;
    float minCorrelation;
    float maxCorrelation;
    float minWidth;
    float maxWidth;
    float minMonoCompat;
};

const std::map<std::string, StereoTestCriteria> CRITERIA = {
    {"MONO_EFFECT", {
        .engineType = "Mono Effect",
        .minCorrelation = 0.95f,
        .maxCorrelation = 1.0f,
        .minWidth = 0.0f,
        .maxWidth = 0.1f,
        .minMonoCompat = 0.95f
    }},
    {"STEREO_REVERB", {
        .engineType = "Stereo Reverb",
        .minCorrelation = 0.0f,
        .maxCorrelation = 0.5f,
        .minWidth = 0.5f,
        .maxWidth = 2.0f,
        .minMonoCompat = 0.6f
    }},
    {"STEREO_DELAY", {
        .engineType = "Stereo Delay",
        .minCorrelation = 0.2f,
        .maxCorrelation = 0.8f,
        .minWidth = 0.3f,
        .maxWidth = 1.5f,
        .minMonoCompat = 0.7f
    }},
    {"STEREO_WIDENER", {
        .engineType = "Stereo Widener",
        .minCorrelation = 0.0f,
        .maxCorrelation = 0.3f,
        .minWidth = 1.0f,
        .maxWidth = 3.0f,
        .minMonoCompat = 0.5f
    }}
};
```

---

## Interpretation Guidelines

### How to Read Test Results

#### 1. Impulse Response

**Visual Inspection**:
```
Sample    Time      Left      Right     Interpretation
------    ----      ----      -----     --------------
0         0.0ms     1.000     1.000     Initial impulse
10        0.2ms     0.000     0.000     Pre-delay gap
500       10.4ms    0.450     0.380     Early reflections
5000      104ms     0.120     0.095     Dense reverb tail
20000     417ms     0.008     0.007     Late tail
48000     1000ms    0.001     0.001     Near RT60 threshold
```

**Good Reverb Signs**:
- Smooth exponential decay (no sudden drops)
- Different L/R values (stereo decorrelation)
- Gradual onset (early reflections → dense tail)
- Long tail (RT60 > 1 second for halls)

**Problem Signs**:
- Sudden cutoffs (buffer size issues)
- Identical L/R (mono collapse)
- No output after impulse (zero output bug)
- Clicks or glitches (buffer boundary issues)

#### 2. Frequency Response

**Visual Inspection**:
```
Freq (Hz)  Gain (dB)  Interpretation
---------  ---------  --------------
100        -0.1       Flat bass response ✓
1000       +0.0       Reference (0dB) ✓
3000       -3.0       Cutoff frequency
10000      -12.0      -12dB/octave rolloff ✓
15000      -24.0      Continued rolloff ✓
```

**Good Filter Signs**:
- Smooth curves (no jagged response)
- Correct slope (-6, -12, -24 dB/octave)
- Flat passband (±0.5dB)
- Deep stopband rejection (> 40dB)

**Problem Signs**:
- Jagged/discontinuous curves (aliasing)
- Unexpected peaks (instability)
- Incorrect cutoff frequency
- Insufficient rejection

#### 3. THD Results

**Example Report**:
```
Fundamental:    -6.0 dB @ 1000 Hz
2nd Harmonic:   -72.0 dB @ 2000 Hz
3rd Harmonic:   -78.0 dB @ 3000 Hz
4th Harmonic:   -85.0 dB @ 4000 Hz
5th Harmonic:   -90.0 dB @ 5000 Hz
THD:            0.025% (-72 dB)
```

**Interpretation**:
- Dominant 2nd harmonic → Tube-like warmth
- THD -72dB → Excellent clean performance
- All harmonics < -70dB → High quality
- **Result**: PASS (Clean engine, THD < 0.1%)

**Problem Example**:
```
Fundamental:    -6.0 dB @ 1000 Hz
2nd Harmonic:   -45.0 dB @ 2000 Hz   ← HIGH!
3rd Harmonic:   -38.0 dB @ 3000 Hz   ← VERY HIGH!
THD:            3.2% (-30 dB)        ← FAIL
```
- THD 3.2% → Significant distortion
- **Cause**: Likely clipping or aliasing
- **Action**: Add oversampling or reduce gain

#### 4. CPU Benchmark

**Example Report**:
```
Configuration:  48 kHz, 512 samples
Iterations:     10,000
Avg Time:       85.3 μs
Max Time:       142.7 μs
Available Time: 10,666 μs
CPU Usage:      0.8%
```

**Interpretation**:
- 0.8% CPU → Excellent efficiency
- Max time still << available → Headroom OK
- Suitable for multi-engine chains
- **Result**: PASS

**Problem Example**:
```
Configuration:  48 kHz, 512 samples
CPU Usage:      32.5%
Max Time:       4,832 μs    ← Occasional spikes!
```
- 32.5% → Heavy CPU load
- Max time spikes → Potential dropouts
- **Concern**: May not work in complex projects
- **Action**: Optimize or mark as "heavy" effect

#### 5. Stereo Analysis

**Example Report**:
```
L/R Correlation:     0.23
Stereo Width:        1.34
Phase Coherence:     0.89
Mid/Side Ratio:      -4.2 dB
Mono Compatibility:  0.81
L/R Balance:         1.05 (0.4 dB imbalance)
```

**Interpretation**:
- Low correlation (0.23) → Wide stereo ✓
- Good width (1.34) → Spacious ✓
- Positive phase (0.89) → No reversal ✓
- Balanced channels (1.05) → Even L/R ✓
- Good mono compat (0.81) → Translates well ✓
- **Result**: PASS (Excellent stereo reverb)

---

## Pass/Fail Criteria

### Complete Pass/Fail Matrix

| Test Type | Engine Category | Pass Threshold | Warning Threshold | Fail Threshold |
|-----------|----------------|----------------|-------------------|----------------|
| **THD** | Utility | < 0.01% | 0.01-0.05% | > 0.05% |
| **THD** | Clean Dynamics | < 0.05% | 0.05-0.1% | > 0.1% |
| **THD** | Clean Filters | < 0.1% | 0.1-0.5% | > 0.5% |
| **THD** | Modulation | < 1.0% | 1.0-2.0% | > 2.0% |
| **CPU** | All Engines | < 5% | 5-10% | > 10% |
| **Latency** | Real-time FX | < 64 samples | 64-512 | > 512 |
| **Stereo Corr** | Reverbs | < 0.5 | 0.5-0.7 | > 0.7 |
| **Stereo Corr** | Mono FX | > 0.95 | 0.9-0.95 | < 0.9 |
| **RT60** | Small Room | 0.3-1.0s | 1.0-1.5s | Outside range |
| **RT60** | Hall | 1.5-4.0s | 1.0-1.5s or 4-5s | Outside range |

### Automated Pass/Fail Decision

```cpp
enum class TestResult { PASS, WARNING, FAIL };

struct TestDecision {
    TestResult result;
    std::string message;
    std::vector<std::string> issues;
    std::vector<std::string> suggestions;
};

TestDecision evaluateEngineQuality(int engineId,
                                   const std::string& engineCategory) {
    TestDecision decision;

    // Run all tests
    auto thdResult = THDAnalyzer::measureTHD(engine);
    auto cpuResult = CPUBenchmark::benchmarkEngine(engine, engineId, engineName);
    auto stereoResult = StereoAnalyzer::analyze(buffer);

    // Evaluate THD
    if (engineCategory == "CLEAN" || engineCategory == "UTILITY") {
        if (thdResult.thdPercent > 0.1f) {
            decision.result = TestResult::FAIL;
            decision.issues.push_back("THD too high for clean engine: " +
                                     std::to_string(thdResult.thdPercent) + "%");
            decision.suggestions.push_back("Add oversampling or reduce gain");
        } else if (thdResult.thdPercent > 0.05f) {
            decision.result = TestResult::WARNING;
            decision.issues.push_back("THD acceptable but not ideal");
        }
    }

    // Evaluate CPU
    if (cpuResult.cpuPercent > 10.0f) {
        decision.result = TestResult::FAIL;
        decision.issues.push_back("CPU usage too high: " +
                                 std::to_string(cpuResult.cpuPercent) + "%");
        decision.suggestions.push_back("Optimize algorithm or reduce quality");
    } else if (cpuResult.cpuPercent > 5.0f) {
        if (decision.result != TestResult::FAIL) {
            decision.result = TestResult::WARNING;
        }
        decision.issues.push_back("CPU usage moderate, use cautiously in chains");
    }

    // Evaluate Stereo
    if (engineCategory == "REVERB" || engineCategory == "SPATIAL") {
        if (stereoResult.hasMonoCollapse) {
            decision.result = TestResult::FAIL;
            decision.issues.push_back("Stereo collapse detected (correlation > 0.95)");
            decision.suggestions.push_back("Check stereo decorrelation implementation");
        }

        if (stereoResult.monoCompatibility < 0.5f) {
            if (decision.result == TestResult::PASS) {
                decision.result = TestResult::WARNING;
            }
            decision.issues.push_back("Poor mono compatibility (may have phase issues)");
            decision.suggestions.push_back("Review phase relationships between channels");
        }
    }

    // Set final message
    if (decision.result == TestResult::PASS) {
        decision.message = "All tests PASSED - Engine is production ready";
    } else if (decision.result == TestResult::WARNING) {
        decision.message = "Tests PASSED with warnings - Review issues before release";
    } else {
        decision.message = "Tests FAILED - Fix critical issues before proceeding";
    }

    return decision;
}
```

---

## Engine-Specific Testing

### Reverb Engines (39-43)

**Required Tests**:
1. Impulse response (RT60, stereo width, echo density)
2. THD < 1.0% (clean signal path)
3. Stereo correlation < 0.5
4. Mono compatibility > 0.6

**Example Test Sequence**:
```cpp
void testReverbEngine(int engineId) {
    auto engine = EngineFactory::createEngine(engineId);

    // 1. Impulse Response
    auto ir = generateImpulseResponse(engine.get());
    float rt60 = measureRT60(ir, 48000.0f);
    float correlation = measureStereoCorrelation(ir);
    float density = measureEchoDensity(ir, 48000.0f);

    // 2. THD
    auto thd = THDAnalyzer::measureTHD(engine.get());

    // 3. CPU
    auto cpu = CPUBenchmark::benchmarkEngine(engine.get(), engineId, "Reverb");

    // 4. Report
    std::cout << "RT60: " << rt60 << "s (expect 1.5-3.0s)" << std::endl;
    std::cout << "Stereo Corr: " << correlation << " (expect < 0.5)" << std::endl;
    std::cout << "Echo Density: " << density << " echoes/sec" << std::endl;
    std::cout << "THD: " << thd.thdPercent << "% (expect < 1.0%)" << std::endl;
    std::cout << "CPU: " << cpu.cpuPercent << "%" << std::endl;
}
```

### Filter/EQ Engines (7-14)

**Required Tests**:
1. Frequency response (sweep 20Hz-20kHz)
2. THD < 0.1%
3. Phase response (if applicable)
4. CPU < 10%

### Distortion Engines (15-22)

**Required Tests**:
1. Harmonic content analysis
2. Clipping behavior (soft vs hard)
3. Oversampling verification (if used)
4. No aliasing artifacts

### Pitch Shifters (31-33)

**Required Tests**:
1. Pitch accuracy (±1 cent for professional)
2. Formant preservation
3. Artifact analysis (shimmer, metallic)
4. CPU < 20% (acceptable for complex algorithm)

---

## Regression Testing

### Purpose

Prevent quality degradation when making changes.

### Regression Test Suite

```bash
#!/bin/bash
# regression_test.sh

echo "Running regression tests..."

# Known-good baseline engines
BASELINE_ENGINES=(
    1   # Vintage Opto Compressor
    7   # Parametric EQ
    23  # Digital Chorus
    35  # Digital Delay
    39  # Plate Reverb
    44  # Stereo Widener
    54  # Gain Utility
)

# Run tests and compare to baseline
for engine in "${BASELINE_ENGINES[@]}"; do
    echo "Testing engine $engine..."

    # Run comprehensive test
    ./build/standalone_test --engine $engine > current_results_$engine.txt

    # Compare to baseline
    if [ -f "baseline_results_$engine.txt" ]; then
        diff baseline_results_$engine.txt current_results_$engine.txt

        if [ $? -ne 0 ]; then
            echo "❌ REGRESSION: Engine $engine results changed!"
            exit 1
        fi
    else
        echo "⚠ No baseline found for engine $engine (creating baseline)"
        cp current_results_$engine.txt baseline_results_$engine.txt
    fi
done

echo "✅ All regression tests passed"
```

---

## Troubleshooting Guide

### Common Issues and Solutions

#### 1. Zero Output

**Debug Checklist**:
```cpp
void debugZeroOutput(EngineBase* engine) {
    // 1. Check if process() is being called
    std::cout << "Process called: " << processCallCount << std::endl;

    // 2. Check input buffer
    float inputPeak = findPeakLevel(inputBuffer);
    std::cout << "Input peak: " << inputPeak << std::endl;

    // 3. Check mix parameter
    float mix = engine->getParameter(0);
    std::cout << "Mix parameter: " << mix << std::endl;

    // 4. Check internal state
    std::cout << "Internal buffer size: " << internalBufferSize << std::endl;

    // 5. Force 100% wet
    engine->setParameter(0, 1.0f);
}
```

#### 2. Mono Collapse in Stereo Effect

**Debug Steps**:
```cpp
void debugMonoCollapse(const juce::AudioBuffer<float>& output) {
    // 1. Check channel independence
    float correlation = measureStereoCorrelation(output);
    std::cout << "L/R Correlation: " << correlation << std::endl;

    // 2. Check if both channels identical
    const float* left = output.getReadPointer(0);
    const float* right = output.getReadPointer(1);

    bool identical = true;
    for (int i = 0; i < output.getNumSamples(); ++i) {
        if (std::abs(left[i] - right[i]) > 1e-6f) {
            identical = false;
            break;
        }
    }

    if (identical) {
        std::cout << "❌ Channels are identical!" << std::endl;
        std::cout << "Check: stereo modulation, decorrelation filters" << std::endl;
    }
}
```

#### 3. High THD in Clean Effect

**Debugging**:
```cpp
void debugHighTHD(EngineBase* engine) {
    // Test at multiple frequencies
    std::vector<float> testFreqs = {100, 440, 1000, 5000};

    for (float freq : testFreqs) {
        auto result = THDAnalyzer::measureTHD(engine, freq);
        std::cout << freq << " Hz: THD = " << result.thdPercent << "%" << std::endl;
        std::cout << "  2nd: " << result.secondHarmonicDB << " dB" << std::endl;
        std::cout << "  3rd: " << result.thirdHarmonicDB << " dB" << std::endl;
    }

    // If THD increases with frequency → aliasing
    // If 2nd harmonic dominant → soft clipping
    // If all harmonics high → hard clipping
}
```

---

## Code Examples Summary

### Quick Test Template

```cpp
// Complete engine test in one function
void quickEngineTest(int engineId, const std::string& engineName) {
    auto engine = EngineFactory::createEngine(engineId);

    std::cout << "\n=== Testing Engine " << engineId << ": "
              << engineName << " ===" << std::endl;

    // 1. Basic functionality
    bool hasOutput = testBasicOutput(engine.get());
    std::cout << "Basic Output: " << (hasOutput ? "PASS" : "FAIL") << std::endl;

    // 2. THD
    auto thd = THDAnalyzer::measureTHD(engine.get());
    std::cout << "THD: " << thd.thdPercent << "% ("
              << (thd.thdPercent < 1.0f ? "PASS" : "FAIL") << ")" << std::endl;

    // 3. CPU
    auto cpu = CPUBenchmark::benchmarkEngine(engine.get(), engineId, engineName);
    std::cout << "CPU: " << cpu.cpuPercent << "% ("
              << (cpu.cpuPercent < 10.0f ? "PASS" : "WARNING") << ")" << std::endl;

    // 4. Stereo (if applicable)
    auto ir = generateImpulseResponse(engine.get());
    auto stereo = StereoAnalyzer::analyze(ir);
    std::cout << "Stereo Corr: " << stereo.lrCorrelation << std::endl;
}
```

---

## References

### Standards
- AES17: THD measurement standards
- AES42: Digital audio quality metrics
- ITU-R BS.1116: Subjective audio assessment

### Papers
- Dattorro: "Effect Design, Part 1: Reverberator"
- Schroeder: "Natural Sounding Artificial Reverberation"
- Jot & Chaigne: "Digital Delay Networks for Reverb"

### Tools
- Audacity: Waveform visualization
- REW: Room acoustics measurement
- MATLAB/Python: Custom analysis scripts

---

**Document Version**: 2.0
**Last Updated**: October 11, 2025
**Maintained By**: ChimeraPhoenix QA Team

---

**END OF TESTING METHODOLOGY**
