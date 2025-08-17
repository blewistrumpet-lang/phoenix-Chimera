# Comprehensive Engine Test Suite Design
## Testing 56 DSP Engines for Correct Audio Processing

### Core Testing Principles

#### 1. Signal Detection Methods
- **Null Test**: Process silence → should produce silence (except generators)
- **Impulse Response**: Send single sample spike → analyze response characteristics
- **Sine Wave Analysis**: Use pure tones to detect frequency/phase changes
- **White Noise Analysis**: Check spectral shaping and filtering
- **Transient Detection**: Use sharp attacks to test dynamics processors

#### 2. Measurable Metrics
```cpp
struct EngineTestMetrics {
    float rmsLevel;           // Overall energy change
    float peakLevel;          // Maximum sample value
    float spectralCentroid;   // Frequency balance shift
    float zeroCrossingRate;   // Harmonic content change
    float correlationCoeff;   // Similarity to input
    float latency;           // Processing delay
    float thd;               // Total harmonic distortion
    float snr;               // Signal-to-noise ratio
};
```

### Engine-Specific Test Strategies

#### DYNAMICS (Compressors, Limiters, Gates)
```cpp
class DynamicsTest {
    bool testCompression() {
        // 1. Send loud signal (-6dB)
        // 2. Verify output is quieter than input
        // 3. Check ratio: output_change < input_change
        // 4. Verify attack/release timing
        return outputRMS < inputRMS * 0.8;
    }
    
    bool testGating() {
        // 1. Send quiet signal (-40dB)
        // 2. Verify gate closes (output near silence)
        // 3. Send loud signal, verify gate opens
        return quietOutput < -60dB && loudOutput > -20dB;
    }
};
```

#### REVERBS (Plate, Spring, Convolution, etc.)
```cpp
class ReverbTest {
    bool testReverb() {
        // 1. Send impulse (single sample spike)
        // 2. Measure decay time (RT60)
        // 3. Verify tail extends beyond input
        // 4. Check frequency diffusion
        // 5. Verify stereo spread (if applicable)
        
        bool hasTail = measureDecayTime() > 0.1; // At least 100ms
        bool hasSpread = leftChannelRMS != rightChannelRMS;
        bool hasFreqDiffusion = spectralVariance > threshold;
        
        return hasTail && hasSpread && hasFreqDiffusion;
    }
};
```

#### DELAYS (Tape Echo, Digital Delay, BBD)
```cpp
class DelayTest {
    bool testDelay() {
        // 1. Send click at t=0
        // 2. Detect echo at t=delayTime
        // 3. Verify feedback creates multiple echoes
        // 4. Check modulation (for analog delays)
        
        auto peaks = detectPeaks(output);
        bool hasEcho = peaks.size() > 1;
        bool correctTiming = abs(peaks[1].time - delayTime) < 0.001;
        
        return hasEcho && correctTiming;
    }
};
```

#### EQs (Parametric, Console, Dynamic)
```cpp
class EQTest {
    bool testEQ() {
        // 1. Send white noise
        // 2. Boost at 1kHz by 12dB
        // 3. Verify spectrum shows peak at 1kHz
        // 4. Test each band independently
        
        auto spectrum = performFFT(output);
        float gain1kHz = spectrum[1000] / inputSpectrum[1000];
        
        return gain1kHz > 3.5; // ~12dB boost
    }
};
```

#### FILTERS (Ladder, State Variable, Formant)
```cpp
class FilterTest {
    bool testFilter() {
        // 1. Send white noise
        // 2. Set cutoff to 1kHz
        // 3. Verify high frequencies attenuated
        // 4. Check resonance creates peak
        
        auto spectrum = performFFT(output);
        bool hasRolloff = spectrum[2000] < spectrum[500] * 0.25;
        bool hasResonance = spectrum[1000] > spectrum[500] * 1.5;
        
        return hasRolloff && (resonance == 0 || hasResonance);
    }
};
```

#### DISTORTION/SATURATION
```cpp
class DistortionTest {
    bool testDistortion() {
        // 1. Send pure sine wave
        // 2. Measure harmonic content
        // 3. Verify harmonics added
        // 4. Check clipping behavior
        
        float thd = measureTHD(output);
        bool hasHarmonics = thd > 0.01; // At least 1% THD
        bool maintainsLevel = abs(outputRMS - inputRMS) < 6.0; // Within 6dB
        
        return hasHarmonics && maintainsLevel;
    }
};
```

#### MODULATION (Chorus, Phaser, Tremolo)
```cpp
class ModulationTest {
    bool testModulation() {
        // 1. Send steady tone
        // 2. Detect periodic amplitude/frequency changes
        // 3. Verify LFO rate matches setting
        // 4. Check stereo phasing
        
        auto envelope = extractEnvelope(output);
        float modRate = detectModulationRate(envelope);
        
        return abs(modRate - expectedRate) < 0.1; // Within 0.1 Hz
    }
};
```

#### PITCH SHIFTERS
```cpp
class PitchTest {
    bool testPitchShift() {
        // 1. Send 440Hz sine wave
        // 2. Shift up one octave
        // 3. Verify output is 880Hz
        
        float detectedPitch = detectPitch(output);
        float expectedPitch = 440.0 * pitchRatio;
        
        return abs(detectedPitch - expectedPitch) < 5.0; // Within 5Hz
    }
};
```

### Comprehensive Test Implementation

```cpp
class ComprehensiveEngineTest {
public:
    struct TestResult {
        int engineID;
        std::string engineName;
        bool passed;
        float confidence;  // 0-100%
        std::string failureReason;
        EngineTestMetrics metrics;
    };
    
    TestResult testEngine(int engineID) {
        auto engine = createEngine(engineID);
        engine->prepareToPlay(48000, 512);
        
        // Reset to defaults
        setDefaultParameters(engine, engineID);
        
        // Run appropriate test based on engine type
        switch(getEngineCategory(engineID)) {
            case DYNAMICS:
                return testDynamicsEngine(engine);
            case REVERB:
                return testReverbEngine(engine);
            case DELAY:
                return testDelayEngine(engine);
            case EQ:
                return testEQEngine(engine);
            case FILTER:
                return testFilterEngine(engine);
            case DISTORTION:
                return testDistortionEngine(engine);
            case MODULATION:
                return testModulationEngine(engine);
            case PITCH:
                return testPitchEngine(engine);
        }
    }
    
private:
    // Test Signals
    AudioBuffer<float> generateImpulse(int samples) {
        AudioBuffer<float> buffer(2, samples);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        return buffer;
    }
    
    AudioBuffer<float> generateSineWave(float freq, int samples) {
        AudioBuffer<float> buffer(2, samples);
        for (int i = 0; i < samples; ++i) {
            float sample = sin(2.0 * M_PI * freq * i / 48000.0);
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        return buffer;
    }
    
    AudioBuffer<float> generateWhiteNoise(int samples) {
        AudioBuffer<float> buffer(2, samples);
        Random rng;
        for (int i = 0; i < samples; ++i) {
            float sample = rng.nextFloat() * 2.0f - 1.0f;
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        return buffer;
    }
    
    // Analysis Functions
    bool detectSignalPresence(const AudioBuffer<float>& buffer) {
        float rms = buffer.getRMSLevel(0, 0, buffer.getNumSamples());
        return rms > 0.001f; // -60dB threshold
    }
    
    bool detectSpectralChange(const AudioBuffer<float>& input,
                              const AudioBuffer<float>& output) {
        auto inputSpectrum = performFFT(input);
        auto outputSpectrum = performFFT(output);
        
        float difference = 0;
        for (int i = 0; i < inputSpectrum.size(); ++i) {
            difference += abs(inputSpectrum[i] - outputSpectrum[i]);
        }
        
        return difference > 0.1f; // Significant change
    }
};
```

### Automated Test Runner

```cpp
class EngineTestRunner {
public:
    void runAllTests() {
        ComprehensiveEngineTest tester;
        std::vector<TestResult> results;
        
        for (int engineID = 0; engineID < 57; ++engineID) {
            std::cout << "Testing " << getEngineName(engineID) << "...\n";
            
            auto result = tester.testEngine(engineID);
            results.push_back(result);
            
            if (!result.passed) {
                std::cout << "  FAILED: " << result.failureReason << "\n";
                std::cout << "  Confidence: " << result.confidence << "%\n";
            } else {
                std::cout << "  PASSED (Confidence: " << result.confidence << "%)\n";
            }
        }
        
        generateHTMLReport(results);
    }
    
    void generateHTMLReport(const std::vector<TestResult>& results) {
        // Create detailed HTML report with:
        // - Pass/fail status for each engine
        // - Spectrograms showing input vs output
        // - Measured metrics and expected ranges
        // - Confidence scores
        // - Specific failure reasons
    }
};
```

### Test Validation Criteria

#### Pass/Fail Thresholds
```cpp
struct ValidationCriteria {
    // Reverbs must add at least 100ms of tail
    float minReverbDecay = 0.1f;
    
    // Delays must have detectable echo
    float minDelayFeedback = 0.1f;
    
    // EQs must change spectrum by at least 3dB
    float minEQEffect = 1.4f; // Linear gain
    
    // Compressors must reduce dynamic range
    float minCompressionRatio = 1.5f;
    
    // Distortion must add harmonics
    float minTHD = 0.01f; // 1%
    
    // Filters must attenuate by at least 12dB/octave
    float minFilterSlope = 12.0f;
    
    // Modulation must have periodic variation
    float minModulationDepth = 0.05f; // 5%
    
    // Pitch shift accuracy
    float maxPitchError = 0.01f; // 1% frequency error
};
```

### Running the Test Suite

```bash
# Build test executable
g++ -o engine_test_suite engine_test_suite.cpp -ljuce_audio_basics -ljuce_core

# Run all tests
./engine_test_suite --all

# Run specific engine test
./engine_test_suite --engine=17  # Test HarmonicExciter

# Generate detailed report
./engine_test_suite --all --report=html --output=test_results.html

# Run with specific test signals
./engine_test_suite --all --test-signal=sine,noise,impulse

# Verbose mode with spectrograms
./engine_test_suite --all --verbose --save-spectrograms
```

### Expected Output

```
=== CHIMERA ENGINE TEST SUITE ===
Testing 57 engines...

[1/57] ClassicCompressor
  ✓ Dynamics reduction detected
  ✓ Attack/release timing correct
  ✓ Ratio within expected range
  PASSED (Confidence: 95%)

[2/57] VintageOptoCompressor  
  ✓ Optical compression behavior detected
  ✓ Soft knee characteristics present
  PASSED (Confidence: 92%)

[17/57] HarmonicExciter
  ✓ Harmonic content added
  ✓ Even/odd harmonics balanced
  ✓ Frequency-dependent processing
  PASSED (Confidence: 88%)

[34/57] PlateReverb
  ✗ No reverb tail detected
  ✗ Decay time < 100ms
  FAILED: Engine not processing audio
  Confidence: 15%

Summary:
  Passed: 51/57 (89.5%)
  Failed: 6/57
  
Failed Engines:
  - PlateReverb: No reverb tail
  - SpringReverb: Insufficient decay
  - GranularCloud: No grain generation
  - SpectralFreeze: No spectral holding
  - PitchShifter: Incorrect pitch
  - ConvolutionReverb: No impulse response

Report saved to: test_results.html
```

This comprehensive test suite will definitively determine which engines are working correctly and identify specific issues with those that aren't.