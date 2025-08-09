# Chimera Phoenix DSP Architecture Guidelines

## Overview
This document defines the studio-grade DSP standards for all Chimera Phoenix audio engines. Following these guidelines ensures consistent, professional-quality audio processing across all 57+ engines.

## Core Architecture

### 1. Base Class Hierarchy
All engines inherit from `EngineBase` which provides:
- **Core API**: Required methods all engines must implement
- **Extended API**: Optional methods with safe defaults for advanced features
- **Feature Discovery**: Runtime capability queries

### 2. Shared Utilities (`DspEngineUtilities.h`)
Every engine should include this header and use its utilities for:
- Denormal protection
- NaN/Inf scrubbing  
- Parameter smoothing
- Lock-free atomics
- Common DSP building blocks

### 3. PIMPL Pattern
Use PIMPL (Pointer to Implementation) for ABI stability:
```cpp
class MyEngine : public EngineBase {
    struct Impl;
    std::unique_ptr<Impl> pImpl;
};
```

## Mandatory Safety Rules

### Rule 1: Denormal Protection
**ALWAYS** wrap `process()` with `DenormalGuard`:
```cpp
void process(juce::AudioBuffer<float>& buffer) {
    DenormalGuard guard;  // RAII - enables FTZ/DAZ
    // ... processing ...
    scrubBuffer(buffer);  // Final safety net
}
```

### Rule 2: Real-Time Safety
**NEVER** in `process()`:
- Allocate memory (`new`, `malloc`, `resize()`)
- Lock mutexes or semaphores
- Perform file I/O
- Call system APIs that may block
- Use `std::cout`, `printf`, or logging

**ALWAYS** pre-allocate in `prepareToPlay()`.

### Rule 3: Parameter Smoothing
**ALL** audible parameters must be smoothed:
```cpp
// Fast (2-5ms): Gain, threshold, gate
// Medium (10-20ms): Frequency, Q, feedback  
// Slow (50-100ms): Room size, character
MultiRateSmoother param;
param.prepare(sampleRate, MultiRateSmoother::Medium);
```

### Rule 4: Thread-Safe Parameters
Use `AtomicParam` for lock-free updates:
```cpp
AtomicParam gain{1.0f};
// UI thread: gain.set(newValue);
// Audio thread: float g = gain.get();
```

### Rule 5: Bypass Handling
Implement clickless bypass with ramping:
```cpp
BypassRamp bypass;
bypass.prepare(sampleRate, 5.0);  // 5ms ramp
```

## DSP Best Practices

### Filters
- Use TPT (Topology Preserving Transform) for modulated filters
- Prefer double precision for coefficients
- Clamp resonance/feedback to prevent instability
- Example: `OnePoleFilter`, `DCBlocker` from utilities

### Delays & Reverbs
- Use `CircularBuffer<T>` for delay lines
- Implement interpolated reads for smooth modulation
- Limit feedback to ±0.95 to prevent runaway
- Apply highpass filtering in feedback paths

### Saturators & Distortion
- Always use `tanh()` or similar bounded functions
- Pre/post filtering to control aliasing
- Oversampling for harsh distortion (2x-8x)
- DC blocking on output

### Dynamic Processors
- Logarithmic domain for envelope detection
- Separate attack/release timing
- Optional lookahead with proper latency reporting
- Smooth gain reduction to prevent artifacts

### Modulation Effects
- Phase accumulators for LFOs (not recursive sin/cos)
- Quadrature outputs for stereo effects
- Tempo sync via `TransportInfo`
- Smooth all modulation depths

## Latency Reporting

Engines **MUST** accurately report latency:
```cpp
int getLatencySamples() const noexcept override {
    return lookaheadSamples + fftLatency;
}
```

Common latency sources:
- Lookahead buffers (limiters, gates)
- FFT processing (block delay + overlap)
- Linear-phase filters
- Oversampling filters

## Quality Levels

Support multiple quality settings when applicable:
```cpp
void setQuality(Quality q) override {
    switch(q) {
        case Quality::Draft:  oversampleFactor = 1; break;
        case Quality::Normal: oversampleFactor = 2; break;
        case Quality::High:   oversampleFactor = 4; break;
        case Quality::Ultra:  oversampleFactor = 8; break;
    }
}
```

## Testing Requirements

### 1. Stability Test
Every engine must pass 1000 iterations without:
- NaN/Inf in output
- Hanging/timeout
- Memory leaks
- Crashes

### 2. Block Size Test
Engines must handle:
- Variable block sizes (1 to 8192 samples)
- Changing block sizes between calls
- No audible differences when concatenating blocks

### 3. Parameter Automation
- Smooth response to rapid parameter changes
- No clicks/pops during automation
- Thread-safe parameter updates

### 4. Bypass Test
- Bit-transparent when Mix=0% or bypassed
- Clickless bypass engagement
- No CPU spikes during bypass

## Reference Implementation

See `StereoChorus_Reference.h/cpp` for a complete example demonstrating:
- Extended EngineBase API
- DspEngineUtilities usage
- PIMPL pattern
- Denormal protection
- Parameter smoothing
- Thread-safe atomics
- Tempo sync
- Bypass ramping
- Equal-power mixing
- Proper latency reporting

## Migration Path

### Phase 1: Critical Engines (Immediate)
1. Engines with latency (limiters, FFT processors)
2. Engines with stability issues
3. High-CPU engines needing optimization

### Phase 2: Time-based Effects
1. Delays, reverbs, chorus, flangers
2. Add tempo sync support
3. Implement proper interpolation

### Phase 3: Full Migration
1. Update all remaining engines
2. Remove redundant safety code
3. Standardize parameter ranges

## Performance Targets

- **CPU**: < 1% per engine @ 96kHz on modern CPU
- **Latency**: Report accurately, minimize where possible
- **Memory**: Pre-allocate, no runtime allocations
- **Quality**: Transparent at Mix=0%, no aliasing, stable feedback

## Code Review Checklist

- [ ] Includes `DspEngineUtilities.h`
- [ ] Uses `DenormalGuard` in `process()`
- [ ] All parameters use `AtomicParam`
- [ ] All audible parameters smoothed
- [ ] No allocations in `process()`
- [ ] Implements `getLatencySamples()` if needed
- [ ] Supports bypass with ramping
- [ ] Final `scrubBuffer()` safety check
- [ ] Tested with variable block sizes
- [ ] Passes 1000-iteration stress test

## Common Pitfalls to Avoid

1. **Recursive filters without clamping** → Explosion
2. **Unsmoothed parameters** → Zipper noise
3. **Direct parameter access** → Race conditions
4. **No denormal handling** → CPU spikes
5. **Unbounded feedback** → Runaway/overflow
6. **Static variables** → Not thread-safe
7. **Integer wraparound in delays** → Crashes
8. **Assuming stereo** → Mono compatibility issues
9. **Ignoring block size changes** → Glitches
10. **Not reporting latency** → Phase issues in DAW

## Questions?

Contact the DSP team or refer to `StereoChorus_Reference` for implementation examples.