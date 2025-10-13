# Parameter Definition Recommendations

## Engines Requiring Proper Parameter Definitions

The following 11 engines currently have placeholder parameter names ("Param 1", "Param 2", etc.) that need to be replaced with actual parameter specifications based on their DSP implementation.

---

## Engine 10: State Variable Filter

**Category:** Filter
**Current State:** 10 generic parameters
**Mix Index:** 9 (currently set to last parameter)

**Recommended Parameters:**
1. **Cutoff** - Filter cutoff frequency (20Hz - 20kHz)
2. **Resonance** - Filter resonance/Q factor (0 - 10)
3. **Mode** - Filter type (LP/HP/BP/Notch/All-pass)
4. **Drive** - Input saturation/overdrive (0 - 1)
5. **Tracking** - Keyboard/envelope tracking amount (0 - 1)
6. **Slope** - Filter slope (12dB/24dB/48dB per octave)
7. **Morph** - Blend between different modes (0 - 1)
8. **Stereo** - Stereo separation/offset (0 - 1)
9. **Mix** - Dry/wet mix (0 - 1)

**Notes:** State variable filters are versatile and can morph between different types. This makes them great for creative filtering.

---

## Engine 11: Formant Filter

**Category:** Filter
**Current State:** 10 generic parameters
**Mix Index:** 5

**Recommended Parameters:**
1. **Vowel** - Formant selection (A/E/I/O/U vowel morphing) (0 - 1)
2. **Morph** - Blend between adjacent vowels (0 - 1)
3. **Frequency** - Overall formant center frequency shift (0 - 1)
4. **Resonance** - Formant peak emphasis (0 - 1)
5. **Formant Shift** - Shift formants up/down (preserving relationships) (-1 - 1)
6. **Gender** - Male/female formant character (-1 - 1)
7. **Nasality** - Nasal/throaty character (0 - 1)
8. **Clarity** - Formant definition vs smoothness (0 - 1)
9. **Tracking** - MIDI note tracking for pitch-aware formants (0 - 1)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Formant filters simulate vocal tract resonances, creating talking/singing effects.

---

## Engine 14: Vocal Formant Filter

**Category:** Filter
**Current State:** 10 generic parameters
**Mix Index:** 7

**Recommended Parameters:**
1. **Vowel Select** - Choose specific vowel (A/E/I/O/U) (0 - 4, stepped)
2. **Vowel Morph** - Continuous morphing between vowels (0 - 1)
3. **Gender** - Male to female voice character (-1 - 1)
4. **Age** - Child to adult voice character (0 - 1)
5. **Throat** - Throat/chest resonance (0 - 1)
6. **Nasality** - Nasal cavity resonance (0 - 1)
7. **Breathiness** - Air/breath noise component (0 - 1)
8. **Clarity** - Consonant definition (0 - 1)
9. **Expression** - Dynamic formant movement (0 - 1)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** More specialized vocal formants with emphasis on realistic speech characteristics.

---

## Engine 16: Wave Folder

**Category:** Distortion
**Current State:** 10 generic parameters
**Mix Index:** 7

**Recommended Parameters:**
1. **Fold Amount** - Number of wave folding iterations (0 - 10)
2. **Symmetry** - Positive/negative wave asymmetry (-1 - 1)
3. **Bias** - DC offset before folding (-1 - 1)
4. **Stages** - Number of folding stages (1 - 8, stepped)
5. **Curve** - Folding curve shape (linear/exponential/sine) (0 - 1)
6. **Pre-Gain** - Input gain before folding (0 - 2)
7. **Post-Gain** - Output gain after folding (0 - 2)
8. **Feedback** - Feedback amount for chaos (0 - 1)
9. **Tone** - Post-folding high-cut filter (0 - 1)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Wave folding creates metallic, bell-like harmonics distinct from clipping.

---

## Engine 18: Bit Crusher

**Category:** Distortion
**Current State:** 10 generic parameters
**Mix Index:** 3

**Recommended Parameters:**
1. **Bit Depth** - Resolution reduction (1 - 24 bits)
2. **Sample Rate** - Sample rate reduction (100Hz - 48kHz)
3. **Dither** - Dithering amount to reduce quantization noise (0 - 1)
4. **Jitter** - Sample rate jitter/instability (0 - 1)
5. **Downsample Mode** - Interpolation method (hold/linear/cubic) (0 - 2, stepped)
6. **Quantize Mode** - Quantization type (floor/round/truncate) (0 - 2, stepped)
7. **Anti-Alias** - Anti-aliasing filter strength (0 - 1)
8. **Tone** - Post-processing tone control (0 - 1)
9. **Drive** - Pre-emphasis before crushing (0 - 2)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Lo-fi digital degradation effect, popular for chip-tune and glitch styles.

---

## Engine 19: Multiband Saturator

**Category:** Distortion
**Current State:** 10 generic parameters
**Mix Index:** 6

**Recommended Parameters:**
1. **Low Drive** - Low frequency band saturation (0 - 10)
2. **Mid Drive** - Mid frequency band saturation (0 - 10)
3. **High Drive** - High frequency band saturation (0 - 10)
4. **Low Freq** - Low/mid crossover frequency (50Hz - 1kHz)
5. **High Freq** - Mid/high crossover frequency (1kHz - 10kHz)
6. **Saturation Type** - Saturation curve (soft/hard/tube/tape) (0 - 3, stepped)
7. **Harmonic Content** - Even/odd harmonic balance (-1 - 1)
8. **Auto Gain** - Automatic gain compensation (0 - 1)
9. **Stereo Link** - Link stereo channels (0 - 1)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Frequency-conscious saturation for surgical harmonic enhancement.

---

## Engine 25: Analog Phaser

**Category:** Modulation
**Current State:** 10 generic parameters
**Mix Index:** 7

**Recommended Parameters:**
1. **Rate** - LFO rate (0.01Hz - 20Hz)
2. **Depth** - Modulation depth (0 - 1)
3. **Stages** - Number of all-pass stages (2 - 12, stepped)
4. **Feedback** - Feedback amount for resonance (-1 - 1)
5. **Frequency** - Center frequency (100Hz - 10kHz)
6. **Spread** - Stereo stage offset (0 - 1)
7. **LFO Shape** - Waveform (sine/triangle/random) (0 - 2, stepped)
8. **Manual** - Manual sweep control (0 - 1)
9. **Resonance** - All-pass Q factor (0 - 10)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Classic analog-style phaser with warm, swooshing character.

---

## Engine 26: Ring Modulator

**Category:** Modulation
**Current State:** 10 generic parameters

**Recommended Parameters:**
1. **Carrier Freq** - Carrier oscillator frequency (20Hz - 10kHz)
2. **Carrier Fine** - Fine tuning for carrier (-100 - +100 cents)
3. **Tracking** - MIDI note tracking amount (0 - 1)
4. **Carrier Wave** - Carrier waveform (sine/square/saw/triangle) (0 - 3, stepped)
5. **Carrier PWM** - Pulse width modulation for square wave (0 - 1)
6. **Envelope** - Carrier envelope follower amount (0 - 1)
7. **LFO Rate** - Carrier frequency LFO (0.01Hz - 20Hz)
8. **LFO Depth** - LFO modulation depth (0 - 1)
9. **Output Gain** - Compensate for level changes (0 - 2)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Creates metallic, bell-like, or robotic tones through amplitude modulation.

---

## Engine 27: Frequency Shifter

**Category:** Modulation
**Current State:** 10 generic parameters
**Mix Index:** 2

**Recommended Parameters:**
1. **Shift Amount** - Frequency shift in Hz (-1000Hz - +1000Hz)
2. **Shift Coarse** - Coarse shift in octaves (-2 - +2 octaves)
3. **Shift Fine** - Fine shift in Hz (-10Hz - +10Hz)
4. **Feedback** - Feedback amount for resonance (0 - 1)
5. **Stereo Mode** - L/R processing (mono/stereo/ping-pong) (0 - 2, stepped)
6. **LFO Rate** - Shift modulation rate (0.01Hz - 20Hz)
7. **LFO Depth** - Shift modulation depth (0 - 1)
8. **Envelope Follow** - Envelope-controlled shifting (0 - 1)
9. **Formant Preserve** - Maintain formant structure (0 - 1)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Unlike pitch shifting, frequency shifting maintains inharmonic relationships, creating unique metallic/alien textures.

---

## Engine 36: Magnetic Drum Echo

**Category:** Delay
**Current State:** 10 generic parameters
**Mix Index:** 7

**Recommended Parameters:**
1. **Delay Time** - Echo delay time (10ms - 2000ms)
2. **Feedback** - Regeneration amount (0 - 1)
3. **Wow** - Slow speed variation (0 - 1)
4. **Flutter** - Fast speed variation (0 - 1)
5. **Wow Rate** - Speed of wow modulation (0.1Hz - 5Hz)
6. **Saturation** - Magnetic tape saturation (0 - 1)
7. **Low Cut** - Remove bass from repeats (20Hz - 500Hz)
8. **High Cut** - Darken repeats (1kHz - 20kHz)
9. **Age** - Tape wear simulation (0 - 1)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Simulates vintage rotating magnetic drum echo units (Echoplex, etc.).

---

## Engine 37: Bucket Brigade Delay

**Category:** Delay
**Current State:** 10 generic parameters
**Mix Index:** 5

**Recommended Parameters:**
1. **Delay Time** - Echo delay time (10ms - 600ms)
2. **Feedback** - Regeneration amount (0 - 1)
3. **Clock Rate** - BBD clock frequency (affects fidelity) (1kHz - 50kHz)
4. **Stages** - Number of BBD stages (128 - 4096, stepped)
5. **Companding** - Noise reduction amount (0 - 1)
6. **Tone** - Anti-aliasing/reconstruction filter (0 - 1)
7. **Modulation** - Chorus-like modulation depth (0 - 1)
8. **Mod Rate** - Modulation speed (0.1Hz - 10Hz)
9. **Drive** - Input overdrive (0 - 2)
10. **Mix** - Dry/wet mix (0 - 1)

**Notes:** Emulates analog BBD chip delays (Boss DM-2, MXR Carbon Copy) with warm, dark character.

---

## Implementation Priority

### High Priority (Most Used Categories)
1. **Engine 18: Bit Crusher** - Popular lo-fi effect
2. **Engine 25: Analog Phaser** - Classic modulation staple
3. **Engine 36: Magnetic Drum Echo** - Vintage delay character
4. **Engine 37: Bucket Brigade Delay** - Analog warmth

### Medium Priority (Specialized but Useful)
5. **Engine 10: State Variable Filter** - Versatile filtering
6. **Engine 16: Wave Folder** - Creative distortion
7. **Engine 19: Multiband Saturator** - Mastering tool
8. **Engine 26: Ring Modulator** - Sound design tool

### Lower Priority (Niche Effects)
9. **Engine 11: Formant Filter** - Vocal effects
10. **Engine 14: Vocal Formant Filter** - Specialized vocal
11. **Engine 27: Frequency Shifter** - Experimental

---

## Implementation Notes

For each engine, you'll need to:

1. **Consult DSP Implementation:** Match parameters to actual C++ code
2. **Test Ranges:** Verify min/max values make sense musically
3. **Set Defaults:** Choose musical starting points
4. **Add Units:** Specify Hz, dB, %, ms, etc.
5. **Set Skew:** Use 0.5 for linear, <0.5 for exponential (frequency)
6. **Write Descriptions:** Clear, concise explanations of sonic impact
7. **Update Mix Index:** Ensure it points to the correct Mix parameter

## Testing Checklist

After updating each engine:
- [ ] Run `validate_trinity_kb.py` to check for errors
- [ ] Verify parameter count matches array length
- [ ] Confirm mix_param_index points to Mix parameter
- [ ] Test default values produce usable sound
- [ ] Verify min/max ranges are musically useful
- [ ] Check that descriptions are clear and helpful
