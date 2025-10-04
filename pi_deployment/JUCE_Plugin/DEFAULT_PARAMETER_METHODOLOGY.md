# Chimera Phoenix Default Parameter Methodology

## Philosophy & Core Principles

The Chimera Phoenix default parameter system is designed around the principle of **immediate musical satisfaction**. Every engine should sound inspiring and professional from the moment it loads, eliminating the barrier of parameter tweaking before users can be creative.

### 10 Core Design Principles

1. **Safety First**: All defaults avoid harsh, unusable, or potentially damaging sounds
2. **Musical Utility**: Each engine sounds good immediately upon loading  
3. **Moderate Values**: Most parameters start in the 0.3-0.7 range for balanced behavior
4. **Conservative Drive**: Drive/gain parameters start low (0.2-0.4) to prevent harshness
5. **Appropriate Mix**: Mix levels optimized per engine type for immediate impact
6. **Musical Timing**: Time-based effects use musical note values (1/16, 1/8, 1/4 notes)
7. **Controlled Feedback**: Conservative feedback (0.2-0.4) to avoid runaway conditions
8. **Smooth Resonance**: Moderate Q/resonance (0.3-0.5) for musicality without self-oscillation
9. **Unity Gain**: Output levels maintain consistent volume across engines
10. **First Impression**: Users should hear something inspiring immediately

## Methodology by Engine Category

### Distortion & Saturation Engines (100% Mix, 20-30% Drive)

**Philosophy**: Distortion engines should add character and warmth without being harsh or overwhelming. The goal is musical saturation that enhances rather than destroys the source material.

**Mix Strategy**: 80-100% wet for full character transformation
- Distortion effects work best when they replace rather than blend with the dry signal
- 100% wet allows users to hear the full character immediately
- Parallel blending can be achieved with external mixing

**Drive Strategy**: Conservative 20-40% for musical saturation
- Low drive prevents harsh digital clipping artifacts
- Allows tube/analog modeling to shine through
- Users can increase drive for more intensity as needed
- Prevents ear fatigue during extended sessions

**Examples**:
- **K-Style Overdrive**: Drive=0.3, Tone=0.5, Mix=1.0
  - Drive at 30% provides smooth Klon-style warmth without harshness
  - Tone centered for balanced frequency response
  - 100% wet to hear full overdrive character

- **Vintage Tube**: Drive=0.4, Warmth=0.3, Mix=1.0  
  - Higher drive acceptable due to softer tube saturation curve
  - Warmth at 30% for subtle harmonic enhancement
  - Full wet for complete tube preamp experience

### Reverb Engines (25-35% Mix, Medium Decay)

**Philosophy**: Reverbs should add space and dimension without making the mix muddy or distant. The goal is to enhance the source with appropriate spatial context.

**Mix Strategy**: 25-35% for noticeable but tasteful ambience
- Lower mix prevents the "swimming in reverb" effect
- Allows source material to remain present and defined
- Provides immediate spatial enhancement
- Professional mixing starting point

**Decay Strategy**: Medium decay times for musical sustainability
- Too short: Sounds unnatural and gated
- Too long: Creates muddy, unfocused mix
- Sweet spot: 0.4-0.6 range for most musical applications

**Examples**:
- **Plate Reverb**: Size=0.6, Decay=0.4, Mix=0.3
  - Size at 60% for medium plate character
  - Decay at 40% for controlled tail length
  - Mix at 30% for professional spatial enhancement

- **Shimmer Reverb**: Size=0.5, Shimmer=0.4, Mix=0.3
  - Moderate shimmer amount for ethereal character without artifacts
  - Conservative mix to prevent overwhelming pitch-shifted content

### Delay Engines (25-35% Mix, Musical Timing, 2-3 Repeats)

**Philosophy**: Delays should provide rhythmic enhancement and depth without cluttering the mix. Musical timing relationships are essential for groove and pocket.

**Mix Strategy**: 25-35% for rhythmic support without competition
- Delays work best as supporting elements
- Too much mix creates confusion with main rhythm
- Enough presence to add groove and interest

**Timing Strategy**: Musical note values based on 120 BPM
- 1/16 note (93.75ms): Tight rhythmic enhancement
- 1/8 note (187.5ms): Classic delay timing  
- 1/4 note (375ms): Spacious echo effect
- Based on 60,000ms ÷ BPM ÷ subdivision

**Feedback Strategy**: 2-3 repeats maximum for clarity
- 0.2-0.4 range provides musical repetition
- Prevents infinite feedback scenarios
- Maintains mix clarity and definition

**Examples**:
- **Tape Echo**: Time=0.375 (1/8 note), Feedback=0.3, Mix=0.5
  - Classic 1/8 note delay timing for versatility
  - Moderate feedback for 2-3 clear repeats
  - Higher mix acceptable due to tape warmth and filtering

- **Digital Delay**: Time=0.375, Feedback=0.25, Mix=0.4  
  - Same timing but lower feedback for cleaner repeats
  - Lower mix due to pristine digital character

### Modulation Engines (30-50% Mix, Subtle Movement, 2-5Hz)

**Philosophy**: Modulation should add life and movement without creating nausea or disorientation. The goal is subtle animation that enhances musicality.

**Rate Strategy**: Musical rates synchronized to human perception
- 0.2-0.4 range corresponds to 2-5Hz modulation
- Avoids nauseating frequencies (6-10Hz)
- Matches natural vibrato and tremolo rates
- Can be increased for special effects

**Depth Strategy**: Noticeable but not overwhelming movement
- 0.3-0.5 range for engaging modulation
- Prevents seasickness effect from excessive depth
- Allows source material to remain recognizable
- Musical sweet spot for most applications

**Mix Strategy**: Balanced blend for movement without artifacts
- 30-50% mix shows clear modulation character
- Prevents phase cancellation issues
- Maintains source definition

**Examples**:
- **Classic Tremolo**: Rate=0.4 (~4Hz), Depth=0.4, Mix=1.0
  - Classic tremolo rate for musical pulsing
  - Moderate depth for engaging amplitude modulation
  - Full wet for traditional tremolo effect

- **Stereo Chorus**: Rate=0.4, Depth=0.4, Mix=0.5
  - Musical chorus rate for lush movement
  - Balanced mix to avoid phase issues

### Filter Engines (Variable Mix, Midrange Cutoff, Musical Resonance)

**Philosophy**: Filters should shape tone and character without being harsh or self-oscillating. The goal is musical filtering that enhances rather than destroys harmonic content.

**Cutoff Strategy**: Midrange starting point for versatility
- 0.4-0.6 range places cutoff around 1kHz
- Allows both low-pass and high-pass movements
- Preserves most of the audible spectrum initially
- Natural starting point for filter sweeps

**Resonance Strategy**: Musical enhancement without self-oscillation
- 0.0-0.4 range for controlled resonance
- Adds character without harsh peaks
- Prevents filter from overwhelming the signal
- Allows musical filter sweeps

**Mix Strategy**: Depends on filter type and purpose
- Creative filters: 50-70% for blending
- Corrective filters: 100% for full processing
- Effects filters: Variable based on desired impact

**Examples**:
- **Ladder Filter**: Cutoff=0.6, Resonance=0.4, Mix=1.0
  - Upper midrange cutoff for natural sound
  - Moderate resonance for Moog-style character
  - Full wet for complete filter processing

### Dynamic Engines (100% Mix, Musical Ratios, Transparent Operation)

**Philosophy**: Dynamics processors should enhance and control without being obvious or pumping. The goal is transparent level control that improves the mix.

**Mix Strategy**: 100% wet for full dynamic control
- Dynamics processors replace rather than blend
- Parallel compression achieved through external routing
- Full processing ensures consistent level control

**Threshold Strategy**: Musical compression amounts
- 0.4-0.7 range for moderate gain reduction
- Prevents over-compression and pumping
- Maintains natural dynamics while controlling peaks
- Professional mixing starting point

**Ratio Strategy**: Musical compression ratios
- 0.3-0.5 range corresponds to 3:1 to 6:1 ratios
- Sweet spot for transparent compression
- Avoids brick-wall limiting effects
- Maintains musical dynamics

**Attack/Release Strategy**: Transparent operation
- Fast attack (0.1-0.3) for peak control
- Medium release (0.3-0.6) for natural decay
- Prevents pumping and breathing artifacts

**Examples**:
- **VCA Compressor**: Threshold=0.4, Ratio=0.5, Attack=0.2, Release=0.4
  - Moderate threshold for musical compression
  - 4:1 ratio for controlled dynamics
  - Fast attack for peak control
  - Medium release for natural sound

### Spatial Engines (Variable Mix, Balanced Processing)

**Philosophy**: Spatial processors should enhance the stereo image and placement without creating phase issues or unnatural positioning.

**Mix Strategy**: Depends on processing type
- Widening effects: 50-70% to avoid phase issues
- M/S processing: 100% for full control
- Positioning effects: Variable based on desired placement

**Processing Strategy**: Conservative enhancement
- Avoid extreme width settings that cause mono compatibility issues
- Maintain center image stability
- Enhance rather than replace natural stereo information

**Examples**:
- **Stereo Widener**: Width=0.5, Bass Mono=0.5, Mix=1.0
  - Moderate widening for enhanced stereo image
  - Bass mono to maintain low-end stability
  - Full processing for complete width control

### Utility Engines (100% Mix, Unity Gain, Neutral Settings)

**Philosophy**: Utility processors should start in neutral positions that don't alter the signal, allowing users to make conscious adjustments.

**Strategy**: Neutral starting points for all parameters
- Gain controls at unity (0.5 = 0dB)
- Processing controls at neutral positions
- Mode switches at default positions
- 100% mix for full utility processing

**Examples**:
- **Gain Utility**: All gains=0.5 (0dB), Mode=0 (Stereo)
  - Unity gain starting point
  - Standard stereo mode
  - No phase inversions or channel swaps

### Experimental/Spectral Engines (20-30% Mix, Conservative Settings)

**Philosophy**: Experimental and spectral processors have the potential for extreme effects that can be unusable or harsh. Conservative defaults allow users to explore safely.

**Mix Strategy**: Low mix for safe exploration
- 20-30% mix prevents overwhelming effects
- Allows users to hear the character without extremes
- Can be increased once users understand the effect

**Processing Strategy**: Conservative parameter values
- Lower intensity settings for safe starting points
- Avoid parameter combinations that create artifacts
- Enable exploration without immediate harshness

**Examples**:
- **Spectral Freeze**: Freeze=0.0, Blend=0.5, Mix=0.3
  - Not frozen initially (normal operation)
  - Balanced blend when freezing is engaged
  - Low mix for subtle spectral effects

## Testing Methodology

### Safety Testing
1. **Peak Level Testing**: Ensure no defaults exceed 0dB output
2. **Frequency Response**: Verify no harsh resonant peaks
3. **Stability Testing**: Confirm no runaway feedback conditions
4. **Phase Correlation**: Check stereo compatibility

### Musical Testing  
1. **Genre Testing**: Validate across multiple musical styles
2. **Instrument Testing**: Test with various source material
3. **Context Testing**: Verify defaults work in mix context
4. **User Testing**: Gather feedback from actual users

### Technical Validation
1. **Parameter Range Validation**: Ensure all values within 0.0-1.0
2. **Engine Coverage**: Confirm defaults for all 57 engines
3. **Performance Testing**: Verify no CPU spikes during default loading
4. **Memory Testing**: Check for memory leaks during engine switching

## Maintenance Guidelines

### When Adding New Engines
1. Identify engine category and follow established patterns
2. Test extensively for safety and musical utility
3. Document reasoning for parameter choices
4. Validate with multiple source materials
5. Update category guidelines if needed

### When Modifying Existing Defaults
1. Document reason for change
2. Test impact on existing user workflows
3. Validate safety and musical utility
4. Consider version compatibility implications
5. Update methodology documentation

### Regular Review Process
1. Quarterly review of user feedback
2. Annual testing with new musical styles
3. Technology updates requiring parameter adjustments
4. Competitive analysis and industry standard alignment

This methodology ensures that Chimera Phoenix maintains its reputation for professional, musical, and inspiring default parameter values that enhance creativity rather than hinder it.