# CRITICAL PARAMETER INTERACTION ISSUES

**Quick Reference Guide for Developers**

---

## 🔴 CRITICAL - Immediate Fix Required

### Engine 56: Phase Align - COMPLETELY UNSTABLE
**Status:** All 21 parameter combinations failed (0% pass rate)

**Symptoms:**
- Signal grows beyond control in all configurations
- Peak levels exceed 10.0 within 50 processing blocks
- No stable parameter combination exists

**Root Cause:** Likely missing gain compensation or unbounded feedback

**Fix Priority:** CRITICAL - Engine is unusable

**Suggested Fix:**
```cpp
// Add output normalization
float processedSample = applyPhaseRotation(input);
processedSample *= gainCompensation; // Add this
return processedSample;
```

**Test Command:**
```bash
./build/test_parameter_interactions 2>&1 | grep -A 20 "Engine 56"
```

---

### Engine 49: Phased Vocoder - COMPLETE FAILURE
**Status:** All 70 parameter combinations failed (0% pass rate)

**Symptoms:**
- Test framework reports consistent failure
- Likely NaN/Inf or silence

**Root Cause:** Possibly uninitialized FFT buffers or state

**Fix Priority:** CRITICAL - Engine is completely broken

**Suggested Investigation:**
1. Check FFT initialization in prepareToPlay()
2. Verify hop size calculations
3. Check window function application
4. Verify synthesis normalization

**Test Command:**
```bash
./build/test_parameter_interactions 2>&1 | grep -A 20 "Engine 49"
```

---

## 🟡 HIGH PRIORITY - Stability Issues

### Engine 3: Transient Shaper - Sustain Parameter Runaway
**Status:** 8/70 tests failed (88.6% pass rate)

**Symptoms:**
- Signal grows beyond control when P1 (Sustain) = 1.0
- Affects any parameter combination where P1 is maxed

**Failing Combinations:**
- P0_P1_Both_Max (Attack=1.0, Sustain=1.0)
- P0_P1_P1_Min_P2_Max (Attack=0.0, Sustain=1.0)
- P1_P2_Both_Max (Sustain=1.0, P2=1.0)
- P1_P3_Both_Max (Sustain=1.0, P3=1.0)
- P1_P4_Both_Max (Sustain=1.0, P4=1.0)
- And all P1_Px where P1=1.0

**Root Cause:** Sustain gain multiplier has no upper bound

**Fix Priority:** HIGH - Common use case affected

**Suggested Fix:**
```cpp
// In TransientShaper::process()
// Current (problematic):
float sustainGain = 1.0f + sustainParam * 10.0f; // Can reach 11.0!

// Proposed fix:
float sustainGain = 1.0f + sustainParam * 2.0f; // Max 3.0x gain
// OR use soft knee:
float sustainGain = 1.0f + std::tanh(sustainParam * 2.0f);
```

**Temporary Workaround:**
- Limit sustain parameter to 0.8 in preset design
- Document as known limitation

---

### Engine 6: Dynamic EQ - Frequency/Gain Instability
**Status:** 13/105 tests failed (87.6% pass rate)

**Symptoms:**
- Signal grows beyond control with specific frequency + gain combinations
- Particularly P0 (Frequency) at extremes + P2 (Gain) high

**Failing Combinations:**
- P0_P2_Both_Min (Low freq + low gain causes resonance)
- P0_P3_Both_High, P0_P4_Both_High, P0_P5_Both_High
- Various P2_Px combinations (Gain parameter)

**Root Cause:** EQ bands can resonate at certain frequencies without limiting

**Fix Priority:** HIGH - Core functionality affected

**Suggested Fix:**
```cpp
// Add per-band gain limiting
for (int band = 0; band < numBands; ++band) {
    float bandGain = calculateBandGain(band);
    bandGain = std::clamp(bandGain, -18.0f, 18.0f); // Limit to ±18dB
    applyBandGain(band, bandGain);
}
```

---

### Engine 12: Envelope Filter - Modulation Depth Issues
**Status:** 11/70 tests failed (84.3% pass rate)

**Symptoms:**
- High envelope modulation depth + high resonance causes instability
- Filter can be pushed into self-oscillation by envelope

**Root Cause:** Envelope modulation doesn't account for filter stability

**Fix Priority:** HIGH - Musical use case

**Suggested Fix:**
```cpp
// Dynamic resonance limiting based on modulation depth
float effectiveQ = baseQ * (1.0f - modulationDepth * 0.3f);
filter.setResonance(effectiveQ);
```

---

## 🟢 MEDIUM PRIORITY - Minor Issues

### Engine 7: Parametric EQ - High Q at Extremes
**Status:** 4/105 tests failed (96.2% pass rate)

**Symptoms:**
- Very high Q at extreme frequencies causes issues
- Only 4 edge cases

**Fix Priority:** MEDIUM - Rarely encountered

**Suggested Fix:**
- Frequency-dependent Q limiting
- Already mostly stable

---

### Engine 22: K-Style Overdrive - Drive/Tone Stacking
**Status:** 4/70 tests failed (94.3% pass rate)

**Symptoms:**
- Max drive + certain tone settings cause slight instability

**Fix Priority:** MEDIUM - Extreme settings

**Suggested Fix:**
- Review tone filter response at max drive
- Consider drive-dependent tone range

---

### Engine 53: Mid-Side Processor - One Edge Case
**Status:** 1/42 tests failed (97.6% pass rate)

**Symptoms:**
- Single parameter combination unstable

**Fix Priority:** LOW - Isolated case

**Investigation Needed:**
- Identify which specific combination fails
- Review M/S matrix math at extremes

---

## Parameter Combination Patterns

### Dangerous Parameter Patterns Identified

1. **Multiplicative Gain Parameters**
   ```
   Pattern: Param1 × Param2 = Unbounded gain
   Example: Transient Shaper Attack × Sustain
   Fix: Use addition or soft limiting
   ```

2. **Resonant Frequency + Modulation**
   ```
   Pattern: High Q + Deep modulation = Instability
   Example: Envelope Filter
   Fix: Dynamic Q limiting
   ```

3. **Feedback + High Frequency**
   ```
   Pattern: Feedback × Frequency = Oscillation
   Example: Some modulation effects
   Fix: Frequency-dependent feedback limiting
   ```

4. **Cascaded EQ Bands**
   ```
   Pattern: Multiple bands boosting same frequency
   Example: Dynamic EQ
   Fix: Global gain limiting after EQ
   ```

---

## Quick Test Commands

### Test Specific Engine
```bash
./build/test_parameter_interactions 2>&1 | grep -A 30 "Engine 3"
```

### Find All Failures
```bash
./build/test_parameter_interactions 2>&1 | grep "Danger Zones" -A 50
```

### Get Summary Stats
```bash
./build/test_parameter_interactions 2>&1 | grep -E "(PASS|FAIL|Unstable)"
```

### Re-run Full Test
```bash
./build/test_parameter_interactions
cat PARAMETER_INTERACTION_TESTING_REPORT.md
```

---

## Testing Checklist for Fixes

After implementing fixes, verify:

- [ ] Engine no longer appears in danger zones
- [ ] Pass rate improves to >95%
- [ ] Sweet spots remain unchanged
- [ ] No new unstable combinations introduced
- [ ] Edge cases (0.0 and 1.0) handled
- [ ] Mid-range values (0.5) still work
- [ ] Output levels reasonable (peak < 2.0)

---

## Parameter Design Best Practices

Based on this testing, follow these guidelines:

### ✅ DO

1. **Use Soft Limiting for Gains**
   ```cpp
   float gain = std::tanh(parameter * scaleFactor);
   ```

2. **Clamp Resonance/Q Values**
   ```cpp
   float q = std::clamp(qParam * 10.0f, 0.5f, 20.0f);
   ```

3. **Add Safety Margins**
   ```cpp
   float maxGain = 2.0f; // Not 10.0f
   ```

4. **Test Extreme Combinations**
   - Both parameters at 0.0
   - Both parameters at 1.0
   - One at 0.0, other at 1.0

5. **Monitor Output Levels**
   ```cpp
   assert(std::abs(output) < 10.0f);
   ```

### ❌ DON'T

1. **Multiply User Parameters Directly**
   ```cpp
   float gain = param1 * param2 * 100.0f; // DANGEROUS!
   ```

2. **Allow Unbounded Feedback**
   ```cpp
   output = input + feedback * 0.999f; // Can grow!
   ```

3. **Assume Parameters Are Independent**
   - Always test interactions
   - Consider cross-parameter limiting

4. **Ignore Edge Cases**
   - Test at 0.0, 0.5, 1.0
   - Test opposing extremes

5. **Skip Denormal Protection**
   ```cpp
   state += input + 1e-30f; // Always flush denormals
   state -= 1e-30f;
   ```

---

## Additional Resources

- Full Report: `PARAMETER_INTERACTION_TESTING_REPORT.md`
- Summary: `PARAMETER_INTERACTION_SUMMARY.md`
- Test Source: `test_parameter_interactions.cpp`

**Questions?** Review the full testing report for detailed per-engine analysis.

---

*Last Updated: October 11, 2025*
*Testing Framework Version: 1.0*
