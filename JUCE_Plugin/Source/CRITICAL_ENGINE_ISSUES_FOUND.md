# Critical Engine Issues Found - Comprehensive Audit

## 1. Mix Parameter Index Mismatches (FIXED)
**Severity: CRITICAL**
**Status: RESOLVED**

We found and fixed the getMixParameterIndex() issue where many engines had incorrect Mix parameter indices, causing them to bypass all processing.

## 2. Early Return on Low Mix Values
**Severity: CRITICAL** 
**Status: ACTIVE**

### Affected Engines:
- **HarmonicExciter_Platinum**: Returns early if `cache.mixAmt < 0.001f`
- **TransientShaper_Platinum**: Returns early if `cache.mixAmount < 0.001f`

### Impact:
These engines completely bypass processing when Mix is very low (< 0.1%), which combined with the wrong Mix index meant they never processed audio.

### Fix Required:
Consider removing these early returns or increasing threshold to a more reasonable value like 0.01f (1%).

## 3. Parameter Count Mismatch
**Severity: HIGH**
**Status: ACTIVE**

### Affected Engine:
- **VintageConsoleEQ_Studio**:
  - Declares: `getNumParameters() = 10`
  - Actually uses: Parameters 0-12 (indices 11 and 12)
  - Location: VintageConsoleEQ_Studio.h:35

### Fix Required:
```cpp
int getNumParameters() const override { return 13; } // Was 10
```

## 4. Hardcoded Sample Rates
**Severity: MEDIUM-HIGH**
**Status: ACTIVE**

### Critical Offenders:
1. **PlateReverb**: `m_sampleRate = 44100.0` - affects all reverb calculations
2. **MuffFuzz**: Explicitly hardcodes 48000 for tone stack
3. **GatedReverb**: Scales from 44100 reference, incorrect at other rates
4. **MultibandSaturator**: Uses 44100 for crossover calculations
5. **WaveFolder**: Multiple hardcoded sample rates (44100/48000)

### Impact:
- Incorrect filter frequencies
- Wrong delay times
- Improper modulation rates
- Audio artifacts at non-44.1kHz sample rates

## 5. Channel Count Limitations
**Severity: MEDIUM**
**Status: ACTIVE**

### Engines Limited to 2 Channels:
- MultibandSaturator: `MAX_CHANNELS = 2`
- WaveFolder: `MAX_CHANNELS = 2` (but has bounds checking)
- PitchShifter: `MAX_CHANNELS = 2`

### Impact:
Cannot process surround or multi-channel configurations properly.

## 6. Incomplete Reset Implementation
**Severity: LOW-MEDIUM**
**Status: ACTIVE**

### Affected Engine:
- **ChaosGenerator**: Empty reset() function, doesn't clear internal state

## 7. Uninitialized Variables
**Severity: LOW**
**Status: ACTIVE**

Several engines have member variables declared without initialization, though most are properly initialized in constructors. Notable patterns:
- Some float/double members without explicit initialization
- Local variables in process functions without initial values

## Summary of Required Actions

### Immediate (Critical):
1. ✅ Fix Mix parameter indices - COMPLETED
2. Remove or adjust early return thresholds in HarmonicExciter and TransientShaper
3. Fix VintageConsoleEQ_Studio parameter count

### High Priority:
1. Replace all hardcoded sample rates with runtime values
2. Update engines to use `getSampleRate()` instead of literals
3. Ensure proper sample rate handling in filter calculations

### Medium Priority:
1. Update channel-limited engines to handle arbitrary channel counts
2. Implement proper reset() in ChaosGenerator
3. Initialize all member variables explicitly

### Code Quality:
1. Add bounds checking for all parameter access
2. Add initialization checks in process() functions
3. Document parameter ranges and expectations

## Testing Recommendations

1. Test all engines at different sample rates (44.1k, 48k, 88.2k, 96k, 192k)
2. Test with mono, stereo, and multi-channel configurations
3. Verify parameter automation works correctly for all parameters
4. Test engine switching and reset behavior
5. Verify Mix parameter properly blends dry/wet at all values

## Next Steps

The most critical issue (Mix parameter indices) has been fixed. The next priority should be:
1. Fix the VintageConsoleEQ_Studio parameter count
2. Address hardcoded sample rates in critical engines
3. Remove/adjust early return thresholds that prevent processing