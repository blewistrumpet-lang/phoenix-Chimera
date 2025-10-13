# PhasedVocoder (Engine 49) - Architecture Overview

## High-Level Architecture

### Purpose
High-quality **phase vocoder** implementation for precision spectral processing, time-stretching, and pitch-shifting.

### Key Characteristics
- **FFT Size:** 2048 samples (FFT_ORDER = 11)
- **Overlap:** 75% (4x overlap-add)
- **Hop Size:** 512 samples
- **Latency:** ~2048 samples (46.4ms @ 44.1kHz)
- **Window:** Hann window with exact normalization
- **Phase Method:** Phase locking for vertical coherence

---

## Processing Pipeline

```
┌─────────────────────────────────────────────────────────────┐
│                    INPUT AUDIO                               │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              CIRCULAR INPUT BUFFER                           │
│  • Size: FFT_SIZE * 8 (16384 samples)                       │
│  • No modulo operations (optimized indexing)                │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼ (Every HOP_SIZE = 512 samples)
┌─────────────────────────────────────────────────────────────┐
│                   ANALYSIS FRAME                             │
│  1. Extract FFT_SIZE samples                                 │
│  2. Apply Hann window                                        │
│  3. Forward FFT → Complex spectrum                           │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              PHASE VOCODER ANALYSIS                          │
│  • Extract magnitude: |X[k]| = sqrt(Re² + Im²)              │
│  • Extract phase: φ[k] = atan2(Im, Re)                      │
│  • Calculate Δφ[k] = φ[k] - φ_prev[k] - ω[k]·H_a            │
│  • Unwrap phase to [-π, π]                                   │
│  • Calculate instantaneous freq: ω_inst[k] = ω[k] + Δφ/H_a  │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              SPECTRAL PROCESSING                             │
│  • Spectral gating (threshold-based)                         │
│  • Spectral smearing (bin averaging)                         │
│  • Freeze effect (save/replay spectrum)                      │
│  • Transient detection                                       │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              PHASE SYNTHESIS                                 │
│  Mode: PHASE_LOCKING (default)                              │
│  1. Find dominant peak bin (max magnitude)                   │
│  2. Advance peak phase: φ_synth[peak] += ω_inst·H_s·pitch   │
│  3. Lock all bins to peak with relative phase offsets       │
│  4. Reconstruct complex: Re + j·Im = |X|·e^(jφ_synth)       │
│  5. Mirror negative frequencies for real IFFT                │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                   INVERSE FFT                                │
│  • IFFT → Time domain signal                                 │
│  • Auto-detected scaling correction                          │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              OVERLAP-ADD SYNTHESIS                           │
│  • Apply synthesis window                                    │
│  • Accumulate to output buffer                               │
│  • Track normalization buffer (sum of window²)               │
│  • Advance by H_s = HOP_SIZE * timeStretch                   │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│              NORMALIZATION & OUTPUT                          │
│  • Read sample: y = outputBuffer[n] / normBuffer[n]         │
│  • Apply dry/wet mix                                         │
│  • Denormal protection                                       │
│  • NaN/Inf scrubbing                                         │
└─────────────────────┬───────────────────────────────────────┘
                      │
                      ▼
┌─────────────────────────────────────────────────────────────┐
│                   OUTPUT AUDIO                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Key Algorithms

### 1. Phase Vocoder Analysis

```cpp
// For each FFT bin k:
double omega_k = 2π * k / N;                    // Expected frequency
double delta = phase[k] - lastPhase[k] - omega_k * H_a;
delta = remainder(delta, 2π);                    // Unwrap to [-π, π]
instFreq[k] = omega_k + delta / H_a;            // Instantaneous frequency
```

### 2. Phase Locking Synthesis

```cpp
// Find dominant peak
int peak = argmax(magnitude[k]);

// Advance only peak phase
synthPhase[peak] += instFreq[peak] * H_s * pitchRatio;

// Lock all bins to peak
for (k = 0; k <= N/2; k++) {
    float relPhase = phase[k] - phase[peak];    // Relative phase
    float synthPh = synthPhase[peak] + relPhase;
    spectrum[k] = magnitude[k] * exp(j * synthPh);
}
```

### 3. Overlap-Add Normalization

```cpp
// Accumulate during synthesis
outputBuffer[i] += grain[i] * window[i];
normBuffer[i] += window[i] * window[i];

// Read during output
output[i] = outputBuffer[i] / normBuffer[i];
```

---

## Buffer Architecture

### Circular Buffers (Per Channel)

1. **Input Buffer**
   - Size: `FFT_SIZE * 8 = 16384` samples
   - Purpose: Store incoming audio for analysis
   - Indexing: `(writePos + i) % bufferSize`

2. **Output Buffer**
   - Size: `FFT_SIZE * 8 = 16384` samples
   - Purpose: Accumulate overlap-add synthesis
   - Write offset: `latency` ahead of read position

3. **Normalization Buffer**
   - Size: `FFT_SIZE * 8 = 16384` samples
   - Purpose: Track sum of overlapping windows
   - Used for proper gain correction

### Spectral Buffers (Per Channel)

1. **FFT Buffer (`fftRI`)**
   - Size: `FFT_SIZE * 2` (interleaved Re/Im)
   - Format: `[Re0, Im0, Re1, Im1, ...]`

2. **Magnitude & Phase**
   - Magnitude: `FFT_SIZE/2 + 1` bins (float)
   - Phase: `FFT_SIZE/2 + 1` bins (double precision)
   - Inst. Freq: `FFT_SIZE/2 + 1` bins (double)
   - Synth Phase: `FFT_SIZE/2 + 1` bins (double)

---

## Parameter Mapping

### Parameter IDs

| ID | Name | Range | Mapping | Default |
|----|------|-------|---------|---------|
| 0 | TimeStretch | 0-1 | 0.25x-4x (0.2 → 1.0x) | 0.2 (1.0x) |
| 1 | PitchShift | 0-1 | ±24 semitones (0.5 → 0st) | 0.5 (0st) |
| 2 | SpectralSmear | 0-1 | 0-100% | 0.0 |
| 3 | TransientPreserve | 0-1 | 0-100% | 0.5 |
| 4 | PhaseReset | 0-1 | 0-100% | 0.0 |
| 5 | SpectralGate | 0-1 | 0-100% | 0.0 |
| 6 | **Mix** | 0-1 | 0-100% | 1.0 |
| 7 | Freeze | 0-1 | OFF/ON (>0.5) | 0.0 |
| 8 | TransientAttack | 0-1 | 0.1-10ms | varies |
| 9 | TransientRelease | 0-1 | 10-500ms | varies |

**Important:** Mix parameter is at index **6**, not 0!

---

## Timing & Latency

### Sample Flow

1. **Input → Analysis**
   - Samples accumulated until `HOP_SIZE` reached
   - Frame analysis triggered every 512 samples

2. **Analysis → Synthesis**
   - Immediate (same frame)
   - Processing time: < 1ms typical

3. **Synthesis → Output**
   - Write to buffer at `writePos` (ahead by `latency`)
   - Read from buffer at `readPos`
   - Delay: `latency = FFT_SIZE = 2048` samples

### Warmup Period

- **Duration:** `warmupSamples = latency = 2048` samples
- **Purpose:** Prime buffers before valid output
- **Behavior:** Output zeros during warmup
- **Status:** ✅ Correctly set (was 4096, now 2048)

---

## Optimization Features

### 1. Memory Optimizations
- **Pre-allocated buffers** - No dynamic allocation in audio thread
- **Aligned buffers** - `alignas(32)` for SIMD vectorization
- **Circular indexing** - Fast modulo-free wrapping

### 2. Numerical Stability
- **Denormal flushing** - Every 256 frames for all feedback paths
- **Double precision** - Phase accumulators use `double`
- **NaN/Inf protection** - Buffer scrubbing after processing

### 3. Thread Safety
- **Atomic parameters** - Lock-free parameter updates
- **Smoothing** - 5ms smoothing for pitch/time, 2ms for mix
- **No shared state** - Each channel independent

### 4. FFT Optimizations
- **Auto-scaling detection** - Runtime measurement of FFT roundtrip gain
- **JUCE FFT** - Hardware-optimized transforms
- **In-place processing** - Minimize memory copies

---

## Quality Features

### 1. Phase Locking
- **Reduces phasiness** - Maintains vertical phase coherence
- **Preserves spectral structure** - Locking to dominant peak
- **Musical quality** - Less robotic than per-bin phase vocoder

### 2. Window Design
- **Hann window** - Smooth rolloff, good overlap properties
- **Exact normalization** - Calculated overlap sum = 1.5 for 75% overlap
- **No artifacts** - Properly normalized overlap-add

### 3. Spectral Processing
- **Spectral gate** - Remove low-level noise
- **Spectral smearing** - Creative frequency blurring
- **Freeze with crossfade** - Glitch-free freeze transitions

---

## Implementation Files

### Source Files
- **PhasedVocoder.cpp** - Main implementation
- **PhasedVocoder.h** - Public interface
- **DspEngineUtilities.h** - Shared utilities (header-only)

### Key Classes
```cpp
class PhasedVocoder : public EngineBase {
    struct Impl {
        struct Parameters { /* Atomic parameters */ };
        struct ChannelState { /* Per-channel processing */ };

        std::vector<std::unique_ptr<ChannelState>> channelStates;
        std::unique_ptr<AtomicSmoother> timeStretchSmoother;
        std::unique_ptr<AtomicSmoother> pitchShiftSmoother;
        std::unique_ptr<AtomicSmoother> mixSmoother;
    };
    std::unique_ptr<Impl> pimpl;
};
```

---

## Testing Recommendations

### Test Signals
1. **Impulse** - Latency measurement
2. **1kHz Sine** - Pitch shift verification
3. **Chirp** - Frequency response
4. **Speech** - Quality assessment
5. **Drums** - Transient handling

### Key Metrics
- **Latency:** ~2048 samples (46.4ms @ 44.1kHz)
- **CPU:** ~5-15% on modern hardware
- **Quality:** High (phase locking reduces artifacts)
- **Range:** ±24 semitones pitch, 0.25x-4x time

### Common Issues
1. **No output in short tests** - Use ≥4096 sample buffers
2. **Parameter confusion** - Mix is param[6], not param[0]
3. **Build issues** - Ensure JUCE modules properly linked

---

## Comparison with Other Engines

### vs Engine 31 (PitchShifter/Vocal Destroyer)
- **31:** Creative vocal effect, 3 modes, strategy pattern
- **49:** Precision spectral processor, phase vocoder
- **Use 31 for:** Vocal mangling, creative FX
- **Use 49 for:** Time/pitch manipulation, spectral processing

### vs Traditional Pitch Shifters
- **SMBPitchShift:** Fast, lower quality
- **SimplePitchShift:** Very fast, basic quality
- **PhasedVocoder (49):** High quality, more CPU
- **SignalsmithPitchShift:** Research-grade quality

---

## Future Enhancements (Optional)

While the current implementation is complete, potential future additions:

1. **Formant preservation** - Independent pitch/formant control
2. **Adaptive windows** - Change FFT size based on content
3. **Multi-resolution** - Multiple FFT sizes simultaneously
4. **Harmonic locking** - Lock to fundamental for harmonic signals
5. **Transient adaptation** - Variable hop size around transients

**Note:** These are enhancements, not bug fixes. Current version is production-ready.

---

## References

### Algorithm Sources
- Phase vocoder theory: Dolson (1986), Laroche & Dolson (1999)
- Phase locking: Puckette (1995)
- Overlap-add: Crochiere (1980)

### JUCE Dependencies
- `juce::dsp::FFT` - FFT/IFFT transforms
- `juce::AudioBuffer` - Audio buffering
- `juce_core` - Utilities and threading

---

**Document Version:** 1.0
**Last Updated:** October 11, 2025
**Status:** Production Ready ✅
