# ChimeraPhoenix Engine Upgrade Plan

## Current Status
- **10 engines** (26%) are already professional quality (Score 80+)
- **18 engines** (47%) need minor upgrades (Score 60-79)
- **4 engines** (11%) need major upgrades (Score 40-59)
- **6 engines** (16%) need complete rewrites (Score <40)
- **Average Quality Score**: 64.3/100

## Upgrade Strategy

### 🔴 Priority 1: Complete Rewrites (Critical)
These engines are severely lacking and used frequently:

1. **ShimmerReverb** (Score: 20)
   - Missing: DC blocking, parameter smoothing, dry/wet mix
   - Upgrade: Implement dual pitch-shifted reverb tails with spectral processing
   - Reference: Strymon BigSky, Eventide Space

2. **MasteringLimiter** (Score: 20)
   - Missing: DC blocking, parameter smoothing, lookahead
   - Upgrade: True-peak limiting with lookahead and soft knee
   - Reference: FabFilter Pro-L2, Waves L2

3. **TransientShaper** (Score: 20)
   - Missing: DC blocking, smoothing, attack/sustain detection
   - Upgrade: Proper envelope follower with attack/sustain control
   - Reference: SPL Transient Designer, Native Instruments Transient Master

4. **MidSideProcessor** (Score: 20)
   - Missing: Proper M/S encoding/decoding, width control
   - Upgrade: Full M/S matrix with independent processing
   - Reference: Waves S1, iZotope Ozone Imager

5. **ResonantChorus** (Score: 20)
   - Missing: DC blocking, smoothing, multiple voices
   - Upgrade: Multi-voice chorus with resonant feedback
   - Reference: Roland Dimension D, TC Electronic Corona

6. **SpectralFreeze** (Score: 35)
   - Missing: DC blocking, smoothing, proper FFT
   - Upgrade: Phase vocoder with bin freezing and spectral blur
   - Reference: Soundtoys Crystallizer, GRM Tools Freeze

### 🟡 Priority 2: Major Upgrades
These need significant improvements:

1. **ParametricEQ** (Score: 55)
   - Add: DC blocking, better filter topology
   - Upgrade to: State-variable filters with proper Q compensation

2. **HarmonicTremolo** (Score: 40)
   - Add: DC blocking, smoothing, proper tube emulation
   - Upgrade to: Fender Vibrolux-style harmonic tremolo

3. **DetuneDoubler** (Score: 45)
   - Add: DC blocking, micro-pitch variation
   - Upgrade to: Eventide H3000-style doubler

4. **SpectralGate** (Score: 45)
   - Add: DC blocking, frequency-dependent gating
   - Upgrade to: Multi-band gate with learn function

### 🟢 Priority 3: Minor Upgrades
These just need polish:

1. **VintageTube** (Score: 75)
   - Add: Better harmonic generation, tube types
   - Already has: DC blocking, smoothing, anti-aliasing

2. **TapeEcho** (Score: 60)
   - Add: Wow/flutter, tape saturation, multi-tap
   - Already has: Basic delay, filtering

3. **PlateReverb** (Score: 75)
   - Add: Proper allpass diffusion network
   - Already has: DC blocking, basic reverb

4. **StereoChorus** (Score: 60)
   - Add: Multiple LFO shapes, stereo spread
   - Already has: Basic chorus, smoothing

### ✅ Already Professional (Reference Quality)
These can serve as templates for other engines:

- **IntelligentHarmonizer** (100) - Granular synthesis, scale quantization
- **GranularCloud** (100) - Advanced granular processing
- **PitchShifter** (95) - High-quality pitch shifting
- **StereoImager** (95) - Professional stereo field control
- **BufferRepeat** (85) - Glitch/stutter effects
- **ClassicCompressor** (85) - Analog-modeled compression
- **ChaosGenerator** (80) - Creative modulation
- **NoiseGate** (80) - Transparent gating
- **DynamicEQ** (80) - Frequency-dependent dynamics
- **ClassicTremolo** (80) - Vintage amplitude modulation

## Implementation Plan

### Phase 1: Week 1-2
**Fix the broken essentials**
- [ ] ShimmerReverb - Complete rewrite with pitch-shifted tails
- [ ] MasteringLimiter - True-peak limiting with lookahead
- [ ] TransientShaper - Proper envelope detection
- [ ] MidSideProcessor - Full M/S matrix

### Phase 2: Week 2-3
**Upgrade core effects**
- [ ] ParametricEQ - State-variable filters
- [ ] PlateReverb - Allpass diffusion network
- [ ] TapeEcho - Multi-tap with modulation
- [ ] VintageTube - Enhanced harmonics

### Phase 3: Week 3-4
**Polish remaining effects**
- [ ] StereoChorus - Multi-voice implementation
- [ ] SpringReverb - Better spring modeling
- [ ] DimensionExpander - Haas effect enhancement
- [ ] All remaining minor upgrades

### Phase 4: Week 4-5
**Regenerate Golden Corpus**
- [ ] Test all upgraded engines
- [ ] Generate new presets with improved engines
- [ ] Update parameter documentation
- [ ] Validate with Trinity AI pipeline

## Technical Standards for All Engines

### Minimum Requirements
- [x] DC blocking on input/output
- [x] Parameter smoothing (20-100ms)
- [x] Dry/wet mix control
- [x] Thread-safe operation
- [x] Proper prepareToPlay()
- [x] Bounds checking on all parameters

### Professional Features
- [x] Anti-aliasing (where applicable)
- [x] Oversampling for nonlinear processing
- [x] Lock-free parameter updates
- [x] SIMD optimization where beneficial
- [x] Modular, reusable DSP components

### Code Quality
- [x] Clear documentation
- [x] Consistent naming conventions
- [x] Unit tests for DSP algorithms
- [x] Performance benchmarking

## Success Metrics
- All engines score 80+ on quality audit
- Zero crashes or audio glitches
- CPU usage <5% per engine at 48kHz
- Musical, immediately usable defaults
- Professional sound quality comparable to commercial plugins

## Next Steps
1. Start with ShimmerReverb complete rewrite
2. Use IntelligentHarmonizer as reference implementation
3. Create reusable DSP components library
4. Implement systematic testing framework