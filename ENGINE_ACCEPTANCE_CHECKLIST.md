# DSP Engine Acceptance Checklist

## Engine: [ENGINE_NAME]
**ID:** [ENGINE_ID]  
**Category:** [CATEGORY]  
**Date Tested:** [DATE]  
**Tester:** [NAME]  

---

## Generic Tests (All Engines)

### Core Functionality
- [ ] **Bypass/Mix Law**: mix=0 → dry only; mix=1 → no dry bleed (≤ −100 dBFS)
- [ ] **Block-size Invariance**: Single block vs random chunks null ≤ −100 dBFS RMS
- [ ] **Sample Rate Invariance**: Behavior scales correctly at 44.1/48/96 kHz
- [ ] **Reset State**: Reset zeroes all state; no pre-ring after reset
- [ ] **NaN/Inf/Denormal Protection**: No invalid values; output bounded
- [ ] **Latency**: Reported latency == measured latency (±1 sample)
- [ ] **CPU Performance**: Fits budget on reference settings (<10% @ 48kHz)

### Code Quality Guardrails
- [ ] **DenormalGuard** present at top of `process()`
- [ ] **Thread-safe RNG**: No `rand()`, uses thread_local generators
- [ ] **Parameter Smoothing**: All audio-rate parameters smoothed
- [ ] **Memory Safety**: No buffer overruns, proper bounds checking

---

## Category-Specific Tests

### [SELECT APPROPRIATE SECTION]

#### REVERB
- [ ] **Impulse Response**: Measurable tail from impulse input
- [ ] **RT60 Accuracy**: Measured RT60 within ±15% of expected
- [ ] **Frequency Response**: HF RT60 < LF RT60 when damping high
- [ ] **Stereo Decorrelation**: L/R tail cross-correlation < 0.9
- [ ] **Tail Decay**: Tail < −90 dBFS by expected time
- [ ] **Reset Behavior**: Reset kills tail immediately

**Measurements:**
- RT60: _____s
- HF RT60: _____s  
- LF RT60: _____s
- L/R Correlation: _____
- Tail Floor: _____ dBFS

#### PITCH
- [ ] **Pitch Accuracy**: 440 Hz → +12 st = 880 Hz (±2 cents)
- [ ] **Pitch Accuracy**: 440 Hz → −7 st = 329.63 Hz (±2 cents)
- [ ] **Latency Stability**: Measured latency stable across buffer sizes
- [ ] **Formant Preservation**: (if applicable) Formants preserved
- [ ] **Artifact Free**: No glitches or discontinuities

**Measurements:**
- +12 st error: _____ cents
- −7 st error: _____ cents
- Latency: _____ samples

#### EQ/FILTER
- [ ] **Frequency Response**: Measured |H(f)| vs designed within ±0.5 dB
- [ ] **Q Accuracy**: Q factor within ±5% of setting
- [ ] **Linear Phase**: (if applicable) Flat group delay
- [ ] **Band Interaction**: Multiple bands sum correctly
- [ ] **Stability**: No oscillation at extreme settings

**Measurements:**
- Peak error: _____ dB
- Q error: _____%
- Group delay variation: _____ samples

#### DYNAMICS
- [ ] **Static Curve**: Matches specified ratio/knee
- [ ] **Attack Time**: Within ±10% of setting
- [ ] **Release Time**: Within ±10% of setting
- [ ] **GR Meter**: Agrees with actual gain reduction ±0.5 dB
- [ ] **Lookahead**: (if applicable) No overshoots

**Measurements:**
- Ratio accuracy: _____
- Attack time: _____ ms
- Release time: _____ ms
- GR meter error: _____ dB

#### DELAY/MODULATION
- [ ] **Delay Accuracy**: Echo time within ±1 sample
- [ ] **LFO Rate**: Within ±2% of setting
- [ ] **Feedback Stability**: Stable below max feedback
- [ ] **Modulation Depth**: Matches expected range
- [ ] **Phase Coherence**: No unwanted cancellation

**Measurements:**
- Delay error: _____ samples
- LFO rate error: _____%
- Max stable feedback: _____

#### DISTORTION
- [ ] **THD+N**: Rises monotonically with drive
- [ ] **IMD**: Lower with oversampling ON vs OFF
- [ ] **Saturation Curve**: Smooth, no discontinuities
- [ ] **Aliasing**: Controlled with oversampling
- [ ] **Gain Staging**: Unity gain at 0 drive

**Measurements:**
- THD+N @ 0dB: _____%
- IMD improvement: _____ dB
- Aliasing suppression: _____ dB

#### CONVOLUTION
- [ ] **Identity IR**: Passthrough with identity IR
- [ ] **Known IR**: Matches offline FFT (≤ −90 dBFS RMS error)
- [ ] **Latency**: Correctly reported
- [ ] **CPU Efficiency**: Optimized FFT implementation
- [ ] **IR Loading**: Handles various IR lengths

**Measurements:**
- Identity error: _____ dBFS
- Known IR error: _____ dBFS
- Latency: _____ samples

#### SPATIAL/UTILITY
- [ ] **Pan Law**: Equal-power, RMS constant ±0.5 dB
- [ ] **ITD/ILD**: Changes in correct direction
- [ ] **Mono Compatibility**: No cancellation in mono
- [ ] **Phase Coherence**: Maintains phase relationships
- [ ] **Width Control**: Smooth from mono to wide

**Measurements:**
- Pan law error: _____ dB
- Max ITD: _____ samples
- Mono sum level: _____ dB

---

## Test Artifacts

### Required Files
- [ ] Input test signal: `[ENGINE]_input.wav`
- [ ] Output signal: `[ENGINE]_output.wav`
- [ ] Measurements CSV: `[ENGINE]_measurements.csv`
- [ ] Frequency response plot: `[ENGINE]_response.png` (if applicable)
- [ ] Static curve plot: `[ENGINE]_curve.png` (if applicable)
- [ ] EDC plot: `[ENGINE]_edc.png` (if reverb)

### File Locations
```
test_artifacts/
├── [ENGINE_NAME]/
│   ├── input.wav
│   ├── output.wav
│   ├── measurements.csv
│   └── plots/
│       ├── response.png
│       └── curve.png
```

---

## Test Summary

### Pass/Fail Status
- **Generic Tests:** ⬜ PASS / ⬜ FAIL
- **Category Tests:** ⬜ PASS / ⬜ FAIL
- **Overall:** ⬜ PASS / ⬜ FAIL

### Issues Found
1. 
2. 
3. 

### Fixes Applied
1. 
2. 
3. 

### Performance Notes
- CPU Usage: _____%
- Memory Usage: _____ MB
- Optimization Opportunities: 

---

## Sign-off

- [ ] All tests passed
- [ ] Artifacts archived
- [ ] Code review complete
- [ ] Documentation updated

**Approved by:** ________________  
**Date:** ________________

---

## Appendix: Test Commands

```bash
# Run this specific engine test
./build_tests/engine_test_suite --engine [ENGINE_NAME]

# Run category tests
./build_tests/engine_test_suite --category [CATEGORY]

# Generate artifacts
./build_tests/engine_test_suite --engine [ENGINE_NAME] --artifacts

# Validate against reference
./build_tests/engine_test_suite --engine [ENGINE_NAME] --reference
```