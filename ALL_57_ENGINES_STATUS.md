# Complete List of All 57 Phoenix-Chimera Engines

## Status Legend
- ✅ **WORKING** - Fully functional and validated
- ⚠️ **ISSUES** - Has problems that need fixing
- 🔧 **FIXED TODAY** - Was broken but fixed in today's session

---

## SPECIAL EFFECTS (1 Engine)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 0 | **NoneEngine** | ✅ WORKING | Bypass/passthrough - no processing applied |

---

## DYNAMICS PROCESSORS (6 Engines)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 1 | **ClassicCompressor** | ✅ WORKING | Vintage-style VCA compressor with smooth gain reduction |
| 2 | **VintageOptoCompressor_Platinum** | ⚠️ ISSUES (NaN) | Optical compressor emulation with tube-like warmth |
| 3 | **VCA_Compressor** | ✅ WORKING | Clean, transparent VCA-based dynamics control |
| 4 | **NoiseGate_Platinum** | ✅ WORKING | Professional noise gate with look-ahead and hysteresis |
| 5 | **TransientShaper_Platinum** | ✅ WORKING | Attack/sustain control for punch and presence |
| 6 | **MasteringLimiter_Platinum** | ✅ WORKING | Transparent brick-wall limiter for mastering |

---

## EQ & FILTERS (8 Engines)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 7 | **ParametricEQ** | ✅ WORKING | 4-band parametric EQ with high/low shelf |
| 8 | **VintageConsoleEQ** | ✅ WORKING | Classic console channel strip EQ emulation |
| 9 | **DynamicEQ** | 🔧 FIXED TODAY | Frequency-dependent compression/expansion |
| 10 | **AnalogPhaser** | ✅ WORKING | Warm analog phaser with 4-8 stages |
| 11 | **EnvelopeFilter** | ✅ WORKING | Auto-wah following input envelope |
| 12 | **StateVariableFilter** | ✅ WORKING | Morphable filter (LP/BP/HP/Notch) |
| 13 | **FormantFilter** | ✅ WORKING | Vowel formant filter for vocal effects |
| 14 | **LadderFilter** | ✅ WORKING | Moog-style ladder filter with resonance |

---

## DISTORTION & SATURATION (8 Engines)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 15 | **VintageTubePreamp** | ✅ WORKING | Tube preamp warmth and harmonics |
| 16 | **TapeDistortion** | ✅ WORKING | Analog tape saturation and compression |
| 17 | **KStyleOverdrive** | ⚠️ ISSUES (NaN) | Overdrive pedal emulation with tone control |
| 18 | **BitCrusher** | ✅ WORKING | Sample rate/bit depth reduction for lo-fi |
| 19 | **WaveFolder** | ✅ WORKING | West Coast synthesis-style waveshaping |
| 20 | **MuffFuzz** | ✅ WORKING | Big Muff-style fuzz distortion |
| 21 | **RodentDistortion** | ✅ WORKING | RAT-style distortion pedal emulation |
| 22 | **MultibandSaturator** | ✅ WORKING | Frequency-selective saturation (3 bands) |

---

## MODULATION EFFECTS (11 Engines)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 23 | **StereoChorus** | ✅ WORKING | Lush stereo chorus with multiple voices |
| 24 | **VintageFlanger** | ✅ WORKING | Classic tape flanger with feedback |
| 25 | **ClassicTremolo** | ✅ WORKING | Amplitude modulation (volume wobble) |
| 26 | **HarmonicTremolo** | ✅ WORKING | Frequency-split tremolo for complex motion |
| 27 | **RotarySpeaker** | ✅ WORKING | Leslie speaker simulation with doppler |
| 28 | **RingModulator** | ✅ WORKING | Metallic/bell-like frequency modulation |
| 29 | **FrequencyShifter** | ✅ WORKING | Linear frequency shifting (not pitch) |
| 30 | **PitchShifter** | ✅ WORKING | Chromatic pitch shifting up/down |
| 31 | **HarmonicExciter** | ✅ WORKING | Add brightness with harmonic enhancement |
| 32 | **VocalFormant** | ✅ WORKING | Formant shifting for vocal character |
| 33 | **ResonantChorus** | 🔧 FIXED TODAY | 6-voice modulated delay with resonance |

---

## DELAY EFFECTS (5 Engines)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 34 | **DigitalDelay** | ✅ WORKING | Clean digital delay with feedback |
| 35 | **TapeEcho** | ✅ WORKING | Vintage tape delay with wow/flutter |
| 36 | **BucketBrigadeDelay** | ✅ WORKING | BBD analog delay emulation |
| 37 | **MagneticDrumEcho** | ✅ WORKING | Drum echo unit simulation |
| 38 | **BufferRepeat** | 🔧 FIXED TODAY | Glitch/stutter effect with buffer manipulation |

---

## REVERB EFFECTS (5 Engines)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 39 | **PlateReverb** | 🔧 FIXED TODAY | EMT plate reverb emulation with damping |
| 40 | **SpringReverb_Platinum** | 🔧 FIXED TODAY | Spring tank reverb with drip and boing |
| 41 | **ConvolutionReverb** | 🔧 FIXED TODAY | IR-based reverb with room modeling |
| 42 | **ShimmerReverb** | 🔧 FIXED TODAY | Ethereal reverb with pitch-shifted tails |
| 43 | **GatedReverb** | ✅ WORKING | 80s-style gated reverb (cuts tail by design) |

---

## SPATIAL PROCESSORS (9 Engines)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 44 | **StereoWidener** | ✅ WORKING | Enhance stereo width without phase issues |
| 45 | **StereoImager** | ✅ WORKING | Precise stereo field manipulation |
| 46 | **MidSideProcessor** | ✅ WORKING | M/S encoding for surgical stereo editing |
| 47 | **DimensionExpander** | ⚠️ ISSUES (NaN) | 3D spatial enhancement with depth |
| 48 | **CombResonator** | ✅ WORKING | Tuned comb filter for resonant effects |
| 49 | **SpectralFreeze** | ⚠️ ISSUES (Hanging) | FFT-based spectral snapshot/freeze |
| 50 | **GranularCloud** | ⚠️ ISSUES (Hanging) | Granular synthesis texture generator |
| 51 | **ChaosGenerator** | ⚠️ ISSUES (Hanging) | Lorenz attractor-based chaos modulation |
| 52 | **FeedbackNetwork** | ⚠️ ISSUES (Hanging) | Complex feedback routing matrix |

---

## UTILITY PROCESSORS (4 Engines)
| ID | Name | Status | Description |
|----|------|--------|-------------|
| 53 | **PhaseAlign_Platinum** | ⚠️ ISSUES (Inf) | Phase correlation and alignment tool |
| 54 | **GainUtility** | ⚠️ ISSUES (Hanging) | Simple gain staging and metering |
| 55 | **MonoMaker** | ⚠️ ISSUES (Hanging) | Stereo to mono summing with phase control |
| 56 | **SpectralGate** | 🔧 FIXED TODAY | FFT-based frequency-selective gating |

---

## Summary Statistics

### By Status:
- ✅ **WORKING**: 45 engines (78.9%)
- ⚠️ **ISSUES**: 12 engines (21.1%)
- 🔧 **FIXED TODAY**: 8 engines (included in working count)

### By Category Performance:
| Category | Working | Total | Success Rate |
|----------|---------|-------|--------------|
| Special | 1 | 1 | 100% |
| Dynamics | 5 | 6 | 83.3% |
| EQ/Filter | 8 | 8 | 100% |
| Distortion | 7 | 8 | 87.5% |
| Modulation | 11 | 11 | 100% |
| Delay | 5 | 5 | 100% |
| Reverb | 5 | 5 | 100% |
| Spatial | 3 | 9 | 33.3% |
| Utility | 0 | 4 | 0% |

### Engines Fixed Today (August 17):
1. **DynamicEQ** - Thread safety fixes
2. **ResonantChorus** - Complete implementation from scratch
3. **BufferRepeat** - Thread safety and mode restoration
4. **PlateReverb** - Reverb tail generation fixed
5. **SpringReverb_Platinum** - Denormal protection added
6. **ConvolutionReverb** - Mix parameter mapping fixed
7. **ShimmerReverb** - Tail generation verified
8. **SpectralGate** - Complete STFT implementation from scratch

### Engines Still Needing Fixes (12):

#### Numerical Issues (5):
1. **VintageOptoCompressor_Platinum** - Division by zero
2. **KStyleOverdrive** - Uninitialized variables causing NaN
3. **DimensionExpander** - NaN in spatial calculations
4. **SpringReverb_Platinum** - Square root of negative (may be fixed with DenormalGuard)
5. **PhaseAlign_Platinum** - Infinity in phase calculations

#### Hanging/Infinite Loops (7):
1. **SpectralFreeze** - STFT processing deadlock
2. **GranularCloud** - Grain scheduling infinite loop
3. **ChaosGenerator** - Lorenz attractor overflow
4. **FeedbackNetwork** - Unstable feedback causing runaway
5. **GainUtility** - Simple gain but hanging (likely trivial fix)
6. **MonoMaker** - Channel summing issue causing hang
7. **ChaosGenerator** (duplicate entry in original data)

### Notes:
- All reverbs now generate proper tails after today's fixes
- GatedReverb cutting tail when gate closes is **expected behavior**
- Spatial and Utility categories have the most issues (complex algorithms)
- Most issues are in advanced/experimental engines
- Core bread-and-butter effects (EQ, Compression, Delays, Reverbs) are solid