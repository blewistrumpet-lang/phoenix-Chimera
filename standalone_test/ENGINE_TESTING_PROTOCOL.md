# ENGINE TESTING PROTOCOL
## Individual Engine Validation - One at a Time

**Created**: 2025-10-11 22:30
**Purpose**: Systematic, exhaustive testing of each engine individually
**Method**: Real plugin code, real measurements, no fabrication

---

## GOLDEN RULES

1. **ONE ENGINE AT A TIME** - No shortcuts, no batch processing claims
2. **REAL CODE ONLY** - Must link against actual `../JUCE_Plugin/Builds/MacOSX/build/Release/libChimeraPhoenix.a`
3. **VERIFY EVERYTHING** - If a test claims a result, the output must be in a file
4. **NO HALLUCINATION** - Only report what actually executed and produced output
5. **EVIDENCE REQUIRED** - Every claim needs corresponding log file or CSV data

---

## TEST TEMPLATE FOR EACH ENGINE

For each engine (0-56), create a standalone test program that:

### 1. **Basic Functionality** (MUST PASS)
```cpp
- ✅ Engine creation via EngineFactory::createEngine(id)
- ✅ Initialization with prepareToPlay(48000, 512)
- ✅ Process 1000 blocks without crash
- ✅ No NaN in output
- ✅ No Inf in output
- ✅ Produces non-silent output (RMS > 0.0001)
- ✅ No extreme values (abs < 10.0)
```

### 2. **Parameter Testing** (REQUIRED)
```cpp
For EACH parameter (get from engine's getParameterCount()):
- ✅ Sweep from 0.0 to 1.0 in 0.1 increments
- ✅ Engine remains stable at all values
- ✅ Output changes (not stuck)
- ✅ No discontinuities/clicks when changing
- ✅ Extreme values (0.0, 1.0) don't crash
```

### 3. **Audio Quality** (MEASURED)
```cpp
Test signals:
- Sine wave 440 Hz at -12 dB
- White noise at -20 dB
- Impulse (single sample)
- Complex tone (multiple harmonics)

Measurements:
- THD (Total Harmonic Distortion)
- Frequency response (20Hz - 20kHz)
- Phase response
- Latency
- DC offset
- Signal-to-noise ratio
```

### 4. **Performance** (MEASURED)
```cpp
- CPU usage (% of available)
- Memory usage (bytes allocated)
- Real-time safety (no allocations in process())
- Buffer size independence (64, 128, 256, 512, 1024, 2048)
```

### 5. **Stress Testing** (VALIDATION)
```cpp
- 10,000 blocks continuous processing
- Rapid parameter changes (every block)
- Extreme input levels (+20 dB, -60 dB)
- Silence input (should not explode)
- DC input (should handle gracefully)
```

---

## TEST PROGRAM STRUCTURE

### Required Includes
```cpp
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <memory>

// REAL PLUGIN CODE - NO MOCKS
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
```

### Build Command Template
```bash
clang++ -std=c++17 -O2 \\
    -I. \\
    -I../JUCE_Plugin/Source \\
    -I../JUCE_Plugin/JuceLibraryCode \\
    -I/Users/Branden/JUCE/modules \\
    -DJUCE_STANDALONE_APPLICATION=1 \\
    -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \\
    test_engine_[ID]_[NAME].cpp \\
    ../JUCE_Plugin/Builds/MacOSX/build/Release/libChimeraPhoenix.a \\
    -framework CoreAudio -framework CoreFoundation \\
    -framework Accelerate -framework AudioToolbox \\
    -framework CoreMIDI -framework Cocoa -framework IOKit \\
    -o test_engine_[ID]_[NAME]
```

### Output Files Required
```
test_engine_[ID]_output.txt      - Full test log
test_engine_[ID]_results.csv     - Measured data
test_engine_[ID]_parameters.csv  - Parameter sweep results
test_engine_[ID]_audio_*.wav     - Test audio files (optional)
```

---

## ENGINE LIST (57 Engines: 0-56)

### Dynamics & Compression (7 engines)
- [ ] Engine 0: None (bypass)
- [ ] Engine 1: VintageOptoCompressor
- [ ] Engine 2: ClassicCompressor
- [ ] Engine 3: TransientShaper
- [ ] Engine 4: NoiseGate
- [ ] Engine 5: MasteringLimiter
- [ ] Engine 6: DynamicEQ

### Filters & EQ (8 engines)
- [ ] Engine 7: ParametricEQ
- [ ] Engine 8: VintageConsoleEQ
- [ ] Engine 9: LadderFilter
- [ ] Engine 10: StateVariableFilter
- [ ] Engine 11: FormantFilter
- [ ] Engine 12: EnvelopeFilter
- [ ] Engine 13: CombResonator
- [ ] Engine 14: VocalFormantFilter

### Distortion (8 engines)
- [ ] Engine 15: VintageTubePreamp
- [ ] Engine 16: WaveFolder
- [ ] Engine 17: HarmonicExciter
- [ ] Engine 18: BitCrusher
- [ ] Engine 19: MultibandSaturator
- [ ] Engine 20: MuffFuzz
- [ ] Engine 21: RodentDistortion
- [ ] Engine 22: KStyleOverdrive

### Modulation & Pitch (11 engines)
- [ ] Engine 23: StereoChorus
- [ ] Engine 24: ResonantChorus
- [ ] Engine 25: AnalogPhaser
- [ ] Engine 26: RingModulator
- [ ] Engine 27: FrequencyShifter
- [ ] Engine 28: HarmonicTremolo
- [ ] Engine 29: ClassicTremolo
- [ ] Engine 30: RotarySpeaker
- [ ] Engine 31: PitchShifter
- [ ] Engine 32: DetuneDoubler
- [ ] Engine 33: IntelligentHarmonizer

### Delay & Time (5 engines)
- [ ] Engine 34: TapeEcho
- [ ] Engine 35: DigitalDelay
- [ ] Engine 36: MagneticDrumEcho
- [ ] Engine 37: BucketBrigadeDelay
- [ ] Engine 38: BufferRepeat

### Reverb (5 engines)
- [ ] Engine 39: PlateReverb
- [ ] Engine 40: SpringReverb
- [ ] Engine 41: ConvolutionReverb
- [ ] Engine 42: ShimmerReverb
- [ ] Engine 43: GatedReverb

### Spatial (3 engines)
- [ ] Engine 44: StereoWidener
- [ ] Engine 45: StereoImager
- [ ] Engine 46: DimensionExpander

### Spectral & Special (6 engines)
- [ ] Engine 47: SpectralFreeze
- [ ] Engine 48: SpectralGate
- [ ] Engine 49: PhasedVocoder
- [ ] Engine 50: GranularCloud
- [ ] Engine 51: ChaosGenerator
- [ ] Engine 52: FeedbackNetwork

### Utility (4 engines)
- [ ] Engine 53: MidSideProcessor
- [ ] Engine 54: GainUtility
- [ ] Engine 55: MonoMaker
- [ ] Engine 56: PhaseAlign

---

## TESTING WORKFLOW

### For Each Engine:

1. **Create test program**: `test_engine_[ID]_[NAME].cpp`
2. **Compile** with real plugin library
3. **Run test** and save output to log file
4. **Verify** log file contains all test sections
5. **Extract** measurements to CSV
6. **Grade** results: PASS/FAIL
7. **Document** any issues found
8. **Move to next engine**

### NEVER:
- ❌ Claim results without corresponding log file
- ❌ Use mock/stub implementations
- ❌ Skip parameter testing
- ❌ Batch process multiple engines and claim individual results
- ❌ Report measurements that weren't actually taken

---

## GRADING CRITERIA

### PASS (Production Ready)
- ✅ All basic functionality tests pass
- ✅ All parameters stable
- ✅ THD < 1% (except intentional distortion)
- ✅ No crashes, NaN, or Inf
- ✅ CPU < 5% per engine instance
- ✅ Passes 10,000 block stress test

### MARGINAL (Works but Issues)
- ⚠️ Passes basic tests
- ⚠️ Minor parameter issues (cosmetic)
- ⚠️ THD 1-3% (higher than ideal)
- ⚠️ CPU 5-10% (acceptable but high)
- ⚠️ Some edge case instability

### FAIL (Broken)
- ❌ Crashes during testing
- ❌ Produces NaN or Inf
- ❌ Zero output (silent)
- ❌ Parameters cause instability
- ❌ THD > 10% (unintentional)
- ❌ Fails stress test

---

## CURRENT STATUS

**Engines Tested**: 0/57 (0%)
**Engines Passing**: TBD
**Engines Failing**: TBD
**Last Updated**: 2025-10-11 22:30

---

## NEXT STEPS

1. Start with Engine 0 (None/Bypass) - simplest case
2. Create test_engine_0_none.cpp
3. Compile and run
4. Verify output log
5. Mark as complete
6. Move to Engine 1

**DO NOT:**
- Skip ahead
- Test multiple engines at once
- Claim results without evidence
- Create reports without actual test runs

---

**This protocol replaces all previous testing claims.**
**Start from zero. Test systematically. Report honestly.**
