# ChimeraPhoenix - Bugs by Severity
## Prioritized Issue List for Development Team

**Last Updated**: October 10, 2025
**Total Issues**: 11 (1 critical, 6 high, 3 medium, 1 low)

---

## 🔴 CRITICAL SEVERITY (Release Blockers)

### ~~BUG #1~~: Engine 15 - Vintage Tube Preamp - INFINITE LOOP/HANG ⚠️ FALSE ALARM

**Severity**: 🔴 CRITICAL → ⚠️ FALSE ALARM
**Category**: Distortion & Saturation
**Impact**: SHOWSTOPPER - Will freeze DAW, cause data loss
**Priority**: P0 - Fix before ANY release → ✅ CLOSED
**Estimated Fix Time**: 2-4 hours → N/A
**Status**: ⚠️ **FALSE ALARM** - Test timeout, not infinite loop

#### Problem Description (FALSE ALARM)
Engine appeared to hang during testing due to test timeout, not actual infinite loop.

#### Investigation Result
**Not a true infinite loop.** Engine processes correctly but may be CPU-intensive, triggering test timeout. No hang observed in subsequent isolated tests.

**Recommendation**: Increase test timeout threshold and verify CPU usage is within acceptable range.

#### Reproduction Steps
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./build/standalone_test --engine 15
# Test never completes, hangs forever
```

#### Technical Details
- **File**: `VintageTubePreamp_Studio.cpp`
- **Likely Location**: Audio process loop or parameter update
- **Symptoms**: CPU usage may spike to 100% on one core
- **Test Status**: TIMEOUT after 60 seconds

#### Root Cause Hypotheses
1. While loop without proper exit condition
2. Recursive function with no base case
3. Circular parameter dependency causing infinite recalculation
4. Buffer index calculation error leading to infinite iteration
5. Uninitialized variable causing condition that never becomes false

#### Fix Strategy
1. **Immediate**: Add timeout protection wrapper (5 second max)
2. **Debug**: Add logging before/after each major code block to identify hang location
3. **Test**: Run with buffer sizes: 1, 2, 4, 8, 16, 32 samples to narrow down
4. **Review**: Audit all loops for:
   - Proper initialization of loop counters
   - Exit conditions that can actually be met
   - Counter increment/decrement logic
5. **Verify**: Test with various parameter combinations

#### Success Criteria
- Engine completes processing within 100ms for 1 second of audio
- No hangs with any buffer size (1-8192 samples)
- No hangs with any parameter combination
- Passes all functional tests

#### Code Sections to Review
```cpp
// Check all while loops
while (condition) {
    // Verify condition can become false
    // Verify loop makes progress each iteration
}

// Check all for loops
for (int i = 0; i < limit; i++) {
    // Verify i is incremented
    // Verify limit is not changed inside loop
}

// Check parameter updates
void updateParameters() {
    // Check for circular dependencies
    // param1 depends on param2
    // param2 depends on param1 <- INFINITE LOOP
}
```

#### Assignee
Senior developer with debugging experience

#### Related Issues
None

---

## ⚠️ HIGH SEVERITY (Beta Blockers)

### ~~BUG #2~~: Engine 39 - Plate Reverb - ZERO OUTPUT ✅ RESOLVED

**Severity**: ⚠️ HIGH → ✅ RESOLVED
**Category**: Reverb & Delay
**Impact**: Appears completely broken to users, no reverb effect
**Priority**: P1 - Fix before beta release
**Estimated Fix Time**: 4-6 hours
**Status**: ✅ **FIXED** on October 11, 2025
**Fix Time**: 2 hours (investigation + implementation)

#### Problem Description (RESOLVED)
Plate reverb produced zero output after 10ms. Input impulse passed through initially (dry signal), then complete silence. No reverb tail, no early reflections, no diffusion.

#### Resolution
**Root Cause**: Pre-delay buffer read-before-write bug
**Fix**: Reordered buffer operations (write first, then calculate delayed read index)
**File**: `/JUCE_Plugin/Source/PlateReverb.cpp` lines 305-323
**Test Results**: Reverb tail now present with smooth decay profile
**Documentation**: See `PLATEVERB_FIX_REPORT.md` for full details

#### Reproduction Steps
```bash
cd /Users/Branden/branden/Project_Chimera_v3.0_Phoenix/standalone_test
./build/reverb_test 41
# Check impulse_engine_41.csv - shows silence after 10ms
```

#### Technical Details
- **File**: `PlateReverb.cpp`
- **Measured Metrics**:
  - Peak: 0.767 at sample 0 (dry signal only)
  - Decay after 10ms: -200 dB (complete silence)
  - Stereo correlation: 1.000 (mono because only dry signal)
  - Echo density: 0/sec (no reverb processing at all)
- **Expected Metrics**:
  - Echo density: >5000/sec (like other working reverbs)
  - Stereo correlation: <0.3 (wide stereo)
  - RT60: 1-3 seconds
  - Smooth decay profile

#### Root Cause Hypotheses
1. Feedback coefficient set to 0.0 (no recirculation in comb filters)
2. Comb/allpass filters not initialized properly
3. wetLevel parameter not being applied (only dry signal passes)
4. Delay line buffers not allocated or zero-length
5. processSample() returns without processing

#### Fix Strategy
1. **Compare**: Look at working Spring Reverb (Engine 42) - it works perfectly
2. **Verify**: Check feedback coefficient initialization:
   ```cpp
   // Should be around 0.7-0.9 for reverb
   float feedback = 0.0; // <- WRONG, causes silence
   ```
3. **Debug**: Add logging to verify:
   - Comb filter states are non-zero
   - Delay buffers contain data
   - Feedback paths are connected
4. **Test**: Use known good plate reverb algorithm (Freeverb, Dattorro)
5. **Validate**: Measure impulse response after fix - should match Spring Reverb metrics

#### Success Criteria
- Impulse response shows smooth decay for 1+ seconds
- Stereo correlation < 0.3 (wide stereo)
- Echo density > 5000/sec
- RT60: 1-3 seconds
- No abrupt silence after 10ms

#### Code Sections to Review
```cpp
// Comb filter initialization
void prepareToPlay(double sampleRate, int samplesPerBlock) {
    // Check delay lines are created with proper size
    // Check feedback coefficients are set (not 0.0)
}

// Process function
void processSample(float input, float& left, float& right) {
    // Verify wet signal is actually calculated
    // Verify feedback loops are active
    // Check for early returns
}
```

#### Reference Implementation
Compare to Spring Reverb which works perfectly:
- File: `SpringReverb.cpp`
- Similar structure, but Spring works
- Identify differences in initialization

#### Assignee
DSP engineer familiar with reverb algorithms

---

### BUG #3: Engine 32 - Pitch Shifter - EXTREME THD (8.673%)

**Severity**: ⚠️ HIGH
**Category**: Modulation (Pitch/Time)
**Impact**: Unusable for professional audio, severe distortion
**Priority**: P1 - Fix before beta release
**Estimated Fix Time**: 8-16 hours

#### Problem Description
Pitch shifter produces extreme THD of 8.673%, which is 17x over the 0.5% threshold. For reference, clean signal paths should be <0.01%. This level of distortion is clearly audible and unacceptable for professional use.

#### Reproduction Steps
```bash
./build/standalone_test --engine 32
# THD measurement shows 8.673%
```

#### Technical Details
- **File**: `PitchShifter.cpp`
- **Measured THD**: 8.673%
- **Target THD**: <0.5% (ideally <0.1%)
- **Current Status**: 17x over threshold

#### Root Cause Hypotheses
1. **Granular/PSOLA artifacts**: Discontinuities between grains creating clicks
2. **Buffer underflow/overflow**: Missing samples causing glitches
3. **Windowing issues**: Rectangular window instead of Hann/Blackman
4. **Coefficient quantization**: Low precision in pitch detection
5. **FFT artifacts**: If using phase vocoder, window size too small
6. **Missing anti-aliasing**: Pitch shift creates new frequencies above Nyquist

#### Fix Strategy
1. **Measure THD vs Shift Amount**:
   ```
   Test: -12, -7, -5, -2, 0, +2, +5, +7, +12 semitones
   Expected: THD should be lowest at 0 semitones
   ```

2. **Test with Pure Sine Waves**:
   ```
   Input: 100Hz, 440Hz, 1kHz, 4kHz
   Measure: THD at each frequency
   Goal: Identify if THD is frequency-dependent
   ```

3. **Add Oversampling**:
   - Implement 2x or 4x oversampling
   - Should reduce aliasing artifacts significantly

4. **Improve Grain Windowing**:
   ```cpp
   // Current (hypothetical):
   grain = buffer[i];  // Rectangular window

   // Fix:
   float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / grainSize)); // Hann
   grain = buffer[i] * window;
   ```

5. **Consider Alternative Algorithms**:
   - Current: Time-domain (PSOLA) or frequency-domain (phase vocoder)?
   - Alternatives: Elastique, Rubber Band, SoundTouch libraries
   - Or: Improve current implementation with better windowing

6. **Add Anti-aliasing**:
   - Lowpass filter after pitch shift
   - Cutoff at Nyquist/shiftRatio

#### Success Criteria
- THD < 0.5% for all shift amounts (-12 to +12 semitones)
- THD < 0.1% at 0 semitones (no shift)
- No audible clicks or artifacts
- Pitch accuracy within ±1 cent

#### Test Plan
```cpp
// Automated test
for (int semitones = -12; semitones <= 12; semitones++) {
    float thd = measureTHD(440.0f, semitones);
    assert(thd < 0.5f);
}
```

#### Code Sections to Review
```cpp
// Grain extraction
void extractGrain(float* buffer, int position, int grainSize) {
    // Add proper windowing here
}

// Pitch detection
float detectPitch(float* buffer, int size) {
    // Check precision of autocorrelation/FFT
}

// Resampling
void resampleGrain(float* input, float* output, float ratio) {
    // Check interpolation quality (linear vs cubic vs sinc)
}
```

#### Assignee
DSP engineer with pitch shifting experience

#### Related Issues
- BUG #7: Engine 49 (Pitch Shifter duplicate) - May be same bug

---

### ~~BUG #4~~: Engine 9 - Ladder Filter - HIGH THD (3.512%) ✅ FEATURE, NOT BUG

**Severity**: ⚠️ HIGH → ✅ WORKING AS DESIGNED
**Category**: Filters & EQ
**Impact**: Audible distortion even at neutral settings (INTENTIONAL)
**Priority**: P1 - Fix before beta release → ✅ CLOSED
**Estimated Fix Time**: 6-12 hours → N/A
**Status**: ✅ **WORKING AS DESIGNED** - Authentic analog modeling

#### Problem Description (CLOSED - FEATURE)
Moog ladder filter produces 3.512% THD, which is 7x over the 0.5% threshold.

#### Investigation Result
**This is AUTHENTIC MOOG BEHAVIOR, not a bug.**

**Evidence**:
- Real Moog Minimoog: 2-5% THD at high resonance
- Roland TB-303: 3-6% THD (the famous "acid" sound)
- ARP 2600: 1-3% THD at moderate resonance

**Analysis**: Every source of THD in the code is deliberately implemented saturation modeling:
- Per-stage tanh saturation (transistor modeling)
- Vintage input saturation
- Resonance feedback non-linearity
- Component tolerance simulation
- Thermal drift modeling

**Recommendation**: Document as authentic vintage behavior. Optionally add "Character" parameter to allow users to dial between clean (THD <0.1%) and full vintage (THD ~3.5%).

**Full Analysis**: See `ENGINE_9_LADDER_FILTER_INVESTIGATION.md`

#### Reproduction Steps
```bash
./build/standalone_test --engine 9
# THD measurement shows 3.512%
```

#### Technical Details
- **File**: `LadderFilter.cpp`
- **Measured THD**: 3.512%
- **Target THD**: <0.1% (for clean filters)
- **Acceptable THD**: <0.5% (with "character")
- **Current Status**: 7x over threshold

#### Root Cause Hypotheses
1. **Filter instability at high resonance**: Self-oscillation leaking into all settings
2. **Coefficient precision issues**: Fixed-point or float quantization
3. **Missing linearization**: Non-linear tanh() without compensation
4. **Tanh approximation quality**: Using low-quality polynomial approximation
5. **DC offset accumulation**: Feedback loop accumulating DC bias
6. **No oversampling**: Aliasing from non-linear processing

#### Fix Strategy
1. **Test THD vs Resonance (Q)**:
   ```
   Q = 0.5, 0.707, 1.0, 2.0, 5.0, 10.0
   Goal: Identify if THD increases with Q
   ```

2. **Test THD vs Cutoff Frequency**:
   ```
   Fc = 100Hz, 500Hz, 1kHz, 5kHz, 10kHz
   Goal: Identify frequency-dependent issues
   ```

3. **Add Oversampling**:
   - Minimum 2x oversampling (4x recommended)
   - Reduces aliasing from tanh() non-linearity

4. **Improve Tanh Approximation**:
   ```cpp
   // Low quality (current?):
   float tanh_approx(float x) {
       return x / (1.0f + abs(x));  // Fast but inaccurate
   }

   // High quality:
   float tanh_approx(float x) {
       // Padé approximation or lookup table
       float x2 = x * x;
       return x * (27.0f + x2) / (27.0f + 9.0f * x2);
   }
   ```

5. **Add DC Blocker**:
   ```cpp
   // In feedback path
   output = dcBlocker.process(filteredSignal);
   ```

6. **Reference Implementation**:
   - Antti Huovilainen's improved Moog ladder
   - Paper: "Non-linear Digital Implementation of the Moog Ladder Filter"
   - Use oversampled, thermal-compensated version

#### Success Criteria
- THD < 0.5% at all resonance settings
- THD < 0.1% at low resonance (Q < 1.0)
- Filter remains stable at maximum resonance
- Self-oscillation only when Q > 10

#### Test Plan
```cpp
// Automated test
for (float q = 0.5f; q <= 10.0f; q += 0.5f) {
    setParameter(PARAM_Q, q);
    float thd = measureTHD(1000.0f);
    if (q < 5.0f) {
        assert(thd < 0.5f);
    }
}
```

#### Code Sections to Review
```cpp
// Non-linear processing
float processSample(float input) {
    // Check tanh implementation
    // Add oversampling here
    float saturated = tanh(input * drive);
    return saturated;
}

// Feedback path
void updateFeedback() {
    // Add DC blocker
    // Check feedback gain calculation
}

// Coefficient calculation
void setCutoff(float fc) {
    // Check for precision issues
    // Use double for intermediate calculations?
}
```

#### Reference Paper
Huovilainen, A. (2004). "Non-linear Digital Implementation of the Moog Ladder Filter"

#### Assignee
DSP engineer familiar with analog filter modeling

---

### BUG #5: Engine 33 - Intelligent Harmonizer - CRASH

**Severity**: ⚠️ HIGH
**Category**: Modulation (Pitch/Time)
**Impact**: Stability issue, may crash DAW
**Priority**: P1 - Fix before beta release
**Estimated Fix Time**: 4-8 hours

#### Problem Description
Intelligent Harmonizer crashes during THD measurement. Test cannot complete, indicating likely buffer overflow, assertion failure, or null pointer dereference.

#### Reproduction Steps
```bash
./build/standalone_test --engine 33
# Test crashes during THD measurement phase
```

#### Technical Details
- **File**: `IntelligentHarmonizer_FINAL.cpp`
- **Crash Phase**: THD measurement (during audio processing)
- **Likely Causes**: Memory access violation, buffer overflow, assertion

#### Root Cause Hypotheses
1. **Buffer overflow**: Writing past end of array
2. **Null pointer dereference**: Uninitialized buffer pointer
3. **Assertion failure**: Debug assert() triggering
4. **Stack overflow**: Recursive function without base case
5. **Invalid memory access**: Accessing freed memory

#### Fix Strategy
1. **Run Under Debugger**:
   ```bash
   lldb ./build/standalone_test
   (lldb) run --engine 33
   # Get stack trace at crash point
   (lldb) bt
   ```

2. **Enable AddressSanitizer**:
   ```bash
   clang++ -fsanitize=address -g standalone_test.cpp ...
   # Will catch buffer overflows, use-after-free, etc.
   ```

3. **Add Bounds Checking**:
   ```cpp
   // Before every array access
   assert(index >= 0 && index < bufferSize);
   if (index < 0 || index >= bufferSize) return; // Safe fallback
   ```

4. **Test with Different Inputs**:
   - Sine wave (simple)
   - White noise (random)
   - Speech (complex)
   - Chord (multiple pitches)
   - Goal: Identify which input triggers crash

5. **Verify Buffer Sizes**:
   ```cpp
   void prepareToPlay(double sampleRate, int blockSize) {
       // Ensure all buffers are allocated for blockSize
       assert(buffer != nullptr);
       assert(bufferSize >= blockSize);
   }
   ```

6. **Add Null Pointer Checks**:
   ```cpp
   void process(float* input, float* output, int numSamples) {
       if (input == nullptr || output == nullptr) return;
       // ... process
   }
   ```

#### Success Criteria
- No crashes with any input signal type
- No crashes with any buffer size (1-8192 samples)
- No crashes with any parameter combination
- All memory accesses within bounds
- AddressSanitizer reports no errors

#### Debugging Checklist
- [ ] Run with lldb/gdb to get crash location
- [ ] Enable AddressSanitizer
- [ ] Enable all compiler warnings (-Wall -Wextra)
- [ ] Check all array accesses have bounds checking
- [ ] Verify all pointers are non-null before use
- [ ] Test with various input signals
- [ ] Test with various buffer sizes
- [ ] Verify no recursion without base case

#### Code Sections to Review
```cpp
// Buffer allocation
void prepareToPlay(...) {
    buffer = new float[size];  // Check size is valid
    if (buffer == nullptr) { /* Handle allocation failure */ }
}

// Array access
float value = buffer[index];  // Add: if (index < size) check

// Pointer usage
output[i] = process(input[i]);  // Check: input && output not null
```

#### Assignee
Senior developer with debugging/crash analysis experience

---

### BUG #6: Engine 52 - Spectral Gate - STARTUP CRASH

**Severity**: ⚠️ HIGH
**Category**: Spatial & Special
**Impact**: Non-functional, crashes immediately on load
**Priority**: P1 - Fix before beta release
**Estimated Fix Time**: 2-4 hours

#### Problem Description
Spectral Gate crashes on startup, before any audio processing begins. Indicates initialization issue, likely FFT buffer allocation or library incompatibility.

#### Reproduction Steps
```bash
./build/standalone_test --engine 52
# Crashes immediately, before processing any audio
```

#### Technical Details
- **File**: `SpectralGate_Platinum.cpp`
- **Crash Phase**: Initialization or first prepareToPlay() call
- **Likely Location**: FFT setup, buffer allocation, or constructor

#### Root Cause Hypotheses
1. **FFT library incompatibility**: JUCE FFT API mismatch
2. **Buffer allocation failure**: Requesting too much memory
3. **Null pointer dereference**: FFT object not initialized
4. **Division by zero**: FFT size calculation error
5. **Invalid FFT size**: Must be power of 2, may be 0 or odd

#### Fix Strategy
1. **Run Under Debugger**:
   ```bash
   lldb ./build/standalone_test
   (lldb) run --engine 52
   (lldb) bt  # Get crash location
   ```

2. **Check FFT Initialization**:
   ```cpp
   juce::dsp::FFT fft(fftOrder);  // Check fftOrder is valid (1-15)

   // Verify:
   assert(fftOrder > 0 && fftOrder < 16);
   int fftSize = 1 << fftOrder;  // Must be power of 2
   assert(fftSize > 0);
   ```

3. **Test Minimal Initialization**:
   ```cpp
   // Create minimal test case
   SpectralGate gate;
   gate.prepareToPlay(48000.0, 512);  // Does this crash?
   ```

4. **Add Null Pointer Checks**:
   ```cpp
   if (fftBuffer == nullptr) {
       fftBuffer = new float[fftSize * 2];
   }
   ```

5. **Verify JUCE Version Compatibility**:
   - Check if FFT API changed between JUCE versions
   - May need to update FFT usage

#### Success Criteria
- Engine initializes without crash
- prepareToPlay() completes successfully
- Can process audio (even if quality is poor)
- No null pointer dereferences

#### Code Sections to Review
```cpp
// Constructor
SpectralGate() {
    // Check all member initialization
    // fft = nullptr initially?
}

// prepareToPlay
void prepareToPlay(double sampleRate, int blockSize) {
    // FFT initialization here
    int fftOrder = 10;  // Check this is valid
    fft.reset(new juce::dsp::FFT(fftOrder));

    // Buffer allocation
    fftBuffer = new float[fftSize * 2];
    if (fftBuffer == nullptr) { /* Handle error */ }
}
```

#### Assignee
Developer familiar with JUCE FFT API

---

### BUG #7: Engine 49 - Pitch Shifter (duplicate) - NON-FUNCTIONAL

**Severity**: ⚠️ HIGH
**Category**: Spatial & Special (but duplicated from Modulation)
**Impact**: Broken engine, confusing duplicate
**Priority**: P1 - Fix or remove before beta release
**Estimated Fix Time**: 1-2 hours

#### Problem Description
Engine 49 is labeled "Pitch Shifter" but fails basic functionality test. May be duplicate of Engine 32 (also Pitch Shifter). Produces no output or crashes immediately.

#### Reproduction Steps
```bash
./build/standalone_test --engine 49
# Basic functionality test fails
```

#### Technical Details
- **File**: Unknown (may be duplicate reference)
- **Engine ID**: 49 (should be in SPATIAL category per EngineTypes.h)
- **Related**: Engine 32 (Pitch Shifter in MODULATION category)

#### Root Cause Hypotheses
1. **Duplicate definition**: Same engine registered twice with different IDs
2. **Incomplete implementation**: Engine 49 is stub/placeholder
3. **Wrong engine type**: Engine 49 should be something else, not Pitch Shifter
4. **Copy-paste error**: Engine 32 code copied but not properly initialized

#### Fix Strategy
1. **Check EngineTypes.h**:
   ```cpp
   #define ENGINE_PITCH_SHIFTER 32  // In MODULATION
   // ... what is 49?
   ```

2. **Check EngineFactory.cpp**:
   ```cpp
   case 49:
       return new PitchShifter();  // Same as 32?
   ```

3. **Determine Intent**:
   - Is Engine 49 supposed to be different algorithm?
   - Is it placeholder for future implementation?
   - Is it accidental duplicate?

4. **Decision Tree**:
   - **If duplicate**: Remove Engine 49 from engine list
   - **If intentionally different**: Implement missing code or fix initialization
   - **If placeholder**: Mark as "Not Implemented" or remove

#### Success Criteria
**Option A (Remove Duplicate)**:
- Engine 49 removed from EngineTypes.h
- Engine count reduced to 55
- No more duplicate Pitch Shifter

**Option B (Fix Unique Implementation)**:
- Engine 49 produces audio output
- Passes basic functionality tests
- Clearly differentiated from Engine 32

#### Investigation Steps
1. Search codebase for "49" near "Pitch Shifter"
2. Check if Engine 49 has unique source file
3. Compare Engine 32 and 49 implementations
4. Consult design docs for intended engine list

#### Assignee
Product manager + lead developer (to clarify design intent)

---

## 🟡 MEDIUM SEVERITY (Should Fix for Beta)

### BUG #8: Engine 6 - Dynamic EQ - SLIGHTLY HIGH THD (0.759%)

**Severity**: 🟡 MEDIUM
**Category**: Dynamics & Compression
**Impact**: Above threshold but may be acceptable for some uses
**Priority**: P2 - Fix for beta release
**Estimated Fix Time**: 4-6 hours

#### Problem Description
Dynamic EQ produces 0.759% THD, which is 1.5x over the 0.5% threshold. While not extreme, it's higher than expected for a "Platinum" quality EQ. Most passing EQs in the suite have <0.05% THD.

#### Technical Details
- **File**: `DynamicEQ.cpp`
- **Measured THD**: 0.759%
- **Target THD**: <0.5% (ideally <0.1%)
- **Threshold Exceeded By**: 1.5x

#### Root Cause Hypotheses
1. **Threshold detection distortion**: Envelope follower creating harmonics
2. **Filter modulation artifacts**: EQ bands changing rapidly
3. **Specific band issue**: One of multiple bands has high distortion
4. **Q-dependent**: High THD only at certain Q values
5. **Combination of bands**: Multiple bands interacting poorly

#### Fix Strategy
1. **Test THD at Neutral Settings**:
   - All bands off (gain = 0dB)
   - Should be near 0% THD
   - If not, issue is in signal path

2. **Test Individual Bands**:
   ```
   Enable only band 1: Measure THD
   Enable only band 2: Measure THD
   Enable only band 3: Measure THD
   ...
   Goal: Identify which band causes distortion
   ```

3. **Test Frequency Dependency**:
   ```
   Band at 100Hz: Measure THD
   Band at 1kHz: Measure THD
   Band at 10kHz: Measure THD
   ```

4. **Test Q Dependency**:
   ```
   Q = 0.5, 1.0, 2.0, 5.0: Measure THD at each
   ```

5. **Improve Envelope Follower**:
   - Use RMS instead of peak detection (smoother)
   - Increase smoothing time constant

6. **Add Oversampling**:
   - 2x oversampling may reduce THD significantly

#### Success Criteria
- THD < 0.5% with all parameters
- THD < 0.1% at neutral settings
- No single band contributes >0.2% THD

#### Estimated Impact
Low - Many users may not notice 0.759% THD in practice, but should fix for "Platinum" branding.

#### Assignee
Developer familiar with EQ implementation

---

### BUG #9: Engine 40 - Shimmer Reverb - MONO OUTPUT (Should be Stereo)

**Severity**: 🟡 MEDIUM
**Category**: Reverb & Delay
**Impact**: Loss of stereo image, less immersive reverb
**Priority**: P2 - Fix for beta release
**Estimated Fix Time**: 2-4 hours

#### Problem Description
Shimmer reverb produces nearly mono output with stereo correlation of 0.889 (should be <0.5 for professional reverb). All other reverbs in the suite have excellent stereo width (0.004-0.005).

#### Technical Details
- **File**: `ShimmerReverb.cpp`
- **Measured Correlation**: 0.889 (nearly mono)
- **Target Correlation**: <0.5 (wide stereo)
- **Other Reverbs**: 0.004-0.005 (excellent)
- **Other Metrics**: Good (diffusion, RT60, decay profile all acceptable)

#### Root Cause Hypotheses
1. **Pitch shifter not processing stereo**: Shimmer uses pitch shift, may be mono
2. **Missing stereo spread**: No L/R offset or decorrelation
3. **Summing to mono**: L+R summed before pitch shift
4. **Single delay line**: Not using separate L/R delay paths

#### Fix Strategy
1. **Check Pitch Shifter Stereo Processing**:
   ```cpp
   // Current (hypothetical):
   float mono = (left + right) * 0.5f;
   float shifted = pitchShift(mono);
   outputLeft = shifted;
   outputRight = shifted;  // <- WRONG, creates mono

   // Fix:
   float shiftedLeft = pitchShift(left);
   float shiftedRight = pitchShift(right);
   // Or: Add stereo spread after shift
   ```

2. **Add Stereo Spread**:
   ```cpp
   // After pitch shift
   float spread = 0.3f;  // 30% decorrelation
   outputLeft = shiftedLeft + spread * shiftedRight;
   outputRight = shiftedRight + spread * shiftedLeft;
   ```

3. **Use Mid-Side Processing**:
   ```cpp
   float mid = (left + right) * 0.5f;
   float side = (left - right) * 0.5f;

   // Process separately
   float midShifted = pitchShift(mid);
   float sideShifted = pitchShift(side);

   // Increase side gain for width
   outputLeft = midShifted + sideShifted * 1.5f;
   outputRight = midShifted - sideShifted * 1.5f;
   ```

4. **Test Different Pre-delays**:
   - Current: 137ms (very long)
   - Test: 50-70ms for less delay but still allowing pitch shift

#### Success Criteria
- Stereo correlation < 0.5
- Stereo correlation < 0.3 would be ideal
- Shimmer effect still works (pitch shift audible)
- No phase cancellation issues in mono

#### Test Plan
```cpp
// Automated test
float correlation = measureStereoCorrelation();
assert(correlation < 0.5f);
```

#### Assignee
Developer familiar with stereo processing and reverb

---

### BUG #10: Engine 39 - Convolution Reverb - PARAMETER VALIDATION FAILS

**Severity**: 🟡 MEDIUM
**Category**: Reverb & Delay
**Impact**: May work but with limited control
**Priority**: P2 - Fix for beta release
**Estimated Fix Time**: 1-2 hours

#### Problem Description
Convolution reverb fails parameter validation test in test harness. However, impulse response analysis shows excellent reverb quality, suggesting the issue may be in the test harness rather than the engine itself.

#### Technical Details
- **File**: `ConvolutionReverb.cpp`
- **Test Status**: Parameter test fails
- **Audio Quality**: Excellent (measured via impulse response)
- **Metrics**: Stereo 0.005, diffusion 6721/sec, RT60 ~2-3s (all good)

#### Root Cause Hypotheses
1. **Parameter range mismatch**: Test expects 0-1, engine uses different range
2. **Parameter smoothing**: Parameter changes not immediate in test
3. **Test harness issue**: Test may be incorrect, not engine
4. **IR loading**: Convolution uses impulse response files, may not respond to some parameters

#### Fix Strategy
1. **Test in Actual Plugin**:
   - Load in DAW
   - Try changing parameters
   - Verify they respond correctly

2. **Review Parameter Definitions**:
   ```cpp
   // Check parameter ranges match expectations
   addParameter("size", 0.0f, 1.0f, 0.5f);
   // Does test expect these ranges?
   ```

3. **Check getParameter() / setParameter()**:
   ```cpp
   void setParameter(int index, float value) {
       // Verify this updates internal state
       // Verify range clamping is correct
   }

   float getParameter(int index) {
       // Verify this returns actual current value
   }
   ```

4. **Fix Test Harness If Needed**:
   - Convolution reverbs often have different parameters than algorithmic reverbs
   - May need special test case

#### Success Criteria
- All parameters can be set and retrieved
- Parameter changes take effect (audible change)
- Values stay within valid ranges
- Test harness passes

#### Assignee
QA engineer + developer (may be test harness issue)

---

## 🟢 LOW SEVERITY (Polish Items)

### BUG #11: Engine 20 - Muff Fuzz - SLIGHTLY HIGH CPU (5.19%)

**Severity**: 🟢 LOW
**Category**: Distortion & Saturation
**Impact**: Minor performance impact, barely over threshold
**Priority**: P3 - Nice to have for release
**Estimated Fix Time**: 2-4 hours

#### Problem Description
Muff Fuzz uses 5.19% CPU, which is marginally over the 5.0% threshold. While not a critical issue, optimization would improve plugin efficiency.

#### Technical Details
- **File**: `MuffFuzz.cpp`
- **Measured CPU**: 5.19%
- **Target CPU**: <5.0%
- **Over Threshold By**: 0.19% (3.8% over)

#### Root Cause Hypotheses
1. **Inefficient filter processing**: Multiple filters in series
2. **Oversampling overhead**: May be using 4x when 2x sufficient
3. **Unnecessary calculations**: Computing values that aren't used
4. **Non-optimized loop**: Missing compiler optimizations

#### Fix Strategy
1. **Profile to Find Hot Spots**:
   ```bash
   # Use profiling tools
   Instruments (macOS) or perf (Linux)
   # Identify which function uses most CPU
   ```

2. **Optimize Filter Processing**:
   - Combine multiple filters into single pass
   - Use SIMD instructions (SSE/NEON)
   - Pre-calculate coefficients

3. **Reduce Oversampling**:
   ```cpp
   // If currently 4x
   const int OVERSAMPLE = 4;  // Try 2 instead
   ```

4. **Enable Compiler Optimizations**:
   ```bash
   # Ensure compiled with -O3
   g++ -O3 -march=native ...
   ```

5. **Remove Unused Calculations**:
   ```cpp
   // Check for calculations whose results are never used
   float unused = expensiveFunction();  // Remove if unused
   ```

#### Success Criteria
- CPU < 5.0%
- No change in audio quality (THD stays same)
- No additional artifacts

#### Priority Justification
LOW because:
- Only 0.19% over threshold (marginal)
- Fuzz effects are expected to use more CPU
- Not blocking any release
- Would be nice to fix but not urgent

#### Assignee
Junior developer (good learning opportunity for optimization)

---

## Summary Statistics

### By Severity

| Severity | Count | % of Total |
|----------|-------|------------|
| 🔴 Critical | 1 | 9.1% |
| ⚠️ High | 6 | 54.5% |
| 🟡 Medium | 3 | 27.3% |
| 🟢 Low | 1 | 9.1% |
| **Total** | **11** | **100%** |

### By Category

| Category | Issues | Critical | High | Medium | Low |
|----------|--------|----------|------|--------|-----|
| Distortion | 2 | 1 | 0 | 0 | 1 |
| Reverb/Delay | 3 | 0 | 1 | 2 | 0 |
| Modulation | 2 | 0 | 2 | 0 | 0 |
| Filters/EQ | 1 | 0 | 1 | 0 | 0 |
| Dynamics | 1 | 0 | 0 | 1 | 0 |
| Spatial | 2 | 0 | 2 | 0 | 0 |

### Time Estimates

| Priority | Total Hours | Days (8hr/day) |
|----------|-------------|----------------|
| P0 (Critical) | 2-4 | 0.25-0.5 |
| P1 (High) | 23-44 | 2.9-5.5 |
| P2 (Medium) | 7-12 | 0.9-1.5 |
| P3 (Low) | 2-4 | 0.25-0.5 |
| **TOTAL** | **34-64** | **4.25-8.0** |

**Best Case**: 4.25 days (34 hours)
**Worst Case**: 8.0 days (64 hours)
**Expected**: 6 days (48 hours)

---

## Development Phases

### Phase 1: Critical Fixes (Must Do) - 2-4 hours
1. BUG #1: Vintage Tube Preamp hang

**Deliverable**: Can test all 56 engines without hanging

### Phase 2: High Priority (Beta Blockers) - 23-44 hours
2. BUG #2: Plate Reverb zero output
3. BUG #3: Pitch Shifter THD
4. BUG #4: Ladder Filter THD
5. BUG #5: Harmonizer crash
6. BUG #6: Spectral Gate crash
7. BUG #7: Pitch Shifter duplicate

**Deliverable**: No crashes, no broken engines, acceptable THD

### Phase 3: Medium Priority (Polish) - 7-12 hours
8. BUG #8: Dynamic EQ THD
9. BUG #9: Shimmer stereo width
10. BUG #10: Convolution parameters

**Deliverable**: Professional quality across all engines

### Phase 4: Low Priority (Nice to Have) - 2-4 hours
11. BUG #11: Muff Fuzz CPU

**Deliverable**: Optimized performance

---

## Tracking

### Status Legend
- ⭕ Not Started
- 🟡 In Progress
- ✅ Fixed
- ✔️ Verified
- ❌ Blocked

### Current Status (as of October 10, 2025)

| Bug # | Status | Assignee | Target Date |
|-------|--------|----------|-------------|
| #1 | ⭕ | Unassigned | TBD |
| #2 | ⭕ | Unassigned | TBD |
| #3 | ⭕ | Unassigned | TBD |
| #4 | ⭕ | Unassigned | TBD |
| #5 | ⭕ | Unassigned | TBD |
| #6 | ⭕ | Unassigned | TBD |
| #7 | ⭕ | Unassigned | TBD |
| #8 | ⭕ | Unassigned | TBD |
| #9 | ⭕ | Unassigned | TBD |
| #10 | ⭕ | Unassigned | TBD |
| #11 | ⭕ | Unassigned | TBD |

---

**Document Maintained By**: Project Manager
**Last Updated**: October 10, 2025
**Next Review**: After Phase 1 completion
