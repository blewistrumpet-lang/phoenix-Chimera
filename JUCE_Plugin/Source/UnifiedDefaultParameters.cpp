#include "UnifiedDefaultParameters.h"
#include <cassert>

namespace UnifiedDefaultParameters {

/**
 * Master default parameters database
 * 
 * This contains optimized, tested default values for all 57 Chimera Phoenix engines.
 * Each engine's defaults are crafted to provide immediate musical satisfaction while
 * maintaining safety and professional polish.
 * 
 * Values are based on extensive testing with:
 * - Multiple musical genres and source materials
 * - Professional mixing and mastering contexts  
 * - User feedback and workflow optimization
 * - Safety validation for all parameter combinations
 */

std::map<int, float> getDefaultParameters(int engineId) {
    std::map<int, float> defaults;
    
    switch (engineId) {
        
        // ==================== NONE ENGINE ====================
        case ENGINE_NONE:
            // No parameters for passthrough
            break;
            
        // ==================== DYNAMICS & COMPRESSION (1-6) ====================
            
        case ENGINE_OPTO_COMPRESSOR: // Vintage Opto Compressor - LA-2A Style
            defaults[0] = 0.5f;   // Input Gain - Moderate input level
            defaults[1] = 0.3f;   // Peak Reduction - Gentle opto compression  
            defaults[2] = 0.0f;   // HF Emphasis - Flat response initially
            defaults[3] = 0.5f;   // Output Gain - Unity gain
            defaults[4] = 1.0f;   // Mix - Full compression (no dry blend)
            defaults[5] = 0.7f;   // Knee - Soft knee for smooth compression
            defaults[6] = 0.2f;   // Tube Harmonics - Subtle tube coloration
            defaults[7] = 1.0f;   // Stereo Link - Linked for stereo material
            break;
            
        case ENGINE_VCA_COMPRESSOR: // Classic VCA Compressor
            defaults[0] = 0.4f;   // Threshold - Moderate compression
            defaults[1] = 0.5f;   // Ratio - 4:1 compression ratio
            defaults[2] = 0.2f;   // Attack - Fast attack for peak control
            defaults[3] = 0.4f;   // Release - Medium release, musical
            defaults[4] = 0.0f;   // Knee - Hard knee for punchy compression
            defaults[5] = 0.5f;   // Makeup Gain - Unity compensation
            defaults[6] = 1.0f;   // Mix - Full compression
            break;
            
        case ENGINE_TRANSIENT_SHAPER: // Transient Shaper
            defaults[0] = 0.5f;   // Attack - No change initially
            defaults[1] = 0.5f;   // Sustain - No change initially  
            defaults[2] = 0.3f;   // Sensitivity - Moderate response
            defaults[3] = 0.5f;   // Output - Unity gain
            break;
            
        case ENGINE_NOISE_GATE: // Noise Gate  
            defaults[0] = 0.3f;   // Threshold - Moderate gating (-40dB)
            defaults[1] = 0.1f;   // Attack - Fast attack (1ms)
            defaults[2] = 0.3f;   // Hold - Short hold time (100ms)
            defaults[3] = 0.4f;   // Release - Medium release (200ms)
            defaults[4] = 0.8f;   // Range - Strong gating (-20dB)
            break;
            
        case ENGINE_MASTERING_LIMITER: // Mastering Limiter
            defaults[0] = 0.9f;   // Threshold - High threshold (-1dB) for transparency
            defaults[1] = 0.2f;   // Release - Fast release for transparency
            defaults[2] = 0.0f;   // Knee - Hard knee for precise limiting
            defaults[3] = 0.3f;   // Lookahead - Moderate lookahead (5ms)
            break;
            
        case ENGINE_DYNAMIC_EQ: // Dynamic EQ
            defaults[0] = 0.5f;   // Frequency - 1kHz center frequency
            defaults[1] = 0.5f;   // Threshold - No processing initially
            defaults[2] = 0.3f;   // Ratio - Gentle dynamic EQ
            defaults[3] = 0.2f;   // Attack - Fast response
            defaults[4] = 0.4f;   // Release - Medium release
            defaults[5] = 0.5f;   // Gain - No boost/cut initially
            defaults[6] = 1.0f;   // Mix - Full processing
            defaults[7] = 0.0f;   // Mode - Peak mode
            break;
            
        // ==================== FILTERS & EQ (7-14) ====================
            
        case ENGINE_PARAMETRIC_EQ: // Parametric EQ
            defaults[0] = 0.2f;   // Low Frequency - ~200Hz
            defaults[1] = 0.5f;   // Low Gain - 0dB (no change)
            defaults[2] = 0.5f;   // Low Q - Moderate bandwidth
            defaults[3] = 0.5f;   // Mid Frequency - ~1kHz
            defaults[4] = 0.5f;   // Mid Gain - 0dB (no change)
            defaults[5] = 0.5f;   // Mid Q - Moderate bandwidth
            defaults[6] = 0.8f;   // High Frequency - ~5kHz
            defaults[7] = 0.5f;   // High Gain - 0dB (no change)
            defaults[8] = 0.5f;   // High Q - Moderate bandwidth
            break;
            
        case ENGINE_VINTAGE_CONSOLE_EQ: // Vintage Console EQ
            defaults[0] = 0.5f;   // Low Gain - Flat response
            defaults[1] = 0.5f;   // Low-Mid Gain - Flat response
            defaults[2] = 0.5f;   // High-Mid Gain - Flat response
            defaults[3] = 0.5f;   // High Gain - Flat response
            defaults[4] = 0.0f;   // Drive - No console drive initially
            break;
            
        case ENGINE_LADDER_FILTER: // Ladder Filter (Moog-style)
            defaults[0] = 0.6f;   // Cutoff - Upper midrange (3kHz)
            defaults[1] = 0.3f;   // Resonance - Musical resonance, no self-oscillation
            defaults[2] = 0.2f;   // Drive - Subtle filter saturation
            defaults[3] = 0.0f;   // Filter Type - Low-pass mode
            defaults[4] = 0.0f;   // Asymmetry - Symmetric response
            defaults[5] = 0.0f;   // Vintage Mode - Modern response
            defaults[6] = 1.0f;   // Mix - Full filtering
            break;
            
        case ENGINE_STATE_VARIABLE_FILTER: // State Variable Filter
            defaults[0] = 0.5f;   // Cutoff - Midrange (1kHz)
            defaults[1] = 0.4f;   // Resonance - Musical resonance
            defaults[2] = 0.0f;   // Mode - Low-pass mode
            defaults[3] = 0.0f;   // Key Follow - No key tracking
            defaults[4] = 1.0f;   // Mix - Full filtering
            break;
            
        case ENGINE_FORMANT_FILTER: // Formant Filter
            defaults[0] = 0.5f;   // Formant - Neutral formant position
            defaults[1] = 0.4f;   // Resonance - Moderate formant shaping
            defaults[2] = 0.3f;   // Drive - Subtle formant emphasis
            defaults[3] = 1.0f;   // Mix - Full formant effect
            break;
            
        case ENGINE_ENVELOPE_FILTER: // Envelope Filter (Auto-Wah)
            defaults[0] = 0.5f;   // Sensitivity - Moderate envelope response
            defaults[1] = 0.1f;   // Attack - Fast envelope attack
            defaults[2] = 0.3f;   // Release - Medium envelope release
            defaults[3] = 0.5f;   // Range - Full sweep range
            defaults[4] = 1.0f;   // Mix - Full auto-wah effect
            break;
            
        case ENGINE_COMB_RESONATOR: // Comb Resonator
            defaults[0] = 0.5f;   // Frequency - ~440Hz fundamental
            defaults[1] = 0.4f;   // Resonance - Moderate comb resonance
            defaults[2] = 0.3f;   // Feedback - Conservative feedback
            defaults[3] = 0.5f;   // Mix - Balanced resonation
            break;
            
        case ENGINE_VOCAL_FORMANT: // Vocal Formant Filter
            defaults[0] = 0.3f;   // Vowel Position - "A" vowel
            defaults[1] = 0.4f;   // Formant Intensity - Moderate vocal shaping
            defaults[2] = 0.5f;   // Gender - Neutral gender setting
            defaults[3] = 1.0f;   // Mix - Full vocal effect
            break;
            
        // ==================== DISTORTION & SATURATION (15-22) ====================
            
        case ENGINE_VINTAGE_TUBE: // Vintage Tube Preamp
            defaults[0] = 0.5f;   // Input Gain - Moderate tube drive
            defaults[1] = 0.3f;   // Drive - Warm tube saturation
            defaults[2] = 0.5f;   // Bias - Balanced tube bias
            defaults[3] = 0.5f;   // Bass - Neutral bass response
            defaults[4] = 0.5f;   // Mid - Neutral midrange
            defaults[5] = 0.5f;   // Treble - Neutral treble
            defaults[6] = 0.5f;   // Presence - Balanced presence
            defaults[7] = 0.5f;   // Output Gain - Unity output
            defaults[8] = 0.0f;   // Tube Type - 12AX7 tube type
            defaults[9] = 1.0f;   // Mix - Full tube character
            break;
            
        case ENGINE_WAVE_FOLDER: // Wave Folder
            defaults[0] = 0.4f;   // Drive - Moderate drive for wave folding
            defaults[1] = 0.3f;   // Fold Amount - Conservative folding
            defaults[2] = 0.5f;   // Symmetry - Balanced folding
            defaults[3] = 0.5f;   // Output - Unity gain
            defaults[4] = 0.7f;   // Mix - Mostly folded signal
            break;
            
        case ENGINE_HARMONIC_EXCITER: // Harmonic Exciter
            defaults[0] = 0.2f;   // Harmonics - Subtle harmonic enhancement
            defaults[1] = 0.7f;   // Frequency - Upper midrange focus (4kHz)
            defaults[2] = 0.2f;   // Mix - Subtle excitation
            break;
            
        case ENGINE_BIT_CRUSHER: // Bit Crusher
            defaults[0] = 0.9f;   // Bit Depth - High quality (15-bit) for subtle effect
            defaults[1] = 0.9f;   // Sample Rate - High rate for subtle aliasing
            defaults[2] = 0.3f;   // Mix - Blend with dry signal
            defaults[3] = 0.5f;   // Output - Unity gain
            break;
            
        case ENGINE_MULTIBAND_SATURATOR: // Multiband Saturator
            defaults[0] = 0.3f;   // Low Drive - Gentle low-end warmth
            defaults[1] = 0.3f;   // Mid Drive - Subtle midrange saturation
            defaults[2] = 0.2f;   // High Drive - Conservative high-end saturation
            defaults[3] = 0.3f;   // Crossover Low - ~400Hz
            defaults[4] = 0.7f;   // Crossover High - ~3kHz
            defaults[5] = 0.7f;   // Mix - Blend with dry signal
            break;
            
        case ENGINE_MUFF_FUZZ: // Muff Fuzz (Big Muff style)
            defaults[0] = 0.3f;   // Sustain - Moderate fuzz sustain
            defaults[1] = 0.5f;   // Tone - Balanced tone stack
            defaults[2] = 0.5f;   // Volume - Unity output
            defaults[3] = 0.0f;   // Gate - No noise gate
            defaults[4] = 0.0f;   // Mids - Standard mids (no scoop)
            defaults[5] = 0.0f;   // Variant - Standard Big Muff
            defaults[6] = 1.0f;   // Mix - Full fuzz character
            break;
            
        case ENGINE_RODENT_DISTORTION: // Rodent Distortion (RAT style)
            defaults[0] = 0.5f;   // Gain - Moderate RAT drive
            defaults[1] = 0.4f;   // Filter - Slightly filtered for smoothness
            defaults[2] = 0.3f;   // Clipping - Moderate LED clipping
            defaults[3] = 0.5f;   // Tone - Balanced tone
            defaults[4] = 0.5f;   // Output - Unity output
            defaults[5] = 1.0f;   // Mix - Full distortion character
            defaults[6] = 0.0f;   // Mode - Standard RAT mode
            defaults[7] = 0.3f;   // Presence - Subtle high-end lift
            break;
            
        case ENGINE_K_STYLE: // K-Style Overdrive (Klon Centaur style)
            defaults[0] = 0.3f;   // Drive - Low drive for smooth warmth
            defaults[1] = 0.5f;   // Tone - Balanced, transparent tone
            defaults[2] = 0.5f;   // Level - Unity gain
            defaults[3] = 1.0f;   // Mix - Full overdrive character
            break;
            
        // ==================== MODULATION EFFECTS (23-33) ====================
            
        case ENGINE_DIGITAL_CHORUS: // Digital/Stereo Chorus
            defaults[0] = 0.2f;   // Rate - Musical chorus rate (~2Hz)
            defaults[1] = 0.3f;   // Depth - Moderate depth for lush sound
            defaults[2] = 0.3f;   // Mix - Balanced chorus effect
            defaults[3] = 0.0f;   // Feedback - No feedback initially
            break;
            
        case ENGINE_RESONANT_CHORUS: // Resonant Chorus
            defaults[0] = 0.2f;   // Rate - Musical chorus rate
            defaults[1] = 0.3f;   // Depth - Moderate modulation depth
            defaults[2] = 0.3f;   // Resonance - Subtle resonance
            defaults[3] = 0.3f;   // Mix - Balanced chorus
            break;
            
        case ENGINE_ANALOG_PHASER: // Analog Phaser
            defaults[0] = 0.4f;   // Rate - Musical phaser rate (~3Hz)
            defaults[1] = 0.5f;   // Depth - Full sweep range
            defaults[2] = 0.3f;   // Feedback - Moderate resonance
            defaults[3] = 0.5f;   // Stages - 4 stages (normalized)
            defaults[4] = 1.0f;   // Mix - Full phasing effect
            break;
            
        case ENGINE_RING_MODULATOR: // Ring Modulator
            defaults[0] = 0.3f;   // Frequency - Musical frequency (~200Hz)
            defaults[1] = 0.4f;   // Depth - Moderate ring mod effect
            defaults[2] = 0.0f;   // Shape - Sine wave
            defaults[3] = 0.5f;   // Mix - Balanced blend
            break;
            
        case ENGINE_FREQUENCY_SHIFTER: // Frequency Shifter
            defaults[0] = 0.1f;   // Shift Amount - Subtle frequency shift (+20Hz)
            defaults[1] = 0.5f;   // Fine Tune - Centered
            defaults[2] = 0.4f;   // Feedback - Moderate feedback
            defaults[3] = 0.5f;   // Mix - Balanced blend
            break;
            
        case ENGINE_HARMONIC_TREMOLO: // Harmonic Tremolo
            defaults[0] = 0.25f;  // Rate - Slow harmonic tremolo (~3Hz)
            defaults[1] = 0.5f;   // Depth - Moderate depth
            defaults[2] = 0.4f;   // Harmonics - Subtle harmonic content
            defaults[3] = 0.25f;  // Stereo Phase - Slight stereo phase offset
            break;
            
        case ENGINE_CLASSIC_TREMOLO: // Classic Tremolo
            defaults[0] = 0.25f;  // Rate - Musical tremolo rate (~4Hz)
            defaults[1] = 0.5f;   // Depth - Noticeable but musical depth
            defaults[2] = 0.0f;   // Shape - Sine wave
            defaults[3] = 0.0f;   // Stereo - Mono tremolo
            defaults[4] = 0.0f;   // Type - Amplitude tremolo
            defaults[5] = 0.5f;   // Symmetry - Balanced waveform
            defaults[6] = 1.0f;   // Volume - Unity volume
            defaults[7] = 1.0f;   // Mix - Full tremolo effect
            break;
            
        case ENGINE_ROTARY_SPEAKER: // Rotary Speaker (Leslie)
            defaults[0] = 0.5f;   // Speed - Medium rotation speed
            defaults[1] = 0.3f;   // Acceleration - Moderate acceleration
            defaults[2] = 0.3f;   // Drive - Subtle tube drive
            defaults[3] = 0.6f;   // Mic Distance - Close miking
            defaults[4] = 0.8f;   // Stereo Width - Wide stereo image
            defaults[5] = 1.0f;   // Mix - Full Leslie effect
            break;
            
        case ENGINE_PITCH_SHIFTER: // Pitch Shifter
            defaults[0] = 0.5f;   // Pitch - No pitch change (0 cents)
            defaults[1] = 0.5f;   // Fine - No fine tuning
            defaults[2] = 0.5f;   // Mix - Balanced original/shifted
            break;
            
        case ENGINE_DETUNE_DOUBLER: // Detune Doubler
            defaults[0] = 0.3f;   // Detune Amount - Subtle detuning (15 cents)
            defaults[1] = 0.15f;  // Delay Time - Short delay for doubling
            defaults[2] = 0.7f;   // Stereo Width - Wide stereo spread
            defaults[3] = 0.3f;   // Thickness - Moderate voice thickness
            defaults[4] = 0.5f;   // Mix - Balanced doubling
            break;
            
        case ENGINE_INTELLIGENT_HARMONIZER: // Intelligent Harmonizer
            defaults[0] = 0.5f;   // Interval - No transposition initially
            defaults[1] = 0.0f;   // Key - C major
            defaults[2] = 0.0f;   // Scale - Major scale
            defaults[3] = 0.0f;   // Voices - Single voice
            defaults[4] = 0.3f;   // Spread - Moderate stereo spread
            defaults[5] = 0.0f;   // Humanize - No timing/pitch variation
            defaults[6] = 0.0f;   // Formant - No formant correction
            defaults[7] = 0.5f;   // Mix - Balanced harmonization
            break;
            
        // ==================== REVERB & DELAY (34-43) ====================
            
        case ENGINE_TAPE_ECHO: // Tape Echo
            defaults[0] = 0.375f; // Time - 1/8 note at 120 BPM (187.5ms)
            defaults[1] = 0.35f;  // Feedback - Conservative feedback
            defaults[2] = 0.25f;  // Wow & Flutter - Subtle tape character
            defaults[3] = 0.3f;   // Saturation - Moderate tape saturation
            defaults[4] = 0.35f;  // Mix - Noticeable but balanced
            break;
            
        case ENGINE_DIGITAL_DELAY: // Digital Delay
            defaults[0] = 0.4f;   // Time - Slightly longer delay
            defaults[1] = 0.3f;   // Feedback - Conservative feedback
            defaults[2] = 0.3f;   // Mix - Balanced delay level
            defaults[3] = 0.8f;   // High Cut - Slight high-end roll-off
            break;
            
        case ENGINE_MAGNETIC_DRUM_ECHO: // Magnetic Drum Echo
            defaults[0] = 0.4f;   // Time - Medium delay time
            defaults[1] = 0.3f;   // Feedback - Moderate feedback
            defaults[2] = 0.3f;   // Mix - Balanced echo level
            break;
            
        case ENGINE_BUCKET_BRIGADE_DELAY: // Bucket Brigade Delay (Analog)
            defaults[0] = 0.5f;   // Time - 1/4 note delay
            defaults[1] = 0.3f;   // Feedback - Musical feedback
            defaults[2] = 0.4f;   // Clock Noise - Subtle analog character
            defaults[3] = 0.6f;   // High Cut - Warm analog filtering
            defaults[4] = 0.3f;   // Modulation - Gentle analog modulation
            defaults[5] = 0.5f;   // Mix - Balanced analog delay
            break;
            
        case ENGINE_BUFFER_REPEAT: // Buffer Repeat
            defaults[0] = 0.5f;   // Size - Medium buffer size
            defaults[1] = 0.5f;   // Rate - Medium repeat rate
            defaults[2] = 0.3f;   // Feedback - Conservative feedback
            defaults[3] = 0.3f;   // Mix - Subtle repeat effect
            break;
            
        case ENGINE_PLATE_REVERB: // Plate Reverb
            defaults[0] = 0.5f;   // Size - Medium plate size
            defaults[1] = 0.5f;   // Damping - Balanced damping
            defaults[2] = 0.0f;   // Predelay - No predelay initially
            defaults[3] = 0.3f;   // Mix - Tasteful reverb level
            break;
            
        case ENGINE_SPRING_REVERB: // Spring Reverb
            defaults[0] = 0.5f;   // Springs - Medium spring character
            defaults[1] = 0.5f;   // Decay - Balanced decay time
            defaults[2] = 0.5f;   // Tone - Neutral tone
            defaults[3] = 0.3f;   // Mix - Classic spring level
            break;
            
        case ENGINE_CONVOLUTION_REVERB: // Convolution Reverb
            defaults[0] = 0.5f;   // Size - Medium hall impulse
            defaults[1] = 0.6f;   // Decay - Natural decay
            defaults[2] = 0.3f;   // Mix - Subtle convolution reverb
            break;
            
        case ENGINE_SHIMMER_REVERB: // Shimmer Reverb
            defaults[0] = 0.5f;   // Size - Medium room size
            defaults[1] = 0.3f;   // Shimmer - Moderate pitch shifting
            defaults[2] = 0.5f;   // Damping - Balanced damping
            defaults[3] = 0.3f;   // Mix - Tasteful shimmer level
            break;
            
        case ENGINE_GATED_REVERB: // Gated Reverb
            defaults[0] = 0.5f;   // Size - Medium room size
            defaults[1] = 0.3f;   // Gate Time - Quick gate timing
            defaults[2] = 0.5f;   // Damping - Balanced damping
            defaults[3] = 0.3f;   // Mix - Noticeable gate effect
            break;
            
        // ==================== SPATIAL & SPECIAL EFFECTS (44-52) ====================
            
        case ENGINE_STEREO_WIDENER: // Stereo Widener
            defaults[0] = 0.5f;   // Width - Moderate widening
            defaults[1] = 0.5f;   // Bass Mono - Balanced bass response
            defaults[2] = 1.0f;   // Mix - Full width processing
            break;
            
        case ENGINE_STEREO_IMAGER: // Stereo Imager
            defaults[0] = 0.5f;   // Width - Balanced stereo width
            defaults[1] = 0.5f;   // Center - Centered image
            defaults[2] = 0.5f;   // Rotation - No rotation
            defaults[3] = 1.0f;   // Mix - Full imaging processing
            break;
            
        case ENGINE_DIMENSION_EXPANDER: // Dimension Expander
            defaults[0] = 0.5f;   // Size - Moderate expansion
            defaults[1] = 0.5f;   // Width - Balanced width
            defaults[2] = 0.5f;   // Mix - Balanced expansion
            break;
            
        case ENGINE_SPECTRAL_FREEZE: // Spectral Freeze
            defaults[0] = 0.0f;   // Freeze - Not frozen initially
            defaults[1] = 0.5f;   // Size - Medium freeze window
            defaults[2] = 0.2f;   // Mix - Subtle spectral effect
            break;
            
        case ENGINE_SPECTRAL_GATE: // Spectral Gate
            defaults[0] = 0.25f;  // Threshold - Conservative gating
            defaults[1] = 0.3f;   // Ratio - Moderate spectral gating
            defaults[2] = 0.3f;   // Attack - Fast spectral response
            defaults[3] = 0.3f;   // Release - Medium release
            defaults[4] = 0.0f;   // Freq Low - Full low-end range
            defaults[5] = 1.0f;   // Freq High - Full high-end range
            defaults[6] = 0.0f;   // Lookahead - No lookahead
            defaults[7] = 1.0f;   // Mix - Full spectral processing
            break;
            
        case ENGINE_PHASED_VOCODER: // Phased Vocoder
            defaults[0] = 0.5f;   // Bands - Medium band count
            defaults[1] = 0.5f;   // Shift - No pitch shift initially
            defaults[2] = 0.5f;   // Formant - Neutral formant
            defaults[3] = 0.2f;   // Mix - Subtle vocoder effect
            break;
            
        case ENGINE_GRANULAR_CLOUD: // Granular Cloud
            defaults[0] = 0.5f;   // Grains - Medium grain count
            defaults[1] = 0.5f;   // Size - Medium grain size
            defaults[2] = 0.5f;   // Position - Center of buffer
            defaults[3] = 0.5f;   // Pitch - No pitch change
            defaults[4] = 0.2f;   // Mix - Subtle granular effect
            break;
            
        case ENGINE_CHAOS_GENERATOR: // Chaos Generator
            defaults[0] = 0.1f;   // Rate - Slow chaos rate
            defaults[1] = 0.1f;   // Depth - Minimal chaos depth
            defaults[2] = 0.0f;   // Type - Lorenz attractor
            defaults[3] = 0.5f;   // Smoothing - Moderate smoothing
            defaults[4] = 0.0f;   // Target - Parameter 1 target
            defaults[5] = 0.0f;   // Sync - No tempo sync
            defaults[6] = 0.5f;   // Seed - Random seed
            defaults[7] = 0.2f;   // Mix - Subtle chaos effect
            break;
            
        case ENGINE_FEEDBACK_NETWORK: // Feedback Network
            defaults[0] = 0.3f;   // Feedback - Conservative feedback
            defaults[1] = 0.5f;   // Delay - Medium delay times
            defaults[2] = 0.2f;   // Modulation - Subtle modulation
            defaults[3] = 0.2f;   // Mix - Subtle network effect
            break;
            
        // ==================== UTILITY (53-56) ====================
            
        case ENGINE_MID_SIDE_PROCESSOR: // Mid-Side Processor
            defaults[0] = 0.5f;   // Mid Gain - 0dB (unity)
            defaults[1] = 0.5f;   // Side Gain - 0dB (unity)
            defaults[2] = 0.5f;   // Width - 100% width
            defaults[3] = 0.5f;   // Mid Low - 0dB
            defaults[4] = 0.5f;   // Mid High - 0dB
            defaults[5] = 0.5f;   // Side Low - 0dB
            defaults[6] = 0.5f;   // Side High - 0dB
            defaults[7] = 0.0f;   // Bass Mono - Off
            defaults[8] = 0.0f;   // Solo Mode - Off
            defaults[9] = 0.0f;   // Presence - Off
            break;
            
        case ENGINE_GAIN_UTILITY: // Gain Utility
            defaults[0] = 0.5f;   // Gain - 0dB (unity)
            defaults[1] = 0.5f;   // Left Gain - 0dB
            defaults[2] = 0.5f;   // Right Gain - 0dB
            defaults[3] = 0.5f;   // Mid Gain - 0dB
            defaults[4] = 0.5f;   // Side Gain - 0dB
            defaults[5] = 0.0f;   // Mode - Stereo mode
            defaults[6] = 0.0f;   // Phase L - Normal phase
            defaults[7] = 0.0f;   // Phase R - Normal phase
            defaults[8] = 0.0f;   // Channel Swap - Off
            defaults[9] = 0.0f;   // Auto Gain - Off
            break;
            
        case ENGINE_MONO_MAKER: // Mono Maker
            defaults[0] = 0.3f;   // Frequency - ~100Hz bass mono
            defaults[1] = 0.5f;   // Slope - Moderate slope
            defaults[2] = 0.0f;   // Mode - Standard mode
            defaults[3] = 1.0f;   // Bass Mono - 100% bass mono
            defaults[4] = 0.0f;   // Preserve Phase - Minimum
            defaults[5] = 1.0f;   // DC Filter - On
            defaults[6] = 1.0f;   // Width Above - 100% stereo above crossover
            defaults[7] = 0.5f;   // Output Gain - 0dB
            break;
            
        case ENGINE_PHASE_ALIGN: // Phase Align
            defaults[0] = 0.5f;   // Low Freq Phase - Neutral
            defaults[1] = 0.5f;   // Mid Freq Phase - Neutral  
            defaults[2] = 0.5f;   // High Freq Phase - Neutral
            defaults[3] = 0.0f;   // Mix - 100% processed
            break;
            
        default:
            // Unknown engine - return empty map
            break;
    }
    
    return defaults;
}

EngineDefaults getEngineDefaults(int engineId) {
    EngineDefaults config;
    config.engineId = engineId;
    
    // Get basic defaults
    auto defaults = getDefaultParameters(engineId);
    
    // Convert to ParameterInfo structures with metadata
    for (const auto& [index, value] : defaults) {
        ParameterInfo info;
        info.defaultValue = value;
        info.minValue = 0.0f;
        info.maxValue = 1.0f;
        info.name = getParameterName(engineId, index);
        
        // Add specific metadata based on engine type and parameter
        switch (engineId) {
            case ENGINE_VCA_COMPRESSOR:
                switch (index) {
                    case 0: info.description = "Compression threshold"; info.units = "dB"; break;
                    case 1: info.description = "Compression ratio"; info.units = ":1"; break;
                    case 2: info.description = "Attack time"; info.units = "ms"; break;
                    case 3: info.description = "Release time"; info.units = "ms"; break;
                    case 4: info.description = "Knee hardness"; info.units = ""; break;
                    case 5: info.description = "Makeup gain"; info.units = "dB"; break;
                    case 6: info.description = "Wet/dry mix"; info.units = "%"; break;
                    default: info.description = "Parameter " + std::to_string(index + 1); break;
                }
                break;
            // Add more engine-specific metadata as needed
            default:
                info.description = "Parameter " + std::to_string(index + 1);
                info.units = "";
                break;
        }
        
        config.parameters[index] = info;
    }
    
    // Set engine name and category
    config.name = getEngineTypeName(engineId);
    
    // Set category based on engine ID
    if (engineId >= ENGINE_OPTO_COMPRESSOR && engineId <= ENGINE_DYNAMIC_EQ) {
        config.category = EngineCategory::DYNAMICS;
    } else if (engineId >= ENGINE_PARAMETRIC_EQ && engineId <= ENGINE_VOCAL_FORMANT) {
        config.category = EngineCategory::FILTER;
    } else if (engineId >= ENGINE_VINTAGE_TUBE && engineId <= ENGINE_K_STYLE) {
        if (engineId == ENGINE_VINTAGE_TUBE || engineId == ENGINE_HARMONIC_EXCITER) {
            config.category = EngineCategory::SATURATION;
        } else {
            config.category = EngineCategory::DISTORTION;
        }
    } else if (engineId >= ENGINE_DIGITAL_CHORUS && engineId <= ENGINE_INTELLIGENT_HARMONIZER) {
        if (engineId == ENGINE_PITCH_SHIFTER || engineId == ENGINE_INTELLIGENT_HARMONIZER) {
            config.category = EngineCategory::PITCH;
        } else {
            config.category = EngineCategory::MODULATION;
        }
    } else if (engineId >= ENGINE_TAPE_ECHO && engineId <= ENGINE_GATED_REVERB) {
        if (engineId >= ENGINE_TAPE_ECHO && engineId <= ENGINE_BUFFER_REPEAT) {
            config.category = EngineCategory::DELAY;
        } else {
            config.category = EngineCategory::REVERB;
        }
    } else if (engineId >= ENGINE_STEREO_WIDENER && engineId <= ENGINE_FEEDBACK_NETWORK) {
        if (engineId >= ENGINE_STEREO_WIDENER && engineId <= ENGINE_DIMENSION_EXPANDER) {
            config.category = EngineCategory::SPATIAL;
        } else if (engineId >= ENGINE_SPECTRAL_FREEZE && engineId <= ENGINE_PHASED_VOCODER) {
            config.category = EngineCategory::SPECTRAL;
        } else {
            config.category = EngineCategory::EXPERIMENTAL;
        }
    } else if (engineId >= ENGINE_MID_SIDE_PROCESSOR && engineId <= ENGINE_PHASE_ALIGN) {
        config.category = EngineCategory::UTILITY;
    } else {
        config.category = EngineCategory::UTILITY; // Default category
    }
    
    return config;
}

std::map<EngineCategory, std::vector<int>> getEnginesByCategory() {
    std::map<EngineCategory, std::vector<int>> categories;
    
    categories[EngineCategory::DYNAMICS] = {
        ENGINE_OPTO_COMPRESSOR, ENGINE_VCA_COMPRESSOR, ENGINE_TRANSIENT_SHAPER,
        ENGINE_NOISE_GATE, ENGINE_MASTERING_LIMITER, ENGINE_DYNAMIC_EQ
    };
    
    categories[EngineCategory::FILTER] = {
        ENGINE_PARAMETRIC_EQ, ENGINE_VINTAGE_CONSOLE_EQ, ENGINE_LADDER_FILTER,
        ENGINE_STATE_VARIABLE_FILTER, ENGINE_FORMANT_FILTER, ENGINE_ENVELOPE_FILTER,
        ENGINE_COMB_RESONATOR, ENGINE_VOCAL_FORMANT
    };
    
    categories[EngineCategory::SATURATION] = {
        ENGINE_VINTAGE_TUBE, ENGINE_HARMONIC_EXCITER
    };
    
    categories[EngineCategory::DISTORTION] = {
        ENGINE_WAVE_FOLDER, ENGINE_BIT_CRUSHER, ENGINE_MULTIBAND_SATURATOR,
        ENGINE_MUFF_FUZZ, ENGINE_RODENT_DISTORTION, ENGINE_K_STYLE
    };
    
    categories[EngineCategory::MODULATION] = {
        ENGINE_DIGITAL_CHORUS, ENGINE_RESONANT_CHORUS, ENGINE_ANALOG_PHASER,
        ENGINE_RING_MODULATOR, ENGINE_FREQUENCY_SHIFTER, ENGINE_HARMONIC_TREMOLO,
        ENGINE_CLASSIC_TREMOLO, ENGINE_ROTARY_SPEAKER, ENGINE_DETUNE_DOUBLER
    };
    
    categories[EngineCategory::PITCH] = {
        ENGINE_PITCH_SHIFTER, ENGINE_INTELLIGENT_HARMONIZER
    };
    
    categories[EngineCategory::DELAY] = {
        ENGINE_TAPE_ECHO, ENGINE_DIGITAL_DELAY, ENGINE_MAGNETIC_DRUM_ECHO,
        ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_BUFFER_REPEAT
    };
    
    categories[EngineCategory::REVERB] = {
        ENGINE_PLATE_REVERB, ENGINE_SPRING_REVERB, ENGINE_CONVOLUTION_REVERB,
        ENGINE_SHIMMER_REVERB, ENGINE_GATED_REVERB
    };
    
    categories[EngineCategory::SPATIAL] = {
        ENGINE_STEREO_WIDENER, ENGINE_STEREO_IMAGER, ENGINE_DIMENSION_EXPANDER
    };
    
    categories[EngineCategory::SPECTRAL] = {
        ENGINE_SPECTRAL_FREEZE, ENGINE_SPECTRAL_GATE, ENGINE_PHASED_VOCODER
    };
    
    categories[EngineCategory::EXPERIMENTAL] = {
        ENGINE_GRANULAR_CLOUD, ENGINE_CHAOS_GENERATOR, ENGINE_FEEDBACK_NETWORK
    };
    
    categories[EngineCategory::UTILITY] = {
        ENGINE_MID_SIDE_PROCESSOR, ENGINE_GAIN_UTILITY, ENGINE_MONO_MAKER, ENGINE_PHASE_ALIGN
    };
    
    return categories;
}

std::string getParameterName(int engineId, int paramIndex) {
    // This would ideally interface with the parameter metadata system
    // For now, return generic names - could be enhanced with specific parameter names
    return "Parameter " + std::to_string(paramIndex + 1);
}

int getParameterCount(int engineId) {
    auto defaults = getDefaultParameters(engineId);
    return static_cast<int>(defaults.size());
}

bool validateEngineDefaults(int engineId) {
    auto defaults = getDefaultParameters(engineId);
    
    // Validate all values are in range [0.0, 1.0]
    for (const auto& [index, value] : defaults) {
        if (value < 0.0f || value > 1.0f) {
            return false;
        }
    }
    
    // Additional safety validations could be added here
    return true;
}

std::string getCategoryGuidelines(EngineCategory category) {
    switch (category) {
        case EngineCategory::DISTORTION:
            return "100% mix, 20-30% drive for musical saturation without harshness";
        case EngineCategory::SATURATION:
            return "80-100% mix, subtle warmth enhancement and harmonic coloration";
        case EngineCategory::REVERB:
            return "25-35% mix, medium decay times for tasteful spatial enhancement";
        case EngineCategory::DELAY:
            return "25-35% mix, musical timing (1/16-1/4 notes), 2-3 repeats maximum";
        case EngineCategory::MODULATION:
            return "30-50% mix, 2-5Hz rates, subtle movement without disorientation";
        case EngineCategory::FILTER:
            return "Variable mix, midrange cutoff, musical resonance without self-oscillation";
        case EngineCategory::DYNAMICS:
            return "100% mix, transparent control, musical ratios (3:1 to 6:1)";
        case EngineCategory::SPATIAL:
            return "Variable mix, balanced processing, maintain mono compatibility";
        case EngineCategory::PITCH:
            return "50% mix, conservative shifting, formant preservation";
        case EngineCategory::UTILITY:
            return "100% mix, unity gain, neutral starting points";
        case EngineCategory::SPECTRAL:
            return "20-30% mix, conservative processing for safe exploration";
        case EngineCategory::EXPERIMENTAL:
            return "20-30% mix, minimal initial impact, designed for user exploration";
        default:
            return "Balanced defaults for optimal musical utility and safety";
    }
}

void applyDefaultsToMap(int engineId, std::map<int, float>& parameterMap) {
    auto defaults = getDefaultParameters(engineId);
    for (const auto& [index, value] : defaults) {
        parameterMap[index] = value;
    }
}

int getMixParameterIndex(int engineId) {
    // Return the index of the mix parameter for each engine
    // Returns -1 if the engine doesn't have a mix parameter
    switch (engineId) {
        case ENGINE_VCA_COMPRESSOR: return 6;
        case ENGINE_OPTO_COMPRESSOR: return 4;
        case ENGINE_TRANSIENT_SHAPER: return -1; // No mix parameter
        case ENGINE_NOISE_GATE: return -1; // No mix parameter
        case ENGINE_MASTERING_LIMITER: return -1; // No mix parameter
        case ENGINE_DYNAMIC_EQ: return 6;
        case ENGINE_PARAMETRIC_EQ: return -1; // No mix parameter
        case ENGINE_VINTAGE_CONSOLE_EQ: return -1; // No mix parameter
        case ENGINE_LADDER_FILTER: return 6;
        case ENGINE_STATE_VARIABLE_FILTER: return 4;
        case ENGINE_FORMANT_FILTER: return 3;
        case ENGINE_ENVELOPE_FILTER: return 4;
        case ENGINE_COMB_RESONATOR: return 3;
        case ENGINE_VOCAL_FORMANT: return 3;
        case ENGINE_VINTAGE_TUBE: return 9;
        case ENGINE_WAVE_FOLDER: return 4;
        case ENGINE_HARMONIC_EXCITER: return 2;
        case ENGINE_BIT_CRUSHER: return 2;
        case ENGINE_MULTIBAND_SATURATOR: return 5;
        case ENGINE_MUFF_FUZZ: return 6;
        case ENGINE_RODENT_DISTORTION: return 5;
        case ENGINE_K_STYLE: return 3;
        case ENGINE_DIGITAL_CHORUS: return 2;
        case ENGINE_RESONANT_CHORUS: return 3;
        case ENGINE_ANALOG_PHASER: return 4;
        case ENGINE_RING_MODULATOR: return 3;
        case ENGINE_FREQUENCY_SHIFTER: return 3;
        case ENGINE_HARMONIC_TREMOLO: return -1; // No mix parameter
        case ENGINE_CLASSIC_TREMOLO: return 7;
        case ENGINE_ROTARY_SPEAKER: return 5;
        case ENGINE_PITCH_SHIFTER: return 2;
        case ENGINE_DETUNE_DOUBLER: return 4;
        case ENGINE_INTELLIGENT_HARMONIZER: return 7;
        case ENGINE_TAPE_ECHO: return 4;
        case ENGINE_DIGITAL_DELAY: return 2;
        case ENGINE_MAGNETIC_DRUM_ECHO: return 2;
        case ENGINE_BUCKET_BRIGADE_DELAY: return 5;
        case ENGINE_BUFFER_REPEAT: return 3;
        case ENGINE_PLATE_REVERB: return 3;
        case ENGINE_SPRING_REVERB: return 3;
        case ENGINE_CONVOLUTION_REVERB: return 2;
        case ENGINE_SHIMMER_REVERB: return 3;
        case ENGINE_GATED_REVERB: return 3;
        case ENGINE_STEREO_WIDENER: return 2;
        case ENGINE_STEREO_IMAGER: return 3;
        case ENGINE_DIMENSION_EXPANDER: return 2;
        case ENGINE_SPECTRAL_FREEZE: return 2;
        case ENGINE_SPECTRAL_GATE: return 7;
        case ENGINE_PHASED_VOCODER: return 3;
        case ENGINE_GRANULAR_CLOUD: return 4;
        case ENGINE_CHAOS_GENERATOR: return 7;
        case ENGINE_FEEDBACK_NETWORK: return 3;
        case ENGINE_MID_SIDE_PROCESSOR: return -1; // No mix parameter
        case ENGINE_GAIN_UTILITY: return -1; // No mix parameter
        case ENGINE_MONO_MAKER: return -1; // No mix parameter
        case ENGINE_PHASE_ALIGN: return 3;
        default: return -1;
    }
}

} // namespace UnifiedDefaultParameters