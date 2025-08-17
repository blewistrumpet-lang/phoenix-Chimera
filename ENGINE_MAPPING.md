# Engine Factory Mapping - OFFICIAL

**Total Engines:** 57 (IDs 0-56)

## ENGINE MAPPING BY ID

### Special (0)
- **0**: NoneEngine

### DYNAMICS & COMPRESSION (1-6)
- **1**: VintageOptoCompressor_Platinum
- **2**: ClassicCompressor
- **3**: TransientShaper_Platinum
- **4**: NoiseGate_Platinum
- **5**: MasteringLimiter_Platinum
- **6**: DynamicEQ

### FILTERS & EQ (7-14)
- **7**: ParametricEQ_Studio
- **8**: VintageConsoleEQ_Studio
- **9**: LadderFilter
- **10**: StateVariableFilter
- **11**: FormantFilter
- **12**: EnvelopeFilter
- **13**: CombResonator
- **14**: VocalFormantFilter

### DISTORTION & SATURATION (15-22)
- **15**: VintageTubePreamp_Studio
- **16**: WaveFolder
- **17**: HarmonicExciter_Platinum
- **18**: BitCrusher
- **19**: MultibandSaturator
- **20**: MuffFuzz
- **21**: RodentDistortion
- **22**: KStyleOverdrive

### MODULATION (23-33)
- **23**: StereoChorus
- **24**: ResonantChorus_Platinum
- **25**: AnalogPhaser
- **26**: PlatinumRingModulator
- **27**: FrequencyShifter
- **28**: HarmonicTremolo
- **29**: ClassicTremolo
- **30**: RotarySpeaker_Platinum
- **31**: PitchShifter
- **32**: DetuneDoubler
- **33**: IntelligentHarmonizer

### REVERB & DELAY (34-43)
- **34**: TapeEcho
- **35**: DigitalDelay
- **36**: MagneticDrumEcho
- **37**: BucketBrigadeDelay
- **38**: BufferRepeat_Platinum
- **39**: PlateReverb
- **40**: SpringReverb_Platinum
- **41**: ConvolutionReverb
- **42**: ShimmerReverb
- **43**: GatedReverb

### SPATIAL & SPECIAL (44-52)
- **44**: StereoWidener
- **45**: StereoImager
- **46**: DimensionExpander
- **47**: SpectralFreeze
- **48**: SpectralGate_Platinum
- **49**: PhasedVocoder
- **50**: GranularCloud
- **51**: ChaosGenerator_Platinum
- **52**: FeedbackNetwork

### UTILITY (53-56)
- **53**: MidSideProcessor_Platinum
- **54**: GainUtility_Platinum
- **55**: MonoMaker_Platinum
- **56**: PhaseAlign_Platinum

## CRITICAL NOTES

1. **ID Mapping is FIXED**: Engine IDs must match exactly as above
2. **Mix Parameter Index**: Use `getMixParameterIndex(engineID)` from PluginProcessor
3. **Parameter Count**: Each engine reports via `getNumParameters()`
4. **Factory Pattern**: All engines created through `EngineFactory::createEngine(id)`

## TEST MATRIX

| Category | Engine IDs | Count | Primary Tests |
|----------|-----------|-------|---------------|
| Dynamics | 1-6 | 6 | Attack/Release, GR, Static Curve |
| EQ/Filter | 7-14 | 8 | Frequency Response, Q accuracy |
| Distortion | 15-22 | 8 | THD+N, IMD, Saturation curve |
| Modulation | 23-33 | 11 | LFO rate, Depth, Phase |
| Reverb/Delay | 34-43 | 10 | RT60, Echo time, Tail decay |
| Spatial | 44-52 | 9 | Pan law, Width, Correlation |
| Utility | 53-56 | 4 | Gain, Phase, M/S processing |

## ENGINE FAMILIES

### Platinum Series (Premium)
- VintageOptoCompressor_Platinum (1)
- TransientShaper_Platinum (3)
- NoiseGate_Platinum (4)
- MasteringLimiter_Platinum (5)
- HarmonicExciter_Platinum (17)
- ResonantChorus_Platinum (24)
- PlatinumRingModulator (26)
- RotarySpeaker_Platinum (30)
- BufferRepeat_Platinum (38)
- SpringReverb_Platinum (40)
- SpectralGate_Platinum (48)
- ChaosGenerator_Platinum (51)
- MidSideProcessor_Platinum (53)
- GainUtility_Platinum (54)
- MonoMaker_Platinum (55)
- PhaseAlign_Platinum (56)

### Studio Series (Professional)
- ParametricEQ_Studio (7)
- VintageConsoleEQ_Studio (8)
- VintageTubePreamp_Studio (15)

### Standard Series
- All others use standard implementations

## PARAMETER MAPPING

Each engine has its own parameter mapping. Common patterns:

1. **Mix/Dry-Wet**: Usually last parameter, verify with `getMixParameterIndex()`
2. **Input/Output Gain**: Often first and last parameters
3. **Core Parameters**: Middle parameters specific to effect type

## TESTING REQUIREMENTS

1. **All engines must**:
   - Return valid instance from factory
   - Process audio without crashes
   - Implement bypass (mix = 0)
   - Reset cleanly
   - Report correct parameter count

2. **Category-specific requirements**:
   - See ENGINE_ACCEPTANCE_CHECKLIST.md for details

## Known Issues to Fix

1. **GritCrusher**: Not in factory - test using wrong name
2. **MetalZone**: Not in factory - test using wrong name
3. **Test suite**: Using incorrect engine list - must use THIS mapping

## Verification Commands

```bash
# Test specific engine by ID
./test_all_engines --engine 20  # Tests MuffFuzz

# Test category
./test_all_engines --category distortion  # Tests IDs 15-22

# Test all
./test_all_engines --all  # Tests IDs 0-56
```