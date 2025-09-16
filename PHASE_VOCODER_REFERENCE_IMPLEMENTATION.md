# Phase Vocoder Reference Implementation - Team C

## Overview

This document describes the simplified, reference implementation of a phase vocoder created by Team C. The goal is to provide a **working, correct implementation** that demonstrates the core STFT phase vocoder algorithm without optimization complexity.

## Key Design Principles

1. **Correctness over Performance**: Focus on algorithm clarity and correct output
2. **Educational Value**: Code should be readable and demonstrate standard techniques
3. **Minimal Dependencies**: Use only JUCE's basic FFT and standard library
4. **Proven Parameters**: Use established values that are known to work well

## Implementation Parameters

### Core Constants
```cpp
constexpr int FFT_ORDER = 11;           // 2^11 = 2048 samples
constexpr int FFT_SIZE = 1 << FFT_ORDER; // 2048 samples
constexpr int HOP_SIZE = 512;            // 75% overlap (FFT_SIZE / 4)
```

**Rationale**: These values provide good time/frequency resolution tradeoff:
- 2048-point FFT: ~23Hz frequency resolution at 44.1kHz
- 512-sample hop: Good time resolution (~11.6ms at 44.1kHz)
- 75% overlap: Ensures proper reconstruction with Hann window

### Window Function
- **Hann Window**: `w[n] = 0.5 * (1 - cos(2π * n / (N-1)))`
- **Normalization**: For 75% overlap, sum ≈ 1.5 (theoretical)

## Core Algorithm

### 1. Analysis Phase (`analyzeFrame`)

```cpp
void analyzeFrame(ChannelState& state) {
    // 1. Apply window to input frame
    for (int i = 0; i < FFT_SIZE; ++i) {
        state.windowedFrame[i] = inputSample[i] * hanningWindow[i];
    }
    
    // 2. Forward FFT (JUCE format)
    state.fft->perform(data, data, false);
    
    // 3. Extract magnitude and phase
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        magnitude[bin] = sqrt(real² + imag²);
        phase[bin] = atan2(imag, real);
    }
}
```

### 2. Spectral Processing (`processFrame`)

The core phase vocoder processing implements:

**Phase Unwrapping**:
```cpp
float phaseDiff = phase[bin] - lastPhase[bin];
// Wrap to [-π, π]
while (phaseDiff > π) phaseDiff -= 2π;
while (phaseDiff < -π) phaseDiff += 2π;
```

**True Frequency Estimation**:
```cpp
float expectedIncrement = 2π * HOP_SIZE / FFT_SIZE;
float deviation = phaseDiff - expectedIncrement * bin;
float trueFreq = (bin + deviation/expectedIncrement) * sampleRate / FFT_SIZE;
```

**Time/Pitch Modification**:
```cpp
float shiftedFreq = trueFreq * pitchRatio;
float phaseIncrement = 2π * shiftedFreq * HOP_SIZE / (timeStretch * sampleRate);
phaseAccumulator[bin] += phaseIncrement;
```

### 3. Synthesis Phase (`synthesizeFrame`)

```cpp
void synthesizeFrame(ChannelState& state) {
    // 1. Reconstruct complex spectrum
    for (int bin = 0; bin <= FFT_SIZE / 2; ++bin) {
        float mag = magnitude[bin];
        float ph = phaseAccumulator[bin];
        fftBuffer[bin] = mag * exp(j * ph);
        
        // Hermitian symmetry
        if (bin > 0 && bin < FFT_SIZE/2) {
            fftBuffer[FFT_SIZE - bin] = conj(fftBuffer[bin]);
        }
    }
    
    // 2. Inverse FFT
    state.fft->perform(data, data, true);
    
    // 3. Overlap-add with scaling
    float scale = 1.0f / (FFT_SIZE * 1.5f); // JUCE normalization + window sum
    for (int i = 0; i < FFT_SIZE; ++i) {
        outputBuffer[i] += fftBuffer[i].real() * hanningWindow[i] * scale;
    }
}
```

## JUCE FFT Integration

### Format Requirements
- **Input**: Complex array as `std::complex<float>[FFT_SIZE]`
- **JUCE Access**: Cast to `float*` for interleaved real/imaginary data
- **Normalization**: JUCE FFT is unnormalized, divide by `FFT_SIZE` for round-trip

### Scaling Factors
```cpp
// Complete scaling chain:
const float scale = 1.0f / (FFT_SIZE * windowSum);
// Where windowSum ≈ 1.5 for Hann window with 75% overlap
```

## Parameter Mapping

### Time Stretch (0.0 to 1.0)
```cpp
float timeStretch = 0.5f + value * 1.5f; // Maps to 0.5x - 2.0x
```

### Pitch Shift (0.0 to 1.0) 
```cpp
float pitchShift = (value - 0.5f) * 24.0f; // Maps to -12 to +12 semitones
float pitchRatio = pow(2.0f, pitchShift / 12.0f);
```

### Mix (0.0 to 1.0)
```cpp
output = input * (1 - mix) + processed * mix;
```

## Buffer Management

### Circular Buffers
- **Input Buffer**: 4 × FFT_SIZE to handle overlap and time stretching
- **Output Buffer**: 4 × FFT_SIZE for synthesis overlap-add
- **Index Wrapping**: Simple modulo operation with bounds checking

### Processing Flow
```
Input → Circular Buffer → Windowing → FFT → Processing → IFFT → Overlap-Add → Output
```

## Key Differences from Production Version

| Aspect | Reference | Production |
|--------|-----------|------------|
| SIMD | None | AVX2/SSE optimizations |
| Threading | Single-threaded | Lock-free atomics |
| Features | 3 parameters | 10+ parameters |
| Denormal Protection | Basic | Comprehensive |
| Memory | Standard vectors | Aligned arrays |
| Error Handling | Basic | Studio-grade |

## Testing and Validation

### Test Signals
1. **Pure Sine Wave**: For frequency accuracy testing
2. **Chirp Signal**: For time-domain analysis
3. **Impulse**: For transient response

### Expected Results
- **Passthrough** (1x time, 0 pitch): Should preserve input characteristics
- **Time Stretch**: Signal duration changes, pitch preserved
- **Pitch Shift**: Frequency changes, timing preserved

### Quality Metrics
- RMS energy preservation (within 50%)
- Frequency accuracy (within 50Hz for test signals)
- No NaN/Inf in output
- Stable processing across parameter changes

## Usage Example

```cpp
// Initialize
PhasedVocoder_Reference vocoder;
vocoder.prepareToPlay(44100.0, 512);

// Set parameters
std::map<int, float> params;
params[0] = 0.75f; // 1.625x time stretch  
params[1] = 0.6f;  // +2.4 semitones pitch shift
params[2] = 0.8f;  // 80% wet mix
vocoder.updateParameters(params);

// Process audio
juce::AudioBuffer<float> buffer = getAudioBuffer();
vocoder.process(buffer); // In-place processing
```

## Known Limitations

1. **Artifacts**: Typical phase vocoder artifacts (phasiness, transient smearing)
2. **Latency**: ~FFT_SIZE samples of algorithmic delay
3. **Performance**: Not optimized for real-time use in complex projects
4. **Stereo**: Processes channels independently (no stereo linking)

## Future Optimization Paths

When moving from reference to production:

1. **SIMD Vectorization**: Process multiple bins simultaneously
2. **Lock-Free Threading**: Atomic parameters, separate analysis/synthesis threads  
3. **Advanced Features**: Spectral gating, transient preservation, formant correction
4. **Memory Optimization**: Pre-allocated aligned buffers, circular indexing
5. **Artifact Reduction**: Phase coherence algorithms, transient detection

## Conclusion

This reference implementation demonstrates the core STFT phase vocoder algorithm in its most straightforward form. It prioritizes understanding and correctness over performance, making it ideal for:

- Algorithm validation and testing
- Educational purposes and learning
- Baseline comparison for optimized versions
- Quick prototyping of parameter changes

The implementation produces correct phase vocoder output and serves as the foundation for more sophisticated, production-ready versions.