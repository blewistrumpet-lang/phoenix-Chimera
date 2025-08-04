#include "DefaultParameterValues.h"
#include "ParameterDefinitions.h"

// Default Parameter Values for all 50 Chimera Phoenix Engines
// 
// This system provides musically useful default values for each engine,
// organized by type and following these principles:
// - Start with moderate, safe values (typically 0.3-0.7 range)
// - Drive/gain parameters: Start low (0.2-0.4) to avoid harsh distortion  
// - Mix parameters: Default to 1.0 (100% wet) for most effects, except distortions (0.7-0.8)
// - Time parameters: Musical defaults (e.g., delays at 1/8 or 1/4 note)
// - Feedback: Conservative (0.2-0.4) to avoid runaway feedback
// - Resonance/Q: Moderate (0.3-0.5) for smooth sound
// - Output/makeup gain: Unity (0.5) to maintain levels

namespace DefaultParameterValues {

std::map<int, float> getDefaultParameters(int engineType) {
    std::map<int, float> defaults;
    
    switch (engineType) {
        
        // ==================== DISTORTION/SATURATION ENGINES ====================
        
        case ENGINE_K_STYLE: // K-Style Overdrive
            defaults[0] = 0.3f;  // Drive - Low drive for smooth warmth
            defaults[1] = 0.5f;  // Tone - Balanced, neither dark nor bright
            defaults[2] = 0.5f;  // Level - Unity gain
            defaults[3] = 1.0f;  // Mix - 100% wet for overdrive
            break;
            
        case ENGINE_RODENT_DISTORTION: // Rodent Distortion
            defaults[0] = 0.4f;  // Gain - Moderate drive for classic RAT sound
            defaults[1] = 0.6f;  // Filter - Slightly bright for cutting through
            defaults[2] = 0.3f;  // Clipping - Moderate clipping for punch
            defaults[3] = 0.5f;  // Tone - Balanced tone
            defaults[4] = 0.5f;  // Output - Unity gain
            defaults[5] = 0.8f;  // Mix - Mostly wet for distortion effect
            defaults[6] = 0.0f;  // Mode - RAT style op-amp clipping
            defaults[7] = 0.3f;  // Presence - Subtle high-end lift
            break;
            
        case ENGINE_MUFF_FUZZ: // Muff Fuzz
            defaults[0] = 0.6f;  // Sustain - Good sustain for violin-like tone
            defaults[1] = 0.5f;  // Tone - Balanced for thick sound
            defaults[2] = 0.5f;  // Volume - Unity gain
            break;
            
        case ENGINE_MULTIBAND_SATURATOR: // Multiband Saturator
            defaults[0] = 0.3f;  // Low Drive - Gentle low-end warmth
            defaults[1] = 0.3f;  // Mid Drive - Subtle midrange saturation
            defaults[2] = 0.2f;  // High Drive - Conservative high-end saturation
            defaults[3] = 0.3f;  // Crossover Low - ~400Hz crossover
            defaults[4] = 0.7f;  // Crossover High - ~3kHz crossover
            defaults[5] = 0.7f;  // Mix - Blend with dry signal
            break;
            
        case ENGINE_BIT_CRUSHER: // Bit Crusher
            defaults[0] = 0.7f;  // Bit Depth - 12-bit for subtle degradation
            defaults[1] = 0.6f;  // Sample Rate - Moderate downsampling
            defaults[2] = 0.5f;  // Mix - 50/50 blend
            defaults[3] = 0.5f;  // Output - Unity gain
            break;
            
        case ENGINE_WAVE_FOLDER: // Wave Folder
            defaults[0] = 0.4f;  // Drive - Moderate drive for wavefold character
            defaults[1] = 0.3f;  // Fold Amount - Conservative folding
            defaults[2] = 0.5f;  // Symmetry - Balanced folding
            defaults[3] = 0.5f;  // Output - Unity gain
            defaults[4] = 0.7f;  // Mix - Mostly wet
            break;
            
        case ENGINE_VINTAGE_TUBE_PREAMP: // Vintage Tube Preamp
            defaults[0] = 0.4f;  // Drive - Warm tube saturation
            defaults[1] = 0.3f;  // Warmth - Subtle tube coloration
            defaults[2] = 0.5f;  // Presence - Balanced high-end
            defaults[3] = 0.5f;  // Output - Unity gain
            break;
            
        case ENGINE_HARMONIC_EXCITER: // Harmonic Exciter
            defaults[0] = 0.3f;  // Excite Amount - Subtle enhancement
            defaults[1] = 0.6f;  // Frequency - Upper midrange focus
            defaults[2] = 0.4f;  // Harmonics - Moderate harmonic generation
            defaults[3] = 0.5f;  // Mix - Balanced blend
            break;
            
        // ==================== DELAY ENGINES ====================
        
        case ENGINE_TAPE_ECHO: // Tape Echo
            defaults[0] = 0.375f; // Time - 1/8 note (375ms at 120 BPM)
            defaults[1] = 0.3f;   // Feedback - Conservative to avoid runaway
            defaults[2] = 0.4f;   // Wow/Flutter - Subtle tape character
            defaults[3] = 0.3f;   // Age - Moderate tape wear
            defaults[4] = 0.5f;   // Tone - Balanced filtering
            defaults[5] = 0.6f;   // Mix - Noticeable but not overwhelming
            break;
            
        case ENGINE_MAGNETIC_DRUM_ECHO: // Magnetic Drum Echo
            defaults[0] = 0.25f;  // Time - 1/16 note for rhythmic echo
            defaults[1] = 0.35f;  // Feedback - Moderate feedback
            defaults[2] = 0.4f;   // Saturation - Subtle magnetic warmth
            defaults[3] = 0.5f;   // High Cut - Balanced filtering
            defaults[4] = 0.2f;   // Low Cut - Clean low end
            defaults[5] = 0.5f;   // Mix - 50/50 blend
            break;
            
        case ENGINE_BUCKET_BRIGADE_DELAY: // Bucket Brigade Delay
            defaults[0] = 0.5f;   // Time - 1/4 note delay
            defaults[1] = 0.3f;   // Feedback - Musical feedback
            defaults[2] = 0.4f;   // Clock Noise - Subtle analog character
            defaults[3] = 0.6f;   // High Cut - Warm analog filtering
            defaults[4] = 0.3f;   // Modulation - Gentle chorus effect
            defaults[5] = 0.5f;   // Mix - Balanced
            break;
            
        case ENGINE_DIGITAL_DELAY: // Digital Delay
            defaults[0] = 0.375f; // Time - 1/8 note
            defaults[1] = 0.25f;  // Feedback - Clean feedback
            defaults[2] = 0.7f;   // High Cut - Slight high-end roll-off
            defaults[3] = 0.1f;   // Low Cut - Clean low end
            defaults[4] = 0.0f;   // Modulation - Clean digital delay
            defaults[5] = 0.4f;   // Mix - Moderate delay level
            break;
            
        // ==================== REVERB ENGINES ====================
        
        case ENGINE_PLATE_REVERB: // Plate Reverb
            defaults[0] = 0.6f;   // Size - Medium plate size
            defaults[1] = 0.4f;   // Decay - Musical decay time
            defaults[2] = 0.5f;   // Damping - Balanced damping
            defaults[3] = 0.3f;   // Pre-delay - Subtle pre-delay
            defaults[4] = 0.6f;   // High Cut - Warm reverb tail
            defaults[5] = 0.3f;   // Mix - Moderate reverb level
            break;
            
        case ENGINE_CONVOLUTION_REVERB: // Convolution Reverb
            defaults[0] = 0.5f;   // IR Selection - Medium hall
            defaults[1] = 0.4f;   // Decay - Natural decay
            defaults[2] = 0.2f;   // Pre-delay - Short pre-delay
            defaults[3] = 0.5f;   // High Cut - Balanced
            defaults[4] = 0.2f;   // Low Cut - Clean low end
            defaults[5] = 0.25f;  // Mix - Subtle reverb
            break;
            
        case ENGINE_SHIMMER_REVERB: // Shimmer Reverb
            defaults[0] = 0.5f;   // Size - Medium room
            defaults[1] = 0.6f;   // Decay - Long shimmer tail
            defaults[2] = 0.4f;   // Shimmer - Moderate pitch shifting
            defaults[3] = 0.3f;   // High Cut - Preserve shimmer brightness
            defaults[4] = 0.4f;   // Feedback - Controlled feedback
            defaults[5] = 0.3f;   // Mix - Noticeable but tasteful
            break;
            
        case ENGINE_GATED_REVERB: // Gated Reverb
            defaults[0] = 0.4f;   // Size - Medium room
            defaults[1] = 0.3f;   // Gate Time - Quick gate
            defaults[2] = 0.6f;   // Threshold - Moderate gating
            defaults[3] = 0.1f;   // Attack - Fast attack
            defaults[4] = 0.2f;   // Release - Quick release
            defaults[5] = 0.4f;   // Mix - Noticeable effect
            break;
            
        case ENGINE_SPRING_REVERB: // Spring Reverb
            defaults[0] = 0.4f;   // Drive - Subtle spring drive
            defaults[1] = 0.5f;   // Tension - Medium spring tension
            defaults[2] = 0.3f;   // Dwell - Moderate dwell time
            defaults[3] = 0.4f;   // Tone - Balanced spring character
            defaults[4] = 0.3f;   // Mix - Classic spring level
            break;
            
        // ==================== MODULATION ENGINES ====================
        
        case ENGINE_CLASSIC_TREMOLO: // Classic Tremolo
            defaults[0] = 0.4f;   // Rate - Musical tremolo rate (~4 Hz)
            defaults[1] = 0.4f;   // Depth - Noticeable but musical
            defaults[2] = 0.0f;   // Shape - Sine wave
            defaults[3] = 1.0f;   // Mix - Full effect
            break;
            
        case ENGINE_HARMONIC_TREMOLO: // Harmonic Tremolo
            defaults[0] = 0.3f;   // Rate - Slower harmonic tremolo
            defaults[1] = 0.3f;   // Depth - Subtle harmonic modulation
            defaults[2] = 0.5f;   // High/Low Balance - Balanced
            defaults[3] = 1.0f;   // Mix - Full effect
            break;
            
        case ENGINE_ROTARY_SPEAKER: // Rotary Speaker
            defaults[0] = 0.0f;   // Speed - Slow (chorale)
            defaults[1] = 0.5f;   // Drive - Moderate tube drive
            defaults[2] = 0.6f;   // Horn Level - Prominent horn
            defaults[3] = 0.4f;   // Bass Level - Balanced bass rotor
            defaults[4] = 0.3f;   // Mic Distance - Close miking
            defaults[5] = 1.0f;   // Mix - Full Leslie effect
            break;
            
        case ENGINE_DETUNE_DOUBLER: // Detune Doubler
            defaults[0] = 0.1f;   // Detune Amount - Subtle detuning (10 cents)
            defaults[1] = 0.0f;   // Delay - No delay for tight doubling
            defaults[2] = 0.5f;   // Mix - Balanced original/doubled
            break;
            
        case ENGINE_STEREO_CHORUS: // Stereo Chorus
            defaults[0] = 0.4f;   // Rate - Musical chorus rate
            defaults[1] = 0.4f;   // Depth - Noticeable but musical
            defaults[2] = 0.2f;   // Delay - Short delay time
            defaults[3] = 0.3f;   // Feedback - Moderate feedback
            defaults[4] = 0.6f;   // Stereo Width - Wide stereo image
            defaults[5] = 0.5f;   // Mix - 50/50 blend
            break;
            
        case ENGINE_ANALOG_RING_MODULATOR: // Analog Ring Modulator
            defaults[0] = 0.3f;   // Frequency - Musical frequency (~200 Hz)
            defaults[1] = 0.4f;   // Depth - Moderate ring mod effect
            defaults[2] = 0.0f;   // Shape - Sine wave
            defaults[3] = 0.5f;   // Mix - Balanced blend
            break;
            
        case ENGINE_FREQUENCY_SHIFTER: // Frequency Shifter
            defaults[0] = 0.1f;   // Shift Amount - Subtle frequency shift
            defaults[1] = 0.5f;   // Fine Tune - Centered
            defaults[2] = 0.4f;   // Feedback - Moderate feedback
            defaults[3] = 0.5f;   // Mix - Balanced blend
            break;
            
        case ENGINE_ANALOG_PHASER: // Analog Phaser
            defaults[0] = 0.4f;   // Rate - Musical phaser rate
            defaults[1] = 0.5f;   // Depth - Full sweep range
            defaults[2] = 0.3f;   // Feedback - Moderate resonance
            defaults[3] = 4.0f/8.0f; // Stages - 4 stages (normalized)
            defaults[4] = 1.0f;   // Mix - Full phasing effect
            break;
            
        case ENGINE_RESONANT_CHORUS: // Resonant Chorus
            defaults[0] = 0.4f;   // Rate - Musical chorus rate
            defaults[1] = 0.4f;   // Depth - Moderate modulation
            defaults[2] = 0.3f;   // Resonance - Subtle resonance
            defaults[3] = 0.5f;   // Frequency - Midrange focus
            defaults[4] = 0.5f;   // Mix - Balanced chorus
            break;
            
        // ==================== FILTER ENGINES ====================
        
        case ENGINE_LADDER_FILTER: // Ladder Filter
            defaults[0] = 0.6f;   // Cutoff - Upper midrange
            defaults[1] = 0.4f;   // Resonance - Musical resonance
            defaults[2] = 0.0f;   // Mode - Low-pass
            defaults[3] = 0.2f;   // Drive - Subtle filter drive
            defaults[4] = 1.0f;   // Mix - Full filtering
            break;
            
        case ENGINE_FORMANT_FILTER: // Formant Filter
            defaults[0] = 0.5f;   // Formant - Neutral vowel position
            defaults[1] = 0.4f;   // Resonance - Moderate formant shaping
            defaults[2] = 0.3f;   // Drive - Subtle formant emphasis
            defaults[3] = 1.0f;   // Mix - Full formant effect
            break;
            
        case ENGINE_STATE_VARIABLE_FILTER: // State Variable Filter
            defaults[0] = 0.5f;   // Cutoff - Midrange
            defaults[1] = 0.4f;   // Resonance - Musical resonance
            defaults[2] = 0.0f;   // Mode - Low-pass
            defaults[3] = 0.0f;   // Key Follow - No key tracking
            defaults[4] = 1.0f;   // Mix - Full filtering
            break;
            
        case ENGINE_VOCAL_FORMANT_FILTER: // Vocal Formant Filter
            defaults[0] = 0.3f;   // Vowel Position - "A" vowel
            defaults[1] = 0.4f;   // Formant Intensity - Moderate shaping
            defaults[2] = 0.5f;   // Gender - Neutral
            defaults[3] = 1.0f;   // Mix - Full vocal effect
            break;
            
        case ENGINE_ENVELOPE_FILTER: // Envelope Filter
            defaults[0] = 0.5f;   // Sensitivity - Moderate response
            defaults[1] = 0.6f;   // Frequency - Upper midrange
            defaults[2] = 0.4f;   // Resonance - Musical resonance
            defaults[3] = 0.0f;   // Direction - Up sweep
            defaults[4] = 1.0f;   // Mix - Full auto-wah effect
            break;
            
        // ==================== DYNAMICS ENGINES ====================
        
        case ENGINE_CLASSIC_COMPRESSOR: // Classic Compressor
            defaults[0] = 0.4f;   // Threshold - Moderate compression
            defaults[1] = 0.5f;   // Ratio - 4:1 compression
            defaults[2] = 0.2f;   // Attack - Fast attack
            defaults[3] = 0.4f;   // Release - Medium release
            defaults[4] = 0.5f;   // Makeup Gain - Unity compensation
            defaults[5] = 1.0f;   // Mix - Full compression
            break;
            
        case ENGINE_VINTAGE_OPTO_COMPRESSOR: // Vintage Opto Compressor
            defaults[0] = 0.5f;   // Gain - Moderate input gain
            defaults[1] = 0.3f;   // Peak Reduction - Gentle compression
            defaults[2] = 0.0f;   // HF Emphasis - Flat response
            defaults[3] = 0.5f;   // Output - Unity gain
            defaults[4] = 1.0f;   // Mix - Full compression
            defaults[5] = 0.7f;   // Knee - Soft knee
            defaults[6] = 0.3f;   // Harmonics - Subtle tube coloration
            defaults[7] = 1.0f;   // Stereo Link - Linked for stereo material
            break;
            
        case ENGINE_MASTERING_LIMITER: // Mastering Limiter
            defaults[0] = 0.8f;   // Threshold - High threshold for transparency
            defaults[1] = 0.1f;   // Release - Fast release
            defaults[2] = 0.3f;   // Lookahead - Moderate lookahead
            defaults[3] = 0.0f;   // Character - Transparent
            defaults[4] = 0.5f;   // Output - Unity gain
            break;
            
        case ENGINE_NOISE_GATE: // Noise Gate
            defaults[0] = 0.3f;   // Threshold - Moderate gating
            defaults[1] = 0.8f;   // Ratio - Strong gating
            defaults[2] = 0.1f;   // Attack - Fast attack
            defaults[3] = 0.3f;   // Hold - Short hold time
            defaults[4] = 0.4f;   // Release - Medium release
            defaults[5] = 1.0f;   // Mix - Full gating effect
            break;
            
        case ENGINE_TRANSIENT_SHAPER: // Transient Shaper
            defaults[0] = 0.5f;   // Attack - No change
            defaults[1] = 0.5f;   // Sustain - No change
            defaults[2] = 0.3f;   // Sensitivity - Moderate response
            defaults[3] = 0.5f;   // Output - Unity gain
            break;
            
        // ==================== SPATIAL/STEREO ENGINES ====================
        
        case ENGINE_DIMENSION_EXPANDER: // Dimension Expander
            defaults[0] = 0.4f;   // Width - Moderate stereo widening
            defaults[1] = 0.3f;   // Depth - Subtle depth enhancement
            defaults[2] = 0.5f;   // Center - Balanced center image
            defaults[3] = 1.0f;   // Mix - Full spatial effect
            break;
            
        case ENGINE_MID_SIDE_PROCESSOR: // Mid-Side Processor
            defaults[0] = 0.5f;   // Mid Level - No change
            defaults[1] = 0.4f;   // Side Level - Slightly reduced sides
            defaults[2] = 0.5f;   // Mid EQ - No change
            defaults[3] = 0.5f;   // Side EQ - No change
            defaults[4] = 1.0f;   // Mix - Full processing
            break;
            
        // ==================== EQ ENGINES ====================
        
        case ENGINE_PARAMETRIC_EQ: // Parametric EQ
            defaults[0] = 0.5f;   // Low Gain - No change
            defaults[1] = 0.3f;   // Low Frequency - ~200 Hz
            defaults[2] = 0.5f;   // Mid Gain - No change
            defaults[3] = 0.5f;   // Mid Frequency - ~1 kHz
            defaults[4] = 0.3f;   // Mid Q - Moderate bandwidth
            defaults[5] = 0.5f;   // High Gain - No change
            defaults[6] = 0.7f;   // High Frequency - ~5 kHz
            defaults[7] = 1.0f;   // Mix - Full EQ processing
            break;
            
        case ENGINE_VINTAGE_CONSOLE_EQ: // Vintage Console EQ
            defaults[0] = 0.5f;   // Low Gain - Flat
            defaults[1] = 0.5f;   // Low-Mid Gain - Flat
            defaults[2] = 0.5f;   // High-Mid Gain - Flat
            defaults[3] = 0.5f;   // High Gain - Flat
            defaults[4] = 0.4f;   // Low-Mid Frequency - ~400 Hz
            defaults[5] = 0.6f;   // High-Mid Frequency - ~3 kHz
            defaults[6] = 0.0f;   // High-Pass Filter - Off
            defaults[7] = 1.0f;   // Mix - Full EQ processing
            break;
            
        // ==================== SPECTRAL/GRANULAR ENGINES ====================
        
        case ENGINE_SPECTRAL_FREEZE: // Spectral Freeze
            defaults[0] = 0.0f;   // Freeze - Not frozen initially
            defaults[1] = 0.5f;   // Blend - Balanced freeze/live
            defaults[2] = 0.4f;   // Filter - Moderate filtering
            defaults[3] = 0.5f;   // Pitch - No pitch change
            defaults[4] = 0.3f;   // Mix - Subtle effect initially
            break;
            
        case ENGINE_GRANULAR_CLOUD: // Granular Cloud
            defaults[0] = 0.2f;   // Grain Size - Small grains
            defaults[1] = 0.5f;   // Position - Center of buffer
            defaults[2] = 0.4f;   // Density - Moderate grain density
            defaults[3] = 0.5f;   // Pitch - No pitch change
            defaults[4] = 0.6f;   // Feedback - Moderate feedback
            defaults[5] = 0.4f;   // Mix - Balanced granular effect
            break;
            
        case ENGINE_SPECTRAL_GATE: // Spectral Gate
            defaults[0] = 0.4f;   // Threshold - Moderate gating (-40 dB)
            defaults[1] = 0.8f;   // Ratio - Strong spectral gating
            defaults[2] = 0.2f;   // Attack - Fast spectral response
            defaults[3] = 0.4f;   // Release - Medium spectral release
            defaults[4] = 32.0f/64.0f; // Bands - 32 bands
            defaults[5] = 1.0f;   // Mix - Full spectral processing
            break;
            
        case ENGINE_PHASED_VOCODER: // Phased Vocoder
            defaults[0] = 0.5f;   // Pitch Shift - No pitch change
            defaults[1] = 0.5f;   // Time Stretch - No time change
            defaults[2] = 0.4f;   // Formant - Slight formant preservation
            defaults[3] = 0.5f;   // Mix - Balanced processing
            break;
            
        // ==================== PITCH ENGINES ====================
        
        case ENGINE_PITCH_SHIFTER: // Pitch Shifter
            defaults[0] = 0.5f;   // Pitch - No pitch change (0 cents)
            defaults[1] = 0.0f;   // Fine Tune - No fine tuning
            defaults[2] = 0.4f;   // Formant - Slight formant correction
            defaults[3] = 0.5f;   // Mix - Balanced original/shifted
            break;
            
        case ENGINE_INTELLIGENT_HARMONIZER: // Intelligent Harmonizer
            defaults[0] = 0.5f;   // Interval - Center (no transposition)
            defaults[1] = 0.0f;   // Key - C major
            defaults[2] = 0.0f;   // Scale - Major scale
            defaults[3] = 0.0f;   // Voices - 1 voice
            defaults[4] = 0.3f;   // Spread - Moderate stereo spread
            defaults[5] = 0.0f;   // Humanize - No pitch/timing variation
            defaults[6] = 0.0f;   // Formant - No formant correction
            defaults[7] = 0.5f;   // Mix - 50% wet
            break;
            
        // ==================== RESONATOR/COMB ENGINES ====================
        
        case ENGINE_COMB_RESONATOR: // Comb Resonator
            defaults[0] = 0.5f;   // Frequency - ~440 Hz
            defaults[1] = 0.4f;   // Resonance - Moderate resonance
            defaults[2] = 0.3f;   // Feedback - Conservative feedback
            defaults[3] = 0.5f;   // Mix - Balanced resonation
            break;
            
        case ENGINE_FEEDBACK_NETWORK: // Feedback Network
            defaults[0] = 0.3f;   // Feedback Amount - Conservative feedback
            defaults[1] = 0.5f;   // Network Size - Medium complexity
            defaults[2] = 0.4f;   // Diffusion - Moderate diffusion
            defaults[3] = 0.6f;   // High Cut - Controlled high frequencies
            defaults[4] = 0.3f;   // Mix - Subtle network effect
            break;
            
        // ==================== EXPERIMENTAL/GLITCH ENGINES ====================
        
        case ENGINE_CHAOS_GENERATOR: // Chaos Generator
            defaults[0] = 0.3f;   // Chaos Amount - Moderate chaos
            defaults[1] = 0.4f;   // Rate - Medium chaos rate
            defaults[2] = 0.2f;   // Feedback - Conservative feedback
            defaults[3] = 0.3f;   // Mix - Subtle chaos effect
            break;
            
        case ENGINE_BUFFER_REPEAT: // Buffer Repeat
            defaults[0] = 0.25f;  // Repeat Length - 1/16 note repeats
            defaults[1] = 0.0f;   // Trigger - Manual trigger
            defaults[2] = 0.6f;   // Feedback - Moderate feedback
            defaults[3] = 0.5f;   // Mix - Balanced repeat effect
            break;
            
        default:
            // Fallback: Return empty map for unknown engines
            break;
    }
    
    return defaults;
}

std::vector<EngineDefaultInfo> getAllEngineDefaults() {
    std::vector<EngineDefaultInfo> allDefaults;
    
    // Engine information with categories for organization
    std::vector<std::tuple<int, std::string, std::string>> engineInfo = {
        // Distortion/Saturation
        {ENGINE_K_STYLE, "K-Style Overdrive", "distortion"},
        {ENGINE_RODENT_DISTORTION, "Rodent Distortion", "distortion"},
        {ENGINE_MUFF_FUZZ, "Muff Fuzz", "distortion"},
        {ENGINE_MULTIBAND_SATURATOR, "Multiband Saturator", "distortion"},
        {ENGINE_BIT_CRUSHER, "Bit Crusher", "distortion"},
        {ENGINE_WAVE_FOLDER, "Wave Folder", "distortion"},
        {ENGINE_VINTAGE_TUBE_PREAMP, "Vintage Tube Preamp", "saturation"},
        {ENGINE_HARMONIC_EXCITER, "Harmonic Exciter", "enhancement"},
        
        // Delays
        {ENGINE_TAPE_ECHO, "Tape Echo", "delay"},
        {ENGINE_MAGNETIC_DRUM_ECHO, "Magnetic Drum Echo", "delay"},
        {ENGINE_BUCKET_BRIGADE_DELAY, "Bucket Brigade Delay", "delay"},
        {ENGINE_DIGITAL_DELAY, "Digital Delay", "delay"},
        
        // Reverbs  
        {ENGINE_PLATE_REVERB, "Plate Reverb", "reverb"},
        {ENGINE_CONVOLUTION_REVERB, "Convolution Reverb", "reverb"},
        {ENGINE_SHIMMER_REVERB, "Shimmer Reverb", "reverb"},
        {ENGINE_GATED_REVERB, "Gated Reverb", "reverb"},
        {ENGINE_SPRING_REVERB, "Spring Reverb", "reverb"},
        
        // Modulation
        {ENGINE_CLASSIC_TREMOLO, "Classic Tremolo", "modulation"},
        {ENGINE_HARMONIC_TREMOLO, "Harmonic Tremolo", "modulation"},
        {ENGINE_ROTARY_SPEAKER, "Rotary Speaker", "modulation"},
        {ENGINE_DETUNE_DOUBLER, "Detune Doubler", "modulation"},
        {ENGINE_STEREO_CHORUS, "Stereo Chorus", "modulation"},
        {ENGINE_ANALOG_RING_MODULATOR, "Analog Ring Modulator", "modulation"},
        {ENGINE_FREQUENCY_SHIFTER, "Frequency Shifter", "modulation"},
        {ENGINE_ANALOG_PHASER, "Analog Phaser", "modulation"},
        {ENGINE_RESONANT_CHORUS, "Resonant Chorus", "modulation"},
        
        // Filters
        {ENGINE_LADDER_FILTER, "Ladder Filter", "filter"},
        {ENGINE_FORMANT_FILTER, "Formant Filter", "filter"},
        {ENGINE_STATE_VARIABLE_FILTER, "State Variable Filter", "filter"},
        {ENGINE_VOCAL_FORMANT_FILTER, "Vocal Formant Filter", "filter"},
        {ENGINE_ENVELOPE_FILTER, "Envelope Filter", "filter"},
        
        // Dynamics
        {ENGINE_CLASSIC_COMPRESSOR, "Classic Compressor", "dynamics"},
        {ENGINE_VINTAGE_OPTO_COMPRESSOR, "Vintage Opto Compressor", "dynamics"},
        {ENGINE_MASTERING_LIMITER, "Mastering Limiter", "dynamics"},
        {ENGINE_NOISE_GATE, "Noise Gate", "dynamics"},
        {ENGINE_TRANSIENT_SHAPER, "Transient Shaper", "dynamics"},
        
        // Spatial/Stereo
        {ENGINE_DIMENSION_EXPANDER, "Dimension Expander", "spatial"},
        {ENGINE_MID_SIDE_PROCESSOR, "Mid-Side Processor", "spatial"},
        
        // EQ
        {ENGINE_PARAMETRIC_EQ, "Parametric EQ", "eq"},
        {ENGINE_VINTAGE_CONSOLE_EQ, "Vintage Console EQ", "eq"},
        
        // Spectral/Granular
        {ENGINE_SPECTRAL_FREEZE, "Spectral Freeze", "spectral"},
        {ENGINE_GRANULAR_CLOUD, "Granular Cloud", "granular"},
        {ENGINE_SPECTRAL_GATE, "Spectral Gate", "spectral"},
        {ENGINE_PHASED_VOCODER, "Phased Vocoder", "spectral"},
        
        // Pitch
        {ENGINE_PITCH_SHIFTER, "Pitch Shifter", "pitch"},
        {ENGINE_INTELLIGENT_HARMONIZER, "Intelligent Harmonizer", "pitch"},
        
        // Resonators
        {ENGINE_COMB_RESONATOR, "Comb Resonator", "filter"},
        {ENGINE_FEEDBACK_NETWORK, "Feedback Network", "experimental"},
        
        // Experimental/Glitch
        {ENGINE_CHAOS_GENERATOR, "Chaos Generator", "experimental"},
        {ENGINE_BUFFER_REPEAT, "Buffer Repeat", "glitch"}
    };
    
    for (const auto& [engineId, name, category] : engineInfo) {
        EngineDefaultInfo info;
        info.engineId = engineId;
        info.name = name;
        info.category = category;
        info.defaults = getDefaultParameters(engineId);
        allDefaults.push_back(info);
    }
    
    return allDefaults;
}

std::map<std::string, std::vector<int>> getEnginesByCategory() {
    std::map<std::string, std::vector<int>> categories;
    
    // Distortion/Saturation
    categories["distortion"] = {
        ENGINE_K_STYLE, ENGINE_RODENT_DISTORTION, ENGINE_MUFF_FUZZ,
        ENGINE_MULTIBAND_SATURATOR, ENGINE_BIT_CRUSHER, ENGINE_WAVE_FOLDER
    };
    
    categories["saturation"] = {
        ENGINE_VINTAGE_TUBE_PREAMP, ENGINE_HARMONIC_EXCITER
    };
    
    // Time-based effects
    categories["delay"] = {
        ENGINE_TAPE_ECHO, ENGINE_MAGNETIC_DRUM_ECHO, 
        ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_DIGITAL_DELAY
    };
    
    categories["reverb"] = {
        ENGINE_PLATE_REVERB, ENGINE_CONVOLUTION_REVERB, ENGINE_SHIMMER_REVERB,
        ENGINE_GATED_REVERB, ENGINE_SPRING_REVERB
    };
    
    // Modulation
    categories["modulation"] = {
        ENGINE_CLASSIC_TREMOLO, ENGINE_HARMONIC_TREMOLO, ENGINE_ROTARY_SPEAKER,
        ENGINE_DETUNE_DOUBLER, ENGINE_STEREO_CHORUS, ENGINE_ANALOG_RING_MODULATOR,
        ENGINE_FREQUENCY_SHIFTER, ENGINE_ANALOG_PHASER, ENGINE_RESONANT_CHORUS
    };
    
    // Filtering
    categories["filter"] = {
        ENGINE_LADDER_FILTER, ENGINE_FORMANT_FILTER, ENGINE_STATE_VARIABLE_FILTER,
        ENGINE_VOCAL_FORMANT_FILTER, ENGINE_ENVELOPE_FILTER, ENGINE_COMB_RESONATOR
    };
    
    // Dynamics
    categories["dynamics"] = {
        ENGINE_CLASSIC_COMPRESSOR, ENGINE_VINTAGE_OPTO_COMPRESSOR,
        ENGINE_MASTERING_LIMITER, ENGINE_NOISE_GATE, ENGINE_TRANSIENT_SHAPER
    };
    
    // Spatial processing
    categories["spatial"] = {
        ENGINE_DIMENSION_EXPANDER, ENGINE_MID_SIDE_PROCESSOR
    };
    
    // EQ
    categories["eq"] = {
        ENGINE_PARAMETRIC_EQ, ENGINE_VINTAGE_CONSOLE_EQ
    };
    
    // Spectral/Advanced
    categories["spectral"] = {
        ENGINE_SPECTRAL_FREEZE, ENGINE_SPECTRAL_GATE, ENGINE_PHASED_VOCODER
    };
    
    categories["granular"] = {
        ENGINE_GRANULAR_CLOUD
    };
    
    // Pitch
    categories["pitch"] = {
        ENGINE_PITCH_SHIFTER, ENGINE_INTELLIGENT_HARMONIZER
    };
    
    // Experimental
    categories["experimental"] = {
        ENGINE_FEEDBACK_NETWORK, ENGINE_CHAOS_GENERATOR
    };
    
    categories["glitch"] = {
        ENGINE_BUFFER_REPEAT
    };
    
    return categories;
}

// Utility function to get a parameter name for display purposes
std::string getParameterName(int engineId, int paramIndex) {
    // This would ideally interface with the metadata system
    // For now, return generic names
    return "Parameter " + std::to_string(paramIndex + 1);
}

// Utility function to get the total number of parameters for an engine
int getParameterCount(int engineId) {
    auto defaults = getDefaultParameters(engineId);
    return static_cast<int>(defaults.size());
}

} // namespace DefaultParameterValues