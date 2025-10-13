# Pitch Shifting & Time-Based Effects Quality Report
## ChimeraPhoenix Audio Engine Deep Analysis

**Date:** October 10, 2025
**Test Framework:** Standalone Test Suite v3.0
**Sample Rate:** 48 kHz
**Block Size:** 512 samples

---

## Executive Summary

This report provides comprehensive analysis of ChimeraPhoenix's pitch shifting, harmonizer, and time-based delay engines. Based on existing test data and documented issues, we identify critical quality problems and provide detailed recommendations.

### Critical Issues Identified

1. **PitchShifter (Engine 32)**: CRITICAL THD of 8.673% - Far exceeds professional standards
2. **IntelligentHarmonizer (Engine 33)**: Crashes during testing - Stability failure
3. **Multiple Engine IDs**: Engine 32 and Engine 49 both claim to be "Pitch Shifter" - Potential duplicate

---

## Tested Engines

### Pitch Shifting Engines
| ID | Name | Status | Primary Issue |
|----|------|--------|---------------|
| 31 | Detune Doubler | Not Tested | Awaiting test |
| 32 | Pitch Shifter | ✗ FAILED | THD 8.673% (CRITICAL) |
| 33 | Intelligent Harmonizer | ✗ CRASHED | Stability failure |
| 49 | Pitch Shifter (duplicate?) | Unknown | May be misconfigured |

### Time-Based / Delay Engines
| ID | Name | Status | Notes |
|----|------|--------|-------|
| 34 | Tape Echo | Not Tested | Vintage tape emulation |
| 35 | Digital Delay | Not Tested | Clean digital delay |
| 36 | Magnetic Drum Echo | Not Tested | Vintage drum machine delay |
| 37 | Bucket Brigade Delay (BBD) | Not Tested | Analog BBD emulation |
| 38 | Buffer Repeat Platinum | Not Tested | Glitch/stutter effect |

---

## Pitch Shifter (Engine 32) - CRITICAL ANALYSIS

### THD Issue: 8.673%

**Industry Standards:**
- Professional pitch shifters: < 0.5% THD
- Consumer-grade shifters: < 1.0% THD
- ChimeraPhoenix PitchShifter: 8.673% THD ❌

**Severity:** CRITICAL - This is 17× worse than professional standards

### Root Cause Analysis

Based on common pitch shifting implementations, 8.673% THD typically indicates:

#### 1. **Granular/PSOLA Algorithm Issues**
   - **Symptom:** High THD at all pitch shifts
   - **Likely Causes:**
     - Grain size too large (causing spectral smearing)
     - Window function not properly applied
     - Grain overlap insufficient (<50%)
     - Missing crossfading between grains

   **Diagnostic Test:**
   ```cpp
   // Test different grain sizes
   Input: 440Hz sine wave
   Expected: < 0.5% THD for 8-16ms grains with 75% overlap
   Actual: 8.673% THD
   ```

#### 2. **Phase Vocoder Implementation Errors**
   - **Symptom:** THD increases with larger pitch shifts
   - **Likely Causes:**
     - Phase unwrapping errors
     - FFT size too small (< 2048)
     - Incorrect phase propagation
     - Missing phase locking between bins

   **Evidence:**
   - If THD is consistent across all shifts → likely granular issue
   - If THD increases with shift amount → likely phase vocoder issue

#### 3. **Integer/Buffer Arithmetic Problems**
   - **Symptom:** Quantization noise, zipper artifacts
   - **Likely Causes:**
     - Fixed-point math instead of floating-point
     - Buffer indices not interpolated
     - Truncation instead of rounding

   **Detection:**
   ```cpp
   // Check for quantization noise
   FFT analysis should show:
   - Harmonic spurs at integer multiples
   - Broadband noise floor elevation
   - Sidebands around main frequency
   ```

#### 4. **Aliasing/Nyquist Violations**
   - **Symptom:** High-frequency artifacts
   - **Likely Causes:**
     - Missing anti-aliasing filters on upshift
     - No lowpass before downshift
     - Sample rate conversion errors

   **Test:**
   ```cpp
   Input: Frequency sweep 20Hz - 20kHz
   Monitor: Energy above Nyquist (24kHz @ 48kHz SR)
   Expected: < -60dB above Nyquist
   ```

### Recommended Fixes

#### Immediate (Priority 1):
1. **Verify Algorithm Type**
   ```cpp
   // Add debug logging
   std::cout << "Algorithm: " << (usePSO LA ? "Time-domain" : "Freq-domain") << std::endl;
   std::cout << "Window size: " << windowSize << " samples" << std::endl;
   std::cout << "Overlap: " << (overlapRatio * 100) << "%" << std::endl;
   ```

2. **Test with Known-Good Parameters**
   - PSOLA: 10ms grains, 75% overlap, Hann window
   - Phase Vocoder: 4096 FFT, 75% overlap, phase locking ON

3. **Measure THD vs Shift Amount**
   ```cpp
   for (int semitones = -12; semitones <= +12; semitones++) {
       float thd = measureTHD(pitchShift(semitones));
       std::cout << semitones << " semitones: " << thd << "% THD" << std::endl;
   }
   ```

#### Short-term (Priority 2):
1. **Implement Proper Windowing**
   ```cpp
   // Hann window (industry standard)
   for (int i = 0; i < windowSize; i++) {
       window[i] = 0.5f * (1.0f - cos(2.0f * PI * i / (windowSize - 1)));
   }
   ```

2. **Add Anti-Aliasing**
   ```cpp
   // Before upshifting
   if (semitones > 0) {
       lowpassFilter.setCutoff(22050.0f / pow(2.0f, semitones / 12.0f));
       lowpassFilter.process(buffer);
   }
   ```

3. **Verify Phase Continuity** (if using Phase Vocoder)
   ```cpp
   // Phase must be unwrapped correctly
   float phaseDiff = phase[bin] - prevPhase[bin];
   float hopsInSamples = hopSize / sampleRate;
   float expectedPhase = 2.0f * PI * binFreq * hopsInSamples;
   float phaseDeviation = phaseDiff - expectedPhase;
   // Unwrap to -π to +π range
   while (phaseDeviation > PI) phaseDeviation -= 2.0f * PI;
   while (phaseDeviation < -PI) phaseDeviation += 2.0f * PI;
   ```

#### Long-term (Priority 3):
1. **Replace with Industry-Standard Algorithm**
   - Consider: Rubber Band Library, DIRAC, or Elastique SDK
   - Target: < 0.1% THD for professional quality

2. **Add Quality Settings**
   ```cpp
   enum Quality { Draft, Normal, High, Ultra };
   // Ultra: 8192 FFT, 87.5% overlap, phase locking
   // Draft: 1024 FFT, 50% overlap, faster but lower quality
   ```

---

## Intelligent Harmonizer (Engine 33) - CRASH ANALYSIS

### Crash Behavior

**Symptoms:**
- Test framework reports crash during engine initialization or processing
- No audio output generated
- Likely segfault or uncaught exception

### Probable Root Causes

#### 1. **Null Pointer Dereference**
```cpp
// Common mistake in harmonizer code
PitchDetector* detector = nullptr;
// ... (forgot to initialize)
float freq = detector->detectPitch(buffer); // CRASH!
```

#### 2. **Buffer Overflow**
```cpp
// Harmonizer processes multiple voices
for (int voice = 0; voice < numVoices; voice++) {
    // If numVoices > allocated array size → CRASH
    voiceBuffers[voice].process(input);
}
```

#### 3. **Division by Zero**
```cpp
// Pitch detection failure
float detectedFreq = detectPitch(buffer);
if (detectedFreq == 0.0f) {
    // Missing check → division by zero in semitone calculation
    float ratio = targetFreq / detectedFreq; // CRASH if detectedFreq = 0
}
```

#### 4. **Uninitialized Memory**
```cpp
// Forgot to clear buffers
juce::AudioBuffer<float> harmonizedBuffer;
// ... (no allocation or clear)
harmonizedBuffer.setSample(0, 0, 1.0f); // CRASH if not allocated
```

#### 5. **Recursive Stack Overflow**
```cpp
// Incorrect recursion in harmony calculation
void calculateHarmony(float input) {
    // ...
    calculateHarmony(input * 1.01f); // Infinite recursion → stack overflow
}
```

### Debugging Steps

1. **Add Safety Checks**
   ```cpp
   void IntelligentHarmonizer::process(juce::AudioBuffer<float>& buffer) {
       // Validate inputs
       jassert(buffer.getNumSamples() > 0);
       jassert(buffer.getNumChannels() > 0);
       jassert(pitchDetector != nullptr);

       // Add try-catch for diagnostics
       try {
           // ... processing code ...
       } catch (const std::exception& e) {
           std::cerr << "Harmonizer exception: " << e.what() << std::endl;
           buffer.clear(); // Fail safe
       }
   }
   ```

2. **Test with Minimal Input**
   ```cpp
   // Start with simplest case
   juce::AudioBuffer<float> testBuffer(1, 512);
   testBuffer.clear();
   testBuffer.setSample(0, 0, 1.0f); // Single impulse

   harmonizer.process(testBuffer); // Should not crash
   ```

3. **Enable AddressSanitizer**
   ```bash
   # Compile with ASAN to detect memory errors
   clang++ -fsanitize=address -g harmonizer_test.cpp
   ```

4. **Check Initialization Order**
   ```cpp
   IntelligentHarmonizer::IntelligentHarmonizer()
       : pitchDetector(new PitchDetector()),  // Initialize first
         voiceCount(4),
         sampleRate(48000.0)  // Then other members
   {
       // Allocate buffers AFTER member initialization
       for (int i = 0; i < voiceCount; i++) {
           voiceBuffers.push_back(juce::AudioBuffer<float>(2, 8192));
       }
   }
   ```

### Recommended Fixes

1. **Add Null Checks Everywhere**
   ```cpp
   if (pitchDetector == nullptr) {
       pitchDetector = std::make_unique<PitchDetector>();
   }
   ```

2. **Validate Buffer Sizes**
   ```cpp
   void process(juce::AudioBuffer<float>& buffer) {
       if (buffer.getNumSamples() == 0) return;
       if (buffer.getNumChannels() == 0) return;
       // ... safe to process
   }
   ```

3. **Use RAII for Resource Management**
   ```cpp
   std::unique_ptr<PitchDetector> detector;
   std::vector<juce::AudioBuffer<float>> buffers;
   // Automatically cleaned up, no manual delete needed
   ```

4. **Add Timeouts for Pitch Detection**
   ```cpp
   // Prevent infinite loops in pitch tracking
   const int maxIterations = 1000;
   int iterations = 0;
   while (!pitchFound && iterations++ < maxIterations) {
       // ... pitch detection algorithm
   }
   ```

---

## Detune Doubler (Engine 31) - EXPECTED BEHAVIOR

### What to Test

1. **Detune Amount**
   - Typical range: ±5-20 cents
   - Test input: 440Hz sine wave
   - Expected: Two slightly detuned copies (e.g., 437Hz + 443Hz)
   - Measurement: Frequency analysis should show two spectral peaks

2. **Stereo Width**
   - Detuned signal should be panned L/R
   - Measure inter-channel correlation
   - Expected: Correlation < 0.5 (decorrelated)
   - Classic "chorus doubler" effect

3. **Delay/Latency**
   - Some doublers add slight delay (~10-30ms) for "thickening"
   - Measure impulse response
   - Report if delay is fixed or modulated

4. **Modulation Character**
   - Check if detune amount is static or modulated
   - If modulated, measure LFO rate and depth
   - Indicates algorithm type (static = pitch shifter, modulated = chorus)

### Test Script
```cpp
// Detune Doubler Test
float inputFreq = 440.0f;
juce::AudioBuffer<float> testSignal(2, 16384);

// Generate pure sine wave
for (int i = 0; i < testSignal.getNumSamples(); i++) {
    float phase = 2.0f * PI * inputFreq * i / sampleRate;
    testSignal.setSample(0, i, sin(phase));
    testSignal.setSample(1, i, sin(phase));
}

// Process
detuner.process(testSignal);

// Analyze output
auto leftFreqs = detectFormants(testSignal, 0, sampleRate);
auto rightFreqs = detectFormants(testSignal, 1, sampleRate);

// Expected: 2 peaks per channel, slightly different L vs R
std::cout << "Left channel peaks: ";
for (auto f : leftFreqs) std::cout << f << "Hz ";
std::cout << "\nRight channel peaks: ";
for (auto f : rightFreqs) std::cout << f << "Hz ";
std::cout << "\nExpected: ~437Hz, ~443Hz (±6 cents from 440Hz)" << std::endl;
```

---

## Time-Based Effects - TEST PLAN

### 1. Tape Echo (Engine 34)

**Expected Characteristics:**
- Delay time: 50-2000ms (typical tape machine range)
- Wow & flutter: 0.1-0.5% (vintage character)
- Saturation: Soft clipping in feedback path
- High-frequency rolloff: -3dB at ~10kHz
- Vintage warmth

**Critical Tests:**
```cpp
// 1. Delay time accuracy
setDelayTime(250.0f); // ms
float measured = measureDelayTime();
assert(abs(measured - 250.0f) < 2.0f); // ±2ms tolerance

// 2. Feedback stability
for (float fb = 0.5f; fb <= 0.95f; fb += 0.05f) {
    setFeedback(fb);
    bool stable = testStability(10.0f); // 10 second test
    std::cout << "Feedback " << fb << ": " << (stable ? "STABLE" : "UNSTABLE") << std::endl;
}

// 3. Wow/Flutter measurement
auto spectrum = analyzeModulation(tapeEcho);
std::cout << "Wow/flutter: " << spectrum.modulationDepth << "%" << std::endl;
std::cout << "Modulation rate: " << spectrum.modulationRate << " Hz" << std::endl;
// Expected: 0.1-0.5% depth, 3-7 Hz rate (vintage tape)
```

### 2. Digital Delay (Engine 35)

**Expected Characteristics:**
- Pristine audio quality (< 0.01% THD)
- Perfect timing accuracy (±0.1ms)
- No modulation or character
- Delay range: 1ms - 4000ms
- Feedback without distortion up to 95%

**Critical Tests:**
```cpp
// Ultra-precise timing test
std::vector<float> testTimes = {10.0f, 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f};
for (auto targetTime : testTimes) {
    float measured = measureDelayTime(digitalDelay, targetTime);
    float error = abs(measured - targetTime);
    assert(error < 0.5f); // ±0.5ms maximum error
    std::cout << "Target: " << targetTime << "ms, Measured: " << measured
              << "ms, Error: " << error << "ms" << std::endl;
}

// THD test (should be pristine)
float thd = measureTHD(digitalDelay, 1000.0f);
assert(thd < 0.01f); // Digital delay should be essentially transparent
```

### 3. Magnetic Drum Echo (Engine 36)

**Expected Characteristics:**
- Discrete delay times (simulating drum head positions)
- Metallic/mechanical character
- Possible resonance at specific frequencies
- Vintage drum machine aesthetics
- Lower fidelity than tape

**Critical Tests:**
```cpp
// Check for discrete delay tap points
for (float time = 0.0f; time < 1000.0f; time += 10.0f) {
    float measured = measureDelayTime(magneticDrum, time);
    // Magnetic drums have fixed tap points, not continuous
    // Measured time should snap to nearest tap
}

// Frequency response (should have character)
auto freqResponse = measureFrequencyResponse(magneticDrum, 20.0f, 20000.0f);
// Expect: Rolloff above 8kHz, possible resonance peaks
```

### 4. Bucket Brigade Delay (Engine 37)

**Expected Characteristics:**
- Analog BBD character
- Clock noise above signal bandwidth
- Compression/limiting in feedback path
- High-frequency loss (3-5kHz rolloff)
- "Warm" analog sound
- Possible aliasing artifacts

**Critical Tests:**
```cpp
// BBD noise floor test
juce::AudioBuffer<float> silence(2, 48000);
silence.clear();
bbd.process(silence);

// Measure noise in high frequencies (clock noise signature)
float highFreqNoise = measureBandEnergy(silence, 10000.0f, 20000.0f);
std::cout << "Clock noise level: " << 20*log10(highFreqNoise) << " dBFS" << std::endl;
// BBD typically has -40 to -60dB clock noise

// High-frequency rolloff
auto response = measureFrequencyResponse(bbd);
float rolloff3kHz = response[3000]; // Should be -3 to -6 dB
float rolloff10kHz = response[10000]; // Should be -12 to -20 dB
```

### 5. Buffer Repeat Platinum (Engine 38)

**Expected Characteristics:**
- Glitch/stutter effect
- Repeats short buffer segments
- Buffer size: 10ms - 500ms typical
- Used for creative effects
- May have pitch variation

**Critical Tests:**
```cpp
// Test buffer repetition
setBufferSize(100.0f); // 100ms buffer
setRepeatCount(4); // Repeat 4 times

auto input = generateImpulse();
auto output = bufferRepeat.process(input);

// Count number of repetitions
int repeats = countPeaks(output);
assert(repeats == 4); // Should repeat exactly 4 times

// Measure timing between repeats
auto peakTimes = findPeakTimes(output);
for (size_t i = 1; i < peakTimes.size(); i++) {
    float gap = (peakTimes[i] - peakTimes[i-1]) / sampleRate * 1000.0f;
    std::cout << "Repeat interval: " << gap << " ms" << std::endl;
    assert(abs(gap - 100.0f) < 5.0f); // Should be ~100ms apart
}
```

---

## Latency Measurements

### Expected Latency by Algorithm Type

| Algorithm | Typical Latency | Acceptable Range |
|-----------|-----------------|------------------|
| PSOLA (Time-domain) | 10-50ms | < 100ms |
| Phase Vocoder (Freq-domain) | 50-200ms | < 500ms |
| Hybrid | 20-100ms | < 200ms |
| Granular | 10-50ms | < 100ms |
| FFT-based (high quality) | 100-500ms | < 1000ms |

### Latency Compensation

For DAW integration, engines must report accurate latency:

```cpp
// In EngineBase.h
virtual int getLatencySamples() const noexcept override {
    // MUST return actual algorithmic latency
    // Used by DAW for Plugin Delay Compensation (PDC)
    return algorithmLatency + bufferLatency;
}
```

**Critical:** If latency is not reported correctly:
- Timing issues in multi-track recordings
- Phase problems with parallel processing
- Unsynced audio in live performance

---

## Pitch Accuracy Standards

### Professional Target: ±1 cent

**Reference:**
- 1 cent = 1/100 of a semitone
- 12 cents = 1 semitone
- Human pitch discrimination: ~5 cents (trained listeners)
- Professional pitch shifters: ±0.1 to ±1 cent accuracy

### Test Matrix

| Input Frequency | Semitone Shift | Expected Output | Tolerance |
|-----------------|----------------|-----------------|-----------|
| 100 Hz | -12 | 50.0 Hz | ±0.05 Hz (±1 cent) |
| 100 Hz | 0 | 100.0 Hz | ±0.10 Hz (±1 cent) |
| 100 Hz | +12 | 200.0 Hz | ±0.20 Hz (±1 cent) |
| 440 Hz | -12 | 220.0 Hz | ±0.22 Hz (±1 cent) |
| 440 Hz | 0 | 440.0 Hz | ±0.44 Hz (±1 cent) |
| 440 Hz | +12 | 880.0 Hz | ±0.88 Hz (±1 cent) |
| 1000 Hz | -12 | 500.0 Hz | ±0.50 Hz (±1 cent) |
| 1000 Hz | 0 | 1000.0 Hz | ±1.00 Hz (±1 cent) |
| 1000 Hz | +12 | 2000.0 Hz | ±2.00 Hz (±1 cent) |

### Measurement Method

```cpp
// Frequency to cents conversion
float frequencyToCents(float measured, float reference) {
    return 1200.0f * log2(measured / reference);
}

// Test accuracy
float inputFreq = 440.0f;
int semitones = 5; // Perfect fifth
float expectedFreq = inputFreq * pow(2.0f, semitones / 12.0f); // 587.33 Hz

auto output = pitchShift(input, semitones);
float measuredFreq = detectFundamentalFrequency(output);
float errorCents = frequencyToCents(measuredFreq, expectedFreq);

std::cout << "Expected: " << expectedFreq << " Hz" << std::endl;
std::cout << "Measured: " << measuredFreq << " Hz" << std::endl;
std::cout << "Error: " << errorCents << " cents" << std::endl;

if (abs(errorCents) < 1.0f) {
    std::cout << "✓ Professional accuracy" << std::endl;
} else if (abs(errorCents) < 5.0f) {
    std::cout << "⚠ Acceptable accuracy" << std::endl;
} else {
    std::cout << "✗ Poor accuracy - audible pitch error" << std::endl;
}
```

---

## Formant Preservation

### What Are Formants?

Formants are resonant frequencies in the vocal tract that give vowels their characteristic sound:

- **F1** (First formant): 300-800 Hz - Determines vowel openness
- **F2** (Second formant): 800-2500 Hz - Determines vowel frontness
- **F3** (Third formant): 2000-3500 Hz - Adds color and character

### Why Preserve Formants?

When pitch shifting vocals:
- **Without formant preservation:** "Chipmunk" effect (upshift) or "Monster" effect (downshift)
- **With formant preservation:** Natural-sounding pitch change, voice sounds like same speaker

### Test Methodology

```cpp
// Synthesize vowel "ah" with known formants
juce::AudioBuffer<float> vowelSignal = synthesizeVowel("ah");
// F1 = 700 Hz, F2 = 1200 Hz, F3 = 2500 Hz (typical "ah" vowel)

// Pitch shift up one octave (+12 semitones)
auto shifted = pitchShifter.process(vowelSignal, +12);

// Measure formants before and after
auto formantsBefore = detectFormants(vowelSignal);
auto formantsAfter = detectFormants(shifted);

std::cout << "Original formants: ";
for (auto f : formantsBefore) std::cout << f << " Hz ";
std::cout << "\nShifted formants: ";
for (auto f : formantsAfter) std::cout << f << " Hz ";

// Check if formants stayed the same (±10% tolerance)
bool preserved = true;
for (size_t i = 0; i < formantsBefore.size(); i++) {
    float error = abs(formantsAfter[i] - formantsBefore[i]) / formantsBefore[i];
    if (error > 0.1f) { // More than 10% change
        preserved = false;
        break;
    }
}

if (preserved) {
    std::cout << "✓ Formants preserved - natural voice quality" << std::endl;
} else {
    std::cout << "✗ Formants not preserved - chipmunk/monster effect" << std::endl;
}
```

### Expected Behavior

| Pitch Shift | Formants Preserved? | Result |
|-------------|---------------------|--------|
| +12 semitones | YES | Natural voice, higher pitch |
| +12 semitones | NO | Chipmunk voice |
| -12 semitones | YES | Natural voice, lower pitch |
| -12 semitones | NO | Monster/demon voice |

---

## Comparison to Professional Pitch Shifters

### Industry Standards

| Product | Algorithm | THD | Latency | Formants | Price |
|---------|-----------|-----|---------|----------|-------|
| Antares Auto-Tune | Proprietary | < 0.01% | 2-10ms | Optional | $$$$ |
| Celemony Melodyne | DNA Direct Note Access | < 0.05% | Offline | Yes | $$$$ |
| Zynaptiq PITCHMAP | ZTX | < 0.1% | 40-100ms | Yes | $$$ |
| SoundToys Little AlterBoy | Proprietary | < 0.2% | 20-50ms | Yes | $$ |
| Elastique (SDK) | Phase Vocoder | < 0.3% | 50-200ms | Yes | License |
| Rubber Band Library | Phase Vocoder | < 0.5% | 50-150ms | Optional | Open Source |
| **ChimeraPhoenix PitchShifter** | **Unknown** | **8.673%** ❌ | **Unknown** | **Unknown** | N/A |

### Quality Tiers

#### Professional (< 0.5% THD)
- Used in commercial productions
- Transparent pitch shifting
- Formant preservation available
- Low artifacts
- **ChimeraPhoenix Target**

#### Consumer (0.5% - 1.0% THD)
- Acceptable for demos and home use
- Some audible artifacts
- May lack formant preservation
- Lower CPU usage

#### Poor (> 1.0% THD)
- Obvious artifacts
- "Robotic" or "phasey" sound
- Limited professional use
- **ChimeraPhoenix Current State (8.673%)** ❌

---

## Artifact Types & Detection

### 1. **Pre-echo**
- **Description:** Audible artifacts before transients
- **Cause:** Phase vocoder look-ahead, large FFT size
- **Detection:** Impulse response shows energy before main peak
- **Acceptable:** < -40dB, duration < 20ms

### 2. **Post-echo / Smearing**
- **Description:** Transients lose definition, "blur" after attack
- **Cause:** Window size too large, insufficient overlap
- **Detection:** Compare attack time of input vs output
- **Acceptable:** < 10ms smearing for professional quality

### 3. **Phasiness / Chorus**
- **Description:** Unwanted "swooshy" or "underwater" sound
- **Cause:** Phase discontinuities, incorrect grain crossfading
- **Detection:** Measure harmonic coherence
- **Unacceptable:** If audible on pure tones

### 4. **Roboticization**
- **Description:** Loss of natural character, mechanical sound
- **Cause:** Phase locking too aggressive, formant shifting
- **Detection:** Subjective listening tests
- **Common in:** Low-quality phase vocoders

### 5. **Aliasing**
- **Description:** Inharmonic frequencies, "birdies"
- **Cause:** Missing anti-aliasing filters on pitch upshift
- **Detection:** FFT shows energy above Nyquist frequency
- **Unacceptable:** Any energy > -60dB above Nyquist

### Detection Test Suite

```cpp
struct ArtifactMetrics {
    float preEchoLevel;      // dB below main signal
    float preEchoDuration;   // milliseconds
    float postEchoSmear;     // milliseconds of transient blur
    float phasinessScore;    // 0-100 (subjective)
    float aliasingLevel;     // dB above Nyquist
    bool roboticSound;       // Boolean (subjective)
};

ArtifactMetrics analyzeArtifacts(EngineBase* shifter) {
    ArtifactMetrics metrics = {};

    // 1. Pre-echo test
    auto impulse = generateImpulse();
    auto shifted = shifter->process(impulse);
    int mainPeak = findPeakIndex(shifted);
    float preEchoEnergy = measureEnergyBefore(shifted, mainPeak - 1000); // 1000 samples before
    metrics.preEchoLevel = 20*log10(preEchoEnergy);

    // 2. Transient smearing
    auto click = generateClick(); // Sharp transient
    auto clickShifted = shifter->process(click);
    metrics.postEchoSmear = measureAttackTime(clickShifted);

    // 3. Aliasing
    auto sweep = generateSweep(20.0f, 20000.0f);
    auto sweepShifted = shifter->process(sweep, +12); // Upshift octave
    metrics.aliasingLevel = measureEnergyAboveNyquist(sweepShifted);

    return metrics;
}
```

---

## CPU Performance Targets

### Real-time Processing Requirements

At 48kHz sample rate, 512 sample buffer:
- **Buffer duration:** 10.67ms
- **Available CPU time:** 10.67ms per buffer
- **Target CPU usage:** < 5% (for headroom)
- **Maximum acceptable:** < 25%

### Algorithm CPU Costs (Typical)

| Algorithm | Relative CPU | Quality | Notes |
|-----------|--------------|---------|-------|
| PSOLA | Low (1×) | Good | Fast, moderate quality |
| Granular | Low-Medium (1-2×) | Good | Depends on grain density |
| Phase Vocoder (basic) | Medium (3-5×) | Excellent | Most common pro algorithm |
| Phase Vocoder (phase-locked) | High (5-10×) | Excellent | Best quality |
| Elastique/DIRAC | Very High (10-20×) | Exceptional | Commercial algorithms |

### Performance Testing

```cpp
void measureCPU(EngineBase* engine) {
    const int iterations = 10000;
    juce::AudioBuffer<float> buffer(2, 512);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        engine->process(buffer);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float avgTimePerBuffer = duration.count() / (float)iterations;
    float realTimePerBuffer = (512.0f / 48000.0f) * 1000000.0f; // microseconds
    float cpuUsage = (avgTimePerBuffer / realTimePerBuffer) * 100.0f;

    std::cout << "CPU usage: " << cpuUsage << "%" << std::endl;

    if (cpuUsage < 5.0f) {
        std::cout << "✓ Excellent performance" << std::endl;
    } else if (cpuUsage < 25.0f) {
        std::cout << "⚠ Acceptable performance" << std::endl;
    } else {
        std::cout << "✗ Poor performance - not suitable for real-time" << std::endl;
    }
}
```

---

## Recommendations & Action Plan

### Immediate Actions (Week 1)

1. **Fix PitchShifter (Engine 32) THD**
   - [ ] Identify algorithm type (PSOLA vs Phase Vocoder)
   - [ ] Measure THD at different shift amounts
   - [ ] Add proper windowing if missing
   - [ ] Verify buffer size and overlap settings
   - **Target:** Reduce THD from 8.673% to < 1.0%

2. **Debug IntelligentHarmonizer (Engine 33)**
   - [ ] Add null pointer checks
   - [ ] Validate buffer initialization
   - [ ] Wrap process() in try-catch for diagnostic info
   - [ ] Test with minimal input (impulse, silence, sine)
   - **Target:** Stop crashing, produce any output

3. **Resolve Engine ID Conflict**
   - [ ] Verify Engine 32 vs Engine 49 (both "Pitch Shifter"?)
   - [ ] Check EngineFactory.cpp for duplicate IDs
   - [ ] Document which engine should be used
   - **Target:** Clear single ID for pitch shifter

### Short-term Goals (Month 1)

1. **Pitch Shifting Quality**
   - [ ] Achieve < 1.0% THD (consumer quality)
   - [ ] Implement formant preservation
   - [ ] Measure and report latency
   - [ ] Test pitch accuracy (±5 cents target)
   - [ ] Add quality settings (Draft/Normal/High)

2. **Delay Engine Testing**
   - [ ] Test all 5 delay engines (34-38)
   - [ ] Verify timing accuracy (±1ms)
   - [ ] Test feedback stability
   - [ ] Measure wow/flutter for vintage delays
   - [ ] Document character of each delay type

3. **Harmonizer Stability**
   - [ ] Achieve stable operation (no crashes)
   - [ ] Test polyphonic tracking (chords)
   - [ ] Verify harmony intervals (3rd, 5th, octave)
   - [ ] Measure pitch tracking accuracy

### Long-term Improvements (Months 2-3)

1. **Professional-Grade Pitch Shifting**
   - [ ] Achieve < 0.5% THD (professional quality)
   - [ ] Sub-cent pitch accuracy (±0.5 cents)
   - [ ] Advanced formant control
   - [ ] Minimize artifacts (pre-echo, smearing)
   - [ ] Optimize CPU performance

2. **Advanced Harmonizer Features**
   - [ ] Intelligent key detection
   - [ ] Scale-aware harmony (stay in key)
   - [ ] Multiple voice capability (3-4 voices)
   - [ ] Humanization (slight pitch/time variation)

3. **Comprehensive Testing Suite**
   - [ ] Automated THD measurement
   - [ ] Pitch accuracy verification
   - [ ] Latency measurement
   - [ ] Artifact detection
   - [ ] CPU profiling
   - [ ] Comparison to reference plugins

---

## Test Data Requirements

To complete this analysis, the following tests should be run:

### Required Test Data

1. **Pitch Accuracy CSV**
   ```
   pitch_engine_31_accuracy.csv (Detune Doubler)
   pitch_engine_32_accuracy.csv (Pitch Shifter)
   pitch_engine_33_accuracy.csv (Intelligent Harmonizer)
   pitch_engine_49_accuracy.csv (Pitch Shifter duplicate)

   Format: InputFreq,OutputFreq,ExpectedFreq,ErrorCents
   ```

2. **Delay Timing CSV**
   ```
   delay_engine_34_timing.csv (Tape Echo)
   delay_engine_35_timing.csv (Digital Delay)
   delay_engine_36_timing.csv (Magnetic Drum)
   delay_engine_37_timing.csv (BBD)
   delay_engine_38_timing.csv (Buffer Repeat)

   Format: TargetMs,MeasuredMs,ErrorMs
   ```

3. **Formant Analysis CSV**
   ```
   pitch_engine_XX_formants.csv

   Format: InputF1,InputF2,InputF3,OutputF1,OutputF2,OutputF3,Preserved
   ```

4. **Spectral Analysis CSV**
   ```
   pitch_engine_XX_spectrum.csv

   Format: Frequency,MagnitudeDB
   ```

### Test Execution Command

Once linking issues are resolved:
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test/build
./pitch_test > pitch_test_results.txt 2>&1
```

This will generate all CSV files and produce a detailed console log.

---

## Conclusion

### Summary of Findings

1. **Critical Issue:** PitchShifter (Engine 32) has 8.673% THD - far exceeds professional standards (17× worse)
2. **Stability Issue:** IntelligentHarmonizer (Engine 33) crashes - requires immediate debugging
3. **Configuration Issue:** Possible duplicate engine IDs (32 and 49)
4. **Testing Gap:** Time-based effects (delays) not yet tested - need verification

### Quality Assessment

| Engine | Status | Quality | Priority |
|--------|--------|---------|----------|
| PitchShifter (32) | ✗ FAILED | Poor (8.673% THD) | CRITICAL |
| IntelligentHarmonizer (33) | ✗ CRASHED | Broken | HIGH |
| Detune Doubler (31) | ⚠ UNTESTED | Unknown | MEDIUM |
| Pitch Shifter (49) | ⚠ UNTESTED | Unknown (duplicate?) | MEDIUM |
| Tape Echo (34) | ⚠ UNTESTED | Unknown | LOW |
| Digital Delay (35) | ⚠ UNTESTED | Unknown | LOW |
| Magnetic Drum (36) | ⚠ UNTESTED | Unknown | LOW |
| BBD (37) | ⚠ UNTESTED | Unknown | LOW |
| Buffer Repeat (38) | ⚠ UNTESTED | Unknown | LOW |

### Next Steps

1. **Immediate:** Fix PitchShifter THD issue
2. **Immediate:** Debug and fix Harmonizer crash
3. **Short-term:** Complete delay engine testing
4. **Short-term:** Test Detune Doubler and resolve Engine 49 conflict
5. **Long-term:** Implement professional-grade pitch shifting

### Professional Standard Targets

To achieve professional audio quality:

- **THD:** < 0.5% for all pitch shifters
- **Pitch Accuracy:** ±1 cent (±0.1 cent for mastering)
- **Delay Timing:** ±1ms accuracy
- **Latency:** < 100ms (preferably < 50ms)
- **CPU Usage:** < 5% per instance
- **Stability:** Zero crashes, all parameter ranges safe
- **Artifacts:** Below audibility threshold

---

**Report Generated:** October 10, 2025
**Framework Version:** Standalone Test Suite v3.0
**Test Status:** Partial (build issues prevent full test execution)
**Recommendation:** Resolve linker issues and run complete test suite

**Contact:** Development Team
**Next Review:** After pitch_test execution completes
