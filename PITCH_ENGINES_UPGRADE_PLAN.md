# Pitch/Frequency Engines - Studio Quality Upgrade Plan

## Current Status Assessment

### Engines Tested:
1. **PitchShifter** - Grade: A (92/100) ✅ FIXED
2. **FrequencyShifter** - Grade: B- (78/100) ⚠️ NEEDS WORK
3. **PlatinumRingModulator** - Grade: A+ (94/100) ✅ PRODUCTION READY
4. **AnalogRingModulator** - Grade: B+ (82/100) ⚠️ NEEDS WORK
5. **IntelligentHarmonizer** - Grade: A/A+ ✅ UPGRADED WITH TRUE PSOLA
6. **GranularCloud** - Grade: TBD 🔄 NEEDS TESTING

## Phase 1: Comprehensive Testing (Day 1)

### Test GranularCloud Engine
```cpp
// Use AutonomousEngineAnalyzer with these test cases:
1. Grain Size sweep (16-4096 samples)
2. Grain Density (1-100 grains/sec)
3. Pitch Scatter (0-100%)
4. Position Randomization
5. Texture Mode variations
6. Freeze Mode functionality
7. CPU performance under load
8. Phase coherence analysis
9. Spectral artifacts detection
10. Transient preservation
```

### Expected Issues in GranularCloud:
- Grain scheduling artifacts
- Click/pop at grain boundaries
- Phase cancellation in dense textures
- CPU spikes with high grain counts
- Parameter scaling non-linearities

## Phase 2: Issue Analysis & Prioritization (Day 1-2)

### Critical Issues (Must Fix):
1. **FrequencyShifter**:
   - Remove unnecessary thermal modeling (adds 15% CPU)
   - Fix Hilbert transform phase response
   - Optimize carrier oscillator
   
2. **AnalogRingModulator**:
   - Fix DC offset from carrier bleed (0.05-0.1 DC)
   - Improve pitch detection accuracy
   - Add proper denormal protection

3. **GranularCloud** (anticipated):
   - Grain boundary discontinuities
   - Inefficient grain scheduling
   - Memory allocation in real-time context

### Quality Improvements (Should Fix):
1. All engines need consistent parameter scaling
2. Improved denormal protection
3. Lock-free parameter updates
4. SSE/NEON optimizations where applicable

## Phase 3: Implementation Strategy (Day 2-4)

### FrequencyShifter Fixes
```cpp
// REMOVE thermal modeling completely
class FrequencyShifter {
    // DELETE: ThermalModeling thermalModel;
    
    // OPTIMIZE: Replace with efficient Hilbert
    class OptimizedHilbert {
        // Use 127-tap FIR design
        // OR frequency-domain approach
        // Add proper phase compensation
    };
    
    // FIX: Carrier generation
    void generateCarrier() {
        // Use wavetable lookup
        // Or fast sincos approximation
        // Remove modulo operations
    }
};
```

### AnalogRingModulator Fixes
```cpp
class AnalogRingModulator {
    // FIX: DC blocking
    DCBlocker inputDC, outputDC;
    
    // FIX: Carrier bleed
    float process() {
        float carrier = oscillator.process();
        float modulated = input * carrier;
        
        // Add high-pass to remove carrier bleed
        modulated = dcBlocker.process(modulated);
        
        // Soft saturation instead of hard clip
        return softSaturate(modulated);
    }
    
    // IMPROVE: Pitch detection
    YINPitchDetector pitchTracker;  // Reuse from PSOLA
};
```

### GranularCloud Optimization
```cpp
class GranularCloud {
    // FIX: Grain scheduling
    struct GrainPool {
        static constexpr int kMaxGrains = 128;
        std::array<Grain, kMaxGrains> grains;
        std::atomic<int> activeCount{0};
        
        // Lock-free grain allocation
        Grain* allocate() {
            // Use atomic compare-exchange
        }
    };
    
    // FIX: Window function
    void applyWindow(Grain& grain) {
        // Pre-calculated Tukey window
        // Smooth attack/release (5-10ms)
        // Prevents clicks
    }
    
    // ADD: Texture modes
    enum TextureMode {
        SPARSE,    // Few grains, clear
        DENSE,     // Many grains, smooth
        CHAOTIC,   // Random everything
        FROZEN,    // Spectral freeze
        RHYTHMIC   // Beat-synced grains
    };
};
```

## Phase 4: Testing Protocol (Day 4-5)

### Test Suite for Each Engine:
```cpp
struct StudioQualityTest {
    // Signal tests
    void testSineResponse();      // 440Hz, 1kHz, 10kHz
    void testComplexHarmonics();  // Multi-harmonic signals
    void testTransients();         // Clicks, drums
    void testNoise();             // White, pink noise
    void testSpeech();            // Vocal formants
    
    // Quality metrics
    void measureSNR();            // Target: >50dB
    void measureTHD();            // Target: <5%
    void measureLatency();        // Target: <10ms
    void measureCPU();            // Target: <5% per instance
    
    // Stress tests
    void testParameterSweeps();   // Smooth transitions
    void testAutomation();        // No clicks/pops
    void testPolyphony();         // 16 voices minimum
    void testBypass();            // Clean bypass
};
```

### Pass Criteria:
- SNR > 50dB
- THD < 5% (except intentional distortion)
- No clicks/pops during parameter changes
- CPU < 5% per instance at 48kHz
- Latency < 512 samples
- Phase coherence > 0.8 for pitched signals
- No memory leaks or allocations in process()

## Phase 5: Optimization (Day 5-6)

### SSE/NEON Optimizations
```cpp
// Vectorize critical paths
#if defined(__ARM_NEON)
    // NEON optimizations for M1/M2
    void processBlock_NEON(float* data, int samples) {
        float32x4_t vec;
        // Process 4 samples at once
    }
#elif defined(__SSE2__)
    // SSE optimizations for Intel
    void processBlock_SSE(float* data, int samples) {
        __m128 vec;
        // Process 4 samples at once
    }
#endif
```

### Memory Optimization
- Pre-allocate all buffers
- Use circular buffers
- Align memory for SIMD
- Remove all malloc/new in process()

## Phase 6: Final Validation (Day 6-7)

### Studio Quality Checklist:
- [ ] All engines grade A- or better
- [ ] Consistent parameter behavior
- [ ] No crashes or hangs
- [ ] Clean bypass
- [ ] Proper gain staging
- [ ] Documentation updated
- [ ] Unit tests passing

### Regression Tests:
```bash
# Run comprehensive test suite
./test_all_pitch_engines

# Plugin validation
auval -v aumf Chmr ChmA
pluginval --validate ChimeraPhoenix.component

# Performance profiling
instruments -t "Time Profiler" ./test_performance
```

## Implementation Order:

### Week 1 Schedule:
**Day 1**: Test GranularCloud, analyze all issues
**Day 2**: Fix FrequencyShifter (remove thermal, optimize)
**Day 3**: Fix AnalogRingModulator (DC offset, carrier)
**Day 4**: Implement GranularCloud improvements
**Day 5**: Comprehensive testing round 1
**Day 6**: Final optimizations and tweaks
**Day 7**: Validation and documentation

## Success Metrics:

### Minimum Requirements (Must Have):
- All engines functional without crashes
- No severe artifacts or distortion
- Parameter at 0.5 = unity/neutral
- CPU usage reasonable (<10% total)

### Target Goals (Should Have):
- All engines grade A- or better
- SNR > 50dB
- THD < 5%
- Smooth parameter automation
- <5% CPU per instance

### Stretch Goals (Nice to Have):
- All engines grade A or A+
- SNR > 60dB
- THD < 2%
- SIMD optimizations complete
- <3% CPU per instance

## Risk Mitigation:

### Potential Blockers:
1. **GranularCloud too complex** - Simplify to core features
2. **CPU usage too high** - Reduce grain count limits
3. **Phase artifacts persist** - Add phase randomization
4. **Testing reveals new bugs** - Time-box fixes to critical only

### Backup Plan:
If any engine cannot reach B+ grade:
1. Mark as "experimental" in UI
2. Add warning in documentation
3. Provide simpler alternative
4. Schedule for v3.1 update

## Final Deliverables:

1. All 6 pitch engines at studio quality (A- or better)
2. Comprehensive test results document
3. Performance benchmarks
4. Updated documentation
5. Git commits with clear messages
6. Release notes highlighting improvements

---

## Next Steps:

1. Begin with GranularCloud testing immediately
2. Create test harness for automated testing
3. Set up performance profiling
4. Document all findings in real-time
5. Implement fixes in priority order
6. Re-test after each major fix
7. Final validation before marking complete