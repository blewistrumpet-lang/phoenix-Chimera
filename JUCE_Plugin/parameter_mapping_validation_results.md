# Parameter Mapping Validation Results

**Test Summary:**
- Engines Tested: 8
- Engines Valid: 3
- Success Rate: 37.5%
- Total Parameters Tested: 60

## Test Results by Engine

### ✅ VintageTubePreamp

**Status:** Parameter mapping is CORRECT

**Parameter Mappings:**
- Index 0: "Input Gain" → m_inputGain
- Index 1: "Drive" → m_drive
- Index 2: "Bias" → m_bias
- Index 3: "Bass" → m_bass
- Index 4: "Mid" → m_mid
- Index 5: "Treble" → m_treble
- Index 6: "Presence" → m_presence
- Index 7: "Output Gain" → m_outputGain
- Index 8: "Tube Type" → m_tubeType
- Index 9: "Mix" → m_mix

### ❌ ClassicCompressor

**Issues Found:**
- Parameter 0 ('Threshold') has no update mapping
- Parameter 1 ('Ratio') has no update mapping
- Parameter 2 ('Attack') has no update mapping
- Parameter 3 ('Release') has no update mapping
- Parameter 4 ('Knee') has no update mapping
- Parameter 5 ('Makeup') has no update mapping
- Parameter 6 ('Mix') has no update mapping
- Parameter 7 ('Lookahead') has no update mapping
- Parameter 8 ('Auto Release') has no update mapping
- Parameter 9 ('Sidechain') has no update mapping

**Parameter Mappings:**
- Index 0: "Threshold" → ❌ No update mapping
- Index 1: "Ratio" → ❌ No update mapping
- Index 2: "Attack" → ❌ No update mapping
- Index 3: "Release" → ❌ No update mapping
- Index 4: "Knee" → ❌ No update mapping
- Index 5: "Makeup" → ❌ No update mapping
- Index 6: "Mix" → ❌ No update mapping
- Index 7: "Lookahead" → ❌ No update mapping
- Index 8: "Auto Release" → ❌ No update mapping
- Index 9: "Sidechain" → ❌ No update mapping

### ❌ RodentDistortion

**Issues Found:**
- Semantic mismatch: 'Mode' → m_distortionType

**Parameter Mappings:**
- Index 0: "Gain" → m_gain
- Index 1: "Filter" → m_filter
- Index 2: "Clipping" → m_clipping
- Index 3: "Tone" → m_tone
- Index 4: "Output" → m_output
- Index 5: "Mix" → m_mix
- Index 6: "Mode" → m_distortionType
- Index 7: "Presence" → m_presence

### ❌ AnalogRingModulator

**Issues Found:**
- Parameter 0 ('Carrier Freq') has no update mapping
- Parameter 1 ('Ring/Shift') has no update mapping
- Parameter 2 ('Drift') has no update mapping
- Parameter 3 ('Tracking') has no update mapping

**Parameter Mappings:**
- Index 0: "Carrier Freq" → ❌ No update mapping
- Index 1: "Ring/Shift" → ❌ No update mapping
- Index 2: "Drift" → ❌ No update mapping
- Index 3: "Tracking" → ❌ No update mapping

### ✅ PlateReverb

**Status:** Parameter mapping is CORRECT

**Parameter Mappings:**
- Index 0: "Size" → m_size
- Index 1: "Damping" → m_damping
- Index 2: "Predelay" → m_predelay
- Index 3: "Mix" → m_mix

### ✅ StateVariableFilter

**Status:** Parameter mapping is CORRECT

**Parameter Mappings:**
- Index 0: "Frequency" → m_frequency
- Index 1: "Resonance" → m_resonance
- Index 2: "Drive" → m_drive
- Index 3: "Filter Type" → m_filterType
- Index 4: "Slope" → m_slope
- Index 5: "Envelope" → m_envelope
- Index 6: "Env Attack" → m_envAttack
- Index 7: "Env Release" → m_envRelease
- Index 8: "Analog" → m_analog
- Index 9: "Mix" → m_mix

### ❌ BitCrusher

**Issues Found:**
- Semantic mismatch: 'Bits' → m_bitDepth
- Semantic mismatch: 'Downsample' → m_sampleRateReduction

**Parameter Mappings:**
- Index 0: "Bits" → m_bitDepth
- Index 1: "Downsample" → m_sampleRateReduction
- Index 2: "Aliasing" → m_aliasing
- Index 3: "Jitter" → m_jitter
- Index 4: "DC Offset" → m_dcOffset
- Index 5: "Gate" → m_gateThreshold
- Index 6: "Dither" → m_dither
- Index 7: "Mix" → m_mix

### ❌ StereoChorus

**Issues Found:**
- Parameter 0 ('Rate') has no update mapping
- Parameter 1 ('Depth') has no update mapping
- Parameter 2 ('Feedback') has no update mapping
- Parameter 3 ('Delay') has no update mapping
- Parameter 4 ('Width') has no update mapping
- Parameter 5 ('Mix') has no update mapping

**Parameter Mappings:**
- Index 0: "Rate" → ❌ No update mapping
- Index 1: "Depth" → ❌ No update mapping
- Index 2: "Feedback" → ❌ No update mapping
- Index 3: "Delay" → ❌ No update mapping
- Index 4: "Width" → ❌ No update mapping
- Index 5: "Mix" → ❌ No update mapping

## Conclusion

⚠️ **5 engines have mapping issues that need attention**
