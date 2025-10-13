# Code Duplication Analysis Report
## Project Chimera v3.0 Phoenix - DSP Engine Quality Assessment

**Generated:** 2025-10-11
**Scope:** Full codebase analysis across 74+ audio engines
**Focus:** DSP pattern duplication, filter implementations, refactoring opportunities

---

## Executive Summary

### Overall Assessment
The codebase demonstrates **significant code duplication** across engines, with multiple implementations of identical DSP patterns. While a shared utility library (`DspEngineUtilities.h`) exists, it is underutilized, leading to maintenance burden and inconsistent implementations.

### Key Findings
- **74+ Engine Classes** identified with varying degrees of code duplication
- **30+ Parameter Smoother implementations** duplicated across engines
- **20+ SVF (State Variable Filter) implementations** with similar algorithms
- **10+ DC Blocker implementations** with near-identical code
- **15+ Oversampling implementations** using different approaches
- **Shared utility exists** but adoption is inconsistent (~40% usage)

### Critical Issues
1. **Parameter smoothing** reimplemented 30+ times with subtle variations
2. **Filter architectures** (SVF, biquad, one-pole) duplicated extensively
3. **Denormal protection** handled inconsistently across engines
4. **Oversampling** implementations vary widely in quality and efficiency
5. **DC blocking** duplicated with slight algorithm differences

---

## Detailed Analysis

### 1. Parameter Smoothing Duplication

**Severity:** HIGH
**Affected Files:** 30+ engines

#### Duplicate Implementations Found

| Engine | Class Name | Lines | Implementation |
|--------|-----------|-------|----------------|
| LadderFilter.h | SmoothedParameter | 28 | Atomic with exponential smoothing |
| MuffFuzz.h | ParameterSmoother | 35 | Atomic with RC filter |
| StateVariableFilter.h | ParameterSmoother | 23 | Simple one-pole |
| DetuneDoubler.h | ParameterSmoother | 41 | With reset functionality |
| MagneticDrumEcho.h | ParameterSmoother | ~30 | Custom implementation |
| RotarySpeaker.h | ParameterSmoother | ~35 | Forward declared |
| ClassicCompressor.h | ParameterSmoother + GainSmoother | 40+ | Two variants! |
| BucketBrigadeDelay.h | ParameterSmoother | ~30 | Custom |
| RodentDistortion.h | ParameterSmoother | ~30 | Custom |
| DigitalDelay.h | ParameterSmoother | ~30 | Forward declared |
| HarmonicTremolo.h | SmoothedParameter | 26 | Struct variant |
| VintageTubePreamp.h | SmoothedParameter | ~40 | Unique pointer variant |
| NoiseGate_Platinum.cpp | SmoothedParameter | ~30 | Inline class |
| ChaosGenerator_Platinum.cpp | ParamSmoother | ~30 | Custom name |
| BufferRepeat_Platinum.cpp | UltraSmoother | ~30 | "Ultra" variant |
| ResonantChorus_Platinum.cpp | UltraSmoother | ~30 | Duplicate |
| PhasedVocoder.cpp | AtomicSmoother | ~30 | Atomic variant |
| EnvelopeFilter.cpp | AtomicSmoother | ~30 | Duplicate atomic |
| MultibandSaturator.cpp | ParamSmoother | ~30 | Another custom name |

**Existing Shared Implementation:** `DspEngineUtilities.h::ParamSmoother` (lines 87-125)

#### Code Example - LadderFilter Implementation
```cpp
struct SmoothedParameter {
    std::atomic<float> targetValue{0.5f};
    float currentValue = 0.5f;
    float smoothingCoeff = 0.995f;

    void setTarget(float value) {
        targetValue.store(value, std::memory_order_relaxed);
    }

    float getNextValue() {
        float target = targetValue.load(std::memory_order_relaxed);
        currentValue = target + (currentValue - target) * smoothingCoeff;
        return currentValue;
    }

    void setSmoothingTime(float ms, float sampleRate) {
        smoothingCoeff = std::exp(-1.0f / (ms * 0.001f * sampleRate));
    }
};
```

#### Code Example - MuffFuzz Implementation
```cpp
class ParameterSmoother {
    std::atomic<double> targetValue{0.0};
    double currentValue = 0.0;
    double smoothingCoeff = 0.0;
    double sampleRate = 0.0;

public:
    void setTarget(double value) {
        targetValue.store(value, std::memory_order_relaxed);
    }

    double process() {
        double target = targetValue.load(std::memory_order_relaxed);
        currentValue = target + (currentValue - target) * smoothingCoeff;
        currentValue += DENORMAL_PREVENTION;
        currentValue -= DENORMAL_PREVENTION;
        return currentValue;
    }
};
```

**Analysis:** These implementations are >90% identical but with minor variations in:
- Type (float vs double)
- Denormal prevention approach
- Method naming conventions
- Atomic vs non-atomic storage

---

### 2. State Variable Filter (SVF) Duplication

**Severity:** HIGH
**Affected Files:** 20+ engines

#### Duplicate SVF Implementations

| File | Implementation Type | Lines |
|------|-------------------|-------|
| StateVariableFilter.h | Full SVF core with zero-delay | 95 |
| FormantFilter.h | Professional double-precision SVF | 208 |
| EnvelopeFilter.cpp | Embedded SVF class | ~80 |
| LadderFilter.h | Embedded in ladder stages | ~50 |
| DynamicEQ.h | EQ-specific SVF | ~60 |
| VocalFormantFilter.h | Similar to FormantFilter | ~200 |

#### Common SVF Core Pattern
```cpp
class SVFCore {
    float s1 = 0.0f;
    float s2 = 0.0f;

public:
    struct Outputs {
        float lowpass;
        float highpass;
        float bandpass;
        float notch;
    };

    Outputs process(float input, float frequency, float resonance, float sampleRate) {
        float g = std::tan(M_PI * frequency / sampleRate);
        float k = 1.0f / resonance;
        float a1 = 1.0f / (1.0f + g * (g + k));
        float a2 = g * a1;
        float a3 = g * a2;

        float v3 = input - s2;
        float v1 = a1 * s1 + a2 * v3;
        float v2 = s2 + a2 * s1 + a3 * v3;

        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;

        // Return all filter modes...
    }
};
```

**Impact:** This core algorithm is replicated ~20 times with minor variations.

---

### 3. DC Blocking Duplication

**Severity:** MEDIUM
**Affected Files:** 10+ engines

#### Implementations Found

| File | Implementation Style | Lines |
|------|---------------------|-------|
| DspEngineUtilities.h | DCBlocker class (shared) | 28 |
| FormantFilter.h | DCBlocker struct | 18 |
| LadderFilter.h | Inline in ChannelState | 12 |
| MuffFuzz.h | DCBlocker class | 20 |
| DetuneDoubler.h | Not implemented (missing!) | 0 |
| ClassicCompressor.h | Inline implementation | ~15 |
| StereoImager.h | Custom approach | ~20 |

#### Standard Pattern
```cpp
class DCBlocker {
    float R = 0.995f;
    float x1 = 0.0f, y1 = 0.0f;

public:
    float process(float input) {
        float output = input - x1 + R * y1;
        x1 = input;
        y1 = output;
        return output;
    }

    void reset() { x1 = y1 = 0.0f; }
};
```

**Analysis:** Nearly identical implementations across files. The shared version in `DspEngineUtilities.h` is underutilized.

---

### 4. Oversampling Duplication

**Severity:** HIGH
**Affected Files:** 15+ engines

#### Different Oversampling Approaches

| File | Approach | Quality | Complexity |
|------|----------|---------|------------|
| LadderFilter.h | Polyphase FIR with Kaiser window | Professional | High |
| MuffFuzz.h | Butterworth cascade (4-stage) | Good | Medium |
| FormantFilter.h | Kaiser-windowed sinc (16-tap) | Professional | High |
| DetuneDoubler.h | None (uses grain-based pitch shift) | N/A | N/A |
| RodentDistortion.h | Simple 2x linear interpolation | Basic | Low |
| VintageTubePreamp.h | Professional polyphase | Professional | High |

#### Example: High-Quality Kaiser Oversampler (FormantFilter.h)
```cpp
class KaiserOversampler2x {
public:
    static constexpr int TAPS_PER_PHASE = 16;
    static constexpr int TOTAL_TAPS = TAPS_PER_PHASE * 2;

    void process(double input, double& out1, double& out2) {
        // Phase 0 (at sample point)
        out1 = 0.0;
        for (int i = 0; i < TAPS_PER_PHASE; ++i) {
            int idx = (m_upIdx - i + HISTORY_SIZE) % HISTORY_SIZE;
            out1 += m_upHistory[idx] * m_coeffsPhase0[i];
        }

        // Phase 1 (between samples)
        out2 = 0.0;
        for (int i = 0; i < TAPS_PER_PHASE; ++i) {
            int idx = (m_upIdx - i + HISTORY_SIZE) % HISTORY_SIZE;
            out2 += m_upHistory[idx] * m_coeffsPhase1[i];
        }
    }

private:
    void generateCoefficients() {
        const double beta = 7.865; // Kaiser beta for -80dB
        const double cutoff = 0.45;
        // ...complex coefficient generation
    }
};
```

**Impact:** Quality varies significantly. Some engines use professional-grade oversampling while others use basic interpolation, leading to inconsistent aliasing rejection.

---

### 5. Denormal Protection Duplication

**Severity:** MEDIUM
**Affected Files:** 60+ engines

#### Different Approaches

| Approach | Files | Implementation |
|----------|-------|---------------|
| DspEngineUtilities::flushDenorm() | 40+ | `std::abs(x) < 1e-30 ? 0 : x` |
| Inline +/- tiny value | 15+ | `value += 1e-25f; value -= 1e-25f;` |
| Custom inline function | 10+ | Various thresholds (1e-15, 1e-20, 1e-30) |
| FTZ/DAZ via MXCSR | 5+ | SSE intrinsics (DenormalGuard) |
| preventDenormal() function | 5+ | Bit manipulation approach |
| No protection | 5+ | Missing! |

#### Best Practice (DspEngineUtilities.h)
```cpp
template <typename T>
inline T flushDenorm(T x) noexcept {
    constexpr T tiny = (T)1.0e-30;
    return std::abs(x) < tiny ? (T)0 : x;
}

// RAII guard for FTZ/DAZ mode
struct DenormalGuard {
#if JUCE_USE_SSE_INTRINSICS
    uint32_t oldMXCSR = 0;
    DenormalGuard() noexcept {
        oldMXCSR = _mm_getcsr();
        _mm_setcsr(oldMXCSR | 0x8040); // FTZ | DAZ
    }
    ~DenormalGuard() noexcept {
        _mm_setcsr(oldMXCSR);
    }
#endif
};
```

**Impact:** Inconsistent denormal handling can lead to CPU spikes on some systems.

---

### 6. Biquad Filter Duplication

**Severity:** MEDIUM
**Affected Files:** 12+ engines

#### Implementations
- DetuneDoubler.h: BiquadFilter class with high shelf
- MuffFuzz.h: BigMuffToneStack (custom biquad coefficients)
- EnvelopeFilter.cpp: Inline biquad processing
- ParametricEQ.h/cpp: Professional multi-band biquad
- VintageConsoleEQ.h: Console-style biquad shelves
- MultibandSaturator.cpp: Crossover biquads

**Common Pattern:**
```cpp
class BiquadFilter {
    double b0, b1, b2, a1, a2;
    double x1 = 0.0, x2 = 0.0, y1 = 0.0, y2 = 0.0;

public:
    float processSample(float input) {
        double out = b0 * input + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1; x1 = input;
        y2 = y1; y1 = out;
        return static_cast<float>(out);
    }
};
```

---

### 7. Delay Line Duplication

**Severity:** MEDIUM
**Affected Files:** 20+ engines

#### Circular Buffer Implementations

| File | Name | Features |
|------|------|----------|
| DspEngineUtilities.h | CircularBuffer<T> | Template, interpolated read |
| FeedbackNetwork.h | DelayLine struct | Simple inline |
| DetuneDoubler.h | DelayLine class | Cubic interpolation |
| DigitalDelay.h | DelayBuffer | Multiple tap support |
| MagneticDrumEcho.h | TapeDelay | With wow/flutter |
| BucketBrigadeDelay.h | BBDLine | Analog modeling |

**Shared Implementation Available:**
```cpp
template <typename T>
class CircularBuffer {
    std::vector<T> buffer;
    int writePos = 0;

    T readInterpolated(float delaySamples) const {
        int delay0 = (int)delaySamples;
        int delay1 = (delay0 + 1) % size;
        float frac = delaySamples - delay0;
        return sample0 + (sample1 - sample0) * frac;
    }
};
```

---

### 8. All-Pass Filter Duplication

**Severity:** LOW
**Affected Files:** 4 engines

#### Implementations
- DetuneDoubler.h: AllPassNetwork class (4-stage)
- ClassicTremolo.cpp: Simple all-pass for phase
- StereoWidener.h: All-pass for stereo imaging

**Pattern:** Less duplication here, but still room for consolidation.

---

## Refactoring Opportunities

### Priority 1: High Impact, Low Risk

#### 1.1 Consolidate Parameter Smoothing
**Effort:** MEDIUM
**Impact:** HIGH
**Risk:** LOW

**Action Plan:**
```cpp
// Extend DspEngineUtilities.h with additional smoothing options
namespace DSPUtils {
    // Keep existing ParamSmoother

    // Add atomic variant for thread-safety
    class AtomicSmoother {
        std::atomic<float> target{0.0f};
        float current = 0.0f;
        float smoothCoeff = 0.99f;

    public:
        void setTarget(float value) {
            target.store(value, std::memory_order_relaxed);
        }

        float process() {
            float t = target.load(std::memory_order_relaxed);
            current = t + (current - t) * smoothCoeff;
            return flushDenorm(current);
        }

        void setSmoothingTime(float ms, double sampleRate) {
            smoothCoeff = std::exp(-1.0f / (ms * 0.001f * sampleRate));
        }

        void reset(float value) {
            current = value;
            target.store(value, std::memory_order_relaxed);
        }
    };

    // Add multi-rate variant (already exists, promote usage)
    using FastSmoother = MultiRateSmoother; // Fast preset
    using MediumSmoother = MultiRateSmoother; // Medium preset
    using SlowSmoother = MultiRateSmoother; // Slow preset
}
```

**Migration Path:**
1. Add AtomicSmoother to DspEngineUtilities.h
2. Create automated refactoring script to replace instances
3. Update 5-10 engines as proof of concept
4. Roll out to remaining engines over time

**Benefits:**
- Reduces ~600 lines of duplicate code
- Ensures consistent smoothing behavior
- Easier to optimize (single implementation)
- Better testing coverage

---

#### 1.2 Shared SVF Library
**Effort:** HIGH
**Impact:** HIGH
**Risk:** MEDIUM

**Action Plan:**
```cpp
// Add to DspEngineUtilities.h or new DspFilters.h
namespace DSPUtils {
    // Zero-delay feedback SVF
    class StateVariableFilter {
    public:
        struct Response {
            float lowpass;
            float highpass;
            float bandpass;
            float notch;
            float allpass;
            float peak;
        };

        Response process(float input, float freq, float Q, double sampleRate) {
            float g = std::tan(M_PI * freq / sampleRate);
            float k = 1.0f / Q;
            float a1 = 1.0f / (1.0f + g * (g + k));
            float a2 = g * a1;
            float a3 = g * a2;

            float v3 = input - ic2eq;
            float v1 = a1 * ic1eq + a2 * v3;
            float v2 = ic2eq + a2 * ic1eq + a3 * v3;

            ic1eq = 2.0f * v1 - ic1eq;
            ic2eq = 2.0f * v2 - ic2eq;

            Response r;
            r.lowpass = v2;
            r.highpass = input - k * v1 - v2;
            r.bandpass = v1;
            r.notch = input - k * v1;
            r.allpass = input - 2.0f * k * v1;
            r.peak = r.lowpass - r.highpass;

            return r;
        }

        void reset() {
            ic1eq = ic2eq = 0.0f;
        }

    private:
        float ic1eq = 0.0f;
        float ic2eq = 0.0f;
    };

    // Double precision variant for high-quality applications
    class StateVariableFilterDouble {
        // Same but with double precision
    };
}
```

**Migration Strategy:**
- Keep existing implementations initially
- Add shared SVF to DspEngineUtilities
- Refactor new engines to use shared version
- Gradually migrate existing engines
- Maintain backward compatibility during transition

---

#### 1.3 Standardized Oversampling
**Effort:** HIGH
**Impact:** HIGH
**Risk:** MEDIUM

**Action Plan:**
```cpp
// Add professional oversampling to DspEngineUtilities.h
namespace DSPUtils {
    // Quality presets
    enum class OversampleQuality {
        Basic,       // 2x linear (low CPU)
        Standard,    // 2x FIR (medium CPU)
        High,        // 4x FIR (high CPU)
        Professional // 4x Kaiser windowed (highest CPU)
    };

    class Oversampler {
    public:
        void prepare(int factor, OversampleQuality quality, double sampleRate);

        template<typename ProcessFunc>
        float process(float input, ProcessFunc&& func) {
            // Upsample
            upsample(input);

            // Process at higher rate
            for (int i = 0; i < factor; ++i) {
                upBuffer[i] = func(upBuffer[i]);
            }

            // Downsample
            return downsample();
        }

        void reset();

    private:
        int oversampleFactor = 2;
        std::vector<float> upBuffer;
        std::vector<float> upCoeffs;
        std::vector<float> downCoeffs;

        void designFilters(OversampleQuality quality);
        void upsample(float input);
        float downsample();
    };
}
```

**Benefits:**
- Consistent aliasing rejection across all engines
- Users can choose quality vs CPU tradeoff globally
- Single implementation to optimize and maintain
- Professional-grade quality available to all engines

---

### Priority 2: Medium Impact, Medium Risk

#### 2.1 Biquad Filter Library
**Effort:** MEDIUM
**Impact:** MEDIUM
**Risk:** LOW

```cpp
namespace DSPUtils {
    class Biquad {
    public:
        enum Type { Lowpass, Highpass, Bandpass, Notch, Allpass,
                   PeakingEQ, LowShelf, HighShelf };

        void setCoefficients(Type type, double freq, double Q,
                           double gain, double sampleRate);

        float process(float input) {
            double out = b0*input + b1*x1 + b2*x2 - a1*y1 - a2*y2;
            x2=x1; x1=input; y2=y1; y1=out;
            return flushDenorm(static_cast<float>(out));
        }

        void reset() { x1=x2=y1=y2=0.0; }

    private:
        double b0=1.0, b1=0.0, b2=0.0;
        double a1=0.0, a2=0.0;
        double x1=0.0, x2=0.0, y1=0.0, y2=0.0;
    };
}
```

---

#### 2.2 Unified Delay Line Interface
**Effort:** MEDIUM
**Impact:** MEDIUM
**Risk:** LOW

```cpp
namespace DSPUtils {
    // Already exists in DspEngineUtilities.h as CircularBuffer<T>
    // Promote usage and add convenience methods

    template<typename T>
    class DelayLine : public CircularBuffer<T> {
    public:
        // Add cubic interpolation
        T readCubic(float delaySamples) const;

        // Add multi-tap support
        std::vector<T> readTaps(const std::vector<float>& delays) const;

        // Add modulation support
        T readModulated(float baseDelay, float modAmount, float modPhase) const;
    };
}
```

---

### Priority 3: Low Impact, Quality of Life

#### 3.1 Consistent Naming Conventions
**Current Issues:**
- ParameterSmoother vs SmoothedParameter vs ParamSmoother vs UltraSmoother
- DCBlocker vs DcBlocker vs DC_Blocker
- SVF vs StateVariableFilter vs SVFCore

**Recommendation:** Standardize on:
- `ParameterSmoother` (not SmoothedParameter)
- `DCBlocker` (capital letters)
- `StateVariableFilter` (full name) or `SVFilter` (abbreviated)

---

#### 3.2 Shared Utility Constants
```cpp
// Add to DspEngineUtilities.h
namespace DSPConstants {
    constexpr double PI = 3.14159265358979323846;
    constexpr double TWO_PI = 2.0 * PI;
    constexpr float DENORMAL_THRESHOLD = 1.0e-30f;
    constexpr float MIN_FREQUENCY = 20.0f;
    constexpr float MAX_FREQUENCY = 20000.0f;
    constexpr float ROOM_TEMP_KELVIN = 298.15;
    constexpr float THERMAL_VOLTAGE = 0.026f; // 26mV
}
```

---

## Adoption Metrics

### Current DspEngineUtilities.h Adoption

| Utility | Total Engines | Using Shared | Using Custom | Adoption % |
|---------|--------------|-------------|--------------|-----------|
| Parameter Smoothing | 84 | 15 | 30 | 33% |
| DC Blocker | 50 | 20 | 30 | 40% |
| Denormal Protection | 84 | 40 | 44 | 48% |
| Circular Buffer | 30 | 5 | 25 | 17% |
| One-Pole Filter | 40 | 10 | 30 | 25% |
| Peak Meter | 20 | 2 | 18 | 10% |

**Overall Shared Utility Adoption: ~35%**

---

## Code Metrics

### Duplication Statistics
```
Total Engine Files: 84
Total Lines of Engine Code: ~45,000
Estimated Duplicate Code: ~8,000 lines (18%)
Duplicate Parameter Smoothers: ~900 lines
Duplicate SVF Implementations: ~1,500 lines
Duplicate DC Blockers: ~300 lines
Duplicate Oversampling: ~2,500 lines
Duplicate Biquads: ~1,200 lines
Other Duplicates: ~1,600 lines
```

### Potential Code Reduction
**After Full Refactoring:**
- Estimated removal: ~6,000 lines (75% of duplicates)
- Remaining justified duplication: ~2,000 lines (specialized variants)
- **Net Reduction: 13% of total engine codebase**

---

## Recommendations

### Immediate Actions (Sprint 1-2)
1. **Audit all Parameter Smoothing usage** - Create migration guide
2. **Promote DspEngineUtilities.h** - Add documentation and examples
3. **Add AtomicSmoother variant** - Covers thread-safe use cases
4. **Standardize denormal protection** - Use `DSPUtils::flushDenorm()` everywhere

### Short Term (Quarter 1)
1. **Create DspFilters.h library** - Consolidate SVF, Biquad, One-pole
2. **Refactor 20 highest-duplication engines** - Proof of concept
3. **Add unit tests for shared utilities** - Ensure correctness
4. **Performance benchmark shared vs custom** - Validate no regression

### Long Term (Quarter 2-3)
1. **Standardized oversampling library** - Multiple quality presets
2. **Migrate all engines to shared utilities** - Gradual rollout
3. **Remove deprecated duplicate implementations** - Clean up
4. **Create DSP cookbook documentation** - Best practices guide

---

## Risk Assessment

### Low Risk Refactorings
- ✅ Parameter smoothing consolidation (behavior nearly identical)
- ✅ DC blocker standardization (simple algorithm)
- ✅ Denormal protection (already working in many engines)

### Medium Risk Refactorings
- ⚠️ SVF consolidation (some engines have specialized variants)
- ⚠️ Biquad standardization (coefficient calculation variations)
- ⚠️ Oversampling (quality differences matter for distortion)

### High Risk Refactorings
- ❌ Reverb algorithm consolidation (algorithms are intentionally different)
- ❌ Saturation/distortion models (character is engine-specific)
- ❌ Component modeling (vintage emulations have unique approaches)

---

## Testing Strategy

### For Each Refactored Component

1. **Unit Tests**
   - Test shared utility in isolation
   - Verify edge cases (DC, denormals, extremes)
   - Benchmark performance

2. **Integration Tests**
   - A/B test: original vs refactored engine
   - Ensure bit-identical output (where expected)
   - THD+N measurements should match

3. **Regression Tests**
   - Run existing engine test suite
   - Verify all engines pass after migration
   - Check for unexpected behavior changes

4. **Performance Tests**
   - CPU usage should not increase
   - Memory usage should not increase
   - Latency should remain constant

---

## Success Criteria

### Quantitative Metrics
- ✅ Reduce duplicate code by >50% (~4,000 lines)
- ✅ Increase shared utility adoption to >70%
- ✅ Maintain or improve performance (0% regression tolerance)
- ✅ All existing tests pass after refactoring
- ✅ Reduce total compilation time by >10%

### Qualitative Metrics
- ✅ Easier to add new engines (less boilerplate)
- ✅ Consistent behavior across engines
- ✅ Improved code maintainability
- ✅ Better documentation and examples
- ✅ Reduced onboarding time for new developers

---

## Conclusion

The Project Chimera codebase exhibits significant but manageable code duplication. The existing `DspEngineUtilities.h` library provides a solid foundation, but adoption is inconsistent (~35%).

**Key Opportunities:**
1. **Parameter smoothing** - Highest impact, lowest risk refactoring
2. **SVF consolidation** - High impact, medium complexity
3. **Oversampling standardization** - High impact, provides quality options

**Recommended Approach:**
- Start with low-risk, high-impact refactorings (parameter smoothing)
- Gradually expand shared utility library
- Migrate engines incrementally with thorough testing
- Maintain backward compatibility during transition
- Document shared utilities extensively

**Expected Outcome:**
- ~6,000 lines of code reduction (13% of engine codebase)
- More consistent behavior across engines
- Easier maintenance and faster development
- Professional-grade DSP utilities available to all engines

---

## Appendix A: Shared Utility Usage Examples

### Example 1: Using Shared Parameter Smoother
```cpp
// Before (custom implementation)
class MyEngine : public EngineBase {
    struct MyParamSmoother {
        float current = 0.0f;
        float target = 0.0f;
        float coeff = 0.99f;
        // ... implementation
    } m_gainSmoother;
};

// After (using shared utility)
class MyEngine : public EngineBase {
    ParamSmoother m_gainSmoother;

    void prepareToPlay(double sr, int block) override {
        m_gainSmoother.setTimeMs(15.0, sr); // 15ms smoothing
    }

    void process(juce::AudioBuffer<float>& buf) override {
        DenormalGuard guard; // Automatic FTZ/DAZ

        for (int i = 0; i < buf.getNumSamples(); ++i) {
            float gain = m_gainSmoother.process(targetGain);
            // ... apply gain
        }
    }
};
```

### Example 2: Using Shared SVF (Proposed)
```cpp
// Before (custom SVF)
class FilterEngine : public EngineBase {
    float s1 = 0.0f, s2 = 0.0f;

    float processSVF(float input, float freq, float Q) {
        // ... 20 lines of SVF math
    }
};

// After (using shared SVF)
class FilterEngine : public EngineBase {
    DSPUtils::StateVariableFilter m_filter;

    float processSVF(float input, float freq, float Q) {
        auto response = m_filter.process(input, freq, Q, m_sampleRate);
        return response.lowpass; // or .bandpass, .highpass, etc.
    }
};
```

---

## Appendix B: Migration Checklist

For each engine being refactored:

- [ ] Identify all custom DSP utilities
- [ ] Map to equivalent shared utilities
- [ ] Replace custom implementations
- [ ] Update prepareToPlay() for initialization
- [ ] Run unit tests
- [ ] Run integration tests
- [ ] Compare output with original (A/B test)
- [ ] Benchmark CPU usage
- [ ] Update documentation
- [ ] Code review
- [ ] Merge to main branch

---

**Report End**

*For questions or clarifications, contact the DSP architecture team.*
