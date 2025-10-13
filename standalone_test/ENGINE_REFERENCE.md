# Chimera Phoenix Engine Reference
## Comprehensive Technical Documentation for All 57 DSP Engines

**Version**: 3.0
**Last Updated**: October 11, 2025
**Total Engines**: 57 (ID 0-56)
**Test Coverage**: 100%

---

## Table of Contents

1. [Engine 0: None (Bypass)](#engine-0-none-bypass)
2. [Dynamics & Compression (Engines 1-6)](#dynamics--compression-engines-1-6)
3. [Filters & EQ (Engines 7-14)](#filters--eq-engines-7-14)
4. [Distortion & Saturation (Engines 15-22)](#distortion--saturation-engines-15-22)
5. [Modulation (Engines 23-33)](#modulation-engines-23-33)
6. [Reverb & Delay (Engines 34-43)](#reverb--delay-engines-34-43)
7. [Spatial & Special Effects (Engines 44-52)](#spatial--special-effects-engines-44-52)
8. [Utility (Engines 53-56)](#utility-engines-53-56)

---

## Engine 0: None (Bypass)

### Purpose
Transparent bypass state with minimal processing overhead. Provides clean passthrough for empty effect slots.

### Algorithm
- **Type**: Direct passthrough
- **Implementation**: Memcpy with denormal protection

### Parameters
- **Count**: 0 (no adjustable parameters)

### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 0.1% (minimal overhead)
- **THD**: N/A (bit-perfect passthrough)

### Special Considerations
- Uses SIMD-optimized memory copy when available
- Applies denormal protection to prevent CPU spikes
- Zero latency makes it ideal for A/B testing

### Status
✅ **Production Ready** - Perfect for bypass/null test scenarios

---

## Dynamics & Compression (Engines 1-6)

### Engine 1: Vintage Opto Compressor Platinum

#### Purpose
Classic optical compressor emulation with smooth, musical gain reduction. Models vintage LA-2A style behavior.

#### Algorithm
- **Type**: Optical gain reduction with RMS detection
- **Detection**: RMS envelope follower with 512-sample window
- **Knee**: Soft knee compression (variable)
- **Processing**: Chunked 32-sample blocks with SIMD optimization

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Threshold | 0.0-1.0 | 0.5 | Input level for compression (-60 to 0 dB) |
| 1 | Ratio | 0.0-1.0 | 0.5 | Compression ratio (1:1 to 20:1) |
| 2 | Attack | 0.0-1.0 | 0.2 | Attack time (1-100 ms) |
| 3 | Release | 0.0-1.0 | 0.5 | Release time (50-500 ms) |
| 4 | Knee | 0.0-1.0 | 0.5 | Soft knee width (0-12 dB) |
| 5 | Makeup Gain | 0.0-1.0 | 0.5 | Output gain compensation (0-20 dB) |
| 6 | Mix | 0.0-1.0 | 1.0 | Parallel compression blend |
| 7 | Sidechain HPF | 0.0-1.0 | 0.2 | Sidechain high-pass filter (20-500 Hz) |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms, zero-latency design)
- **CPU Usage**: 2.1% (very efficient)
- **THD**: 0.016% (excellent transparency)
- **Complexity**: LOW

#### Special Considerations
- ⚠️ **Real-time Safety Issue**: Contains disabled file I/O code in process() that must be removed before production
- Optical response modeling provides natural, program-dependent compression
- RMS detection provides smoother gain reduction than peak detection
- Suitable for vocals, bass, and full mix processing

#### Status
✅ **Production Ready** (remove debug code)
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 2: Classic VCA Compressor Pro

#### Purpose
Professional VCA-style compressor with precision gain control. Models SSL/API console dynamics.

#### Algorithm
- **Type**: Feed-forward VCA with peak detection
- **Detection**: Advanced peak/RMS hybrid detector
- **Knee**: Hard or soft knee (adjustable)
- **Processing**: Chunked processing (32-sample blocks) with SIMD optimization
- **Special**: Fixed chunked processing bug (parameter smoothing per sub-block)

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Threshold | 0.0-1.0 | 0.5 | Compression threshold (-60 to 0 dB) |
| 1 | Ratio | 0.0-1.0 | 0.4 | Compression ratio (1:1 to 20:1) |
| 2 | Attack | 0.0-1.0 | 0.15 | Attack time (0.1-30 ms) |
| 3 | Release | 0.0-1.0 | 0.5 | Release time (10-1000 ms) |
| 4 | Knee | 0.0-1.0 | 0.3 | Knee hardness (0=hard, 1=soft) |
| 5 | Makeup Gain | 0.0-1.0 | 0.5 | Output gain (0-20 dB) |
| 6 | Mix | 0.0-1.0 | 1.0 | Dry/wet blend |
| 7 | Auto Makeup | 0.0-1.0 | 0.0 | Automatic gain compensation |
| 8 | Detector | 0.0-1.0 | 0.5 | Peak/RMS blend |
| 9 | Lookahead | 0.0-1.0 | 0.0 | Lookahead time (0-5 ms) |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms, with lookahead disabled)
- **CPU Usage**: 1.8% (optimized)
- **THD**: 0.027% (excellent)
- **Complexity**: LOW

#### Special Considerations
- ✅ **Bug Fixed**: Chunked processing now correctly smooths parameters per sub-block
- World-class implementation with professional-grade metering
- Contains 600+ lines of disabled code for sidechain and lookahead (can be re-enabled)
- Peak detection provides aggressive, punchy compression
- Suitable for drums, percussion, and mastering

#### Status
✅ **Production Ready** - World-class quality
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 3: Transient Shaper Platinum

#### Purpose
Dual-band transient designer for enhancing or reducing attack and sustain independently.

#### Algorithm
- **Type**: Envelope follower with separate attack/sustain processing
- **Detection**: Fast envelope detector with adjustable response
- **Processing**: Dual-band (low/high) with crossover
- **Bands**: 2-band crossover at adjustable frequency

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Attack | 0.0-1.0 | 0.5 | Enhance/reduce attack transients (-12 to +12 dB) |
| 1 | Sustain | 0.0-1.0 | 0.5 | Enhance/reduce sustain/body (-12 to +12 dB) |
| 2 | Speed | 0.0-1.0 | 0.5 | Envelope detector speed (fast/slow) |
| 3 | Crossover | 0.0-1.0 | 0.5 | Crossover frequency (100-5000 Hz) |
| 4 | Low Attack | 0.0-1.0 | 0.5 | Low band attack adjustment |
| 5 | High Attack | 0.0-1.0 | 0.5 | High band attack adjustment |
| 6 | Output Gain | 0.0-1.0 | 0.5 | Output level compensation |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 5.2% (dual-band processing)
- **THD**: 0.041% (good)
- **Complexity**: MODERATE

#### Special Considerations
- ⚠️ **Debug Code**: Contains printf statements that should be removed
- Dual-band processing allows frequency-dependent transient shaping
- Fast envelope follower responds to sub-millisecond transients
- Excellent for drums, enhancing kick punch, or reducing cymbal harshness

#### Status
✅ **Production Ready** (remove debug code)
⭐⭐⭐⭐ Very good quality

---

### Engine 4: Noise Gate Platinum

#### Purpose
Professional noise gate with adjustable threshold, hysteresis, and envelope shaping.

#### Algorithm
- **Type**: RMS-based gate with soft knee and hysteresis
- **Detection**: RMS envelope follower
- **Hysteresis**: Separate open/close thresholds prevent chattering

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Threshold | 0.0-1.0 | 0.3 | Gate threshold (-80 to 0 dB) |
| 1 | Ratio | 0.0-1.0 | 0.9 | Gate ratio (1:∞ = full gate) |
| 2 | Attack | 0.0-1.0 | 0.1 | Gate open time (0.1-50 ms) |
| 3 | Release | 0.0-1.0 | 0.5 | Gate close time (10-1000 ms) |
| 4 | Hold | 0.0-1.0 | 0.2 | Hold time before release (0-500 ms) |
| 5 | Hysteresis | 0.0-1.0 | 0.3 | Open/close threshold difference |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 1.5% (very efficient)
- **THD**: 0.012% (excellent)
- **Complexity**: LOW

#### Special Considerations
- ⚠️ **Real-time Safety Issue**: Heap allocation in process() - MUST FIX
- Hysteresis prevents gate chattering on borderline signals
- Soft knee provides musical gating without harsh on/off transitions
- Hold time keeps gate open for natural decay

#### Status
⚠️ **Fix Required** (heap allocation issue)
⭐⭐⭐⭐ Very good quality (after fix)

---

### Engine 5: Mastering Limiter Platinum

#### Purpose
Transparent brick-wall limiter with lookahead for mastering and broadcast applications.

#### Algorithm
- **Type**: True-peak limiter with lookahead
- **Detection**: Inter-sample peak detection (oversampled)
- **Lookahead**: 5ms lookahead buffer
- **Release**: Multi-stage adaptive release

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Threshold | 0.0-1.0 | 0.9 | Limiting threshold (-20 to 0 dBFS) |
| 1 | Ceiling | 0.0-1.0 | 0.95 | Output ceiling (-0.5 to 0 dBFS) |
| 2 | Release | 0.0-1.0 | 0.5 | Release time (10-1000 ms) |
| 3 | Lookahead | 0.0-1.0 | 0.5 | Lookahead time (1-10 ms) |
| 4 | Knee | 0.0-1.0 | 0.2 | Soft knee width (0-6 dB) |
| 5 | ISP Detection | 0.0-1.0 | 1.0 | Inter-sample peak detection on/off |

#### Performance Characteristics
- **Expected Latency**: 240 samples (5 ms at 48kHz, lookahead buffer)
- **CPU Usage**: 8.3% (lookahead and oversampling)
- **THD**: 0.023% (excellent for limiter)
- **Complexity**: MODERATE

#### Special Considerations
- ⚠️ **Debug Code**: Printf statements should be removed
- True-peak detection prevents inter-sample clipping
- Lookahead allows transparent limiting without distortion
- Adaptive release prevents pumping artifacts
- Suitable for mastering, broadcast, and streaming loudness

#### Status
✅ **Production Ready** (remove debug code)
⭐⭐⭐⭐ Very good quality

---

### Engine 6: Dynamic EQ

#### Purpose
Multi-band parametric EQ with dynamics processing per band. Combines EQ and compression.

#### Algorithm
- **Type**: Multi-band parametric EQ with per-band dynamics
- **Bands**: 4 independent bands
- **Detection**: RMS per band
- **EQ**: IIR biquad filters (bell, shelf, notch)

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Band 1 Freq | 0.0-1.0 | 0.2 | Band 1 center frequency (20-20000 Hz) |
| 1 | Band 1 Gain | 0.0-1.0 | 0.5 | Band 1 gain (-12 to +12 dB) |
| 2 | Band 1 Q | 0.0-1.0 | 0.5 | Band 1 Q factor (0.1-10) |
| 3 | Band 1 Threshold | 0.0-1.0 | 0.5 | Band 1 dynamics threshold |
| 4-15 | Bands 2-4 | 0.0-1.0 | Varies | Same parameters for bands 2-4 |
| 16 | Output Gain | 0.0-1.0 | 0.5 | Master output level |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 18.7% (multi-band processing)
- **THD**: 0.759% ⚠️ (slightly above 0.5% threshold)
- **Complexity**: HIGH

#### Special Considerations
- ⚠️ **Quality Issue**: THD 0.759% is above threshold (target <0.5%)
- Likely cause: Filter interaction or coefficient quantization
- Each band can dynamically adjust EQ based on input level
- Useful for de-essing, controlling resonances, mastering

#### Status
⚠️ **Needs Improvement** - THD too high
⭐⭐⭐ Good quality (needs optimization)

---

## Filters & EQ (Engines 7-14)

### Engine 7: Parametric EQ Studio

#### Purpose
Professional 8-band parametric equalizer with vintage analog modeling.

#### Algorithm
- **Type**: IIR biquad cascaded filters
- **Bands**: 8 fully parametric bands
- **Filter Types**: Bell, low-shelf, high-shelf, notch, high-pass, low-pass
- **Topology**: Series cascade with coefficient smoothing

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0-7 | Band Freq | 0.0-1.0 | Varies | Center frequency (20 Hz - 20 kHz) |
| 8-15 | Band Gain | 0.0-1.0 | 0.5 | Gain adjustment (-18 to +18 dB) |
| 16-23 | Band Q | 0.0-1.0 | 0.5 | Q factor/bandwidth (0.1-10) |
| 24-31 | Band Type | 0.0-1.0 | Varies | Filter type (bell/shelf/notch/etc) |
| 32 | Output Gain | 0.0-1.0 | 0.5 | Master output level |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 6.4% (moderate)
- **THD**: 0.008% (excellent)
- **Complexity**: MODERATE

#### Special Considerations
- Zero-latency IIR design suitable for real-time monitoring
- Coefficient smoothing prevents zipper noise during parameter changes
- Excellent THD makes it suitable for mastering applications
- Professional topology matches high-end hardware EQs

#### Status
✅ **Production Ready** - Professional quality
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 8: Vintage Console EQ Studio

#### Purpose
Classic British console EQ emulation with vintage saturation and curves.

#### Algorithm
- **Type**: IIR with vintage component modeling
- **Saturation**: Analog tape-style soft saturation
- **Topology**: 4-band semi-parametric (vintage style)
- **Character**: Models classic SSL/Neve console EQ curves

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Low Freq | 0.0-1.0 | 0.3 | Low shelf frequency (30-300 Hz) |
| 1 | Low Gain | 0.0-1.0 | 0.5 | Low shelf gain (-12 to +12 dB) |
| 2 | Low-Mid Freq | 0.0-1.0 | 0.4 | Low-mid bell frequency (200-2000 Hz) |
| 3 | Low-Mid Gain | 0.0-1.0 | 0.5 | Low-mid gain (-12 to +12 dB) |
| 4 | High-Mid Freq | 0.0-1.0 | 0.6 | High-mid frequency (1-10 kHz) |
| 5 | High-Mid Gain | 0.0-1.0 | 0.5 | High-mid gain (-12 to +12 dB) |
| 6 | High Freq | 0.0-1.0 | 0.7 | High shelf frequency (3-20 kHz) |
| 7 | High Gain | 0.0-1.0 | 0.5 | High shelf gain (-12 to +12 dB) |
| 8 | Saturation | 0.0-1.0 | 0.2 | Vintage saturation amount |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 7.1% (analog modeling overhead)
- **THD**: 0.015% (excellent)
- **Complexity**: MODERATE

#### Special Considerations
- Saturation adds vintage warmth and harmonic color
- EQ curves match classic console EQ response
- Suitable for mixing applications requiring vintage character
- Intentional harmonic distortion at high saturation settings

#### Status
✅ **Production Ready** - Vintage character
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 9: Ladder Filter Pro

#### Purpose
Moog-style 4-pole resonant ladder filter with drive and vintage character.

#### Algorithm
- **Type**: Zero-delay feedback (ZDF) 4-pole ladder
- **Topology**: Four cascaded integrators with global feedback
- **Saturation**: Transistor-style tanh saturation per stage
- **Oversampling**: 2x oversampling to reduce aliasing
- **Modes**: Low-pass (LP), high-pass (HP), band-pass (BP)

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Cutoff | 0.0-1.0 | 0.5 | Filter cutoff frequency (20 Hz - 20 kHz) |
| 1 | Resonance | 0.0-1.0 | 0.3 | Resonance/Q (0.1 to self-oscillation) |
| 2 | Drive | 0.0-1.0 | 0.2 | Input drive/saturation (0-40 dB) |
| 3 | Filter Type | 0.0-1.0 | 0.0 | LP/BP/HP mode selector |
| 4 | Asymmetry | 0.0-1.0 | 0.0 | Asymmetric saturation for vintage character |
| 5 | Vintage Mode | 0.0-1.0 | 0.0 | Component tolerance modeling |
| 6 | Mix | 0.0-1.0 | 1.0 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms, ZDF design)
- **CPU Usage**: 5.8% (oversampling overhead)
- **THD**: 3.512% ❌ (7x above 0.5% threshold)
- **Complexity**: MODERATE

#### Special Considerations
- ❌ **CRITICAL ISSUE**: THD 3.512% is far too high for clean signal path
- Likely causes:
  - Filter instability at high resonance settings
  - Coefficient quantization errors
  - Insufficient oversampling (only 2x)
  - Tanh approximation inaccuracy
- **Recommended Fixes**:
  - Increase oversampling to 4x
  - Add DC blocker in feedback path
  - Improve coefficient precision
  - Review resonance limiting algorithm
- Zero-delay feedback provides better response than traditional ladder
- Excellent for synth bass, leads, and creative filtering
- Self-oscillation allows use as sine wave oscillator

#### Status
❌ **FIX REQUIRED** - THD too high for production
⭐⭐ Fair quality (needs significant improvement)

---

### Engine 10: State Variable Filter

#### Purpose
Versatile multi-mode filter with simultaneous LP/BP/HP/Notch outputs.

#### Algorithm
- **Type**: State-variable topology (Chamberlin/Hal)
- **Outputs**: Simultaneous low-pass, band-pass, high-pass, notch
- **Topology**: Two integrators with feedback
- **Modes**: Morphable between all filter types

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Cutoff | 0.0-1.0 | 0.5 | Filter cutoff frequency (20-20000 Hz) |
| 1 | Resonance | 0.0-1.0 | 0.2 | Resonance/Q factor (0.1-10) |
| 2 | Filter Mode | 0.0-1.0 | 0.0 | LP/BP/HP/Notch selector |
| 3 | Mix | 0.0-1.0 | 1.0 | Dry/wet balance |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 3.2% (very efficient topology)
- **THD**: 0.019% (excellent)
- **Complexity**: LOW

#### Special Considerations
- Efficient topology provides multiple outputs with minimal overhead
- Stable at all resonance settings (unlike ladder filter)
- Suitable for creative sound design and mixing
- Can morph between filter types in real-time

#### Status
✅ **Production Ready** - Versatile and clean
⭐⭐⭐⭐ Very good quality

---

### Engine 11: Formant Filter Pro

#### Purpose
Vocal formant filter for vowel synthesis and character effects.

#### Algorithm
- **Type**: Multiple parallel bandpass filters (formants)
- **Formants**: 5 resonant peaks modeling vocal tract
- **Morphing**: Smooth interpolation between vowel sounds
- **Topology**: Parallel IIR bandpass cascade

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Vowel | 0.0-1.0 | 0.5 | Vowel selector (A/E/I/O/U morphing) |
| 1 | Resonance | 0.0-1.0 | 0.5 | Overall formant resonance |
| 2 | Brightness | 0.0-1.0 | 0.5 | Formant frequency scaling |
| 3 | Gender | 0.0-1.0 | 0.5 | Male/female formant shift |
| 4 | Mix | 0.0-1.0 | 0.8 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 9.3% (5 parallel filters)
- **THD**: 0.034% (good)
- **Complexity**: MODERATE

#### Special Considerations
- Models human vocal tract resonances
- 5 formants provide convincing vowel sounds
- Excellent for vocoder-style effects, robotic vocals, talk box
- Gender parameter shifts formant frequencies (male = lower)

#### Status
✅ **Production Ready** - Creative vocal effects
⭐⭐⭐⭐ Very good quality

---

### Engine 12: Envelope Filter

#### Purpose
Auto-wah/envelope-controlled filter with envelope follower.

#### Algorithm
- **Type**: State-variable filter with envelope control
- **Envelope**: Fast RMS envelope follower
- **Modulation**: Envelope modulates filter cutoff
- **Sensitivity**: Adjustable envelope response curve

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Sensitivity | 0.0-1.0 | 0.5 | Envelope sensitivity/gain |
| 1 | Base Cutoff | 0.0-1.0 | 0.3 | Minimum cutoff frequency |
| 2 | Range | 0.0-1.0 | 0.7 | Modulation depth/range |
| 3 | Attack | 0.0-1.0 | 0.1 | Envelope attack time |
| 4 | Release | 0.0-1.0 | 0.5 | Envelope release time |
| 5 | Resonance | 0.0-1.0 | 0.5 | Filter resonance |
| 6 | Filter Type | 0.0-1.0 | 0.0 | LP/BP/HP selector |
| 7 | Direction | 0.0-1.0 | 1.0 | Up-sweep/down-sweep |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 6.7% (envelope + filter)
- **THD**: 0.027% (excellent)
- **Complexity**: MODERATE

#### Special Considerations
- Fast envelope response captures transients
- Up/down sweep modes for different musical effects
- Classic funk/disco auto-wah sound
- Suitable for guitar, bass, keyboards

#### Status
✅ **Production Ready** - Classic auto-wah
⭐⭐⭐⭐ Very good quality

---

### Engine 13: Comb Resonator

#### Purpose
Resonant comb filter for metallic/robotic effects and pitch-based filtering.

#### Algorithm
- **Type**: Feedforward/feedback comb filter
- **Delay**: Variable delay line for tuning
- **Feedback**: Adjustable for resonance control
- **Topology**: Parallel comb banks for richness

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Frequency | 0.0-1.0 | 0.5 | Comb frequency/pitch (20-2000 Hz) |
| 1 | Resonance | 0.0-1.0 | 0.4 | Feedback amount/decay time |
| 2 | Spread | 0.0-1.0 | 0.3 | Detuning between comb banks |
| 3 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 4.1% (delay line processing)
- **THD**: 0.041% (good)
- **Complexity**: LOW

#### Special Considerations
- Tunable to musical pitches for harmonic resonance
- Creates metallic, robotic, or hollow timbres
- Useful for special effects, sound design
- Can be used for subtle stereo widening

#### Status
✅ **Production Ready** - Creative resonance
⭐⭐⭐⭐ Very good quality

---

### Engine 14: Vocal Formant Filter

#### Purpose
Advanced vocal formant synthesis with vowel morphing and realistic articulation.

#### Algorithm
- **Type**: Multi-formant IIR cascade
- **Formants**: 5 formant peaks with precise tuning
- **Morphing**: Real-time vowel interpolation
- **Modeling**: Based on human vocal tract measurements

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Vowel A-E | 0.0-1.0 | 0.0 | Morph between A and E |
| 1 | Vowel I-O | 0.0-1.0 | 0.0 | Morph between I and O |
| 2 | Vowel U | 0.0-1.0 | 0.0 | Amount of U vowel |
| 3 | Resonance | 0.0-1.0 | 0.5 | Formant bandwidth |
| 4 | Brightness | 0.0-1.0 | 0.5 | Formant frequency shift |
| 5 | Gender | 0.0-1.0 | 0.5 | Male/female formant tuning |
| 6 | Articulation | 0.0-1.0 | 0.5 | Transition speed |
| 7 | Mix | 0.0-1.0 | 0.8 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 10.2% (complex formant synthesis)
- **THD**: 0.000% (bit-perfect at neutral settings)
- **Complexity**: MODERATE

#### Special Considerations
- Bit-perfect THD indicates excellent implementation
- Realistic vowel sounds for vocoder and talk box effects
- Gender parameter shifts all formants proportionally
- Articulation controls formant transition smoothness

#### Status
✅ **Production Ready** - Excellent formant synthesis
⭐⭐⭐⭐⭐ Excellent quality

---

## Distortion & Saturation (Engines 15-22)

### Engine 15: Vintage Tube Preamp Studio

#### Purpose
Vacuum tube preamp emulation with harmonic saturation and warmth.

#### Algorithm
- **Type**: Non-linear waveshaping with tube characteristic curves
- **Saturation**: Even/odd harmonic generation
- **Bias**: Adjustable bias shift for asymmetry
- **Tone Stack**: Classic tube amp tone controls

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Drive | 0.0-1.0 | 0.3 | Input gain/drive amount |
| 1 | Bias | 0.0-1.0 | 0.5 | Tube bias point |
| 2 | Tone | 0.0-1.0 | 0.5 | Tilt EQ (dark/bright) |
| 3 | Warmth | 0.0-1.0 | 0.4 | Low-frequency emphasis |
| 4 | Output | 0.0-1.0 | 0.5 | Output level |

#### Performance Characteristics
- **Expected Latency**: UNKNOWN (test hangs)
- **CPU Usage**: UNKNOWN (test timeout)
- **THD**: UNKNOWN (cannot measure)
- **Complexity**: MODERATE (estimated)

#### Special Considerations
- ❌ **CRITICAL SHOWSTOPPER**: Engine hangs indefinitely during processing
- **Issue**: Infinite loop or blocking operation in DSP code
- **Impact**: Will freeze DAW, cause data loss for users
- **Likely Causes**:
  - While loop without exit condition
  - Recursive function without base case
  - Circular parameter dependency
  - Buffer calculation error
- **DO NOT SHIP** until this is fixed

#### Status
🔴 **BROKEN** - BLOCKS ALL RELEASES
⭐ Poor (non-functional)

---

### Engine 16: Wave Folder

#### Purpose
Wave folding distortion for complex harmonic generation and synthesis textures.

#### Algorithm
- **Type**: Additive wave folding (Buchla-style)
- **Folding**: Multiple fold stages for rich harmonics
- **Symmetry**: Adjustable symmetry for even/odd harmonics
- **Topology**: Cascaded folding with DC offset compensation

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Fold Amount | 0.0-1.0 | 0.5 | Number of folds (complexity) |
| 1 | Bias | 0.0-1.0 | 0.5 | DC bias/asymmetry |
| 2 | Shape | 0.0-1.0 | 0.5 | Fold shape curve |
| 3 | Mix | 0.0-1.0 | 0.8 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 3.5% (efficient algorithm)
- **THD**: 0.023% (excellent for distortion effect)
- **Complexity**: LOW

#### Special Considerations
- Wave folding creates inharmonic spectra at high settings
- DC offset compensation prevents bias drift
- Excellent for synthesizer sounds and aggressive distortion
- Suitable for experimental sound design

#### Status
✅ **Production Ready** - Creative distortion
⭐⭐⭐⭐ Very good quality

---

### Engine 17: Harmonic Exciter Platinum

#### Purpose
Psychoacoustic harmonic enhancement for brightness and presence without EQ.

#### Algorithm
- **Type**: Selective harmonic synthesis
- **Synthesis**: Even harmonics (2nd, 4th) and odd harmonics (3rd, 5th)
- **Detection**: Transient-sensitive processing
- **Topology**: High-pass filtered distortion with mixing

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Amount | 0.0-1.0 | 0.3 | Exciter intensity |
| 1 | Frequency | 0.0-1.0 | 0.6 | Crossover frequency (2-16 kHz) |
| 2 | Harmonics | 0.0-1.0 | 0.5 | Even/odd harmonic balance |
| 3 | Clarity | 0.0-1.0 | 0.5 | Transient emphasis |
| 4 | Mix | 0.0-1.0 | 0.3 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 7.6% (harmonic synthesis)
- **THD**: 0.089% (good for enhancement effect)
- **Complexity**: MODERATE

#### Special Considerations
- Adds perceived brightness without harsh EQ boost
- Selective frequency processing prevents muddiness
- Useful for mastering, vocals, guitars
- Transient sensitivity preserves punch and dynamics

#### Status
✅ **Production Ready** - Psychoacoustic enhancement
⭐⭐⭐⭐ Very good quality

---

### Engine 18: Bit Crusher

#### Purpose
Digital degradation effect with bit depth and sample rate reduction.

#### Algorithm
- **Type**: Bit depth reduction + downsampling
- **Quantization**: Adjustable bit depth (1-16 bits)
- **Decimation**: Sample rate reduction with hold
- **Dithering**: Optional dithering to reduce quantization harshness

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Bit Depth | 0.0-1.0 | 0.5 | Bit resolution (1-16 bits) |
| 1 | Sample Rate | 0.0-1.0 | 0.5 | Decimation amount (100 Hz - 48 kHz) |
| 2 | Dither | 0.0-1.0 | 0.0 | Dithering amount |
| 3 | Mix | 0.0-1.0 | 1.0 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 2.3% (very efficient)
- **THD**: 0.156% (good for lo-fi effect)
- **Complexity**: LOW

#### Special Considerations
- Creates retro video game and lo-fi hip-hop textures
- Sample rate reduction creates aliasing (intentional)
- Dithering can reduce harshness at extreme settings
- Very CPU efficient for creative effect

#### Status
✅ **Production Ready** - Lo-fi character
⭐⭐⭐⭐ Very good quality

---

### Engine 19: Multiband Saturator

#### Purpose
Frequency-dependent saturation with independent band processing.

#### Algorithm
- **Type**: FFT-based band splitting + waveshaping
- **Bands**: 4 frequency bands with crossover
- **Saturation**: Per-band tube/tape/transistor models
- **Topology**: FFT split → saturate → FFT merge

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0-3 | Band Drive | 0.0-1.0 | Varies | Drive amount per band |
| 4-7 | Band Type | 0.0-1.0 | Varies | Saturation type (tube/tape/transistor) |
| 8-10 | Crossover Freq | 0.0-1.0 | Varies | Band split frequencies |
| 11 | Output Gain | 0.0-1.0 | 0.5 | Master output level |

#### Performance Characteristics
- **Expected Latency**: 512-1024 samples (~10-21 ms at 48 kHz, FFT window)
- **CPU Usage**: 15.4% (FFT processing)
- **THD**: 0.278% (acceptable for saturation effect)
- **Complexity**: HIGH

#### Special Considerations
- FFT introduces latency but enables clean band separation
- Each band can use different saturation character
- Prevents bass muddiness while saturating highs
- Suitable for mastering and creative mixing

#### Status
✅ **Production Ready** - Sophisticated saturation
⭐⭐⭐⭐ Very good quality

---

### Engine 20: Muff Fuzz

#### Purpose
Classic Big Muff style fuzz distortion with sustain and tone controls.

#### Algorithm
- **Type**: Cascaded clipping stages with filtering
- **Clipping**: Asymmetric diode clipping
- **Filtering**: Pre/post EQ shaping
- **Topology**: 4-stage gain + clip cascade

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Sustain | 0.0-1.0 | 0.6 | Input gain/distortion amount |
| 1 | Tone | 0.0-1.0 | 0.5 | Tilt EQ (dark to bright) |
| 2 | Volume | 0.0-1.0 | 0.5 | Output level |
| 3 | Bias | 0.0-1.0 | 0.5 | Clipping asymmetry |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 6.8% (multiple clipping stages)
- **THD**: N/A (intentional heavy distortion)
- **Complexity**: MODERATE

#### Special Considerations
- ⚠️ **Minor Issue**: CPU 5.19% slightly over 5.0% threshold (optimization recommended)
- Classic fuzz topology with vintage character
- Asymmetric clipping creates rich harmonics
- Suitable for guitar, bass, synth leads

#### Status
✅ **Production Ready** (minor optimization recommended)
⭐⭐⭐ Good quality

---

### Engine 21: Rodent Distortion

#### Purpose
RAT-style distortion pedal emulation with hard clipping and filtering.

#### Algorithm
- **Type**: Op-amp clipping with tone filtering
- **Clipping**: Hard clipping with adjustable threshold
- **Filtering**: Post-distortion low-pass tone control
- **Topology**: Gain → clip → filter

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Distortion | 0.0-1.0 | 0.5 | Clipping threshold/amount |
| 1 | Filter | 0.0-1.0 | 0.5 | Tone control (bright to dark) |
| 2 | Volume | 0.0-1.0 | 0.5 | Output level |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 5.9% (clipping + filtering)
- **THD**: 0.234% (good for distortion effect)
- **Complexity**: MODERATE

#### Special Considerations
- Hard clipping provides aggressive, cutting tone
- Filter control shapes distortion character
- Classic 80s/90s alternative rock sound
- Suitable for guitar, bass, aggressive synths

#### Status
✅ **Production Ready** - Classic distortion
⭐⭐⭐⭐ Very good quality

---

### Engine 22: K-Style Overdrive

#### Purpose
Klon-style transparent overdrive with buffered signal path.

#### Algorithm
- **Type**: Soft clipping with EQ shaping
- **Clipping**: Diode soft clipping (symmetric/asymmetric)
- **Buffering**: Clean blend for transparency
- **Topology**: Clean buffer + overdrive blend

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Drive | 0.0-1.0 | 0.4 | Overdrive amount |
| 1 | Tone | 0.0-1.0 | 0.5 | Treble control |
| 2 | Volume | 0.0-1.0 | 0.5 | Output level |
| 3 | Clean Blend | 0.0-1.0 | 0.3 | Clean signal blend for transparency |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 6.2% (blend processing)
- **THD**: 0.198% (good for overdrive)
- **Complexity**: MODERATE

#### Special Considerations
- Clean blend provides transparent overdrive character
- Soft clipping preserves dynamics and touch sensitivity
- Legendary "transparent overdrive" sound
- Suitable for guitar, bass, subtle saturation

#### Status
✅ **Production Ready** - Transparent overdrive
⭐⭐⭐⭐ Very good quality

---

## Modulation (Engines 23-33)

### Engine 23: Digital Chorus (Stereo Chorus)

#### Purpose
Lush stereo chorus with multiple delay voices and modulation.

#### Algorithm
- **Type**: Multi-voice delay with LFO modulation
- **Voices**: 4 independent delay lines
- **LFOs**: Per-voice phase-offset sine/triangle LFOs
- **Topology**: Parallel delay network with stereo spread

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Rate | 0.0-1.0 | 0.3 | LFO rate (0.1-10 Hz) |
| 1 | Depth | 0.0-1.0 | 0.5 | Modulation depth |
| 2 | Delay | 0.0-1.0 | 0.4 | Base delay time (5-50 ms) |
| 3 | Feedback | 0.0-1.0 | 0.2 | Feedback amount |
| 4 | Voices | 0.0-1.0 | 0.7 | Number of voices (2-8) |
| 5 | Stereo Width | 0.0-1.0 | 0.8 | Stereo spread |
| 6 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: ~10-25 ms (variable, base delay time)
- **CPU Usage**: 8.1% (multi-voice processing)
- **THD**: 0.012% (excellent)
- **Complexity**: MODERATE

#### Special Considerations
- Phase-offset LFOs create wide stereo image
- Multiple voices provide rich, lush chorus
- Suitable for guitars, synths, vocals, mixing

#### Status
✅ **Production Ready** - Beautiful chorus
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 24: Resonant Chorus Platinum

#### Purpose
Chorus with resonant filtering for unique tonal character.

#### Algorithm
- **Type**: Chorus with resonant comb filtering
- **Resonance**: Tunable resonant peaks
- **Modulation**: Chorus + resonance modulation
- **Topology**: Delay → comb filter → chorus

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Rate | 0.0-1.0 | 0.3 | Chorus rate |
| 1 | Depth | 0.0-1.0 | 0.5 | Modulation depth |
| 2 | Resonance | 0.0-1.0 | 0.4 | Resonant peak intensity |
| 3 | Frequency | 0.0-1.0 | 0.5 | Resonance frequency |
| 4 | Mix | 0.0-1.0 | 0.6 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: ~15 ms (variable)
- **CPU Usage**: 9.7% (chorus + filtering)
- **THD**: 0.034% (good)
- **Complexity**: MODERATE

#### Special Considerations
- Resonance adds metallic/robotic character
- Tunable resonance enables musical effects
- Unique sound compared to standard chorus
- Suitable for experimental sound design

#### Status
✅ **Production Ready** - Unique chorus variant
⭐⭐⭐⭐ Very good quality

---

### Engine 25: Analog Phaser

#### Purpose
Classic analog phaser emulation with multiple stages and feedback.

#### Algorithm
- **Type**: All-pass filter cascade (4-12 stages)
- **Modulation**: LFO-modulated all-pass cutoff
- **Feedback**: Adjustable feedback for intensity
- **Topology**: Cascaded all-pass with mix

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Rate | 0.0-1.0 | 0.3 | LFO rate (0.05-10 Hz) |
| 1 | Depth | 0.0-1.0 | 0.6 | Modulation depth |
| 2 | Stages | 0.0-1.0 | 0.5 | Number of stages (4/6/8/12) |
| 3 | Feedback | 0.0-1.0 | 0.4 | Feedback intensity |
| 4 | Stereo | 0.0-1.0 | 0.7 | L/R phase offset |
| 5 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms, all-pass is zero-latency)
- **CPU Usage**: 7.3% (cascaded all-pass)
- **THD**: 0.019% (excellent)
- **Complexity**: MODERATE

#### Special Considerations
- More stages = deeper, more dramatic sweeps
- Feedback adds resonance and intensity
- Classic phaser sound (MXR Phase 90/100 style)
- Suitable for guitars, keys, mixing

#### Status
✅ **Production Ready** - Classic phaser
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 26: Ring Modulator (Platinum Ring Modulator)

#### Purpose
Ring modulation for metallic, inharmonic textures and frequency shifting.

#### Algorithm
- **Type**: Four-quadrant multiplication
- **Carrier**: Internal oscillator or external sidechain
- **Waveforms**: Sine, triangle, square, saw
- **Topology**: Signal × carrier

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Frequency | 0.0-1.0 | 0.5 | Carrier frequency (20 Hz - 5 kHz) |
| 1 | Waveform | 0.0-1.0 | 0.0 | Carrier waveform (sine/tri/square/saw) |
| 2 | Depth | 0.0-1.0 | 1.0 | Modulation depth |
| 3 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 3.8% (simple multiplication)
- **THD**: 0.045% (good)
- **Complexity**: LOW

#### Special Considerations
- Creates sum and difference frequencies (f1 ± f2)
- Inharmonic spectra for bell-like, metallic sounds
- Different waveforms produce different textures
- Suitable for special effects, experimental sound design

#### Status
✅ **Production Ready** - Classic ring mod
⭐⭐⭐⭐ Very good quality

---

### Engine 27: Frequency Shifter

#### Purpose
True frequency shifting (not pitch shifting) using Hilbert transform.

#### Algorithm
- **Type**: Single-sideband modulation (SSB)
- **Hilbert**: Hilbert transform for 90° phase shift
- **Shift**: Adds constant frequency offset
- **Topology**: Hilbert → complex modulation

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Shift Amount | 0.0-1.0 | 0.5 | Frequency shift (-2 kHz to +2 kHz) |
| 1 | Mode | 0.0-1.0 | 0.5 | Up/down/both |
| 2 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 128-256 samples (~2-5 ms, Hilbert transform)
- **CPU Usage**: 16.2% (complex DSP)
- **THD**: 0.067% (good)
- **Complexity**: HIGH

#### Special Considerations
- Frequency shifting ≠ pitch shifting (inharmonic result)
- Creates dissonant, otherworldly textures
- Hilbert transform provides accurate 90° phase shift
- Suitable for experimental effects, sound design

#### Status
✅ **Production Ready** - Unique frequency effects
⭐⭐⭐⭐ Very good quality

---

### Engine 28: Harmonic Tremolo

#### Purpose
Dual-band tremolo with crossover for vintage amp-style harmonic tremolo.

#### Algorithm
- **Type**: Dual-band amplitude modulation
- **Bands**: Low/high split with crossover
- **Phase**: 180° phase offset between bands
- **LFO**: Sine/triangle LFO

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Rate | 0.0-1.0 | 0.3 | Tremolo rate (0.5-20 Hz) |
| 1 | Depth | 0.0-1.0 | 0.6 | Modulation depth (0-100%) |
| 2 | Crossover | 0.0-1.0 | 0.5 | Band split frequency (200-2000 Hz) |
| 3 | Waveform | 0.0-1.0 | 0.0 | LFO shape (sine/triangle) |
| 4 | Phase | 0.0-1.0 | 1.0 | Band phase relationship |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 6.5% (dual-band processing)
- **THD**: 0.023% (excellent)
- **Complexity**: MODERATE

#### Special Considerations
- Dual-band creates complex, swirling effect
- 180° phase offset is classic harmonic tremolo sound
- Vintage amp character (Fender Vibrolux style)
- Suitable for guitars, keys, vintage sounds

#### Status
✅ **Production Ready** - Authentic harmonic tremolo
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 29: Classic Tremolo

#### Purpose
Simple, classic tremolo with sine/triangle/square LFO modulation.

#### Algorithm
- **Type**: Amplitude modulation
- **LFO**: Multiple waveform options
- **Topology**: Signal × (1 + LFO × depth)

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Rate | 0.0-1.0 | 0.3 | Tremolo rate (0.1-20 Hz) |
| 1 | Depth | 0.0-1.0 | 0.5 | Modulation depth (0-100%) |
| 2 | Waveform | 0.0-1.0 | 0.0 | LFO waveform (sine/tri/square) |
| 3 | Stereo | 0.0-1.0 | 0.0 | L/R phase offset |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 2.1% (very efficient)
- **THD**: 0.018% (excellent)
- **Complexity**: LOW

#### Special Considerations
- Very CPU efficient for classic effect
- Square wave LFO creates vintage "chop" effect
- Stereo offset creates auto-panning effect
- Suitable for guitars, keys, mixing

#### Status
✅ **Production Ready** - Simple and effective
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 30: Rotary Speaker Platinum

#### Purpose
Leslie rotating speaker cabinet emulation with Doppler effect and horn/rotor simulation.

#### Algorithm
- **Type**: Physical modeling of rotating speaker
- **Doppler**: Frequency/amplitude modulation for Doppler shift
- **Horns**: Separate treble horn and bass rotor simulation
- **Cabinet**: Cabinet resonance and crossover modeling

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Speed | 0.0-1.0 | 0.5 | Slow/fast (chorale/tremolo) |
| 1 | Horn Rate | 0.0-1.0 | 0.6 | Treble horn rotation speed |
| 2 | Rotor Rate | 0.0-1.0 | 0.4 | Bass rotor rotation speed |
| 3 | Crossover | 0.0-1.0 | 0.5 | Horn/rotor crossover frequency |
| 4 | Depth | 0.0-1.0 | 0.7 | Doppler effect depth |
| 5 | Drive | 0.0-1.0 | 0.3 | Tube amp drive |
| 6 | Mix | 0.0-1.0 | 1.0 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: Variable (~5-15 ms, Doppler delay modulation)
- **CPU Usage**: 19.8% (complex physical modeling)
- **THD**: 0.089% (good for vintage emulation)
- **Complexity**: HIGH

#### Special Considerations
- Authentic Leslie cabinet sound with Doppler shift
- Separate horn/rotor speeds for realism
- Tube drive adds vintage character
- Suitable for organs, guitars, keys

#### Status
✅ **Production Ready** - Authentic rotary speaker
⭐⭐⭐⭐ Very good quality

---

### Engine 31: Pitch Shifter

#### Purpose
Real-time pitch shifting for harmonization and transposition effects.

#### Algorithm
- **Type**: Time-domain pitch shifting (PSOLA or granular)
- **Window**: Windowed overlap-add
- **Buffers**: Large analysis/synthesis buffers
- **Latency**: Lookahead for quality

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Pitch | 0.0-1.0 | 0.5 | Pitch shift (-12 to +12 semitones) |
| 1 | Formant | 0.0-1.0 | 0.5 | Formant preservation on/off |
| 2 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 2048 samples (~42.7 ms at 48 kHz)
- **CPU Usage**: 47.3% (very high - complex algorithm)
- **THD**: 8.673% ❌ (17x above threshold - CRITICAL ISSUE)
- **Complexity**: EXTREME

#### Special Considerations
- ❌ **CRITICAL ISSUE**: THD 8.673% is unusable for professional audio
- Likely causes:
  - Grain discontinuities in granular synthesis
  - Window function artifacts
  - Buffer underflow/overflow
  - Missing anti-aliasing
- **Recommended Fixes**:
  - Increase oversampling
  - Improve grain windowing (Hann/Blackman-Harris)
  - Add crossfading between grains
  - Consider using frequency-domain algorithm
- High latency is acceptable for pitch shifting but THD must be fixed
- Formant preservation maintains natural vocal character

#### Status
❌ **FIX REQUIRED** - THD unacceptable
⭐⭐ Fair quality (needs major improvement)

---

### Engine 32: Detune Doubler

#### Purpose
Stereo detuning and doubling effect for width and thickness.

#### Algorithm
- **Type**: Dual pitch shifters (slight detune)
- **Detune**: ±5-50 cents per voice
- **Delay**: Optional pre-delay for thickening
- **Stereo**: Wide stereo placement

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Detune Amount | 0.0-1.0 | 0.3 | Detune range (±5 to ±50 cents) |
| 1 | Delay | 0.0-1.0 | 0.2 | Pre-delay time (0-50 ms) |
| 2 | Stereo Width | 0.0-1.0 | 0.8 | L/R spread |
| 3 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 512 samples (~10.7 ms at 48 kHz)
- **CPU Usage**: 22.6% (dual pitch shifters)
- **THD**: 0.034% (good)
- **Complexity**: HIGH

#### Special Considerations
- Uses two pitch shifters internally (high CPU)
- Creates natural doubling and stereo width
- Excellent for vocals, guitars, synth leads
- Slight detune creates chorus-like effect without modulation

#### Status
✅ **Production Ready** - Natural doubling
⭐⭐⭐⭐ Very good quality

---

### Engine 33: Intelligent Harmonizer

#### Purpose
Multi-voice pitch harmonizer with scale quantization and key tracking.

#### Algorithm
- **Type**: Multi-voice pitch shifting with music theory
- **Voices**: Up to 4 harmony voices
- **Scale**: Diatonic scale quantization
- **Key**: Key/scale selection

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Interval 1 | 0.0-1.0 | 0.5 | First harmony interval (-12 to +12 semitones) |
| 1 | Interval 2 | 0.0-1.0 | 0.5 | Second harmony interval |
| 2 | Key | 0.0-1.0 | 0.0 | Musical key (C/D/E/F/G/A/B) |
| 3 | Scale | 0.0-1.0 | 0.0 | Scale type (major/minor/etc) |
| 4 | Quantize | 0.0-1.0 | 1.0 | Scale quantization on/off |
| 5 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 2048 samples (~42.7 ms at 48 kHz)
- **CPU Usage**: 52.8% (multiple pitch shifters + analysis)
- **THD**: UNKNOWN (test crashes)
- **Complexity**: EXTREME

#### Special Considerations
- ❌ **CRITICAL ISSUE**: Engine crashes during testing
- Likely causes:
  - Buffer overflow in multi-voice processing
  - Assertion failure in pitch detection
  - Memory access violation
- **Recommended Fixes**:
  - Add bounds checking on all array accesses
  - Verify buffer sizes are sufficient for all voices
  - Test with debugger/sanitizers
- Scale quantization provides musical harmonies
- Multiple voices create rich vocal/guitar harmonies

#### Status
❌ **FIX REQUIRED** - Crashes
⭐⭐ Fair quality (non-functional)

---

## Reverb & Delay (Engines 34-43)

### Engine 34: Tape Echo

#### Purpose
Vintage tape delay emulation with wow/flutter, saturation, and aging characteristics.

#### Algorithm
- **Type**: Delay line with analog modeling
- **Wow/Flutter**: LFO-modulated delay time
- **Saturation**: Tape saturation per repeat
- **Aging**: High-frequency damping and noise

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Time | 0.0-1.0 | 0.4 | Delay time (50-2000 ms) |
| 1 | Feedback | 0.0-1.0 | 0.4 | Number of repeats |
| 2 | Wow/Flutter | 0.0-1.0 | 0.3 | Pitch variation amount |
| 3 | Saturation | 0.0-1.0 | 0.4 | Tape saturation per repeat |
| 4 | Age | 0.0-1.0 | 0.3 | High-frequency loss |
| 5 | Mix | 0.0-1.0 | 0.4 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 128 samples (~2.7 ms at 48 kHz) - Variable with tape speed
- **CPU Usage**: 9.1% (analog modeling)
- **THD**: 0.027% (good)
- **Complexity**: MODERATE

#### Special Considerations
- ⚠️ **Variable Latency**: Tape speed affects latency (intentional)
- Wow/flutter creates pitch warble for vintage character
- Saturation adds warmth to repeats
- Age parameter models old, degraded tape
- Suitable for dub, vintage production, creative delays

#### Status
✅ **Production Ready** - Authentic tape delay
⭐⭐⭐⭐ Very good quality

---

### Engine 35: Digital Delay

#### Purpose
Clean, transparent digital delay with precise timing.

#### Algorithm
- **Type**: Simple delay line (circular buffer)
- **Interpolation**: Linear interpolation for smooth modulation
- **Filtering**: Optional high/low cut filters

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Time | 0.0-1.0 | 0.4 | Delay time (1-5000 ms) |
| 1 | Feedback | 0.0-1.0 | 0.4 | Repeat amount (0-98%) |
| 2 | Low Cut | 0.0-1.0 | 0.0 | High-pass filter (20-500 Hz) |
| 3 | High Cut | 0.0-1.0 | 1.0 | Low-pass filter (1-20 kHz) |
| 4 | Stereo | 0.0-1.0 | 0.5 | L/R timing offset |
| 5 | Mix | 0.0-1.0 | 0.4 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms at minimum delay)
- **CPU Usage**: 4.2% (very efficient)
- **THD**: 0.015% (excellent transparency)
- **Complexity**: LOW

#### Special Considerations
- Zero-latency at minimum delay time
- Bit-perfect transparency at unity settings
- Suitable for tight rhythmic delays, doubling
- Can be used for precision timing effects

#### Status
✅ **Production Ready** - Pristine digital delay
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 36: Magnetic Drum Echo

#### Purpose
Vintage drum echo emulation (Binson Echorec style) with mechanical character.

#### Algorithm
- **Type**: Multi-head delay simulation
- **Heads**: Multiple playback heads
- **Degradation**: Frequency-dependent aging
- **Modulation**: Mechanical wow/flutter

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Time | 0.0-1.0 | 0.4 | Base delay time (50-1000 ms) |
| 1 | Feedback | 0.0-1.0 | 0.5 | Repeat intensity |
| 2 | Heads | 0.0-1.0 | 0.6 | Number of active heads (1-4) |
| 3 | Age | 0.0-1.0 | 0.4 | Mechanical wear/degradation |
| 4 | Mix | 0.0-1.0 | 0.4 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 256 samples (~5.3 ms at 48 kHz)
- **CPU Usage**: 10.3% (multi-head simulation)
- **THD**: 0.045% (good)
- **Complexity**: MODERATE

#### Special Considerations
- Multiple heads create complex rhythmic patterns
- Mechanical character adds vintage authenticity
- Frequency-dependent aging models magnetic drum response
- Suitable for psychedelic rock, dub, vintage production

#### Status
✅ **Production Ready** - Vintage drum echo
⭐⭐⭐⭐ Very good quality

---

### Engine 37: Bucket Brigade Delay

#### Purpose
Analog BBD (Bucket-Brigade Device) delay emulation with clock noise and filtering.

#### Algorithm
- **Type**: Sampled delay line with downsampling
- **Aliasing**: Intentional aliasing (BBD character)
- **Filtering**: Pre/post low-pass for warmth
- **Clock Noise**: Simulated clock bleed

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Time | 0.0-1.0 | 0.4 | Delay time (20-600 ms) |
| 1 | Feedback | 0.0-1.0 | 0.4 | Repeat amount |
| 2 | Tone | 0.0-1.0 | 0.5 | Low-pass filter cutoff |
| 3 | Clock Noise | 0.0-1.0 | 0.3 | BBD clock bleed amount |
| 4 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 512 samples (~10.7 ms at 48 kHz)
- **CPU Usage**: 8.7% (analog modeling)
- **THD**: 0.067% (good for analog emulation)
- **Complexity**: MODERATE

#### Special Considerations
- Aliasing and clock noise are intentional (analog character)
- Dark, warm sound characteristic of BBD delays
- Limited delay time matches vintage hardware
- Suitable for ambient, vintage production, guitar

#### Status
✅ **Production Ready** - Authentic BBD character
⭐⭐⭐⭐ Very good quality

---

### Engine 38: Buffer Repeat Platinum

#### Purpose
Live buffer looping and stuttering effect for creative rhythmic manipulation.

#### Algorithm
- **Type**: Buffer capture and repeat
- **Capture**: On-demand buffer freeze
- **Playback**: Forward/reverse/pingpong
- **Quantization**: Beat-synced capture

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Buffer Size | 0.0-1.0 | 0.5 | Capture length (1/16 to 4 beats) |
| 1 | Repeat Rate | 0.0-1.0 | 0.5 | Playback rate (1/4 to 4x) |
| 2 | Playback Mode | 0.0-1.0 | 0.0 | Forward/reverse/pingpong |
| 3 | Trigger | 0.0-1.0 | 0.0 | Manual trigger |
| 4 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 64 samples (~1.3 ms at 48 kHz)
- **CPU Usage**: 3.6% (buffer manipulation)
- **THD**: 0.012% (excellent)
- **Complexity**: LOW

#### Special Considerations
- Real-time buffer capture enables live performance effects
- Beat sync requires host tempo information
- Reverse/pingpong modes create interesting rhythmic textures
- Suitable for electronic music, DJ effects, creative mixing

#### Status
✅ **Production Ready** - Creative buffer effects
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 39: Convolution Reverb

#### Purpose
High-quality convolution reverb using impulse responses of real spaces.

#### Algorithm
- **Type**: FFT-based convolution
- **Partitioning**: Partitioned convolution for efficiency
- **IR Loading**: Loads custom impulse responses
- **FFT Size**: 2048-8192 samples

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Mix | 0.0-1.0 | 0.4 | Dry/wet balance |
| 1 | Pre-delay | 0.0-1.0 | 0.1 | Pre-delay time (0-200 ms) |
| 2 | Decay | 0.0-1.0 | 0.7 | Early reflections decay scaling |
| 3 | Damping | 0.0-1.0 | 0.5 | High-frequency damping |
| 4 | Width | 0.0-1.0 | 0.8 | Stereo width control |
| 5 | ER/Tail | 0.0-1.0 | 0.5 | Early reflections vs tail balance |

#### Performance Characteristics
- **Expected Latency**: 4096 samples (~85.3 ms at 48 kHz, FFT window)
- **CPU Usage**: 68.9% (EXTREME - highest of all engines)
- **THD**: N/A (impulse response dependent)
- **Complexity**: EXTREME

#### Special Considerations
- ⚠️ **Parameter Validation Issues**: Test harness reports parameter problems
- **Extremely High CPU**: 68.9% makes concurrent usage difficult
- FFT-based processing provides accurate IR convolution
- Partitioned convolution reduces latency vs. direct convolution
- Quality depends on impulse response quality
- **Impulse Response Analysis**:
  - Stereo correlation: 0.005 (excellent stereo width)
  - Echo density: 6721/sec (very smooth, professional-grade)
  - RT60: ~2-3 seconds (good)
  - Pre-delay: 25.3 ms (natural)
- Suitable for mastering, post-production, realistic space simulation

#### Status
⚠️ **Needs Investigation** - Parameter issues (reverb quality is excellent)
⭐⭐⭐⭐ Very good quality

---

### Engine 40: Shimmer Reverb

#### Purpose
Reverb with octave-up pitch shifting for ethereal, shimmer effects.

#### Algorithm
- **Type**: Algorithmic reverb + pitch shifting
- **Pitch Shift**: +1 octave on reverb tail
- **Reverb**: FDN (Feedback Delay Network) base
- **Mixing**: Blend of dry reverb and pitched reverb

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Mix | 0.0-1.0 | 0.5 | Dry/wet balance |
| 1 | Size | 0.0-1.0 | 0.7 | Room size/decay time |
| 2 | Damping | 0.0-1.0 | 0.4 | High-frequency damping |
| 3 | Shimmer Amount | 0.0-1.0 | 0.5 | Pitch-shifted signal level |
| 4 | Pre-delay | 0.0-1.0 | 0.1 | Pre-delay time (0-200 ms) |
| 5 | Width | 0.0-1.0 | 0.7 | Stereo width |

#### Performance Characteristics
- **Expected Latency**: 1024 samples (~21.3 ms at 48 kHz)
- **CPU Usage**: 38.2% (reverb + pitch shifting)
- **THD**: N/A (reverb + pitch shift combination)
- **Complexity**: VERY_HIGH

#### Special Considerations
- ⚠️ **Stereo Width Issue**: Correlation 0.889 indicates nearly mono output (should be <0.5)
- **Recommended Fix**:
  - Check pitch shifter stereo processing
  - Verify separate L/R processing paths
  - Add stereo spread to pitched signal
- Pre-delay: 137ms is intentionally long for shimmer effect
- Echo density: 5413/sec (good diffusion)
- RT60: ~2-3 seconds (good)
- Creates ethereal, heavenly soundscapes
- Suitable for ambient, pads, guitars, cinematic production

#### Status
⚠️ **Fix Stereo Width** - Otherwise good quality
⭐⭐⭐ Good quality (needs stereo improvement)

---

### Engine 41: Plate Reverb

#### Purpose
Classic plate reverb emulation (EMT 140 style) for vintage character.

#### Algorithm
- **Type**: Schroeder-Moorer architecture
- **Topology**: Parallel comb filters + series all-pass
- **Modulation**: Vintage plate wobble simulation
- **Damping**: Frequency-dependent damping

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Mix | 0.0-1.0 | 0.5 | Dry/wet balance |
| 1 | Size | 0.0-1.0 | 0.6 | Plate size/decay (0.2-10s) |
| 2 | Damping | 0.0-1.0 | 0.5 | High-frequency damping |
| 3 | Pre-delay | 0.0-1.0 | 0.1 | Pre-delay time (0-200 ms) |
| 4 | Diffusion | 0.0-1.0 | 0.7 | Reflection density |
| 5 | Modulation Rate | 0.0-1.0 | 0.3 | Vintage wobble rate |
| 6 | Modulation Depth | 0.0-1.0 | 0.2 | Wobble amount |
| 7 | Low Cut | 0.0-1.0 | 0.1 | High-pass filter (20-1000 Hz) |
| 8 | High Cut | 0.0-1.0 | 0.9 | Low-pass filter (1-20 kHz) |
| 9 | Width | 0.0-1.0 | 0.8 | Stereo spread |

#### Performance Characteristics
- **Expected Latency**: 512 samples (~10.7 ms at 48 kHz)
- **CPU Usage**: 24.5% (algorithmic reverb)
- **THD**: N/A (cannot measure - broken)
- **Complexity**: HIGH

#### Special Considerations
- ❌ **CRITICAL ISSUE**: Zero output after 10ms - COMPLETELY BROKEN
- **Impulse Response Analysis**:
  - Peak: 0.767 at sample 0 (dry signal only)
  - Output goes to complete silence after 10ms
  - No reverb tail, no early reflections
  - Stereo correlation: 1.000 (mono, because only dry signal passes)
- **Likely Causes**:
  - Feedback coefficient set to 0.0 (no recirculation)
  - Comb/all-pass filters not initialized
  - Delay line buffer initialization error
  - wetLevel not being applied correctly
- **DO NOT SHIP** - This engine appears completely non-functional

#### Status
🔴 **BROKEN** - Zero reverb output
⭐ Poor quality (non-functional)

---

### Engine 42: Spring Reverb

#### Purpose
Physical modeling of spring reverb tank (guitar amp/surf rock style).

#### Algorithm
- **Type**: Physical modeling + dispersion network
- **Springs**: Multiple spring simulations
- **Dispersion**: Frequency-dependent delay
- **Character**: Boingy, metallic spring sound

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Mix | 0.0-1.0 | 0.5 | Dry/wet balance |
| 1 | Size | 0.0-1.0 | 0.5 | Spring tank length |
| 2 | Tension | 0.0-1.0 | 0.6 | Spring tension (affects tone) |
| 3 | Damping | 0.0-1.0 | 0.4 | Mechanical damping |
| 4 | Splash | 0.0-1.0 | 0.3 | Spring impact/splash amount |
| 5 | Width | 0.0-1.0 | 0.6 | Stereo width |

#### Performance Characteristics
- **Expected Latency**: 256 samples (~5.3 ms at 48 kHz)
- **CPU Usage**: 12.8% (physical modeling)
- **THD**: 0.056% (good)
- **Complexity**: MODERATE

#### Special Considerations
- **Impulse Response Analysis**:
  - Stereo correlation: 0.004 (excellent stereo width)
  - Echo density: 4998/sec (smooth diffusion)
  - RT60: ~1.5-2 seconds (appropriate for spring)
  - Pre-delay: 25.3 ms (natural)
- Physical modeling provides authentic spring character
- Boingy, metallic sound characteristic of vintage spring reverbs
- Suitable for guitars, surf rock, vintage production

#### Status
✅ **Production Ready** - Authentic spring reverb
⭐⭐⭐⭐ Very good quality

---

### Engine 43: Gated Reverb

#### Purpose
Classic 80s gated reverb with abrupt reverb cutoff for dramatic effect.

#### Algorithm
- **Type**: Algorithmic reverb + dynamics gate
- **Gate**: Envelope-controlled reverb cutoff
- **Hold**: Adjustable gate hold time
- **Release**: Fast gate release for tight sound

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Mix | 0.0-1.0 | 0.6 | Dry/wet balance |
| 1 | Size | 0.0-1.0 | 0.5 | Pre-gate reverb size |
| 2 | Gate Threshold | 0.0-1.0 | 0.5 | Gate threshold level |
| 3 | Hold Time | 0.0-1.0 | 0.3 | Gate hold time (50-500 ms) |
| 4 | Release | 0.0-1.0 | 0.2 | Gate release speed |
| 5 | Width | 0.0-1.0 | 0.7 | Stereo width |

#### Performance Characteristics
- **Expected Latency**: 512 samples (~10.7 ms at 48 kHz)
- **CPU Usage**: 21.7% (reverb + gate)
- **THD**: 0.041% (good)
- **Complexity**: HIGH

#### Special Considerations
- **Impulse Response Analysis**:
  - Stereo correlation: -0.001 (perfect stereo)
  - Echo density: 5588/sec (smooth)
  - RT60: 143ms (gated correctly for short, tight sound)
  - Gate action: Cuts at ~500ms (classic 80s gated reverb)
- Dramatic, punchy reverb sound popularized in 1980s
- Excellent for drums (snare especially), vocals
- Gate creates abrupt cutoff for rhythmic effect
- Suitable for pop, rock, electronic music

#### Status
✅ **Production Ready** - Classic gated reverb
⭐⭐⭐⭐ Very good quality

---

## Spatial & Special Effects (Engines 44-52)

### Engine 44: Stereo Widener

#### Purpose
Stereo width enhancement using Mid-Side processing and delay-based techniques.

#### Algorithm
- **Type**: Mid-Side matrix + Haas effect
- **Processing**: Amplify Side channel, delay L/R slightly
- **Safety**: Mono compatibility checks

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Width | 0.0-1.0 | 0.6 | Stereo width (0=mono, 1=ultra-wide) |
| 1 | Low Freq Limit | 0.0-1.0 | 0.2 | Frequency below which width is reduced |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 3.1% (simple M/S processing)
- **THD**: 0.008% (excellent)
- **Complexity**: LOW

#### Special Considerations
- Preserves mono compatibility at low frequencies (bass stays centered)
- Can create phase issues if over-used
- Suitable for mixing, mastering, adding width to narrow sources

#### Status
✅ **Production Ready** - Clean stereo widening
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 45: Stereo Imager

#### Purpose
Advanced stereo imaging with independent width control per frequency band.

#### Algorithm
- **Type**: Multi-band M/S processing
- **Bands**: 3-band frequency splitting
- **Width**: Independent width control per band

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Low Width | 0.0-1.0 | 0.3 | Low frequency width (20-250 Hz) |
| 1 | Mid Width | 0.0-1.0 | 0.6 | Mid frequency width (250-2000 Hz) |
| 2 | High Width | 0.0-1.0 | 0.8 | High frequency width (2000-20000 Hz) |
| 3 | Crossover 1 | 0.0-1.0 | 0.3 | Low/mid crossover frequency |
| 4 | Crossover 2 | 0.0-1.0 | 0.6 | Mid/high crossover frequency |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 5.7% (multi-band processing)
- **THD**: 0.019% (excellent)
- **Complexity**: MODERATE

#### Special Considerations
- Independent band control prevents bass phase issues
- Typical use: narrow bass, natural mids, wide highs
- Professional mastering tool
- Maintains mono compatibility

#### Status
✅ **Production Ready** - Professional stereo imaging
⭐⭐⭐⭐ Very good quality

---

### Engine 46: Dimension Expander

#### Purpose
Chorus-based spatial enhancement (Roland Dimension D style) for subtle width.

#### Algorithm
- **Type**: Multi-tap modulated delays
- **Taps**: 4 delay taps with phase-offset LFOs
- **Character**: Subtle pitch/time variation

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Mode | 0.0-1.0 | 0.5 | Dimension mode (1-4 buttons) |
| 1 | Intensity | 0.0-1.0 | 0.5 | Effect intensity |
| 2 | Width | 0.0-1.0 | 0.7 | Stereo spread |

#### Performance Characteristics
- **Expected Latency**: Variable (~5-15 ms, modulated delays)
- **CPU Usage**: 11.6% (multi-tap modulation)
- **THD**: 0.027% (good)
- **Complexity**: MODERATE

#### Special Considerations
- Famous for subtle, professional stereo enhancement
- Used extensively in mixing and mastering
- Multiple modes provide different character
- Suitable for vocals, keys, full mix

#### Status
✅ **Production Ready** - Subtle spatial enhancement
⭐⭐⭐⭐ Very good quality

---

### Engine 47: Spectral Freeze

#### Purpose
FFT-based freeze effect that captures and sustains spectral content.

#### Algorithm
- **Type**: STFT (Short-Time Fourier Transform) freeze
- **Capture**: Freeze magnitude spectrum, randomize phase
- **Sustain**: Infinite sustain with smooth crossfading

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Freeze | 0.0-1.0 | 0.0 | Freeze trigger (off/on) |
| 1 | Blur | 0.0-1.0 | 0.3 | Spectral smearing amount |
| 2 | Shimmer | 0.0-1.0 | 0.2 | Pitch shift on frozen spectrum |
| 3 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 1024-2048 samples (~21-43 ms, FFT window)
- **CPU Usage**: 31.4% (continuous FFT processing)
- **THD**: 0.067% (good)
- **Complexity**: VERY_HIGH

#### Special Considerations
- Creates ambient, sustaining pads from any source
- Phase randomization prevents tonal artifacts
- Shimmer adds pitch shifting to frozen spectrum
- Suitable for ambient, cinematic, experimental music

#### Status
✅ **Production Ready** - Creative spectral effect
⭐⭐⭐⭐ Very good quality

---

### Engine 48: Spectral Gate (Platinum)

#### Purpose
FFT-based noise gate that removes spectral content below threshold per bin.

#### Algorithm
- **Type**: FFT bin-wise gating
- **Gate**: Independent gate per frequency bin
- **Smoothing**: Inter-bin smoothing for musicality

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Threshold | 0.0-1.0 | 0.5 | Gate threshold level |
| 1 | Attack | 0.0-1.0 | 0.1 | Gate attack time |
| 2 | Release | 0.0-1.0 | 0.5 | Gate release time |
| 3 | Smoothing | 0.0-1.0 | 0.5 | Inter-bin smoothing |
| 4 | Mix | 0.0-1.0 | 1.0 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 1024 samples (~21 ms at 48 kHz)
- **CPU Usage**: 29.8% (FFT processing)
- **THD**: UNKNOWN (cannot measure - crashes)
- **Complexity**: VERY_HIGH

#### Special Considerations
- ❌ **CRITICAL ISSUE**: Engine crashes on startup
- Likely causes:
  - FFT buffer initialization failure
  - FFT library incompatibility
  - Null pointer in FFT setup
- **Recommended Fixes**:
  - Check FFT library compatibility
  - Verify buffer allocation
  - Add null pointer checks
  - Test with minimal FFT initialization
- Spectral gating provides frequency-selective noise reduction
- Useful for cleaning up recordings, creative effects

#### Status
❌ **FIX REQUIRED** - Crashes on startup
⭐ Poor quality (non-functional)

---

### Engine 49: Phased Vocoder

#### Purpose
Phase vocoder for time stretching and formant shifting without pitch change.

#### Algorithm
- **Type**: STFT phase vocoder
- **Time Stretch**: Independent time/pitch manipulation
- **Formant**: Formant preservation/shifting
- **Phase**: Phase-locked processing

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Time Stretch | 0.0-1.0 | 0.5 | Time scaling (0.5x to 2x) |
| 1 | Pitch Shift | 0.0-1.0 | 0.5 | Pitch shift (-12 to +12 semitones) |
| 2 | Formant | 0.0-1.0 | 0.5 | Formant shift (-12 to +12 semitones) |
| 3 | Quality | 0.0-1.0 | 0.7 | FFT size (quality vs latency) |

#### Performance Characteristics
- **Expected Latency**: 2048-4096 samples (~43-85 ms at 48 kHz)
- **CPU Usage**: 55.2% (second highest CPU usage)
- **THD**: 0.134% (acceptable for time stretching)
- **Complexity**: EXTREME

#### Special Considerations
- One of the most CPU-intensive engines (55.2%)
- High latency due to large FFT windows
- Independent time/pitch control for creative effects
- Formant preservation maintains natural vocal character
- Suitable for sound design, experimental production

#### Status
✅ **Production Ready** - Advanced vocoder (high CPU)
⭐⭐⭐⭐ Very good quality

---

### Engine 50: Granular Cloud

#### Purpose
Granular synthesis effect with cloud textures and grain manipulation.

#### Algorithm
- **Type**: Granular synthesis engine
- **Grains**: 32-128 simultaneous grains
- **Cloud**: Stochastic grain triggering
- **Parameters**: Grain size, density, pitch, position

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Grain Size | 0.0-1.0 | 0.5 | Grain duration (10-500 ms) |
| 1 | Density | 0.0-1.0 | 0.5 | Grains per second (1-200) |
| 2 | Pitch Variation | 0.0-1.0 | 0.3 | Random pitch range (±2 octaves) |
| 3 | Position | 0.0-1.0 | 0.5 | Grain position in buffer |
| 4 | Spread | 0.0-1.0 | 0.7 | Stereo spread of grains |
| 5 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: Variable (grain size dependent)
- **CPU Usage**: 35.6% (many simultaneous grains)
- **THD**: 0.156% (acceptable for granular)
- **Complexity**: VERY_HIGH

#### Special Considerations
- Stochastic grain triggering creates evolving textures
- Pitch variation creates rich, detuned clouds
- CPU usage varies with density parameter
- Suitable for ambient, sound design, experimental music

#### Status
✅ **Production Ready** - Creative granular synthesis
⭐⭐⭐⭐ Very good quality

---

### Engine 51: Chaos Generator

#### Purpose
Non-linear chaotic system for evolving textures and rhythmic patterns.

#### Algorithm
- **Type**: Lorenz attractor / chaotic oscillator
- **Equations**: Coupled differential equations
- **Mapping**: Map chaos to audio parameters
- **Filtering**: Post-processing filtering

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Chaos Amount | 0.0-1.0 | 0.5 | System sensitivity/chaos level |
| 1 | Rate | 0.0-1.0 | 0.3 | Evolution speed |
| 2 | Tone | 0.0-1.0 | 0.5 | Filtering/brightness |
| 3 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 8.4% (chaos calculations)
- **THD**: 0.234% (acceptable for generative effect)
- **Complexity**: MODERATE

#### Special Considerations
- Creates unpredictable, evolving patterns
- Non-repeating sequences for generative music
- Can be used as modulation source
- Suitable for experimental, ambient, glitch music

#### Status
✅ **Production Ready** - Unique generative textures
⭐⭐⭐⭐ Very good quality

---

### Engine 52: Feedback Network

#### Purpose
Complex feedback matrix for rich, evolving resonances.

#### Algorithm
- **Type**: Feedback Delay Network (FDN)
- **Matrix**: Householder or Hadamard feedback matrix
- **Delays**: Multiple delay lines with cross-feedback
- **Topology**: All-to-all feedback connections

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Feedback | 0.0-1.0 | 0.6 | Global feedback amount |
| 1 | Delay Time | 0.0-1.0 | 0.5 | Base delay times (10-500 ms) |
| 2 | Spread | 0.0-1.0 | 0.7 | Delay time variation |
| 3 | Damping | 0.0-1.0 | 0.4 | High-frequency damping |
| 4 | Mix | 0.0-1.0 | 0.5 | Dry/wet blend |

#### Performance Characteristics
- **Expected Latency**: Variable (delay times)
- **CPU Usage**: 16.9% (matrix calculations + delays)
- **THD**: 0.089% (good)
- **Complexity**: HIGH

#### Special Considerations
- Complex feedback creates rich, evolving textures
- Householder matrix ensures energy conservation
- Can be used for reverb-like effects or special effects
- Suitable for ambient, sound design, creative mixing

#### Status
✅ **Production Ready** - Complex feedback textures
⭐⭐⭐⭐ Very good quality

---

## Utility (Engines 53-56)

### Engine 53: Mid-Side Processor Platinum

#### Purpose
Professional Mid-Side encoding/decoding and independent M/S channel processing.

#### Algorithm
- **Type**: Mid-Side matrix transformation
- **Matrix**: L/R → M/S → process → L/R
- **Processing**: Independent gain/EQ per channel

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Mid Gain | 0.0-1.0 | 0.5 | Mid channel gain (-20 to +20 dB) |
| 1 | Side Gain | 0.0-1.0 | 0.5 | Side channel gain (-20 to +20 dB) |
| 2 | Width | 0.0-1.0 | 0.5 | Stereo width (0=mono, 1=wide) |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 1.2% (simple matrix math)
- **THD**: 0.000% (bit-perfect at unity)
- **Complexity**: LOW

#### Special Considerations
- Bit-perfect implementation
- Essential for mastering and mixing
- Independent M/S control enables surgical stereo adjustments
- Mono-compatible (can collapse to mono without issues)

#### Status
✅ **Production Ready** - Professional M/S processing
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 54: Gain Utility Platinum

#### Purpose
Precision gain control with dB or linear scaling and polarity inversion.

#### Algorithm
- **Type**: Simple multiplication
- **Precision**: 64-bit float internal calculation
- **Smoothing**: Parameter smoothing prevents clicks

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Gain | 0.0-1.0 | 0.5 | Gain amount (-60 to +20 dB) |
| 1 | Polarity | 0.0-1.0 | 0.0 | Phase invert on/off |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 0.5% (trivial processing)
- **THD**: 0.000% (bit-perfect)
- **Complexity**: MINIMAL

#### Special Considerations
- Bit-perfect at unity gain (0 dB)
- Useful for gain staging, trim, phase alignment
- Essential utility for signal routing

#### Status
✅ **Production Ready** - Bit-perfect gain control
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 55: Mono Maker Platinum

#### Purpose
Mono summing with adjustable frequency range for selective mono compatibility.

#### Algorithm
- **Type**: Channel summing with optional frequency filtering
- **Filtering**: High-pass filter to preserve stereo highs
- **Summing**: L+R / 2 (proper mono sum)

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Frequency Limit | 0.0-1.0 | 0.3 | Frequency below which mono is applied |
| 1 | Amount | 0.0-1.0 | 1.0 | Mono amount (0=stereo, 1=full mono) |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms)
- **CPU Usage**: 0.7% (minimal)
- **THD**: 0.000% (bit-perfect)
- **Complexity**: MINIMAL

#### Special Considerations
- Preserves stereo highs while making bass mono
- Prevents phase issues in bass frequencies
- Improves club/PA system compatibility
- Essential for mastering and mixing

#### Status
✅ **Production Ready** - Clean mono summing
⭐⭐⭐⭐⭐ Excellent quality

---

### Engine 56: Phase Align Platinum

#### Purpose
Phase alignment and correction using all-pass filters for time alignment.

#### Algorithm
- **Type**: All-pass filter cascade
- **Phase Shift**: Adjustable phase rotation (0-360°)
- **Per-Band**: Optional per-frequency-band phase correction

#### Parameters
| ID | Name | Range | Default | Description |
|----|------|-------|---------|-------------|
| 0 | Phase Shift | 0.0-1.0 | 0.0 | Global phase rotation (0-360°) |
| 1 | Auto-Align | 0.0-1.0 | 0.0 | Automatic phase alignment on/off |

#### Performance Characteristics
- **Expected Latency**: 0 samples (0 ms, all-pass is zero-latency)
- **CPU Usage**: 2.4% (all-pass filtering)
- **THD**: 0.000% (bit-perfect phase shift)
- **Complexity**: LOW

#### Special Considerations
- All-pass filters provide phase shift without magnitude change
- Useful for phase-aligning multi-mic recordings
- Corrects phase issues between tracks
- Essential for mixing and mastering

#### Status
✅ **Production Ready** - Precision phase alignment
⭐⭐⭐⭐⭐ Excellent quality

---

## Summary Statistics

### Overall Performance Profile

| Category | Engines | Avg CPU% | Avg THD% | Pass Rate |
|----------|---------|----------|----------|-----------|
| **Utility** | 4 | 1.2% | 0.000% | 100% |
| **Dynamics** | 6 | 6.3% | 0.047% | 83.3% |
| **Filters** | 8 | 6.6% | 0.039% | 87.5% |
| **Distortion** | 8 | 7.1% | 0.168% | 75.0% |
| **Spatial** | 9 | 6.8% | 0.056% | 77.8% |
| **Delay** | 5 | 7.2% | 0.029% | 100% |
| **Modulation** | 11 | 17.8% | 0.038% | 81.8% |
| **Reverb** | 5 | 33.2% | N/A | 80.0% |
| **Special** | 6 | 29.6% | 0.123% | 66.7% |

### Latency Profile

| Latency Category | Engine Count | Examples |
|------------------|--------------|----------|
| **Zero Latency (0 ms)** | 38 | Most filters, dynamics, modulation |
| **Ultra-Low (<5 ms)** | 8 | Digital delay, buffer repeat, tremolo |
| **Low (5-20 ms)** | 6 | BBD, spring reverb, magnetic drum |
| **Moderate (20-50 ms)** | 7 | Pitch shifters, granular, spectral |
| **High (50-100 ms)** | 3 | Convolution reverb, phased vocoder |

### CPU Usage Distribution

| CPU Category | Engine Count | % of Total |
|--------------|--------------|------------|
| **<1%** (Ultra-Efficient) | 3 | 5.3% |
| **1-5%** (Efficient) | 31 | 54.4% |
| **5-15%** (Moderate) | 12 | 21.1% |
| **15-30%** (High) | 4 | 7.0% |
| **30-50%** (Very High) | 4 | 7.0% |
| **>50%** (Extreme) | 3 | 5.3% |

### THD Quality Tiers

| THD Range | Quality | Engine Count |
|-----------|---------|--------------|
| **0.000%** (Bit-Perfect) | Excellent | 4 |
| **0.001-0.050%** | Excellent | 31 |
| **0.051-0.100%** | Very Good | 7 |
| **0.101-0.300%** | Good | 4 |
| **0.301-0.500%** | Acceptable | 0 |
| **>0.500%** | Needs Work | 3 |

---

## Production Readiness Summary

### ✅ Production Ready (46 engines - 82.1%)
All dynamics, filters (except Ladder), most modulation, all delays, most reverbs, most spatial, all utility

### ⚠️ Needs Minor Fixes (7 engines - 12.5%)
- Engine 6: Dynamic EQ (THD 0.759%)
- Engine 20: Muff Fuzz (CPU 5.19%)
- Engine 39: Convolution Reverb (parameter validation)
- Engine 40: Shimmer Reverb (stereo width)

### ❌ Critical Issues (4 engines - 7.1%)
- Engine 9: Ladder Filter (THD 3.512%)
- Engine 15: Vintage Tube Preamp (hangs/infinite loop)
- Engine 31/32: Pitch Shifters (THD 8.673%, crashes)
- Engine 41: Plate Reverb (zero output - broken)
- Engine 48: Spectral Gate (crashes on startup)

---

## Recommendations for Use

### Best Engines (Highest Quality/Performance Ratio)
1. **Gain Utility** (Engine 54) - Bit-perfect, 0.5% CPU
2. **Classic Tremolo** (Engine 29) - 0.018% THD, 2.1% CPU
3. **Vintage Opto Compressor** (Engine 1) - 0.016% THD, 2.1% CPU
4. **Digital Delay** (Engine 35) - 0.015% THD, 4.2% CPU
5. **Parametric EQ** (Engine 7) - 0.008% THD, 6.4% CPU

### Most CPU-Intensive (Use Sparingly)
1. **Convolution Reverb** (Engine 41) - 68.9% CPU
2. **Phased Vocoder** (Engine 49) - 55.2% CPU
3. **Intelligent Harmonizer** (Engine 33) - 52.8% CPU
4. **Pitch Shifter** (Engine 31) - 47.3% CPU
5. **Shimmer Reverb** (Engine 42) - 38.2% CPU

### Recommended for Real-Time Performance
All engines with <5% CPU and zero latency:
- Utility engines (53-56)
- Simple dynamics (1-5)
- Basic modulation (23, 24, 25, 28, 29)
- Simple filters (10, 13)

---

**Document Version**: 1.0
**Generated**: October 11, 2025
**Data Sources**: CPU_PERFORMANCE_REPORT.md, LATENCY_MEASUREMENT_SUITE.md, MASTER_QUALITY_REPORT.md, Engine source code analysis
**Test Platform**: macOS ARM64, 48kHz, 512-sample blocks
