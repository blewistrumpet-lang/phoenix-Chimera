// Generated Parameter Database for ChimeraPhoenix
// Generated from parameter_database.json on 2025-08-04 02:06:38
// DO NOT EDIT MANUALLY - Edit parameter_database.json and regenerate

#pragma once
#include <array>
#include <string>
#include <vector>

namespace ChimeraParameters {

// Parameter information structure
struct ParameterInfo {
    const char* name;
    float defaultValue;
    float minValue;
    float maxValue;
    const char* description;
    const char* units;
    float skew;
};

// Engine information structure  
struct EngineInfo {
    const char* stringId;
    const char* displayName;
    int legacyId;
    const char* cppEnum;
    int dropdownIndex;
    const char* category;
    int parameterCount;
    const ParameterInfo* parameters;
};

// K-Style Overdrive parameters
static constexpr ParameterInfo k_style_params[] = {
    {"Drive", 0.3f, 0.0f, 1.0f, "Amount of tube saturation", "percent", 0.5f},
    {"Tone", 0.5f, 0.0f, 1.0f, "EQ balance from dark to bright", "percent", 0.5f},
    {"Level", 0.5f, 0.0f, 1.0f, "Output level with makeup gain", "percent", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet balance", "percent", 0.5f},
};

// Vintage Tube Preamp parameters
static constexpr ParameterInfo vintage_tube_params[] = {
    {"Input Gain", 0.5f, 0.0f, 1.0f, "Input amplification ±20dB", "dB", 0.5f},
    {"Drive", 0.3f, 0.0f, 1.0f, "Tube saturation amount", "percent", 0.5f},
    {"Bias", 0.5f, 0.0f, 1.0f, "Tube bias point", "percent", 0.5f},
    {"Bass", 0.5f, 0.0f, 1.0f, "Low frequency control", "percent", 0.5f},
    {"Mid", 0.5f, 0.0f, 1.0f, "Mid frequency control", "percent", 0.5f},
    {"Treble", 0.5f, 0.0f, 1.0f, "High frequency control", "percent", 0.5f},
    {"Presence", 0.5f, 0.0f, 1.0f, "High frequency clarity", "percent", 0.5f},
    {"Output Gain", 0.5f, 0.0f, 1.0f, "Output level ±20dB", "dB", 0.5f},
    {"Tube Type", 0.0f, 0.0f, 1.0f, "Tube characteristic (12AX7/12AU7/12AT7/6SN7/ECC88/6V6/EL34/EL84)", "type", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet balance", "percent", 0.5f},
};

// Tape Echo parameters
static constexpr ParameterInfo tape_echo_params[] = {
    {"Time", 0.375f, 0.0f, 1.0f, "Delay time from 10ms to 2000ms", "percent", 0.3f},
    {"Feedback", 0.35f, 0.0f, 1.0f, "Regeneration amount (>0.75 = self-oscillation)", "percent", 0.5f},
    {"Wow & Flutter", 0.25f, 0.0f, 1.0f, "Tape transport instability and wobble", "percent", 0.5f},
    {"Saturation", 0.3f, 0.0f, 1.0f, "Tape compression and harmonic distortion", "percent", 0.5f},
    {"Mix", 0.35f, 0.0f, 1.0f, "Dry/wet balance", "percent", 0.5f},
};

// Shimmer Reverb parameters
static constexpr ParameterInfo shimmer_reverb_params[] = {
    {"Size", 0.5f, 0.0f, 1.0f, "Room size", "percent", 0.5f},
    {"Shimmer", 0.3f, 0.0f, 1.0f, "Octave-up shimmer amount", "percent", 0.5f},
    {"Damping", 0.5f, 0.0f, 1.0f, "High frequency damping", "percent", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Plate Reverb parameters
static constexpr ParameterInfo plate_reverb_params[] = {
    {"Size", 0.5f, 0.0f, 1.0f, "Plate size", "percent", 0.5f},
    {"Damping", 0.5f, 0.0f, 1.0f, "High frequency damping", "percent", 0.5f},
    {"Predelay", 0.0f, 0.0f, 1.0f, "Pre-delay time up to 100ms", "ms", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Spring Reverb parameters
static constexpr ParameterInfo spring_reverb_params[] = {
    {"Springs", 0.5f, 0.0f, 1.0f, "Number of springs", "percent", 0.5f},
    {"Decay", 0.5f, 0.0f, 1.0f, "Decay time", "percent", 0.5f},
    {"Tone", 0.5f, 0.0f, 1.0f, "Tone control", "percent", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Vintage Opto parameters
static constexpr ParameterInfo opto_compressor_params[] = {
    {"Threshold", 0.7f, 0.0f, 1.0f, "Compression threshold", "percent", 0.5f},
    {"Ratio", 0.3f, 0.0f, 1.0f, "Compression ratio", "ratio", 0.5f},
    {"Speed", 0.5f, 0.0f, 1.0f, "Attack/release speed", "percent", 0.5f},
    {"Makeup", 0.5f, 0.0f, 1.0f, "Makeup gain", "percent", 0.5f},
};

// Classic Compressor parameters
static constexpr ParameterInfo classic_compressor_params[] = {
    {"Threshold", 0.7f, 0.0f, 1.0f, "Compression threshold", "percent", 0.5f},
    {"Ratio", 0.3f, 0.0f, 1.0f, "Compression ratio", "ratio", 0.5f},
    {"Attack", 0.2f, 0.0f, 1.0f, "Attack time", "ms", 0.3f},
    {"Release", 0.4f, 0.0f, 1.0f, "Release time", "ms", 0.3f},
    {"Knee", 0.0f, 0.0f, 1.0f, "Knee softness", "percent", 0.5f},
    {"Makeup", 0.5f, 0.0f, 1.0f, "Makeup gain", "percent", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Stereo Chorus parameters
static constexpr ParameterInfo stereo_chorus_params[] = {
    {"Rate", 0.2f, 0.0f, 1.0f, "LFO rate", "Hz", 0.3f},
    {"Depth", 0.3f, 0.0f, 1.0f, "Modulation depth", "percent", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
    {"Feedback", 0.0f, 0.0f, 0.7f, "Feedback amount", "percent", 0.5f},
};

// Digital Delay parameters
static constexpr ParameterInfo digital_delay_params[] = {
    {"Time", 0.4f, 0.0f, 1.0f, "Delay time", "ms", 0.3f},
    {"Feedback", 0.3f, 0.0f, 0.9f, "Feedback amount", "percent", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
    {"High Cut", 0.8f, 0.0f, 1.0f, "High frequency cutoff", "Hz", 0.3f},
};

// Classic Tremolo parameters
static constexpr ParameterInfo classic_tremolo_params[] = {
    {"Rate", 0.25f, 0.0f, 1.0f, "Tremolo rate", "Hz", 0.3f},
    {"Depth", 0.5f, 0.0f, 1.0f, "Tremolo depth", "percent", 0.5f},
    {"Shape", 0.0f, 0.0f, 1.0f, "LFO waveform shape", "type", 0.5f},
    {"Stereo", 0.0f, 0.0f, 1.0f, "Stereo phase", "degrees", 0.5f},
    {"Type", 0.0f, 0.0f, 1.0f, "Tremolo type", "mode", 0.5f},
    {"Symmetry", 0.5f, 0.0f, 1.0f, "Waveform symmetry", "percent", 0.5f},
    {"Volume", 1.0f, 0.0f, 1.0f, "Output volume", "percent", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Harmonic Tremolo parameters
static constexpr ParameterInfo harmonic_tremolo_params[] = {
    {"Rate", 0.25f, 0.0f, 1.0f, "Tremolo rate", "Hz", 0.3f},
    {"Depth", 0.5f, 0.0f, 1.0f, "Tremolo depth", "percent", 0.5f},
    {"Harmonics", 0.4f, 0.0f, 1.0f, "Harmonic content (crossover frequency)", "Hz", 0.5f},
    {"Stereo Phase", 0.25f, 0.0f, 1.0f, "Phase offset", "degrees", 0.5f},
};

// Dimension Expander parameters
static constexpr ParameterInfo dimension_expander_params[] = {
    {"Size", 0.5f, 0.0f, 1.0f, "Dimension size", "percent", 0.5f},
    {"Width", 0.5f, 0.0f, 1.0f, "Stereo width", "percent", 0.5f},
    {"Mix", 0.5f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Harmonic Exciter parameters
static constexpr ParameterInfo harmonic_exciter_params[] = {
    {"Harmonics", 0.2f, 0.0f, 1.0f, "Harmonic generation", "percent", 0.5f},
    {"Frequency", 0.7f, 0.0f, 1.0f, "Frequency range", "Hz", 0.5f},
    {"Mix", 0.2f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Mid/Side Processor parameters
static constexpr ParameterInfo mid_side_processor_params[] = {
    {"Mid Level", 0.5f, 0.0f, 1.0f, "Mid signal level", "percent", 0.5f},
    {"Side Level", 0.5f, 0.0f, 1.0f, "Side signal level", "percent", 0.5f},
    {"Width", 0.5f, 0.0f, 1.0f, "Stereo width", "percent", 0.5f},
};

// Vintage Console EQ parameters
static constexpr ParameterInfo vintage_console_eq_params[] = {
    {"Low", 0.5f, 0.0f, 1.0f, "Low shelf", "dB", 0.5f},
    {"Low Mid", 0.5f, 0.0f, 1.0f, "Low mid band", "dB", 0.5f},
    {"High Mid", 0.5f, 0.0f, 1.0f, "High mid band", "dB", 0.5f},
    {"High", 0.5f, 0.0f, 1.0f, "High shelf", "dB", 0.5f},
    {"Drive", 0.0f, 0.0f, 1.0f, "Console saturation", "percent", 0.5f},
};

// Parametric EQ parameters
static constexpr ParameterInfo parametric_eq_params[] = {
    {"Freq 1", 0.2f, 0.0f, 1.0f, "Band 1 frequency", "Hz", 0.3f},
    {"Gain 1", 0.5f, 0.0f, 1.0f, "Band 1 gain", "dB", 0.5f},
    {"Q 1", 0.5f, 0.0f, 1.0f, "Band 1 Q", "Q", 0.5f},
    {"Freq 2", 0.5f, 0.0f, 1.0f, "Band 2 frequency", "Hz", 0.3f},
    {"Gain 2", 0.5f, 0.0f, 1.0f, "Band 2 gain", "dB", 0.5f},
    {"Q 2", 0.5f, 0.0f, 1.0f, "Band 2 Q", "Q", 0.5f},
    {"Freq 3", 0.8f, 0.0f, 1.0f, "Band 3 frequency", "Hz", 0.3f},
    {"Gain 3", 0.5f, 0.0f, 1.0f, "Band 3 gain", "dB", 0.5f},
    {"Q 3", 0.5f, 0.0f, 1.0f, "Band 3 Q", "Q", 0.5f},
};

// Transient Shaper parameters
static constexpr ParameterInfo transient_shaper_params[] = {
    {"Attack", 0.5f, 0.0f, 1.0f, "Attack enhancement", "percent", 0.5f},
    {"Sustain", 0.5f, 0.0f, 1.0f, "Sustain control", "percent", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Pitch Shifter parameters
static constexpr ParameterInfo pitch_shifter_params[] = {
    {"Pitch", 0.5f, 0.0f, 1.0f, "Pitch shift amount", "semitones", 0.5f},
    {"Fine", 0.5f, 0.0f, 1.0f, "Fine tuning", "cents", 0.5f},
    {"Mix", 0.5f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Spectral Freeze parameters
static constexpr ParameterInfo spectral_freeze_params[] = {
    {"Freeze", 0.0f, 0.0f, 1.0f, "Freeze trigger", "gate", 0.5f},
    {"Size", 0.5f, 0.0f, 1.0f, "FFT size", "samples", 0.5f},
    {"Mix", 0.2f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Granular Cloud parameters
static constexpr ParameterInfo granular_cloud_params[] = {
    {"Grains", 0.5f, 0.0f, 1.0f, "Grain density", "grains/sec", 0.5f},
    {"Size", 0.5f, 0.0f, 1.0f, "Grain size", "ms", 0.5f},
    {"Position", 0.5f, 0.0f, 1.0f, "Playhead position", "percent", 0.5f},
    {"Pitch", 0.5f, 0.0f, 1.0f, "Pitch variation", "semitones", 0.5f},
    {"Mix", 0.2f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Buffer Repeat parameters
static constexpr ParameterInfo buffer_repeat_params[] = {
    {"Size", 0.5f, 0.0f, 1.0f, "Buffer size", "beats", 0.5f},
    {"Rate", 0.5f, 0.0f, 1.0f, "Repeat rate", "Hz", 0.3f},
    {"Feedback", 0.3f, 0.0f, 0.85f, "Feedback amount", "percent", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Chaos Generator parameters
static constexpr ParameterInfo chaos_generator_params[] = {
    {"Rate", 0.1f, 0.0f, 1.0f, "Chaos rate", "Hz", 0.3f},
    {"Depth", 0.1f, 0.0f, 1.0f, "Chaos depth", "percent", 0.5f},
    {"Type", 0.0f, 0.0f, 1.0f, "Chaos type", "type", 0.5f},
    {"Smoothing", 0.5f, 0.0f, 1.0f, "Smoothing amount", "percent", 0.5f},
    {"Target", 0.0f, 0.0f, 1.0f, "Target parameter", "param", 0.5f},
    {"Sync", 0.0f, 0.0f, 1.0f, "Tempo sync", "beats", 0.5f},
    {"Seed", 0.5f, 0.0f, 1.0f, "Random seed", "seed", 0.5f},
    {"Mix", 0.2f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Intelligent Harmonizer parameters
static constexpr ParameterInfo intelligent_harmonizer_params[] = {
    {"Interval", 0.5f, 0.0f, 1.0f, "Harmony interval", "semitones", 0.5f},
    {"Key", 0.0f, 0.0f, 1.0f, "Musical key", "note", 0.5f},
    {"Scale", 0.0f, 0.0f, 1.0f, "Scale type", "scale", 0.5f},
    {"Voices", 0.0f, 0.0f, 1.0f, "Number of voices", "voices", 0.5f},
    {"Spread", 0.3f, 0.0f, 1.0f, "Stereo spread", "percent", 0.5f},
    {"Humanize", 0.0f, 0.0f, 1.0f, "Humanization amount", "percent", 0.5f},
    {"Formant", 0.0f, 0.0f, 1.0f, "Formant correction", "percent", 0.5f},
    {"Mix", 0.5f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Gated Reverb parameters
static constexpr ParameterInfo gated_reverb_params[] = {
    {"Size", 0.5f, 0.0f, 1.0f, "Room size", "percent", 0.5f},
    {"Gate Time", 0.3f, 0.0f, 1.0f, "Gate duration", "ms", 0.5f},
    {"Damping", 0.5f, 0.0f, 1.0f, "High frequency damping", "percent", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Detune Doubler parameters
static constexpr ParameterInfo detune_doubler_params[] = {
    {"Detune Amount", 0.3f, 0.0f, 1.0f, "Detune amount (0-50 cents)", "cents", 0.5f},
    {"Delay Time", 0.15f, 0.0f, 1.0f, "Base delay time (10-60ms)", "ms", 0.5f},
    {"Stereo Width", 0.7f, 0.0f, 1.0f, "Stereo spread of doubled voices", "percent", 0.5f},
    {"Thickness", 0.3f, 0.0f, 1.0f, "Voice blending thickness", "percent", 0.5f},
    {"Mix", 0.5f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Phased Vocoder parameters
static constexpr ParameterInfo phased_vocoder_params[] = {
    {"Bands", 0.5f, 0.0f, 1.0f, "Number of bands", "bands", 0.5f},
    {"Shift", 0.5f, 0.0f, 1.0f, "Frequency shift", "Hz", 0.5f},
    {"Formant", 0.5f, 0.0f, 1.0f, "Formant shift", "percent", 0.5f},
    {"Mix", 0.2f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Spectral Gate parameters
static constexpr ParameterInfo spectral_gate_params[] = {
    {"Threshold", 0.3f, 0.0f, 1.0f, "Gate threshold", "dB", 0.5f},
    {"Frequency", 0.5f, 0.0f, 1.0f, "Center frequency", "Hz", 0.3f},
    {"Q", 0.5f, 0.0f, 1.0f, "Filter Q", "Q", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Noise Gate parameters
static constexpr ParameterInfo noise_gate_params[] = {
    {"Threshold", 0.2f, 0.0f, 1.0f, "Gate threshold", "dB", 0.5f},
    {"Attack", 0.1f, 0.0f, 1.0f, "Attack time", "ms", 0.3f},
    {"Hold", 0.3f, 0.0f, 1.0f, "Hold time", "ms", 0.3f},
    {"Release", 0.4f, 0.0f, 1.0f, "Release time", "ms", 0.3f},
    {"Range", 0.8f, 0.0f, 1.0f, "Gate range", "dB", 0.5f},
};

// Envelope Filter parameters
static constexpr ParameterInfo envelope_filter_params[] = {
    {"Sensitivity", 0.5f, 0.0f, 1.0f, "Envelope sensitivity", "percent", 0.5f},
    {"Attack", 0.1f, 0.0f, 1.0f, "Attack time", "ms", 0.3f},
    {"Release", 0.3f, 0.0f, 1.0f, "Release time", "ms", 0.3f},
    {"Range", 0.5f, 0.0f, 1.0f, "Filter range", "octaves", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Feedback Network parameters
static constexpr ParameterInfo feedback_network_params[] = {
    {"Feedback", 0.3f, 0.0f, 0.85f, "Feedback amount", "percent", 0.5f},
    {"Delay", 0.3f, 0.0f, 1.0f, "Delay time", "ms", 0.3f},
    {"Modulation", 0.2f, 0.0f, 1.0f, "Modulation amount", "percent", 0.5f},
    {"Mix", 0.2f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Mastering Limiter parameters
static constexpr ParameterInfo mastering_limiter_params[] = {
    {"Threshold", 0.9f, 0.0f, 1.0f, "Limiting threshold", "dB", 0.5f},
    {"Release", 0.2f, 0.0f, 1.0f, "Release time", "ms", 0.3f},
    {"Knee", 0.0f, 0.0f, 1.0f, "Knee softness", "percent", 0.5f},
    {"Lookahead", 0.0f, 0.0f, 1.0f, "Lookahead time", "ms", 0.5f},
};

// Stereo Widener parameters
static constexpr ParameterInfo stereo_widener_params[] = {
    {"Width", 0.5f, 0.0f, 1.0f, "Stereo width", "percent", 0.5f},
    {"Bass Mono", 0.5f, 0.0f, 1.0f, "Bass mono frequency", "Hz", 0.3f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Resonant Chorus parameters
static constexpr ParameterInfo resonant_chorus_params[] = {
    {"Rate", 0.2f, 0.0f, 1.0f, "LFO rate", "Hz", 0.3f},
    {"Depth", 0.3f, 0.0f, 1.0f, "Modulation depth", "percent", 0.5f},
    {"Resonance", 0.3f, 0.0f, 0.9f, "Filter resonance", "Q", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Dynamic EQ parameters
static constexpr ParameterInfo dynamic_eq_params[] = {
    {"Frequency", 0.5f, 0.0f, 0.9f, "Center frequency", "Hz", 0.3f},
    {"Threshold", 0.5f, 0.0f, 1.0f, "Dynamic threshold", "dB", 0.5f},
    {"Ratio", 0.3f, 0.0f, 1.0f, "Compression ratio", "ratio", 0.5f},
    {"Attack", 0.2f, 0.0f, 1.0f, "Attack time", "ms", 0.3f},
    {"Release", 0.4f, 0.0f, 1.0f, "Release time", "ms", 0.3f},
    {"Gain", 0.5f, 0.0f, 1.0f, "EQ gain", "dB", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
    {"Mode", 0.0f, 0.0f, 1.0f, "Compression/expansion mode", "mode", 0.5f},
};

// Stereo Imager parameters
static constexpr ParameterInfo stereo_imager_params[] = {
    {"Width", 0.5f, 0.0f, 1.0f, "Stereo width", "percent", 0.5f},
    {"Center", 0.5f, 0.0f, 1.0f, "Center level", "percent", 0.5f},
    {"Rotation", 0.5f, 0.0f, 1.0f, "Stereo rotation", "degrees", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Rodent Distortion parameters
static constexpr ParameterInfo rodent_distortion_params[] = {
    {"Gain", 0.5f, 0.0f, 1.0f, "Input gain (0-60dB)", "dB", 0.5f},
    {"Filter", 0.4f, 0.0f, 1.0f, "Pre-distortion filter (60Hz-5kHz)", "Hz", 0.3f},
    {"Clipping", 0.5f, 0.0f, 1.0f, "Clipping intensity", "percent", 0.5f},
    {"Tone", 0.5f, 0.0f, 1.0f, "Tone control (500Hz-12kHz)", "Hz", 0.5f},
    {"Output", 0.5f, 0.0f, 1.0f, "Output level", "percent", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
    {"Mode", 0.0f, 0.0f, 1.0f, "Circuit mode (RAT/TS/Muff/Fuzz)", "type", 0.5f},
    {"Presence", 0.3f, 0.0f, 1.0f, "High frequency emphasis", "percent", 0.5f},
};

// Muff Fuzz parameters
static constexpr ParameterInfo muff_fuzz_params[] = {
    {"Sustain", 0.3f, 0.0f, 1.0f, "Sustain amount", "percent", 0.5f},
    {"Tone", 0.5f, 0.0f, 1.0f, "Tone control", "percent", 0.5f},
    {"Volume", 0.5f, 0.0f, 1.0f, "Output volume", "percent", 0.5f},
    {"Gate", 0.0f, 0.0f, 1.0f, "Noise gate threshold", "percent", 0.5f},
    {"Mids", 0.0f, 0.0f, 1.0f, "Mid scoop depth", "percent", 0.5f},
    {"Variant", 0.0f, 0.0f, 1.0f, "Big Muff variant", "type", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Rotary Speaker parameters
static constexpr ParameterInfo rotary_speaker_params[] = {
    {"Speed", 0.5f, 0.0f, 1.0f, "Rotor speed (chorale to tremolo)", "percent", 0.5f},
    {"Acceleration", 0.3f, 0.0f, 1.0f, "Speed transition acceleration", "percent", 0.5f},
    {"Drive", 0.3f, 0.0f, 1.0f, "Tube preamp drive", "percent", 0.5f},
    {"Mic Distance", 0.6f, 0.0f, 1.0f, "Microphone distance", "percent", 0.5f},
    {"Stereo Width", 0.8f, 0.0f, 1.0f, "Stereo microphone angle", "percent", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Comb Resonator parameters
static constexpr ParameterInfo comb_resonator_params[] = {
    {"Frequency", 0.5f, 0.0f, 1.0f, "Resonance frequency", "Hz", 0.3f},
    {"Resonance", 0.5f, 0.0f, 0.95f, "Resonance amount", "Q", 0.5f},
    {"Mix", 0.3f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Ladder Filter Pro parameters
static constexpr ParameterInfo ladder_filter_params[] = {
    {"Cutoff", 0.5f, 0.0f, 1.0f, "Filter cutoff frequency (20Hz-20kHz)", "Hz", 0.3f},
    {"Resonance", 0.3f, 0.0f, 1.0f, "Filter resonance/feedback", "percent", 0.5f},
    {"Drive", 0.2f, 0.0f, 1.0f, "Input saturation drive", "percent", 0.5f},
    {"Filter Type", 0.0f, 0.0f, 1.0f, "Morphable filter type (LP24/LP12/BP/HP/Notch/AP)", "type", 0.5f},
    {"Asymmetry", 0.0f, 0.0f, 1.0f, "Saturation asymmetry", "percent", 0.5f},
    {"Vintage Mode", 0.0f, 0.0f, 1.0f, "Vintage vs modern character", "mode", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.5f},
};

// Utility Engine parameters
static constexpr ParameterInfo gain_utility_params[] = {
    {"Gain", 0.5f, 0.0f, 1.0f, "Input/Output gain (-24dB to +24dB)", "dB", 0.5f},
    {"High Pass", 0.0f, 0.0f, 1.0f, "High-pass filter cutoff (20Hz-1kHz)", "Hz", 0.0f},
    {"Low Pass", 1.0f, 0.0f, 1.0f, "Low-pass filter cutoff (1kHz-20kHz)", "Hz", 1.0f},
    {"Phase Invert", 0.0f, 0.0f, 1.0f, "Invert signal phase", "toggle", 0.0f},
};

static constexpr ParameterInfo mono_maker_params[] = {
    {"Frequency", 0.0f, 0.0f, 1.0f, "Transition frequency (all/low/mid/high)", "Hz", 0.0f},
    {"Stereo Width", 0.5f, 0.0f, 1.0f, "Stereo width control", "percent", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.0f},
};

static constexpr ParameterInfo phase_align_params[] = {
    {"Low Phase", 0.5f, 0.0f, 1.0f, "Low frequency phase (-180° to +180°)", "degrees", 0.5f},
    {"Mid Phase", 0.5f, 0.0f, 1.0f, "Mid frequency phase (-180° to +180°)", "degrees", 0.5f},
    {"High Phase", 0.5f, 0.0f, 1.0f, "High frequency phase (-180° to +180°)", "degrees", 0.5f},
    {"Mix", 1.0f, 0.0f, 1.0f, "Dry/wet mix", "percent", 0.0f},
};

// Complete engine database
static constexpr EngineInfo engineDatabase[] = {
    {"k_style", "K-Style Overdrive", 38, "ENGINE_K_STYLE", 0, "Distortion", 4, k_style_params},
    {"vintage_tube", "Vintage Tube Preamp", 0, "ENGINE_VINTAGE_TUBE", 48, "Saturation", 10, vintage_tube_params},
    {"tape_echo", "Tape Echo", 1, "ENGINE_TAPE_ECHO", 16, "Delay", 5, tape_echo_params},
    {"shimmer_reverb", "Shimmer Reverb", 2, "ENGINE_SHIMMER_REVERB", 13, "Reverb", 4, shimmer_reverb_params},
    {"plate_reverb", "Plate Reverb", 3, "ENGINE_PLATE_REVERB", 10, "Reverb", 4, plate_reverb_params},
    {"spring_reverb", "Spring Reverb", 5, "ENGINE_SPRING_REVERB", 49, "Reverb", 4, spring_reverb_params},
    {"opto_compressor", "Vintage Opto", 6, "ENGINE_OPTO_COMPRESSOR", 42, "Dynamics", 4, opto_compressor_params},
    {"classic_compressor", "Classic Compressor", 7, "ENGINE_VCA_COMPRESSOR", 7, "Dynamics", 7, classic_compressor_params},
    {"stereo_chorus", "Stereo Chorus", 11, "ENGINE_DIGITAL_CHORUS", 15, "Modulation", 4, stereo_chorus_params},
    {"digital_delay", "Digital Delay", 53, "ENGINE_DIGITAL_DELAY", 17, "Delay", 4, digital_delay_params},
    {"classic_tremolo", "Classic Tremolo", 22, "ENGINE_CLASSIC_TREMOLO", 8, "Modulation", 8, classic_tremolo_params},
    {"harmonic_tremolo", "Harmonic Tremolo", 21, "ENGINE_HARMONIC_TREMOLO", 20, "Modulation", 4, harmonic_tremolo_params},
    {"dimension_expander", "Dimension Expander", 18, "ENGINE_DIMENSION_EXPANDER", 22, "Spatial", 3, dimension_expander_params},
    {"harmonic_exciter", "Harmonic Exciter", 32, "ENGINE_HARMONIC_EXCITER", 36, "Enhancement", 3, harmonic_exciter_params},
    {"mid_side_processor", "Mid/Side Processor", 25, "ENGINE_MID_SIDE_PROCESSOR", 47, "Spatial", 3, mid_side_processor_params},
    {"vintage_console_eq", "Vintage Console EQ", 26, "ENGINE_VINTAGE_CONSOLE_EQ", 46, "EQ", 5, vintage_console_eq_params},
    {"parametric_eq", "Parametric EQ", 27, "ENGINE_PARAMETRIC_EQ", 39, "EQ", 9, parametric_eq_params},
    {"transient_shaper", "Transient Shaper", 20, "ENGINE_TRANSIENT_SHAPER", 28, "Dynamics", 3, transient_shaper_params},
    {"pitch_shifter", "Pitch Shifter", 14, "ENGINE_PITCH_SHIFTER", 31, "Pitch", 3, pitch_shifter_params},
    {"spectral_freeze", "Spectral Freeze", 39, "ENGINE_SPECTRAL_FREEZE", 33, "Spectral", 3, spectral_freeze_params},
    {"granular_cloud", "Granular Cloud", 16, "ENGINE_GRANULAR_CLOUD", 34, "Texture", 5, granular_cloud_params},
    {"buffer_repeat", "Buffer Repeat", 40, "ENGINE_BUFFER_REPEAT", 45, "Glitch", 4, buffer_repeat_params},
    {"chaos_generator", "Chaos Generator", 41, "ENGINE_CHAOS_GENERATOR", 44, "Experimental", 8, chaos_generator_params},
    {"intelligent_harmonizer", "Intelligent Harmonizer", 42, "ENGINE_INTELLIGENT_HARMONIZER", 38, "Pitch", 8, intelligent_harmonizer_params},
    {"gated_reverb", "Gated Reverb", 43, "ENGINE_GATED_REVERB", 35, "Reverb", 4, gated_reverb_params},
    {"detune_doubler", "Detune Doubler", 44, "ENGINE_DETUNE_DOUBLER", 12, "Pitch", 5, detune_doubler_params},
    {"phased_vocoder", "Phased Vocoder", 45, "ENGINE_PHASED_VOCODER", 30, "Spectral", 4, phased_vocoder_params},
    {"spectral_gate", "Spectral Gate", 46, "ENGINE_SPECTRAL_GATE", 43, "Dynamics", 4, spectral_gate_params},
    {"noise_gate", "Noise Gate", 47, "ENGINE_NOISE_GATE", 41, "Dynamics", 5, noise_gate_params},
    {"envelope_filter", "Envelope Filter", 48, "ENGINE_ENVELOPE_FILTER", 29, "Filter", 5, envelope_filter_params},
    {"feedback_network", "Feedback Network", 49, "ENGINE_FEEDBACK_NETWORK", 37, "Experimental", 4, feedback_network_params},
    {"mastering_limiter", "Mastering Limiter", 50, "ENGINE_MASTERING_LIMITER", 40, "Dynamics", 4, mastering_limiter_params},
    {"stereo_widener", "Stereo Widener", 51, "ENGINE_STEREO_WIDENER", 51, "Spatial", 3, stereo_widener_params},
    {"resonant_chorus", "Resonant Chorus", 52, "ENGINE_RESONANT_CHORUS", 50, "Modulation", 4, resonant_chorus_params},
    {"dynamic_eq", "Dynamic EQ", 54, "ENGINE_DYNAMIC_EQ", 52, "EQ", 8, dynamic_eq_params},
    {"stereo_imager", "Stereo Imager", 55, "ENGINE_STEREO_IMAGER", 53, "Spatial", 4, stereo_imager_params},
    {"rodent_distortion", "Rodent Distortion", 36, "ENGINE_RODENT_DISTORTION", 2, "Distortion", 8, rodent_distortion_params},
    {"muff_fuzz", "Muff Fuzz", 35, "ENGINE_MUFF_FUZZ", 3, "Distortion", 7, muff_fuzz_params},
    {"rotary_speaker", "Rotary Speaker", 30, "ENGINE_ROTARY_SPEAKER", 54, "Modulation", 6, rotary_speaker_params},
    {"comb_resonator", "Comb Resonator", 23, "ENGINE_COMB_RESONATOR", 9, "Filter", 3, comb_resonator_params},
    {"ladder_filter", "Ladder Filter Pro", 9, "ENGINE_LADDER_FILTER", 55, "Filter", 7, ladder_filter_params},
    {"gain_utility", "Gain Utility", 54, "ENGINE_GAIN_UTILITY", 53, "Utility", 4, gain_utility_params},
    {"mono_maker", "Mono Maker", 55, "ENGINE_MONO_MAKER", 54, "Utility", 3, mono_maker_params},
    {"phase_align", "Phase Align", 56, "ENGINE_PHASE_ALIGN", 55, "Utility", 4, phase_align_params},
};

// Helper functions
inline const EngineInfo* getEngineInfo(const std::string& stringId) {
    for (const auto& engine : engineDatabase) {
        if (engine.stringId == stringId) {
            return &engine;
        }
    }
    return nullptr;
}

inline const EngineInfo* getEngineInfoByLegacyId(int legacyId) {
    for (const auto& engine : engineDatabase) {
        if (engine.legacyId == legacyId) {
            return &engine;
        }
    }
    return nullptr;
}

inline int getParameterCount(const std::string& stringId) {
    const auto* info = getEngineInfo(stringId);
    return info ? info->parameterCount : 0;
}

inline float getDefaultValue(const std::string& stringId, int paramIndex) {
    const auto* info = getEngineInfo(stringId);
    if (info && paramIndex >= 0 && paramIndex < info->parameterCount) {
        return info->parameters[paramIndex].defaultValue;
    }
    return 0.5f; // Safe default
}

} // namespace ChimeraParameters
