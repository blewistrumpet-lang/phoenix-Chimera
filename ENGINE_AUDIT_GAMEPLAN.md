# Chimera Phoenix Engine Audit Game Plan

## Overview
57 engines total, grouped by DSP category. Each group will be audited systematically to ensure all parameters work correctly.

---

## ENGINE GROUPS AND PRIORITIES

### 🔴 GROUP 1: PITCH/FREQUENCY ENGINES (Priority: CRITICAL)
**Why Critical:** User specifically mentioned these are broken
**Engines (6):**
1. **PitchShifter (31)** ✅ FIXED
2. **IntelligentHarmonizer (19)** - Likely has same phase vocoder bug
3. **PitchCorrection (30)** - Auto-tune functionality 
4. **FrequencyShifter (13)** - Single sideband modulation
5. **RingModulator (33)** - Carrier frequency modulation
6. **GranularDelay (15)** - Has pitch parameter

**Common Issues to Check:**
- Phase vocoder implementation (like PitchShifter had)
- FFT processing bugs
- Parameter scaling (semitones vs ratio)
- Formant preservation

---

### 🟡 GROUP 2: TIME/DELAY ENGINES (Priority: HIGH)
**Why High:** Core effects, widely used
**Engines (8):**
1. **Delay (5)**
2. **PingPongDelay (29)**
3. **DigitalDelay (35)**
4. **TapeDelay (42)**
5. **VintageDelay (48)**
6. **GranularDelay (15)** (also in pitch group)
7. **ConvolutionReverb (3)**
8. **Reverb (32)**

**Common Issues to Check:**
- Feedback loops (like PitchShifter had)
- Buffer management
- Sample rate handling
- Tempo sync parameters

---

### 🟡 GROUP 3: DYNAMICS ENGINES (Priority: HIGH)
**Why High:** Essential for mixing
**Engines (7):**
1. **Compressor (2)**
2. **Limiter (20)**
3. **Gate (14)**
4. **MultibandCompressor (24)**
5. **VintageCompressor (47)**
6. **TransientShaper (43)**
7. **EnvelopeFollower (8)**

**Common Issues to Check:**
- Threshold detection
- Attack/Release timing
- Sidechain routing
- Lookahead buffers

---

### 🟢 GROUP 4: FILTER ENGINES (Priority: MEDIUM)
**Engines (6):**
1. **Filter (11)**
2. **LowPass (21)**
3. **HighPass (18)**
4. **DualFilter (7)**
5. **Equalizer (9)**
6. **VintageEQ (49)**

**Common Issues to Check:**
- Frequency scaling (logarithmic)
- Resonance stability
- Filter type switching
- Coefficient calculation

---

### 🟢 GROUP 5: MODULATION ENGINES (Priority: MEDIUM)
**Engines (7):**
1. **Chorus (1)**
2. **Flanger (12)**
3. **Phaser (28)**
4. **VintageChorus (46)**
5. **TremoloEffect (44)**
6. **Vibrato** (if exists)
7. **Wobble (56)**

**Common Issues to Check:**
- LFO implementation
- Rate/Depth scaling
- Stereo processing
- Feedback paths

---

### 🟢 GROUP 6: DISTORTION ENGINES (Priority: MEDIUM)
**Engines (8):**
1. **Distortion (6)**
2. **Overdrive (26)**
3. **TubeDistortion (45)**
4. **Saturator (34)**
5. **HardClip (16)**
6. **Waveshaper (54)**
7. **BitCrusher (0)**
8. **Decimator (4)**

**Common Issues to Check:**
- Gain staging
- Anti-aliasing
- Tone controls
- Output levels

---

### 🔵 GROUP 7: SPATIAL/REVERB ENGINES (Priority: LOW)
**Engines (5):**
1. **Reverb (32)**
2. **ConvolutionReverb (3)**
3. **VintageReverb (50)**
4. **SpringReverb** (if exists)
5. **StereoImager (38)**

**Common Issues to Check:**
- Impulse response loading
- Size/Decay scaling
- Early/Late reflections
- CPU optimization

---

### 🔵 GROUP 8: SPECTRAL ENGINES (Priority: LOW)
**Engines (4):**
1. **SpectralFreeze (36)**
2. **SpectralGate (37)**
3. **Vocoder (52)**
4. **Synthesizer (41)**

**Common Issues to Check:**
- FFT implementation
- Freeze buffer management
- Band processing
- Parameter response

---

### ⚪ GROUP 9: UTILITY ENGINES (Priority: LOWEST)
**Engines (6):**
1. **MonoToStereo (23)**
2. **StereoToMono (39)**
3. **Panner (27)**
4. **Widener (55)**
5. **MidSideProcessor (22)**
6. **NoiseGenerator (25)**

---

## SYSTEMATIC AUDIT PROCESS

### For Each Engine Group:

#### 1. **Code Analysis Phase**
```cpp
// Check for these patterns in each engine:
- Parameter update function
- Process block implementation  
- Parameter count matches UI
- Default values initialized
- Smoothing applied correctly
```

#### 2. **Parameter Mapping Audit**
```cpp
// Verify each parameter:
- UI range (0-1) maps correctly
- DSP receives actual values
- Units are appropriate (Hz, dB, ms)
- Default produces expected result
```

#### 3. **Common Bug Patterns**
- [ ] Unused parameters (like PitchShifter grain/window)
- [ ] Wrong buffer indexing (like PitchShifter feedback)
- [ ] Missing DSP implementation
- [ ] Incorrect scaling formulas
- [ ] Phase/timing issues

#### 4. **Testing Checklist**
- [ ] Load in test harness
- [ ] Sine wave input test
- [ ] Each parameter affects output
- [ ] No clicks/pops on changes
- [ ] Bypass works correctly

---

## EXECUTION PLAN

### Week 1: Critical Fixes
**Day 1-2:** Pitch/Frequency Group
- Fix IntelligentHarmonizer 
- Fix PitchCorrection
- Fix FrequencyShifter
- Test all pitch engines

**Day 3-4:** Time/Delay Group  
- Audit all delay engines
- Fix feedback/buffer issues
- Test tempo sync

**Day 5:** Dynamics Group
- Check compressor ratios
- Fix attack/release timing
- Test all dynamics

### Week 2: Medium Priority
**Day 6-7:** Filters & Modulation
- Fix frequency scaling
- Check LFO implementations
- Test all modulation

**Day 8-9:** Distortion & Spatial
- Fix gain staging
- Check reverb tails
- Test all distortion

**Day 10:** Final Testing
- Run complete test suite
- Document remaining issues
- Prepare for release

---

## TEST AUTOMATION

### Create Test Framework:
```cpp
class EngineAuditor {
    void auditEngine(int engineId) {
        // 1. Create engine instance
        // 2. Test each parameter
        // 3. Measure audio changes
        // 4. Report issues
    }
    
    void auditGroup(EngineGroup group) {
        for (auto engine : group) {
            auditEngine(engine.id);
        }
    }
};
```

### Success Metrics:
- ✅ All parameters produce audible change
- ✅ No crashes or hangs
- ✅ Smooth parameter automation
- ✅ Correct value display (Hz, dB, etc)
- ✅ CPU usage acceptable

---

## PRIORITY ORDER

1. **FIX FIRST:** Pitch engines (user's main complaint)
2. **FIX SECOND:** Popular effects (delays, compressors)
3. **FIX THIRD:** Creative effects (modulation, distortion)
4. **FIX LAST:** Utility processors

---

## TRACKING PROGRESS

### Per Engine Status:
- 🔴 Broken (parameters don't work)
- 🟡 Partial (some parameters work)
- 🟢 Working (all parameters functional)
- ✅ Verified (tested in Logic Pro)

### Current Status:
- PitchShifter: 🟢 Working (needs Logic test)
- All others: 🔴 Unknown (need audit)

---

## NEXT IMMEDIATE STEPS

1. Create automated test harness for engine groups
2. Start with IntelligentHarmonizer (likely same bug as PitchShifter)
3. Test each pitch engine systematically
4. Document all issues found
5. Apply fixes in batches
6. Retest after each batch

This systematic approach ensures all 57 engines work correctly without missing any issues.