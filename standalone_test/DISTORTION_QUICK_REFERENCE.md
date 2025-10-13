# DISTORTION ENGINES - QUICK REFERENCE GUIDE

## At-a-Glance Parameter Reference

### ENGINE 15: Vintage Tube Preamp Studio (14 params)
```
[0] Bypass        → 0/1 (bool)
[1] Voicing       → 0=Vox AC30, 1=Fender Deluxe, 2=Marshall Plexi
[2] Input Trim    → -24 to +24 dB
[3] Output Trim   → -24 to +24 dB
[4] Drive         → 0-1 (grid bias + plate voltage)
[5] Bright        → 0-1 (bright cap at V1 plate)
[6] Bass          → 0-1 (±15 dB @ voicing-dependent freq)
[7] Mid           → 0-1 (±10 dB @ voicing-dependent freq)
[8] Treble        → 0-1 (±15 dB @ voicing-dependent freq)
[9] Presence      → 0-1 (NFB HF reduction)
[10] Microphonics → 0-1 (mechanical coupling)
[11] Ghost Notes  → 0-1 (secondary resonance)
[12] Noise        → 0-1 (hiss/hum injection)
[13] OS Mode      → 0=Auto, 1=On, 2=Off
```

**Drive Curve:** Exponential, soft knee
**Oversampling:** 4× polyphase (excellent anti-aliasing)
**CPU:** Very High

---

### ENGINE 16: Wave Folder (8 params)
```
[0] Fold Amount   → 0-1 (folding intensity, 0-4 iterations)
[1] Asymmetry     → 0-1 (pos/neg threshold difference)
[2] DC Offset     → 0-1 (0.5=center, ±0.1 max)
[3] Pre Gain      → 0-1 (+0 to +9.5 dB)
[4] Post Gain     → 0-1 (+0 to +3.5 dB)
[5] Smoothing     → 0-1 (anti-derivative AA)
[6] Harmonics     → 0-1 (emphasis @ 1.5k, 2.5k, 3.5k)
[7] Mix           → 0-1 (dry/wet)
```

**Drive Curve:** Threshold-based iterative
**Oversampling:** 4× polyphase (64-tap Kaiser)
**CPU:** Medium-High

---

### ENGINE 17: Harmonic Exciter Platinum (8 params)
```
[0] Frequency     → 0-1 (1 kHz → 10 kHz, log scale)
[1] Drive         → 0-1 (harmonic generation amount)
[2] Harmonics     → 0-1 (even vs odd balance)
[3] Clarity       → 0-1 (phase coherence)
[4] Warmth        → 0-1 (+0 to +6 dB @ 100 Hz)
[5] Presence      → 0-1 (+0 to +6 dB @ 8 kHz)
[6] Color         → 0-1 (0=tube/even, 1=transistor/odd)
[7] Mix           → 0-1 (dry/wet)
```

**Drive Curve:** Per-band scaling (freq-dependent)
**Oversampling:** 2× per-band (dynamic engagement)
**CPU:** High

---

### ENGINE 18: Bit Crusher (3 params)
```
[0] Bits          → 0-1 (24/12/8/4/1 bit, stepped)
                    0.0-0.2 = 24-bit (clean)
                    0.2-0.4 = 12-bit (vintage sampler)
                    0.4-0.6 = 8-bit (classic)
                    0.6-0.8 = 4-bit (crunchy)
                    0.8-1.0 = 1-bit (destroyed)
[1] Downsample    → 0-1 (1×/2×/4×/8×/16×, stepped)
                    0.0-0.2 = 1× (no downsample)
                    0.2-0.4 = 2× (half rate)
                    0.4-0.6 = 4× (quarter rate)
                    0.6-0.8 = 8× (1/8 rate)
                    0.8-1.0 = 16× (1/16 rate)
[2] Mix           → 0-1 (dry/wet)
```

**Drive Curve:** Stepped quantization (intentional)
**Oversampling:** None (aliasing is the feature)
**CPU:** Very Low

---

### ENGINE 19: Multiband Saturator (7 params)
```
[0] Low Drive     → 0-1 (low band saturation)
[1] Mid Drive     → 0-1 (mid band saturation)
[2] High Drive    → 0-1 (high band saturation)
[3] Saturation    → 0-1 (type selection)
    Type          → 0.00-0.25 = Tube (soft, even)
                    0.25-0.50 = Tape (smooth compression)
                    0.50-0.75 = Transistor (harder, odd)
                    0.75-1.00 = Diode (extreme)
[4] Harmonics     → 0-1 (adds 2nd + 3rd harmonics)
[5] Output Gain   → 0-1 (0.1× to 1.9×, unity @ 0.5)
[6] Mix           → 0-1 (dry/wet)
```

**Drive Curve:** Per-band, type-dependent (1× to 20×)
**Oversampling:** None (aliasing possible at high drive)
**CPU:** Medium

---

### ENGINE 20: Muff Fuzz (7 params)
```
[0] Sustain       → 0-1 (gain: 1× → 100×, exponential)
[1] Tone          → 0-1 (0=bass, 1=treble, Big Muff stack)
[2] Volume        → 0-1 (output level: 0× → 2×)
[3] Gate          → 0-1 (noise gate threshold)
[4] Mids          → 0-1 (mid scoop depth @ 750 Hz)
[5] Variant       → 0-1 (6 variants, stepped)
                    0.00-0.17 = Triangle 1971 (warm, loose)
                    0.17-0.33 = Ram's Head 1973 (classic)
                    0.33-0.50 = NYC Reissue (modern, tight)
                    0.50-0.67 = Russian Sovtek (cold, aggressive)
                    0.67-0.83 = Op-Amp Version (clean)
                    0.83-1.00 = Modern Deluxe (precision)
[6] Mix           → 0-1 (dry/wet)
```

**Drive Curve:** Exponential (massive gain range)
**Oversampling:** 4× Butterworth (DISABLED in code)
**CPU:** High (even without oversampling)

---

### ENGINE 21: Rodent Distortion (8 params)
```
[0] Gain          → 0-1 (+0 to +60 dB, log scale)
[1] Filter        → 0-1 (60 Hz → 5 kHz pre-distortion HP)
[2] Clipping      → 0-1 (circuit-dependent clipping depth)
[3] Tone          → 0-1 (500 Hz → 12 kHz post-distortion LP)
[4] Output        → 0-1 (0× → 1.5× gain)
[5] Mix           → 0-1 (dry/wet)
[6] Mode          → 0-1 (circuit selection)
                    0.00-0.25 = RAT (ProCo RAT, aggressive)
                    0.25-0.50 = Tube Screamer (mid-focused)
                    0.50-0.75 = Big Muff (thick, sustaining)
                    0.75-1.00 = Fuzz Face (dynamic, vintage)
[7] Presence      → 0-1 (high-frequency emphasis)
```

**Drive Curve:** Logarithmic dB scale (0-60 dB)
**Oversampling:** 4× with 8th order elliptic (excellent)
**CPU:** Very High

---

### ENGINE 22: K-Style Overdrive (4 params)
```
[0] Drive         → 0-1 (+0 to +15 dB pre-gain)
[1] Tone          → 0-1 (tilt: 0=dark/LP, 1=bright/HP @ 1kHz)
[2] Level         → 0-1 (-12 to +12 dB output)
[3] Mix           → 0-1 (dry/wet)
```

**Drive Curve:** Simple exponential (transparent)
**Oversampling:** 2× with TPT filter (good)
**CPU:** Low

---

## Parameter Behavior Quick Chart

| Engine | Drive Type | Drive Range | Tone Centers | Oversampling | CPU |
|--------|------------|-------------|--------------|--------------|-----|
| 15. Tube | Exponential | Bias+Plate | Voice-dep | 4× Polyphase | Very High |
| 16. Folder | Iterative | 0-4 folds | 1.5k/2.5k/3.5k | 4× Kaiser | Med-High |
| 17. Exciter | Per-band | Freq-dep | 800Hz/5kHz xover | 2× Dynamic | High |
| 18. Crusher | Stepped | 1-24 bit | None | None | Very Low |
| 19. Multiband | Per-band | 1-20× | 200Hz/2kHz xover | None | Medium |
| 20. Muff | Exponential | 1-100× | Big Muff stack | 4× (disabled) | High |
| 21. Rodent | Logarithmic | 0-60 dB | Mode-dep | 4× Elliptic | Very High |
| 22. K-Style | Simple exp | 0-15 dB | 1 kHz tilt | 2× TPT | Low |

---

## Harmonic Character Reference

```
Even Harmonic Dominant (Warm/Tube-like):
  - Vintage Tube Preamp (70% even)
  - Harmonic Exciter - Tube Mode (65% even)

Balanced (Versatile):
  - Rodent - RAT Mode (45% even / 55% odd)
  - Multiband Saturator - Tube/Tape (adjustable)

Odd Harmonic Dominant (Aggressive/Fuzzy):
  - Wave Folder (80% odd)
  - Muff Fuzz (70% odd)
  - K-Style Overdrive (70% odd)
  - Rodent - Fuzz Face (60% odd)
```

---

## Oversampling Quality Rankings

```
1. Vintage Tube:    4× Polyphase Halfband       <-100 dB aliasing
2. Rodent:          4× 8th Order Elliptic       <-80 dB aliasing
3. Wave Folder:     4× 64-tap Kaiser            <-80 dB aliasing
4. Harmonic Exciter: 2× Per-band Linear         <-60 dB aliasing
5. K-Style:         2× TPT Filter               <-60 dB aliasing
6. Muff Fuzz:       4× Butterworth (disabled)   Moderate aliasing
7. Multiband Sat:   None                        Aliasing possible
8. Bit Crusher:     None                        Aliasing intentional
```

---

## When to Use Which Engine

**Clean to Warm Enhancement:**
- Harmonic Exciter (Color=0, low Drive)
- Vintage Tube Preamp (low Drive, Vox voicing)

**Transparent Overdrive:**
- K-Style Overdrive
- Vintage Tube Preamp (Fender voicing, moderate Drive)

**Classic Distortion Pedals:**
- Rodent - RAT Mode (aggressive rock)
- Rodent - Tube Screamer (blues/rock)

**Heavy Fuzz:**
- Muff Fuzz (sustaining lead tones)
- Rodent - Fuzz Face (dynamic rhythm)
- Rodent - Big Muff (thick rhythm)

**Surgical Shaping:**
- Multiband Saturator (frequency-specific saturation)
- Harmonic Exciter (multiband enhancement)

**Creative/Extreme:**
- Wave Folder (metallic, folded harmonics)
- Bit Crusher (lo-fi digital)

**Maximum Authenticity:**
- Vintage Tube Preamp (real amp simulation)
- Muff Fuzz (with variant selection)
- Rodent (4 classic pedals in one)

---

## CPU Budget Planning

```
Low CPU Session (8+ instances):
  - K-Style Overdrive
  - Bit Crusher

Medium CPU Session (4-6 instances):
  - Multiband Saturator
  - Wave Folder

High CPU Session (2-4 instances):
  - Harmonic Exciter
  - Muff Fuzz

Very High CPU Session (1-2 instances):
  - Vintage Tube Preamp
  - Rodent Distortion
```

---

## Preset Starting Points

### Vintage Tube - Clean Fender
```
Drive=0.2, Voicing=1 (Fender), Bright=0.3, Bass=0.5, Mid=0.5, Treble=0.6
```

### Vintage Tube - Crunchy Marshall
```
Drive=0.6, Voicing=2 (Marshall), Bass=0.4, Mid=0.7, Treble=0.5, Presence=0.6
```

### Muff Fuzz - Classic Lead
```
Sustain=0.7, Tone=0.6, Volume=0.5, Variant=2 (NYC), Mids=0.3
```

### Rodent - Modern Rock
```
Gain=0.6, Mode=0 (RAT), Clipping=0.5, Tone=0.4, Output=0.5, Presence=0.4
```

### Harmonic Exciter - Tape Warmth
```
Drive=0.3, Color=0 (Tube), Warmth=0.6, Presence=0.3, Mix=0.5
```

### Wave Folder - Synth Bass
```
Fold=0.6, Asymmetry=0.3, Pre Gain=0.7, Harmonics=0.5
```

### Multiband - Mastering Glue
```
Low=0.3, Mid=0.2, High=0.3, Type=0.15 (Tape), Harmonics=0.2, Mix=0.3
```

### K-Style - Transparent Boost
```
Drive=0.3, Tone=0.5, Level=0.7, Mix=1.0
```

---

**Document Version:** 1.0
**Last Updated:** 2025-10-11
**Source:** ChimeraPhoenix v3.0 Validation
