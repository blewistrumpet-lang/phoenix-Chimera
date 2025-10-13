# ChimeraPhoenix Bug Fix Patterns
## Common Issues and Reusable Solutions

**Document Version**: 1.0
**Last Updated**: October 11, 2025
**Purpose**: Catalog recurring bug patterns and proven solutions for rapid debugging

---

## Table of Contents

1. [Pre-delay Buffer Read-Before-Write](#1-pre-delay-buffer-read-before-write)
2. [Pitch Shifter Latency Issues](#2-pitch-shifter-latency-issues)
3. [Mix Parameter Defaults](#3-mix-parameter-defaults)
4. [FFT Initialization Crashes](#4-fft-initialization-crashes)
5. [Parameter Smoothing Artifacts](#5-parameter-smoothing-artifacts)
6. [Filter Instability at High Resonance](#6-filter-instability-at-high-resonance)
7. [Stereo Correlation Issues](#7-stereo-correlation-issues)
8. [CPU Hotspots](#8-cpu-hotspots)
9. [Memory Leaks in Process](#9-memory-leaks-in-process)
10. [Build System Issues](#10-build-system-issues)

---

## 1. Pre-delay Buffer Read-Before-Write

### Pattern Name
**Circular Buffer Initialization Bug**

### Symptoms
- Zero output after initial impulse
- Reverb tail completely silent
- Delays produce no echo
- First N samples work, then silence (N = buffer size)

### Root Cause
Reading from circular buffer before writing to it during fill-up period. Buffer contains uninitialized data (zeros) until fully written.

### Affected Engines
- ✅ Engine 39 (PlateReverb) - **FIXED**
- ⚠️ Engine 33 (IntelligentHarmonizer) - **Suspected**
- ⚠️ Any engine with delay lines or pre-delay

### Bug Example
```cpp
// WRONG: Read before write
if (predelaySize > 0) {
    delayedL = predelayBufferL[predelayIndex];  // Reads zeros during fill-up!
    delayedR = predelayBufferR[predelayIndex];

    predelayBufferL[predelayIndex] = inputL;    // Writes too late
    predelayBufferR[predelayIndex] = inputR;

    if (++predelayIndex >= predelaySize) {
        predelayIndex = 0;
    }
}
```

**Problem**: During the first `predelaySize` samples, reads happen from uninitialized buffer locations containing zeros.

### Solution Pattern
```cpp
// CORRECT: Write before read
if (predelaySize > 0) {
    // 1. Write current input to buffer FIRST
    predelayBufferL[predelayIndex] = inputL;
    predelayBufferR[predelayIndex] = inputR;

    // 2. Calculate read index (predelaySize samples ago, with wraparound)
    int readIndex = predelayIndex - predelaySize;
    if (readIndex < 0) {
        readIndex += static_cast<int>(predelayBufferL.size());
    }

    // 3. Read delayed signal
    delayedL = predelayBufferL[readIndex];
    delayedR = predelayBufferR[readIndex];

    // 4. Advance write pointer
    if (++predelayIndex >= static_cast<int>(predelayBufferL.size())) {
        predelayIndex = 0;
    }
}
```

### Key Points
1. **Always write before read**
2. **Use full buffer size** for wraparound check (not delay size)
3. **Calculate read index** as: `(writeIndex - delaySize + bufferSize) % bufferSize`
4. **Ensure buffer is larger than delay**: `bufferSize >= delaySize`

### Test Verification
```cpp
// Impulse response test
buffer[0] = 1.0f; // Impulse
process(buffer);

// Check for output after delay period
assert(buffer[delaySize + 1] != 0.0f); // Should have signal
```

### Applies To
- Delay lines
- Pre-delay buffers
- Comb filters
- Allpass filters
- Echo buffers
- Any circular buffer implementation

### Files to Audit
```bash
# Search for potential issues
grep -n "Buffer\[.*index\]" JUCE_Plugin/Source/*.cpp | grep -B2 "index.*="
```

---

## 2. Pitch Shifter Latency Issues

### Pattern Name
**Grain Processing Latency Compensation**

### Symptoms
- Initial impulse passes through
- Then silence for N milliseconds
- Pitch-shifted signal appears late
- Sounds like a pre-delay effect

### Root Cause
Pitch shifters using granular or PSOLA methods need to fill grain buffer before output begins. This creates latency that must be compensated.

### Affected Engines
- ⚠️ Engine 32 (Pitch Shifter) - High THD issue
- ⚠️ Engine 33 (IntelligentHarmonizer) - Zero output
- ⚠️ Engine 40 (ShimmerReverb) - Uses pitch shifting

### Bug Example
```cpp
// Simplified pitch shifter
class PitchShifter {
    static const int GRAIN_SIZE = 2048;
    float grainBuffer[GRAIN_SIZE];
    int grainPosition = 0;

    float process(float input) {
        grainBuffer[grainPosition++] = input;

        // BUG: No output until grain is full!
        if (grainPosition < GRAIN_SIZE) {
            return 0.0f; // Silence during fill-up
        }

        // Process grain
        return processGrain();
    }
};
```

### Solution Pattern
```cpp
class PitchShifter {
    static const int GRAIN_SIZE = 2048;
    static const int OVERLAP = 4;  // 4x overlap for smooth transitions

    std::vector<float> inputBuffer;
    std::vector<float> outputBuffer;
    int writePos = 0;
    int readPos = 0;
    bool initialized = false;

    float process(float input) {
        // Write to input buffer
        inputBuffer[writePos] = input;
        writePos = (writePos + 1) % inputBuffer.size();

        // Return dry signal during initialization
        if (!initialized) {
            if (writePos >= GRAIN_SIZE) {
                initialized = true;
            }
            return input; // Pass through during warm-up
        }

        // Process grains with overlap
        float output = processOverlappedGrains();

        return output;
    }

    float processOverlappedGrains() {
        float output = 0.0f;
        int hopSize = GRAIN_SIZE / OVERLAP;

        for (int i = 0; i < OVERLAP; ++i) {
            int grainStart = (readPos - i * hopSize + inputBuffer.size()) % inputBuffer.size();
            float grain = processGrain(grainStart);
            float window = hannWindow(i, OVERLAP);
            output += grain * window;
        }

        readPos = (readPos + 1) % inputBuffer.size();
        return output / OVERLAP;
    }
};
```

### Key Points
1. **Pre-fill buffer** with dry signal during initialization
2. **Use overlapping grains** (4x overlap typical)
3. **Apply windowing** (Hann, Blackman-Harris) to prevent clicks
4. **Compensate latency** by reporting latency to host
5. **Crossfade** between grains for smooth transitions

### Windowing Functions
```cpp
// Hann window
float hannWindow(float position, float size) {
    return 0.5f * (1.0f - std::cos(2.0f * M_PI * position / size));
}

// Blackman-Harris window (better sidelobe rejection)
float blackmanHarrisWindow(float position, float size) {
    float a0 = 0.35875f;
    float a1 = 0.48829f;
    float a2 = 0.14128f;
    float a3 = 0.01168f;

    float phase = 2.0f * M_PI * position / size;
    return a0 - a1 * std::cos(phase) +
           a2 * std::cos(2.0f * phase) -
           a3 * std::cos(3.0f * phase);
}
```

### THD Reduction
Windowing is critical for reducing THD in pitch shifters:

| Method | Typical THD |
|--------|-------------|
| Rectangular (no window) | 5-10% |
| Triangular | 2-5% |
| Hann | 0.5-2% |
| Blackman-Harris | 0.1-0.5% |

---

## 3. Mix Parameter Defaults

### Pattern Name
**Dry/Wet Mix Testing Configuration**

### Symptoms
- Reverbs appear mono when tested
- Frequency response shows -6dB (sum of dry + wet)
- RT60 measures as 0 seconds
- Effect appears weak or not working

### Root Cause
Default mix parameter is often 50% dry / 50% wet. For testing reverb characteristics, need 100% wet to isolate the effect.

### Affected Engines
- All reverb engines (39-43)
- All delay engines (34-38)
- Any effect with dry/wet mix

### Bug Example
```cpp
// Test code
void testReverb(EngineBase* engine) {
    engine->prepareToPlay(48000.0, 512);
    // BUG: No parameters set - using defaults (50% mix)

    // Generate impulse
    buffer[0] = 1.0f;
    engine->process(buffer);

    // Measure RT60
    float rt60 = measureRT60(buffer);
    // WRONG: Measures ~0s because 50% dry signal dominates
}
```

### Solution Pattern
```cpp
void testReverb(EngineBase* engine) {
    engine->prepareToPlay(48000.0, 512);

    // Set to 100% wet for testing
    engine->setParameter(0, 1.0f); // Assuming param 0 is mix

    // Generate impulse
    buffer[0] = 1.0f;
    engine->process(buffer);

    // Now RT60 measures correctly
    float rt60 = measureRT60(buffer);
}
```

### Parameter Standards
```cpp
// Standard parameter indices (if possible, make consistent)
enum CommonParameters {
    MIX = 0,      // 0.0 = 100% dry, 1.0 = 100% wet
    SIZE = 1,     // Room size / delay time
    DECAY = 2,    // Feedback / decay time
    DAMPING = 3,  // High-frequency damping
    PREDELAY = 4, // Pre-delay time
    WIDTH = 5     // Stereo width
};
```

### Test Presets
```cpp
struct TestPreset {
    std::string name;
    std::map<int, float> parameters;
};

// Standard test presets
TestPreset TEST_100_WET = {"100% Wet", {{0, 1.0f}}};
TestPreset TEST_LARGE_ROOM = {"Large Room", {{0, 1.0f}, {1, 0.8f}, {2, 0.9f}}};
TestPreset TEST_SMALL_ROOM = {"Small Room", {{0, 1.0f}, {1, 0.3f}, {2, 0.5f}}};
```

---

## 4. FFT Initialization Crashes

### Pattern Name
**Spectral Processing Initialization Failure**

### Symptoms
- Crash on engine startup (before audio processing)
- Crash in prepareToPlay()
- Null pointer dereference
- Invalid FFT size error

### Affected Engines
- ❌ Engine 52 (Spectral Gate) - Crashes on startup

### Bug Example
```cpp
class SpectralGate {
    std::unique_ptr<juce::dsp::FFT> fft;
    float* fftBuffer = nullptr;
    int fftOrder = 0;

    void prepareToPlay(double sampleRate, int blockSize) {
        // BUG: fftOrder not set
        fft.reset(new juce::dsp::FFT(fftOrder)); // Crash! fftOrder = 0

        // BUG: Buffer size calculation wrong
        fftBuffer = new float[fftSize]; // fftSize undefined!
    }
};
```

### Solution Pattern
```cpp
class SpectralGate {
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> fftBuffer;
    int fftOrder = 10; // Initialize to valid value

    void prepareToPlay(double sampleRate, int blockSize) {
        // 1. Validate FFT order
        fftOrder = std::max(1, std::min(15, fftOrder)); // JUCE FFT: 1-15

        // 2. Calculate FFT size
        int fftSize = 1 << fftOrder; // Power of 2

        // 3. Allocate buffers BEFORE creating FFT
        fftBuffer.resize(fftSize * 2, 0.0f); // *2 for complex data

        // 4. Create FFT object
        fft.reset(new juce::dsp::FFT(fftOrder));

        // 5. Verify allocation
        if (!fft) {
            throw std::runtime_error("FFT allocation failed");
        }
    }

    void process(float** channels, int numSamples) {
        // Check FFT is initialized
        if (!fft || fftBuffer.empty()) {
            return; // Failsafe: pass through
        }

        // Process...
    }
};
```

### Key Points
1. **Validate FFT order** (1-15 for JUCE FFT)
2. **Calculate size as power of 2**: `fftSize = 1 << fftOrder`
3. **Allocate buffers before FFT creation**
4. **Use std::vector** instead of raw pointers (RAII)
5. **Add null pointer checks** in process()
6. **Provide failsafe** (pass through if not initialized)

### FFT Size Guidelines
```cpp
// Recommended FFT sizes for different applications
int getRecommendedFFTSize(double sampleRate, const std::string& application) {
    if (application == "spectral_gate") {
        return 2048; // Order 11: Good frequency resolution
    } else if (application == "pitch_detection") {
        return 4096; // Order 12: Better pitch accuracy
    } else if (application == "vocoder") {
        return 1024; // Order 10: Lower latency
    } else if (application == "freeze") {
        return 8192; // Order 13: High resolution
    }
    return 2048; // Default
}
```

---

## 5. Parameter Smoothing Artifacts

### Pattern Name
**Discontinuous Parameter Changes**

### Symptoms
- Clicks when changing parameters
- Zipper noise on modulation
- Audible steps in sweeps
- Pops at parameter automation

### Root Cause
Parameters change instantaneously instead of ramping smoothly.

### Bug Example
```cpp
void setParameter(int index, float value) {
    if (index == 0) {
        cutoffFreq = value * 20000.0f; // Instant change - CLICK!
    }
}

void process(float* buffer, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Filter uses instant cutoffFreq value
        output[i] = filter.process(input[i], cutoffFreq);
    }
}
```

### Solution Pattern
```cpp
class ParameterSmoother {
    float currentValue = 0.0f;
    float targetValue = 0.0f;
    float rampSamples = 0.0f;
    float increment = 0.0f;

public:
    void setTarget(float target, double sampleRate, float rampTimeMs = 10.0f) {
        targetValue = target;
        rampSamples = (rampTimeMs / 1000.0f) * sampleRate;
        increment = (targetValue - currentValue) / rampSamples;
    }

    float getNextValue() {
        if (std::abs(targetValue - currentValue) < 0.0001f) {
            currentValue = targetValue; // Snap to target
            return currentValue;
        }

        currentValue += increment;
        return currentValue;
    }
};

// Usage
class Filter {
    ParameterSmoother cutoffSmoother;

    void setParameter(int index, float value) {
        if (index == 0) {
            cutoffSmoother.setTarget(value * 20000.0f, sampleRate, 10.0f);
        }
    }

    void process(float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            float smoothedCutoff = cutoffSmoother.getNextValue();
            output[i] = filter.process(input[i], smoothedCutoff);
        }
    }
};
```

### Alternative: JUCE SmoothedValue
```cpp
#include <juce_dsp/juce_dsp.h>

class Filter {
    juce::SmoothedValue<float> cutoffSmoother;

    void prepareToPlay(double sampleRate, int blockSize) {
        cutoffSmoother.reset(sampleRate, 0.01); // 10ms ramp
    }

    void setParameter(int index, float value) {
        if (index == 0) {
            cutoffSmoother.setTargetValue(value * 20000.0f);
        }
    }

    void process(float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            float smoothedCutoff = cutoffSmoother.getNextValue();
            output[i] = filter.process(input[i], smoothedCutoff);
        }
    }
};
```

### Ramp Time Guidelines
| Parameter Type | Ramp Time |
|----------------|-----------|
| Filter cutoff | 5-10ms |
| Gain/Volume | 10-20ms |
| Pan | 5-10ms |
| Mix | 10-20ms |
| Delay time | 50-100ms (pitch artifacts) |
| Reverb size | 100-200ms |

---

## 6. Filter Instability at High Resonance

### Pattern Name
**Resonance Feedback Explosion**

### Symptoms
- Loud burst at high Q values
- Self-oscillation when not expected
- Signal explodes to clipping
- NaN or Inf in output

### Root Cause
Feedback gain > 1.0 causing exponential growth.

### Bug Example
```cpp
float process(float input, float resonance) {
    float feedback = resonance * 4.0f; // Can exceed 1.0!

    for (int i = 0; i < 4; ++i) {
        stages[i] = tanh(input + feedback * stages[3]);
        // Feedback > 1.0 causes explosion!
    }

    return stages[3];
}
```

### Solution Pattern
```cpp
float process(float input, float resonance) {
    // 1. Clamp resonance to safe range
    resonance = std::clamp(resonance, 0.0f, 0.995f); // Never quite 1.0

    // 2. Calculate feedback with thermal compensation
    float feedback = resonance * (1.0f + 0.3f * resonance);
    feedback = std::min(feedback, 0.999f); // Hard limit

    // 3. Apply saturation to prevent explosion
    for (int i = 0; i < 4; ++i) {
        float stage_in = input + feedback * fastTanh(stages[3]);
        stages[i] = fastTanh(stage_in);
    }

    // 4. Output limiting
    float output = stages[3];
    output = std::clamp(output, -1.0f, 1.0f);

    // 5. NaN check (safety)
    if (std::isnan(output) || std::isinf(output)) {
        reset();
        return 0.0f;
    }

    return output;
}
```

### Key Points
1. **Clamp resonance** to < 1.0 (0.995 typical max)
2. **Limit feedback gain** before applying
3. **Apply saturation** at each stage
4. **Output limiting** as final safety
5. **NaN/Inf checks** for recovery

### Self-Oscillation (Intentional)
If self-oscillation is desired (synth filter):
```cpp
float process(float input, float resonance, bool allowOscillation) {
    float maxResonance = allowOscillation ? 1.01f : 0.995f;
    resonance = std::clamp(resonance, 0.0f, maxResonance);

    // When resonance > 1.0, filter oscillates even with zero input
    // This is the classic Moog "whistle" effect
}
```

---

## 7. Stereo Correlation Issues

### Pattern Name
**Mono Output from Stereo Processor**

### Symptoms
- Stereo correlation near 1.0 (mono)
- No stereo width despite stereo processing
- L and R channels identical

### Affected Engines
- ⚠️ Engine 40 (ShimmerReverb) - 0.889 correlation

### Root Cause
Processing stereo as mono, or missing stereo decorrelation.

### Bug Example
```cpp
void process(float** channels, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // BUG: Sum to mono before processing
        float mono = (channels[0][i] + channels[1][i]) * 0.5f;

        // Process mono signal
        float processed = pitchShift(mono);

        // BUG: Output same signal to both channels
        channels[0][i] = processed;
        channels[1][i] = processed; // Identical = mono!
    }
}
```

### Solution Pattern A: Separate L/R Processing
```cpp
void process(float** channels, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Process channels independently
        float processedL = pitchShift(channels[0][i], 0); // Left instance
        float processedR = pitchShift(channels[1][i], 1); // Right instance

        channels[0][i] = processedL;
        channels[1][i] = processedR;
    }
}
```

### Solution Pattern B: Stereo Spread
```cpp
void process(float** channels, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        float mono = (channels[0][i] + channels[1][i]) * 0.5f;
        float processed = pitchShift(mono);

        // Add stereo spread via decorrelation
        float spreadAmount = 0.3f; // 30% decorrelation

        // Use allpass filters with different delays for L/R
        float decorrelatedL = allpassL.process(processed);
        float decorrelatedR = allpassR.process(processed);

        channels[0][i] = processed * (1.0f - spreadAmount) + decorrelatedL * spreadAmount;
        channels[1][i] = processed * (1.0f - spreadAmount) + decorrelatedR * spreadAmount;
    }
}
```

### Solution Pattern C: Mid-Side Processing
```cpp
void process(float** channels, int numSamples) {
    for (int i = 0; i < numSamples; ++i) {
        // Encode to Mid-Side
        float mid = (channels[0][i] + channels[1][i]) * 0.5f;
        float side = (channels[0][i] - channels[1][i]) * 0.5f;

        // Process Mid and Side separately
        float processedMid = pitchShift(mid);
        float processedSide = pitchShift(side) * 1.5f; // Boost side for width

        // Decode to L/R
        channels[0][i] = processedMid + processedSide;
        channels[1][i] = processedMid - processedSide;
    }
}
```

### Allpass Decorrelation
```cpp
class AllpassDecorrelator {
    std::vector<float> buffer;
    int writePos = 0;
    int delayL = 37; // Prime number (in samples)
    int delayR = 41; // Different prime number

public:
    void init(int maxDelay) {
        buffer.resize(maxDelay * 2, 0.0f);
    }

    void process(float input, float& left, float& right) {
        buffer[writePos] = input;

        int readPosL = (writePos - delayL + buffer.size()) % buffer.size();
        int readPosR = (writePos - delayR + buffer.size()) % buffer.size();

        left = buffer[readPosL];
        right = buffer[readPosR];

        writePos = (writePos + 1) % buffer.size();
    }
};
```

---

## 8. CPU Hotspots

### Pattern Name
**Performance Optimization Checklist**

### Symptoms
- CPU usage > 5% per engine
- Crackles/dropouts at low buffer sizes
- Fan spins up during processing

### Common Hotspots

#### 1. Expensive Math Functions
```cpp
// SLOW
float output = std::sin(phase);
float saturated = std::tanh(input);
float root = std::sqrt(value);

// FAST: Use lookup tables
class FastMath {
    static const int TABLE_SIZE = 1024;
    float sinTable[TABLE_SIZE];
    float tanhTable[TABLE_SIZE];

    void init() {
        for (int i = 0; i < TABLE_SIZE; ++i) {
            float x = (float)i / TABLE_SIZE;
            sinTable[i] = std::sin(x * 2.0f * M_PI);
            tanhTable[i] = std::tanh(x * 10.0f - 5.0f); // -5 to +5
        }
    }

    float fastSin(float x) {
        int index = (int)(fmod(x / (2.0f * M_PI), 1.0f) * TABLE_SIZE);
        return sinTable[index];
    }

    float fastTanh(float x) {
        x = std::clamp(x, -5.0f, 5.0f);
        int index = (int)((x + 5.0f) / 10.0f * TABLE_SIZE);
        return tanhTable[index];
    }
};
```

#### 2. Branch Misprediction
```cpp
// SLOW: Branches in inner loop
for (int i = 0; i < numSamples; ++i) {
    if (input[i] > 0.0f) {
        output[i] = input[i] * 2.0f;
    } else {
        output[i] = input[i];
    }
}

// FAST: Branchless
for (int i = 0; i < numSamples; ++i) {
    float gain = 1.0f + (input[i] > 0.0f); // 1 or 2
    output[i] = input[i] * gain;
}
```

#### 3. Memory Allocation
```cpp
// SLOW: Allocation in process()
void process(float** channels, int numSamples) {
    std::vector<float> tempBuffer(numSamples); // Allocates every call!
    // ...
}

// FAST: Pre-allocated buffer
class Engine {
    std::vector<float> tempBuffer;

    void prepareToPlay(double sampleRate, int maxBlockSize) {
        tempBuffer.resize(maxBlockSize);
    }

    void process(float** channels, int numSamples) {
        // Use pre-allocated tempBuffer
    }
};
```

#### 4. Redundant Calculations
```cpp
// SLOW: Recalculate every sample
for (int i = 0; i < numSamples; ++i) {
    float coeff = 2.0f * std::sin(M_PI * cutoff / sampleRate);
    output[i] = process(input[i], coeff);
}

// FAST: Calculate once per block
float coeff = 2.0f * std::sin(M_PI * cutoff / sampleRate);
for (int i = 0; i < numSamples; ++i) {
    output[i] = process(input[i], coeff);
}
```

### Profiling Tools
```bash
# macOS
instruments -t "Time Profiler" ./build/standalone_test --engine 32

# Linux
perf record ./build/standalone_test --engine 32
perf report

# Valgrind
valgrind --tool=callgrind ./build/standalone_test --engine 32
```

---

## 9. Memory Leaks in Process

### Pattern Name
**Real-time Safety Violations**

### Symptoms
- Memory usage grows over time
- Increasing CPU usage
- Eventual crash after extended use

### Detection
```cpp
// Enable AddressSanitizer
// Compile with: -fsanitize=address

// Test for leaks
class LeakDetector {
    size_t initialMemory = 0;

    void startTest() {
        initialMemory = getCurrentMemoryUsage();
    }

    bool checkForLeaks() {
        size_t currentMemory = getCurrentMemoryUsage();
        size_t growth = currentMemory - initialMemory;

        // More than 1MB growth = likely leak
        return (growth > 1024 * 1024);
    }
};
```

### Common Sources
```cpp
// LEAK: Allocating in process()
void process(float** channels, int numSamples) {
    float* temp = new float[numSamples]; // NEVER FREED!
    // ...
    // Missing: delete[] temp;
}

// LEAK: std::vector resize
void process(float** channels, int numSamples) {
    std::vector<float> temp;
    temp.resize(numSamples); // Allocates every call!
}

// LEAK: std::string creation
void process(float** channels, int numSamples) {
    std::string name = "Engine_" + std::to_string(engineID); // Allocates!
}
```

### Solutions
```cpp
// FIX: Pre-allocate in prepareToPlay()
class Engine {
    std::vector<float> tempBuffer;

    void prepareToPlay(double sampleRate, int maxBlockSize) {
        tempBuffer.resize(maxBlockSize);
    }

    void process(float** channels, int numSamples) {
        // Use pre-allocated buffer
        for (int i = 0; i < numSamples; ++i) {
            tempBuffer[i] = channels[0][i];
        }
    }
};
```

### Real-time Safe Practices
✅ Pre-allocate all buffers in prepareToPlay()
✅ Use fixed-size arrays where possible
✅ Avoid std::string, std::vector resize/push_back
✅ Avoid std::cout, printf (file I/O)
✅ Avoid mutex locks (use lock-free structures)
❌ Never use new/delete in process()
❌ Never use malloc/free in process()
❌ Never call functions that allocate

---

## 10. Build System Issues

### Pattern Name
**Duplicate Symbols and Linking Errors**

### Symptoms
- "duplicate symbol" errors during linking
- Undefined reference errors
- Multiple definition errors

### Common Causes

#### 1. Duplicate Object Files
```bash
# Error: duplicate symbol _main
# Cause: Linking same .o file twice

# FIX: Exclude duplicates
g++ -o test main.o engine1.o engine2.o \
    $(find build/obj -name "*.o" ! -name "main.o") # Exclude main.o duplicate
```

#### 2. Missing Friend Declarations
```cpp
// Error: 'loadEngine' is private within this context

// Class A wants to access private member of Class B
class PluginProcessor {
private:
    void loadEngine(int engineID);

    // FIX: Add friend declaration
    friend class PluginEditorNexusStatic;
};
```

#### 3. Missing Headers
```cpp
// Error: 'juce::AudioProcessor' does not name a type

// FIX: Include required headers
#include <juce_audio_processors/juce_audio_processors.h>
```

### Build Script Pattern
```bash
#!/bin/bash
# build_safe.sh - Handles duplicates automatically

set -e # Exit on error

# Clean old build
rm -rf build/obj/*.o

# Compile sources
for source in JUCE_Plugin/Source/*.cpp; do
    obj="build/obj/$(basename $source .cpp).o"
    if [ ! -f "$obj" ] || [ "$source" -nt "$obj" ]; then
        echo "Compiling $(basename $source)..."
        g++ -c "$source" -o "$obj" [OTHER FLAGS]
    fi
done

# Link (automatically excludes duplicates)
echo "Linking..."
g++ -o build/test \
    $(find build/obj -name "*.o" -type f | sort -u) \
    [LINK FLAGS]

echo "Build complete!"
```

---

## Summary: Quick Reference

| Pattern | Symptom | Quick Fix |
|---------|---------|-----------|
| Pre-delay buffer | Zero output after impulse | Write before read |
| Pitch shifter latency | Delayed/zero output | Pass through during init |
| Mix parameter | Weak effect in tests | Set mix to 100% |
| FFT crash | Startup crash | Validate fftOrder (1-15) |
| Parameter clicks | Zipper noise | Add 10ms ramp |
| Filter explosion | Loud burst at high Q | Clamp resonance < 1.0 |
| Mono stereo | Correlation near 1.0 | Process L/R separately |
| High CPU | > 5% usage | Use lookup tables |
| Memory leak | Growing memory | Pre-allocate in prepare() |
| Build error | Duplicate symbols | Exclude duplicate .o files |

---

## Testing Checklist

After applying any fix:
- [ ] Test with impulse input
- [ ] Test with sine wave (100Hz, 1kHz, 10kHz)
- [ ] Test with white noise
- [ ] Test at multiple buffer sizes (64, 128, 512, 2048)
- [ ] Test at multiple sample rates (44.1kHz, 48kHz, 96kHz)
- [ ] Run regression tests on 5+ other engines
- [ ] Check for memory leaks (AddressSanitizer)
- [ ] Profile CPU usage
- [ ] Verify THD < threshold
- [ ] Document fix in bug tracker

---

**Document Maintained By**: ChimeraPhoenix Development Team
**Last Updated**: October 11, 2025
**Next Review**: After each major bug fix
**Version**: 1.0

---

**END OF FIX PATTERNS DOCUMENT**
