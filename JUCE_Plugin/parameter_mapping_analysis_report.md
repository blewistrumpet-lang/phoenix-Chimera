# Chimera Phoenix Parameter Mapping Analysis Report

**Total Engines Analyzed:** 82
**Engines with Issues:** 32
**Total Issues Found:** 82
**Success Rate:** 61.0%

## Issues by Engine

### ❌ VintageConsoleEQ (Confidence: 0.30)

- Parameters with names but no updates: [1, 3, 5, 7, 9, 10]
- Potential name/variable mismatch at index 0: 'Low Gain' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'Mid Gain' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Mid Q' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'High Freq' -> 'params.at(7)'
- Potential name/variable mismatch at index 8: 'Console Type' -> 'params.at(9)'

**Parameter Mappings:**
- Index 0: "Low Gain" → params.at(1)
- Index 1: "Low Freq"
- Index 2: "Mid Gain" → params.at(3)
- Index 3: "Mid Freq"
- Index 4: "Mid Q" → params.at(5)
- Index 5: "High Gain"
- Index 6: "High Freq" → params.at(7)
- Index 7: "Drive"
- Index 8: "Console Type" → params.at(9)
- Index 9: "Vintage"
- Index 10: "Mix"

### ❌ FrequencyShifter (Confidence: 0.35)

- Parameters with names but no updates: [1, 3, 5, 7]
- Mix parameter at index 2, expected near end (total: 8)
- Potential name/variable mismatch at index 0: 'Shift' -> 'params.at(1) * 0.95f'
- Potential name/variable mismatch at index 2: 'Mix' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Resonance' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Mod Rate' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Shift" → params.at(1) * 0.95f
- Index 1: "Feedback"
- Index 2: "Mix" → params.at(3)
- Index 3: "Spread"
- Index 4: "Resonance" → params.at(5)
- Index 5: "Mod Depth"
- Index 6: "Mod Rate" → params.at(7)
- Index 7: "Direction"

### ❌ VintageOptoCompressor (Confidence: 0.35)

- Parameters with names but no updates: [1, 3, 5, 7]
- Mix parameter at index 4, expected near end (total: 8)
- Potential name/variable mismatch at index 0: 'Gain' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'HF Emphasis' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Mix' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Harmonics' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Gain" → params.at(1)
- Index 1: "Peak Reduction"
- Index 2: "HF Emphasis" → params.at(3)
- Index 3: "Output"
- Index 4: "Mix" → params.at(5)
- Index 5: "Knee"
- Index 6: "Harmonics" → params.at(7)
- Index 7: "Stereo Link"

### ❌ StereoImager (Confidence: 0.40)

- Parameters with names but no updates: [1, 3, 5, 7]
- Potential name/variable mismatch at index 0: 'Width' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'Mid Width' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Low X-over' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Phase' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Width" → params.at(1)
- Index 1: "Low Width"
- Index 2: "Mid Width" → params.at(3)
- Index 3: "High Width"
- Index 4: "Low X-over" → params.at(5)
- Index 5: "High X-over"
- Index 6: "Phase" → params.at(7)
- Index 7: "Mix"

### ❌ BufferRepeat (Confidence: 0.40)

- Parameters with names but no updates: [1, 3, 5, 7]
- Potential name/variable mismatch at index 0: 'Division' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'Feedback' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Pitch' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Stutter' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Division" → params.at(1)
- Index 1: "Probability"
- Index 2: "Feedback" → params.at(3)
- Index 3: "Filter"
- Index 4: "Pitch" → params.at(5)
- Index 5: "Reverse"
- Index 6: "Stutter" → params.at(7)
- Index 7: "Mix"

### ❌ DynamicEQ (Confidence: 0.40)

- Parameters with names but no updates: [1, 3, 5, 7]
- Potential name/variable mismatch at index 0: 'Frequency' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'Ratio' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Release' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Mix' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Frequency" → params.at(1)
- Index 1: "Threshold"
- Index 2: "Ratio" → params.at(3)
- Index 3: "Attack"
- Index 4: "Release" → params.at(5)
- Index 5: "Gain"
- Index 6: "Mix" → params.at(7)
- Index 7: "Mode"

### ❌ NoiseGate (Confidence: 0.40)

- Parameters with names but no updates: [1, 3, 5, 7]
- Potential name/variable mismatch at index 0: 'Threshold' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'Attack' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Release' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'SC Filter' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Threshold" → params.at(1)
- Index 1: "Range"
- Index 2: "Attack" → params.at(3)
- Index 3: "Hold"
- Index 4: "Release" → params.at(5)
- Index 5: "Hysteresis"
- Index 6: "SC Filter" → params.at(7)
- Index 7: "Lookahead"

### ❌ ChaosGenerator (Confidence: 0.40)

- Parameters with names but no updates: [1, 3, 5, 7]
- Potential name/variable mismatch at index 0: 'Rate' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'Type' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Target' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Seed' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Rate" → params.at(1)
- Index 1: "Depth"
- Index 2: "Type" → params.at(3)
- Index 3: "Smoothing"
- Index 4: "Target" → params.at(5)
- Index 5: "Sync"
- Index 6: "Seed" → params.at(7)
- Index 7: "Mix"

### ❌ HarmonicExciter (Confidence: 0.40)

- Parameters with names but no updates: [1, 3, 5, 7]
- Potential name/variable mismatch at index 0: 'Frequency' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'Harmonics' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Warmth' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Color' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Frequency" → params.at(1)
- Index 1: "Drive"
- Index 2: "Harmonics" → params.at(3)
- Index 3: "Clarity"
- Index 4: "Warmth" → params.at(5)
- Index 5: "Presence"
- Index 6: "Color" → params.at(7)
- Index 7: "Mix"

### ❌ StereoWidener (Confidence: 0.40)

- Parameters with names but no updates: [1, 3, 5, 7]
- Potential name/variable mismatch at index 0: 'Width' -> 'params.at(1)'
- Potential name/variable mismatch at index 2: 'HF Shelf Freq' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'Haas Delay' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Correlation' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Width" → params.at(1)
- Index 1: "Bass Mono"
- Index 2: "HF Shelf Freq" → params.at(3)
- Index 3: "HF Shelf Gain"
- Index 4: "Haas Delay" → params.at(5)
- Index 5: "Delay Gain"
- Index 6: "Correlation" → params.at(7)
- Index 7: "Mix"

### ❌ BitCrusher (Confidence: 0.40)

- Parameters with names but no updates: [1, 3, 5, 7]
- Potential name/variable mismatch at index 0: 'Bits' -> '1.0f + params.at(1) * 99.0f'
- Potential name/variable mismatch at index 2: 'Aliasing' -> 'params.at(3)'
- Potential name/variable mismatch at index 4: 'DC Offset' -> 'params.at(5)'
- Potential name/variable mismatch at index 6: 'Dither' -> 'params.at(7)'

**Parameter Mappings:**
- Index 0: "Bits" → 1.0f + params.at(1) * 99.0f
- Index 1: "Downsample"
- Index 2: "Aliasing" → params.at(3)
- Index 3: "Jitter"
- Index 4: "DC Offset" → params.at(5)
- Index 5: "Gate"
- Index 6: "Dither" → params.at(7)
- Index 7: "Mix"

### ❌ ParametricEQ_Studio (Confidence: 0.75)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5]
- Mix parameter at index 2, expected near end (total: 6)

**Parameter Mappings:**
- Index 0: "Bypass"
- Index 1: "Output Trim"
- Index 2: "Mix"
- Index 3: "Vintage"
- Index 4: "M/S Mode"
- Index 5: "Analyzer"

### ❌ KStyleOverdrive (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3]

**Parameter Mappings:**
- Index 0: "Drive"
- Index 1: "Tone"
- Index 2: "Level"
- Index 3: "Mix"

### ❌ ClassicTremolo (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7]

**Parameter Mappings:**
- Index 0: "Rate"
- Index 1: "Depth"
- Index 2: "Shape"
- Index 3: "Stereo"
- Index 4: "Type"
- Index 5: "Symmetry"
- Index 6: "Volume"
- Index 7: "Mix"

### ❌ BucketBrigadeDelay (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6]

**Parameter Mappings:**
- Index 0: "Delay Time"
- Index 1: "Feedback"
- Index 2: "Modulation"
- Index 3: "Tone"
- Index 4: "Age"
- Index 5: "Mix"
- Index 6: "Sync"

### ❌ ParametricEQ (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7, 8]

**Parameter Mappings:**
- Index 0: "Low Gain"
- Index 1: "Low Freq"
- Index 2: "Mid Gain"
- Index 3: "Mid Freq"
- Index 4: "Mid Q"
- Index 5: "High Gain"
- Index 6: "High Freq"
- Index 7: "Output"
- Index 8: "Mix"

### ❌ SpringReverb_Platinum (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7]

**Parameter Mappings:**
- Index 0: "Tension"
- Index 1: "Damping"
- Index 2: "Decay"
- Index 3: "Modulation"
- Index 4: "Chirp"
- Index 5: "Drive"
- Index 6: "Width"
- Index 7: "Mix"

### ❌ DetuneDoubler (Confidence: 0.80)

- Potential name/variable mismatch at index 0: 'Detune Amount' -> 'detuneParam'
- Potential name/variable mismatch at index 2: 'Stereo Width' -> 'widthParam'

**Parameter Mappings:**
- Index 0: "Detune Amount" → detuneParam
- Index 1: "Delay Time" → delayParam
- Index 2: "Stereo Width" → widthParam
- Index 3: "Thickness" → thicknessParam
- Index 4: "Mix" → mixParam

### ❌ MagneticDrumEcho (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7, 8]

**Parameter Mappings:**
- Index 0: "Drum Speed"
- Index 1: "Head 1"
- Index 2: "Head 2"
- Index 3: "Head 3"
- Index 4: "Feedback"
- Index 5: "Saturation"
- Index 6: "Wow/Flutter"
- Index 7: "Mix"
- Index 8: "Sync"

### ❌ BufferRepeat_Platinum (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7]

**Parameter Mappings:**
- Index 0: "Division"
- Index 1: "Probability"
- Index 2: "Feedback"
- Index 3: "Filter"
- Index 4: "Pitch"
- Index 5: "Reverse"
- Index 6: "Stutter"
- Index 7: "Mix"

### ❌ TapeEcho (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5]

**Parameter Mappings:**
- Index 0: "Time"
- Index 1: "Feedback"
- Index 2: "Wow & Flutter"
- Index 3: "Saturation"
- Index 4: "Mix"
- Index 5: "Sync"

### ❌ IntelligentHarmonizer (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7]

**Parameter Mappings:**
- Index 0: "Interval"
- Index 1: "Key"
- Index 2: "Scale"
- Index 3: "Voices"
- Index 4: "Spread"
- Index 5: "Humanize"
- Index 6: "Formant"
- Index 7: "Mix"

### ❌ PlatinumRingModulator (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11]

**Parameter Mappings:**
- Index 0: "Carrier Frequency"
- Index 1: "Ring Amount"
- Index 2: "Frequency Shift"
- Index 3: "Feedback"
- Index 4: "Pulse Width"
- Index 5: "Phase Modulation"
- Index 6: "Harmonic Stretch"
- Index 7: "Spectral Tilt"
- Index 8: "Resonance"
- Index 9: "Shimmer"
- Index 10: "Thermal Drift"
- Index 11: "Pitch Tracking"

### ❌ AnalogRingModulator (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3]

**Parameter Mappings:**
- Index 0: "Carrier Freq"
- Index 1: "Ring/Shift"
- Index 2: "Drift"
- Index 3: "Tracking"

### ❌ CombResonator (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7]

**Parameter Mappings:**
- Index 0: "Root Freq"
- Index 1: "Resonance"
- Index 2: "Harmonic Spread"
- Index 3: "Decay Time"
- Index 4: "Damping"
- Index 5: "Mod Depth"
- Index 6: "Stereo Width"
- Index 7: "Mix"

### ❌ DigitalDelay (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4]

**Parameter Mappings:**
- Index 0: "Time"
- Index 1: "Feedback"
- Index 2: "Mix"
- Index 3: "High Cut"
- Index 4: "Sync"

### ❌ SpectralFreeze (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7]

**Parameter Mappings:**
- Index 0: "Freeze"
- Index 1: "Smear"
- Index 2: "Shift"
- Index 3: "Resonance"
- Index 4: "Decay"
- Index 5: "Brightness"
- Index 6: "Density"
- Index 7: "Shimmer"

### ❌ RodentDistortion (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5, 6, 7]

**Parameter Mappings:**
- Index 0: "Gain"
- Index 1: "Filter"
- Index 2: "Clipping"
- Index 3: "Tone"
- Index 4: "Output"
- Index 5: "Mix"
- Index 6: "Mode"
- Index 7: "Presence"

### ❌ StereoChorus (Confidence: 0.80)

- Parameters with names but no updates: [0, 1, 2, 3, 4, 5]

**Parameter Mappings:**
- Index 0: "Rate"
- Index 1: "Depth"
- Index 2: "Feedback"
- Index 3: "Delay"
- Index 4: "Width"
- Index 5: "Mix"

### ❌ ConvolutionReverb (Confidence: 0.85)

- Mix parameter at index 0, expected near end (total: 8)
- Potential name/variable mismatch at index 6: 'Early/Late' -> 'params.end()) {
        float newEarlyLate = params.at(6)'

**Parameter Mappings:**
- Index 0: "Mix" → mixAmount
- Index 1: "PreDelay" → preDelay
- Index 2: "Damping" → params.end()) {
        float newDamping = params.at(2)
- Index 3: "Size" → params.end()) {
        float newSize = params.at(3)
- Index 4: "Width" → width
- Index 5: "Modulation" → modulation
- Index 6: "Early/Late" → params.end()) {
        float newEarlyLate = params.at(6)
- Index 7: "HighCut" → highCut

### ❌ SpringReverb (Confidence: 0.90)

- Potential name/variable mismatch at index 3: 'Pre-Delay' -> 'preDelay'

**Parameter Mappings:**
- Index 0: "Spring Count" → springCount
- Index 1: "Tension" → tension
- Index 2: "Damping" → damping
- Index 3: "Pre-Delay" → preDelay
- Index 4: "Modulation" → modulation
- Index 5: "Drip" → drip
- Index 6: "Tone" → tone
- Index 7: "Mix" → mix

### ❌ ClassicCompressor (Confidence: 0.95)

- Mix parameter at index 6, expected near end (total: 10)

**Parameter Mappings:**
- Index 0: "Threshold" → threshold
- Index 1: "Ratio" → ratio
- Index 2: "Attack" → attack
- Index 3: "Release" → release
- Index 4: "Knee" → knee
- Index 5: "Makeup" → makeupGain
- Index 6: "Mix" → mix
- Index 7: "Lookahead" → lookahead
- Index 8: "Auto Release" → autoRelease
- Index 9: "Sidechain" → sidechain

### ✅ NoiseGate_Platinum - No issues detected

### ✅ EnvelopeFilter - No issues detected

### ✅ StateVariableFilter - No issues detected

### ✅ ChaosGenerator_Platinum - No issues detected

### ✅ PresetSerializer - No issues detected

### ✅ VintageOptoCompressor_Platinum - No issues detected

### ✅ VintageTubePreamp_Studio - No issues detected

### ✅ GenerateDetailedCorpus - No issues detected

### ✅ ResonantChorus - No issues detected

### ✅ MuffFuzz - No issues detected

### ✅ GatedReverb - No issues detected

### ✅ TransientShaper_Platinum - No issues detected

### ✅ RotarySpeaker_Platinum - No issues detected

### ✅ VintageConsoleEQ_Studio - No issues detected

### ✅ ParametricEQ_Platinum - No issues detected

### ✅ GenerateGoldenCorpus - No issues detected

### ✅ LadderFilter - No issues detected

### ✅ MonoMaker_Platinum - No issues detected

### ✅ SpectralGate - No issues detected

### ✅ ResonantChorus_Platinum - No issues detected

### ✅ MultibandSaturator - No issues detected

### ✅ PhasedVocoder - No issues detected

### ✅ BoutiquePresetGenerator - No issues detected

### ✅ PresetVariationGenerator - No issues detected

### ✅ GainUtility_Platinum - No issues detected

### ✅ EngineMetadataInit - No issues detected

### ✅ GoldenCorpusPresets - No issues detected

### ✅ VintageTubePreamp - No issues detected

### ✅ RotarySpeaker - No issues detected

### ✅ GranularCloud - No issues detected

### ✅ DimensionExpander - No issues detected

### ✅ GoldenCorpusBuilder - No issues detected

### ✅ FormantFilter - No issues detected

### ✅ FeedbackNetwork - No issues detected

### ✅ PresetExporter - No issues detected

### ✅ SpectralFreezeTest - No issues detected

### ✅ PhaseAlign_Platinum - No issues detected

### ✅ HarmonicExciter_Platinum - No issues detected

### ✅ MasteringLimiter_Platinum - No issues detected

### ✅ PitchShifter - No issues detected

### ✅ VocalFormantFilter - No issues detected

### ✅ VintageConsoleEQ_Platinum - No issues detected

### ✅ MidSideProcessor_Platinum - No issues detected

### ✅ AnalogPhaser - No issues detected

### ✅ HarmonicTremolo - No issues detected

### ✅ fix_ui_and_audio - No issues detected

### ✅ SpectralGate_Platinum - No issues detected

### ✅ ShimmerReverb - No issues detected

### ✅ WaveFolder - No issues detected

### ✅ PlateReverb - No issues detected

## Summary of Common Issues

- **Name/Variable Mismatch:** 49 occurrences
- **Mix Position:** 5 occurrences

## Recommendations

1. **Fix Generic Parameter Names**: Replace "Param X" with descriptive names
2. **Standardize Mix Parameter Position**: Consider moving Mix to consistent position
3. **Align Parameter Names with Variables**: Ensure UI names match internal variable purposes
4. **Fill Parameter Gaps**: Add names for all parameter indices
5. **Add Parameter Documentation**: Document expected functionality for each parameter
