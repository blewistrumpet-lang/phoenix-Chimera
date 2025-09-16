# Chimera Phoenix Architecture Assessment & Recovery Plan

## Executive Summary
We have a partially working plugin with architectural issues that need systematic resolution. The core framework is solid, but library integrations and some implementations need careful fixing.

## Current Architecture

### Core Framework (WORKING ✅)
- **EngineBase Interface**: Clean abstraction for all DSP engines
- **Parameter System**: std::map-based parameter passing
- **Plugin Framework**: JUCE-based AU/VST3 plugin structure
- **Engine Factory**: Dynamic engine instantiation

### DSP Engine Categories

#### 1. Pitch/Formant Engines (6 total)
- **IntelligentHarmonizer**: ⚠️ Partially working (low-latency mode only)
- **PitchShifter**: ❌ Not outputting audio (Signalsmith issue)
- **DetuneDoubler**: ❓ Unknown status
- **FormantFilter**: ❓ Unknown status  
- **VocalFormantFilter**: ❓ Unknown status
- **PhasedVocoder**: ⚠️ Intermittent output (latency issues)

#### 2. Dynamics/Compression Engines
- Status: Reportedly working ✅

#### 3. Distortion Engines  
- Status: Reportedly working ✅

#### 4. Reverb/Delay Engines
- **ShimmerReverb**: ⚠️ No shimmer, weak reverb
- Others: ❓ Unknown status

## Critical Issues Identified

### 1. Signalsmith Library Integration FAILURE
**Problem**: The Signalsmith pitch-stretch library outputs silence
**Impact**: 
- IntelligentHarmonizer high-quality mode broken
- PitchShifter completely broken
- ShimmerReverb shimmer effect broken

**Root Cause**: Library was added but never properly initialized/tested
```cpp
stretcher.presetCheaper(1, sampleRate);  // This isn't enough
stretcher.process(...);  // Outputs zeros
```

### 2. Inconsistent Fallback Strategies
- IntelligentHarmonizer: Has low-latency fallback ✅
- PitchShifter: No fallback ❌
- ShimmerReverb: No fallback ❌

### 3. Parameter Design Issues
- ShimmerReverb: No reverb decay time control
- Feedback range too limited (0.4-0.55)
- Missing critical parameters for some engines

### 4. Testing Gap
- No systematic integration tests
- Library dependencies not validated
- Parameter ranges not verified

## Proposed Architecture Improvements

### Phase 1: Stabilization (Immediate)

#### 1.1 Create Pitch Shifting Abstraction
```cpp
class PitchShiftProcessor {
public:
    virtual void process(const float* in, float* out, int samples, float ratio) = 0;
    virtual int getLatency() const = 0;
};

class SimplePitchShift : public PitchShiftProcessor {
    // Delay-buffer based implementation (working)
};

class SignalsmithPitchShift : public PitchShiftProcessor {
    // Properly integrated Signalsmith (if fixable)
};
```

#### 1.2 Fix Critical Engines
1. **PitchShifter**: Add SimplePitchShift fallback
2. **ShimmerReverb**: Fix reverb parameters, add simple pitch shift
3. **PhasedVocoder**: Fix latency compensation

### Phase 2: Library Assessment

#### 2.1 Signalsmith Investigation
- [ ] Check if library needs more setup/configuration
- [ ] Test with manufacturer's examples
- [ ] Determine if fixable or needs replacement

#### 2.2 Alternative Libraries
If Signalsmith unfixable, consider:
- SoundTouch (proven, LGPL)
- Rubber Band (high quality, GPL/commercial)
- Custom PSOLA implementation (already working in low-latency mode)

### Phase 3: Systematic Testing

#### 3.1 Create Comprehensive Test Suite
```cpp
class EngineTestSuite {
    void testPassthrough();      // Unity gain test
    void testParameters();        // All parameter ranges
    void testProcessing();        // Actual DSP functionality
    void testLatency();          // Latency compensation
    void testStability();        // Long-running stability
};
```

#### 3.2 Parameter Validation Framework
```cpp
class ParameterValidator {
    bool validateRange(int id, float min, float max);
    bool validateDefault(int id, float defaultVal);
    bool validateResponse(int id, float value, float expected);
};
```

## Implementation Plan

### Week 1: Emergency Fixes
**Goal**: Get all engines passing audio

1. **Day 1-2**: Fix PitchShifter
   - Add simple delay-buffer pitch shift
   - Test thoroughly
   - Document limitations

2. **Day 3-4**: Fix ShimmerReverb
   - Increase feedback range (0.4 → 0.95)
   - Add decay time parameter
   - Implement simple pitch shift for shimmer

3. **Day 5**: Fix PhasedVocoder
   - Add proper latency reporting
   - Fix warmup/priming

### Week 2: Architecture Cleanup
**Goal**: Consistent, maintainable codebase

1. **Day 1-2**: Create abstraction layer
   - PitchShiftProcessor interface
   - Consistent fallback pattern

2. **Day 3-4**: Investigate Signalsmith
   - Deep dive into why it's not working
   - Decision: fix or replace

3. **Day 5**: Documentation
   - Document all engine parameters
   - Create usage examples
   - Update test suite

### Week 3: Quality Assurance
**Goal**: Robust, production-ready

1. Comprehensive testing
2. Performance optimization
3. Parameter tuning
4. Beta testing preparation

## Risk Mitigation

### Do's ✅
- Test EVERY change in isolation
- Keep working fallbacks
- Document parameter ranges
- Version control aggressively
- Create rollback points

### Don'ts ❌
- Don't remove working code without replacement
- Don't assume library integration "just works"
- Don't change multiple engines simultaneously
- Don't skip testing "simple" changes

## Success Metrics

1. **Functional**: All engines pass audio
2. **Quality**: No clicks, pops, or artifacts
3. **Performance**: <10% CPU per engine at 48kHz
4. **Stability**: 24-hour continuous operation test
5. **Usability**: Intuitive parameter responses

## Next Immediate Steps

1. **Create backup** of current state
2. **Fix PitchShifter** with simple implementation
3. **Fix ShimmerReverb** parameters
4. **Test thoroughly** before moving forward
5. **Document changes** meticulously

---

## Recommendation

**STOP** trying to fix everything at once. 

**START** with the simplest possible working implementation for each broken engine, using the delay-buffer approach that's already proven in IntelligentHarmonizer's low-latency mode.

Once everything is working (even if not optimal), we can carefully upgrade individual components with better algorithms.

The goal is: **Working first, optimal later.**