#include "EngineMetadata.h"
#include "ParameterDefinitions.h"
#include "MetadataHelpers.h"

// This file contains complete, thoughtful metadata for all 50 engines
// Each engine has rich descriptions that help the AI understand not just
// what the engine does, but HOW it sounds and WHEN to use it

void initializeAllEngineMetadata() {
    auto& registry = EngineMetadataRegistry::getInstance();
    
    // ==================== DISTORTION/SATURATION ENGINES ====================
    
    // ENGINE 0: K-Style Overdrive
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_K_STYLE, "K-Style Overdrive", "distortion",
                     "Smooth tube-style overdrive with asymmetric clipping for even harmonics")
        .setSonicTags({
            "warm", "smooth", "creamy", "musical", "tube-like", "harmonic-rich",
            "midrange-focused", "touch-sensitive", "dynamic", "analog-warmth",
            "gentle-compression", "singing-sustain", "bluesy", "vintage-voiced",
            "responsive", "organic", "natural-breakup", "amp-like"
        })
        .setEmotionalTags({
            "confident", "passionate", "soulful", "expressive", "powerful",
            "raw", "honest", "gritty", "bluesy", "rock-n-roll", "authentic"
        })
        .setFrequencyFocus("midrange")
        .setUseCases({
            "guitar_lead", "guitar_rhythm", "bass_warmth", "drum_saturation",
            "vocal_grit", "mix_glue", "analog_warmth", "tube_emulation",
            "rock_production", "blues_tone"
        })
        .setInstrumentTags({
            "electric_guitar", "bass_guitar", "synthesizer", "organ",
            "electric_piano", "drums", "male_vocals", "harmonica"
        })
        .setTechnicalProps(0.15f, 0)
        .addParameter("Drive", 0.3f, 0.0f, 1.0f, "%", "logarithmic", {
            {"0-10", "clean boost, no clipping"},
            {"10-25", "edge of breakup, touch sensitive"},
            {"25-40", "warm overdrive, singing sustain"},
            {"40-60", "saturated overdrive, compressed"},
            {"60-80", "heavy overdrive, harmonically rich"},
            {"80-100", "fuzz territory, heavily compressed"}
        })
        .addParameter("Tone", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "dark, vintage amp, rolled-off highs"},
            {"20-40", "warm, British stack, smooth"},
            {"40-60", "balanced, American clean, neutral"},
            {"60-80", "bright, modern amp, presence"},
            {"80-100", "aggressive highs, cutting, fizzy"}
        })
        .addParameter("Output", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-40", "attenuated, quieter than input"},
            {"40-60", "unity gain, level matched"},
            {"60-100", "boosted, driving next stage hard"}
        })
        .setTriggerWords({
            "overdrive", "drive", "warm", "tube", "valve", "k-style", "boutique",
            "smooth", "creamy", "bluesy", "lead", "sustain", "breakup", "edge",
            "grit", "saturation", "analog", "vintage", "classic", "amp"
        })
        .setCompatibility({
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_PLATE_REVERB, 0.8f},
            {ENGINE_VINTAGE_TUBE_PREAMP, 0.6f},
            {ENGINE_BIT_CRUSHER, 0.3f},
            {ENGINE_RODENT_DISTORTION, 0.2f}
        })
        .setPairsWellWith({
            "compression", "eq", "reverb", "delay", "modulation", "wah"
        })
        .setAvoidWith({
            "other_distortion", "bit_crusher", "heavy_fuzz"
        })
        .setMoodAdjustments({
            {"warmer", 0.1f}, {"cleaner", -0.2f}, {"brighter", 0.2f},
            {"darker", -0.2f}, {"aggressive", 0.3f}, {"subtle", -0.15f},
            {"vintage", -0.1f}, {"modern", 0.15f}
        })
        .build()
    );
    
    // ENGINE 3: Rodent Distortion
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_RODENT_DISTORTION, "Rodent Distortion", "distortion",
                     "Aggressive op-amp clipping distortion with signature filter curve")
        .setSonicTags({
            "aggressive", "harsh", "fuzzy", "compressed", "tight", "focused",
            "mid-scooped", "cutting", "metallic", "industrial", "raw", "gnarly",
            "saturated", "clipped", "abrasive", "punk", "grunge", "dirty"
        })
        .setEmotionalTags({
            "angry", "rebellious", "fierce", "intense", "brutal", "uncompromising",
            "edgy", "dangerous", "chaotic", "confrontational", "energetic"
        })
        .setFrequencyFocus("high-mid")
        .setUseCases({
            "metal_guitar", "punk_guitar", "grunge_guitar", "aggressive_bass",
            "industrial_synth", "drum_destruction", "noise_music", "lo-fi_production"
        })
        .setInstrumentTags({
            "electric_guitar", "bass_guitar", "synthesizer", "drum_machine"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Gain", 0.4f, 0.0f, 1.0f, "%", "logarithmic", {
            {"0-20", "clean boost, subtle grit"},
            {"20-40", "moderate drive, crunch"},
            {"40-60", "heavy saturation, compressed"},
            {"60-80", "extreme overdrive, sustain"},
            {"80-100", "maximum gain, fuzz territory"}
        })
        .addParameter("Filter", 0.6f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "dark, muffled, sludgy"},
            {"20-40", "scooped mids, modern metal"},
            {"40-60", "balanced, classic RAT sound"},
            {"60-80", "bright, cutting, punk rock"},
            {"80-100", "harsh, fizzy, extreme highs"}
        })
        .addParameter("Clipping", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "soft clipping, warm saturation"},
            {"25-50", "moderate clipping, punchy"},
            {"50-75", "hard clipping, aggressive"},
            {"75-100", "extreme clipping, brutal"}
        })
        .addParameter("Tone", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "dark, vintage, rolled-off"},
            {"25-50", "balanced, classic tone"},
            {"50-75", "bright, modern, cutting"},
            {"75-100", "harsh, trebly, aggressive"}
        })
        .addParameter("Output", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-30", "quiet, attenuated"},
            {"30-60", "moderate level"},
            {"60-80", "unity gain, matched"},
            {"80-100", "boosted, loud"}
        })
        .addParameter("Mix", 1.0f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "mostly dry signal"},
            {"25-50", "blend of dry/wet"},
            {"50-75", "mostly distorted"},
            {"75-100", "fully wet, distorted"}
        })
        .addParameter("Mode", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "RAT style, op-amp clipping"},
            {"25-50", "Tube Screamer style, diode"},
            {"50-75", "Big Muff style, transistor"},
            {"75-100", "Fuzz Face style, germanium"}
        })
        .addParameter("Presence", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "smooth, no emphasis"},
            {"25-50", "subtle high-end lift"},
            {"50-75", "moderate presence boost"},
            {"75-100", "aggressive high-end, cutting"}
        })
        .setTriggerWords({
            "rat", "rodent", "distortion", "fuzz", "metal", "punk", "grunge",
            "aggressive", "harsh", "dirty", "gnarly", "industrial", "noise"
        })
        .setCompatibility({
            {ENGINE_NOISE_GATE, 0.9f},
            {ENGINE_PARAMETRIC_EQ, 0.85f},
            {ENGINE_TAPE_ECHO, 0.7f},
            {ENGINE_K_STYLE, 0.2f},
            {ENGINE_MUFF_FUZZ, 0.3f}
        })
        .setPairsWellWith({
            "noise_gate", "eq", "delay", "reverb", "cabinet_sim"
        })
        .setAvoidWith({
            "other_distortion", "overdrive", "fuzz"
        })
        .setMoodAdjustments({
            {"heavier", 0.2f}, {"brighter", 0.15f}, {"darker", -0.2f},
            {"tighter", -0.1f}, {"looser", 0.1f}, {"more_aggressive", 0.25f}
        })
        .build()
    );
    
    // ENGINE 4: Muff Fuzz
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_MUFF_FUZZ, "Muff Fuzz", "distortion",
                     "Thick, creamy fuzz with massive sustain and violin-like tone")
        .setSonicTags({
            "thick", "creamy", "sustained", "compressed", "wall-of-sound",
            "violin-like", "smooth", "massive", "saturated", "woolly",
            "vintage", "psychedelic", "stoner", "doom", "sludgy"
        })
        .setEmotionalTags({
            "epic", "massive", "dreamy", "psychedelic", "heavy", "monolithic",
            "transcendent", "overwhelming", "cosmic", "powerful"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "lead_guitar", "stoner_rock", "doom_metal", "psychedelic_rock",
            "shoegaze", "noise_rock", "ambient_guitar", "bass_fuzz"
        })
        .setInstrumentTags({
            "electric_guitar", "bass_guitar", "synthesizer"
        })
        .setTechnicalProps(0.25f, 0)
        .addParameter("Sustain", 0.6f, 0.0f, 1.0f, "%", "logarithmic", {
            {"0-20", "light fuzz, dynamic"},
            {"20-40", "moderate fuzz, compressed"},
            {"40-60", "heavy fuzz, sustained"},
            {"60-80", "massive sustain, violin-like"},
            {"80-100", "infinite sustain, feedback"}
        })
        .addParameter("Tone", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-30", "dark, muffled, bass-heavy"},
            {"30-50", "balanced, classic muff"},
            {"50-70", "bright, cutting mids"},
            {"70-100", "harsh, trebly, thin"}
        })
        .addParameter("Volume", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-50", "quiet to unity"},
            {"50-100", "unity to very loud"}
        })
        .setTriggerWords({
            "muff", "fuzz", "sustain", "thick", "creamy", "wall", "stoner",
            "doom", "psychedelic", "massive", "vintage", "60s", "70s"
        })
        .setCompatibility({
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_SHIMMER_REVERB, 0.95f},
            {ENGINE_PLATE_REVERB, 0.85f},
            {ENGINE_K_STYLE, 0.2f},
            {ENGINE_RODENT_DISTORTION, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "delay", "phaser", "octave", "wah"
        })
        .setAvoidWith({
            "other_fuzz", "distortion", "overdrive"
        })
        .setMoodAdjustments({
            {"thicker", 0.15f}, {"brighter", 0.2f}, {"darker", -0.15f},
            {"more_sustain", 0.2f}, {"cleaner", -0.3f}, {"wilder", 0.25f}
        })
        .build()
    );
    
    // ENGINE 20: Multiband Saturator
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_MULTIBAND_SATURATOR, "Multiband Saturator", "distortion",
                     "Frequency-conscious saturation with independent band control")
        .setSonicTags({
            "controlled", "frequency-specific", "transparent", "musical",
            "warm", "enhancing", "thickening", "polished", "professional",
            "mastering-grade", "subtle", "complex", "refined"
        })
        .setEmotionalTags({
            "polished", "expensive", "professional", "controlled", "refined",
            "sophisticated", "modern", "clean", "enhanced"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "mastering", "mix_bus", "drum_bus", "vocal_enhancement",
            "full_mix_warmth", "frequency_control", "subtle_saturation"
        })
        .setInstrumentTags({
            "full_mix", "drum_bus", "vocal_bus", "any"
        })
        .setTechnicalProps(0.5f, 64)  // Small lookahead for crossover
        .addParameter("Low Drive", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-30", "clean, no saturation"},
            {"30-60", "warm, subtle harmonics"},
            {"60-100", "heavy bass saturation"}
        })
        .addParameter("Mid Drive", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-30", "transparent mids"},
            {"30-60", "presence, forward"},
            {"60-100", "aggressive, crunchy"}
        })
        .addParameter("High Drive", 0.2f, 0.0f, 1.0f, "%", "linear", {
            {"0-30", "clean highs"},
            {"30-60", "smooth, silky"},
            {"60-100", "excited, sparkly"}
        })
        .addParameter("Crossover Low", 0.3f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-50", "80-250Hz"},
            {"50-100", "250-800Hz"}
        })
        .addParameter("Crossover High", 0.7f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-50", "2-5kHz"},
            {"50-100", "5-12kHz"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "multiband", "saturation", "warmth", "enhancement", "mastering",
            "transparent", "musical", "frequency", "control"
        })
        .setCompatibility({
            {ENGINE_MASTERING_LIMITER, 0.95f},
            {ENGINE_PARAMETRIC_EQ, 0.9f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.85f},
            {ENGINE_BIT_CRUSHER, 0.3f}
        })
        .setPairsWellWith({
            "eq", "compression", "limiting", "stereo_enhancement"
        })
        .setAvoidWith({
            "heavy_distortion", "bit_crusher", "extreme_effects"
        })
        .setMoodAdjustments({
            {"warmer", 0.1f}, {"punchier", 0.15f}, {"smoother", -0.1f},
            {"more_presence", 0.1f}, {"darker", -0.15f}, {"brighter", 0.1f}
        })
        .build()
    );
    
    // ==================== DELAY ENGINES ====================
    
    // ENGINE 1: Tape Echo
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_TAPE_ECHO, "Tape Echo", "delay",
                     "Authentic tape delay with wow, flutter, saturation, and degradation")
        .setSonicTags({
            "warm", "analog", "vintage", "wobbly", "saturated", "degraded",
            "lo-fi", "modulated", "unstable", "characterful", "feedback-capable",
            "self-oscillating", "dub", "psychedelic", "organic", "unpredictable"
        })
        .setEmotionalTags({
            "nostalgic", "dreamy", "hypnotic", "mysterious", "spacious",
            "floating", "ethereal", "retro", "experimental", "trippy"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "vocal_throw", "guitar_ambience", "dub_effects", "psychedelic_production",
            "vintage_emulation", "space_creation", "rhythmic_delays", "ambient_washes"
        })
        .setInstrumentTags({
            "vocals", "electric_guitar", "synthesizer", "drums", "percussion"
        })
        .setTechnicalProps(0.3f, 0)
        .addParameter("Time", 0.375f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-5", "comb filter, metallic"},
            {"5-15", "doubling, thickening"},
            {"15-50", "slapback echo, rockabilly"},
            {"50-150", "short echo, rhythmic"},
            {"150-400", "medium delay, musical"},
            {"400-800", "long delay, ambient"},
            {"800-1000", "very long, spacious"}
        })
        .addParameter("Feedback", 0.35f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "single repeat"},
            {"20-40", "few repeats, decaying"},
            {"40-60", "multiple repeats, building"},
            {"60-75", "many repeats, sustained"},
            {"75-85", "near oscillation, dubby"},
            {"85-95", "self-oscillation, runaway"},
            {"95-100", "chaos, uncontrolled"}
        })
        .addParameter("Wow & Flutter", 0.25f, 0.0f, 1.0f, "%", "linear", {
            {"0-10", "stable, no modulation"},
            {"10-30", "subtle vintage character"},
            {"30-50", "noticeable wobble"},
            {"50-70", "heavy degradation"},
            {"70-90", "extreme warping"},
            {"90-100", "broken machine"}
        })
        .addParameter("Saturation", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "clean repeats"},
            {"20-40", "warm compression"},
            {"40-60", "obvious saturation"},
            {"60-80", "heavy distortion"},
            {"80-100", "completely overdriven"}
        })
        .addParameter("Mix", 0.35f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "tape", "echo", "delay", "vintage", "analog", "space", "dub",
            "feedback", "oscillation", "psychedelic", "retro", "wobble"
        })
        .setCompatibility({
            {ENGINE_PLATE_REVERB, 0.95f},
            {ENGINE_SPRING_REVERB, 0.9f},
            {ENGINE_K_STYLE, 0.85f},
            {ENGINE_SPECTRAL_FREEZE, 0.4f},
            {ENGINE_DIGITAL_DELAY, 0.3f}
        })
        .build()
    );
    
    // ENGINE 5: Classic Tremolo
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_CLASSIC_TREMOLO, "Classic Tremolo", "modulation",
                     "Vintage amplitude modulation with smooth waveforms")
        .setSonicTags({
            "rhythmic", "vintage", "amplitude-modulation", "pulsing", "classic",
            "warm", "musical", "smooth", "surf", "western", "50s", "60s",
            "tube-amp", "wobble", "helicopter"
        })
        .setEmotionalTags({
            "nostalgic", "retro", "hypnotic", "dreamy", "romantic", "melancholic",
            "mysterious", "cinematic", "evocative", "timeless"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "vintage_guitar", "surf_rock", "country_music", "atmospheric_pads",
            "cinematic_effects", "retro_production", "tremolo_picking_emulation"
        })
        .setInstrumentTags({
            "electric_guitar", "electric_piano", "organ", "synthesizer", "vibraphone"
        })
        .setTechnicalProps(0.1f, 0)
        .addParameter("Rate", 0.3f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-10", "very slow, 0.1-1Hz"},
            {"10-30", "slow, 1-3Hz"},
            {"30-50", "moderate, 3-6Hz"},
            {"50-70", "fast, 6-10Hz"},
            {"70-90", "very fast, 10-15Hz"},
            {"90-100", "extreme, 15-20Hz"}
        })
        .addParameter("Depth", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle movement"},
            {"20-40", "noticeable pulse"},
            {"40-60", "strong modulation"},
            {"60-80", "deep tremolo"},
            {"80-100", "choppy, gating"}
        })
        .addParameter("Wave Shape", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "sine wave, smooth"},
            {"25-50", "triangle, vintage"},
            {"50-75", "square, choppy"},
            {"75-100", "sawtooth, ramping"}
        })
        .setTriggerWords({
            "tremolo", "amplitude", "volume", "pulse", "throb", "wobble",
            "vintage", "surf", "classic", "rhythmic", "helicopter"
        })
        .setCompatibility({
            {ENGINE_SPRING_REVERB, 0.95f},
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_K_STYLE, 0.85f},
            {ENGINE_HARMONIC_TREMOLO, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "delay", "overdrive", "compression"
        })
        .setAvoidWith({
            "other_tremolo", "extreme_modulation"
        })
        .setMoodAdjustments({
            {"faster", 0.2f}, {"slower", -0.2f}, {"deeper", 0.15f},
            {"subtler", -0.15f}, {"choppier", 0.3f}, {"smoother", -0.3f}
        })
        .build()
    );
    
    // ENGINE 6: Magnetic Drum Echo
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_MAGNETIC_DRUM_ECHO, "Magnetic Drum Echo", "delay",
                     "Vintage drum-based echo with multiple heads and feedback paths")
        .setSonicTags({
            "vintage", "warm", "analog", "multi-tap", "complex", "organic",
            "saturated", "spacious", "rhythmic", "textured", "degraded",
            "magnetic", "mechanical", "unpredictable", "lush"
        })
        .setEmotionalTags({
            "nostalgic", "dreamy", "expansive", "mysterious", "vintage",
            "psychedelic", "ethereal", "haunting", "cinematic"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "dub_production", "psychedelic_guitar", "ambient_textures", "vocal_effects",
            "rhythmic_delays", "vintage_production", "space_creation", "sound_design"
        })
        .setInstrumentTags({
            "electric_guitar", "vocals", "synthesizer", "drums", "brass", "organ"
        })
        .setTechnicalProps(0.35f, 0)
        .addParameter("Delay Time", 0.3f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-10", "very short, 10-50ms"},
            {"10-30", "slapback, 50-150ms"},
            {"30-50", "short echo, 150-300ms"},
            {"50-70", "medium echo, 300-600ms"},
            {"70-90", "long echo, 600-1200ms"},
            {"90-100", "very long, 1200-2000ms"}
        })
        .addParameter("Head 2", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0", "head 2 disabled"},
            {"1-30", "subtle second tap"},
            {"30-60", "prominent second echo"},
            {"60-100", "strong multi-tap effect"}
        })
        .addParameter("Head 3", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0", "head 3 disabled"},
            {"1-30", "subtle third tap"},
            {"30-60", "complex rhythm"},
            {"60-100", "dense echo pattern"}
        })
        .addParameter("Feedback", 0.4f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "single repeat"},
            {"20-40", "few repeats"},
            {"40-60", "sustained echoes"},
            {"60-80", "building feedback"},
            {"80-95", "self-oscillation edge"},
            {"95-100", "runaway feedback"}
        })
        .addParameter("Saturation", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "clean repeats"},
            {"20-40", "warm compression"},
            {"40-60", "obvious saturation"},
            {"60-80", "heavy distortion"},
            {"80-100", "extreme overdrive"}
        })
        .addParameter("Mix", 0.35f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "drum echo", "echorec", "multi-tap", "vintage echo", "space echo",
            "magnetic", "complex delay", "rhythmic echo", "dub delay"
        })
        .setCompatibility({
            {ENGINE_PLATE_REVERB, 0.9f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.85f},
            {ENGINE_TAPE_ECHO, 0.4f},
            {ENGINE_DIGITAL_DELAY, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "compression", "eq", "modulation", "filters"
        })
        .setAvoidWith({
            "other_delays", "extreme_modulation"
        })
        .setMoodAdjustments({
            {"spacier", 0.2f}, {"cleaner", -0.15f}, {"weirder", 0.25f},
            {"tighter", -0.2f}, {"more_complex", 0.2f}, {"simpler", -0.3f}
        })
        .build()
    );
    
    // ENGINE 7: Bucket Brigade Delay
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_BUCKET_BRIGADE_DELAY, "Bucket Brigade Delay", "delay",
                     "Analog BBD delay with characteristic dark repeats and modulation")
        .setSonicTags({
            "analog", "dark", "warm", "degraded", "lo-fi", "murky",
            "compressed", "modulated", "vintage", "bucket-brigade", "filtered",
            "organic", "imperfect", "characterful"
        })
        .setEmotionalTags({
            "nostalgic", "melancholic", "dreamy", "intimate", "lo-fi",
            "vintage", "mysterious", "moody", "atmospheric"
        })
        .setFrequencyFocus("low-mid")
        .setUseCases({
            "analog_delay", "guitar_solos", "vintage_production", "lo-fi_aesthetic",
            "ambient_music", "dub_delays", "warm_echoes", "chorus_effects"
        })
        .setInstrumentTags({
            "electric_guitar", "synthesizer", "electric_piano", "bass", "vocals"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Time", 0.25f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-10", "chorus/flanger, 1-20ms"},
            {"10-20", "doubling, 20-50ms"},
            {"20-40", "slapback, 50-150ms"},
            {"40-60", "short echo, 150-300ms"},
            {"60-80", "medium echo, 300-500ms"},
            {"80-100", "long echo, 500-800ms"}
        })
        .addParameter("Feedback", 0.4f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "single repeat"},
            {"20-40", "2-3 repeats"},
            {"40-60", "4-6 repeats"},
            {"60-75", "many repeats"},
            {"75-90", "oscillation edge"},
            {"90-100", "self-oscillation"}
        })
        .addParameter("Modulation", 0.2f, 0.0f, 1.0f, "%", "linear", {
            {"0", "no modulation"},
            {"1-20", "subtle movement"},
            {"20-40", "gentle chorus"},
            {"40-60", "obvious wobble"},
            {"60-80", "heavy modulation"},
            {"80-100", "seasick warble"}
        })
        .addParameter("Tone", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "very dark, muffled"},
            {"20-40", "dark, vintage"},
            {"40-60", "balanced, warm"},
            {"60-80", "brighter, modern"},
            {"80-100", "bright, clear"}
        })
        .addParameter("Mix", 0.35f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "bucket brigade", "bbd", "analog delay", "dark delay", "warm echo",
            "vintage delay", "lo-fi delay", "murky", "degraded"
        })
        .setCompatibility({
            {ENGINE_SPRING_REVERB, 0.9f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.85f},
            {ENGINE_ANALOG_PHASER, 0.8f},
            {ENGINE_DIGITAL_DELAY, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "overdrive", "compression", "modulation"
        })
        .setAvoidWith({
            "digital_delays", "pristine_effects"
        })
        .setMoodAdjustments({
            {"darker", -0.2f}, {"brighter", 0.2f}, {"weirder", 0.2f},
            {"cleaner", 0.15f}, {"more_lofi", -0.25f}, {"spacier", 0.15f}
        })
        .build()
    );
    
    // ENGINE 8: Digital Delay
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_DIGITAL_DELAY, "Digital Delay", "delay",
                     "Pristine digital delay with precise timing and clear repeats")
        .setSonicTags({
            "pristine", "clear", "precise", "digital", "clean", "accurate",
            "modern", "transparent", "surgical", "hi-fi", "crystal-clear",
            "predictable", "stable", "bright"
        })
        .setEmotionalTags({
            "modern", "clean", "precise", "clinical", "futuristic",
            "professional", "polished", "clear", "direct"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "precise_delays", "rhythmic_patterns", "modern_production", "vocal_delays",
            "tempo_synced", "clean_echoes", "ambient_delays", "studio_delay"
        })
        .setInstrumentTags({
            "vocals", "synthesizer", "drums", "any", "acoustic_guitar", "piano"
        })
        .setTechnicalProps(0.15f, 0)
        .addParameter("Time", 0.25f, 0.0f, 1.0f, "ms", "linear", {
            {"0-5", "haas effect, 1-10ms"},
            {"5-15", "doubling, 10-30ms"},
            {"15-30", "slapback, 30-100ms"},
            {"30-50", "short delay, 100-250ms"},
            {"50-70", "medium delay, 250-500ms"},
            {"70-85", "long delay, 500-1000ms"},
            {"85-100", "very long, 1000-2000ms"}
        })
        .addParameter("Feedback", 0.3f, 0.0f, 1.0f, "%", "linear", ParamRanges::FEEDBACK_RANGES)
        .addParameter("High Cut", 0.8f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "very dark, 1-2kHz"},
            {"20-40", "dark, 2-5kHz"},
            {"40-60", "warm, 5-8kHz"},
            {"60-80", "bright, 8-12kHz"},
            {"80-100", "full range, 12-20kHz"}
        })
        .addParameter("Low Cut", 0.1f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "full bass, 20-50Hz"},
            {"20-40", "controlled, 50-100Hz"},
            {"40-60", "clean, 100-200Hz"},
            {"60-80", "thin, 200-400Hz"},
            {"80-100", "telephone, 400-800Hz"}
        })
        .addParameter("Mix", 0.3f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "digital delay", "pristine", "clean delay", "precise", "modern delay",
            "clear", "studio delay", "tempo sync"
        })
        .setCompatibility({
            {ENGINE_PLATE_REVERB, 0.95f},
            {ENGINE_PARAMETRIC_EQ, 0.9f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.85f},
            {ENGINE_TAPE_ECHO, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "eq", "compression", "modulation", "filters"
        })
        .setAvoidWith({
            "analog_delays", "lo-fi_effects"
        })
        .setMoodAdjustments({
            {"warmer", -0.2f}, {"cleaner", 0.1f}, {"darker", -0.15f},
            {"brighter", 0.15f}, {"tighter", -0.1f}, {"spacier", 0.2f}
        })
        .build()
    );
    
    // ENGINE 9: Harmonic Tremolo
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_HARMONIC_TREMOLO, "Harmonic Tremolo", "modulation",
                     "Frequency-split tremolo with independent high/low modulation")
        .setSonicTags({
            "complex", "harmonic", "frequency-split", "vintage", "unique",
            "phasey", "swirling", "dimensional", "rich", "textured",
            "brownface", "psychedelic", "musical", "organic"
        })
        .setEmotionalTags({
            "mysterious", "dreamy", "psychedelic", "sophisticated", "vintage",
            "ethereal", "complex", "mesmerizing", "trippy"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "psychedelic_guitar", "vintage_keys", "atmospheric_production", "unique_modulation",
            "studio_effects", "creative_mixing", "character_effects"
        })
        .setInstrumentTags({
            "electric_guitar", "electric_piano", "organ", "synthesizer", "vibraphone"
        })
        .setTechnicalProps(0.25f, 0)
        .addParameter("Rate", 0.35f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-15", "very slow, 0.1-1Hz"},
            {"15-35", "slow, 1-3Hz"},
            {"35-55", "medium, 3-6Hz"},
            {"55-75", "fast, 6-10Hz"},
            {"75-90", "very fast, 10-15Hz"},
            {"90-100", "extreme, 15-20Hz"}
        })
        .addParameter("Depth", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle movement"},
            {"20-40", "gentle modulation"},
            {"40-60", "obvious effect"},
            {"60-80", "deep modulation"},
            {"80-100", "extreme, throbbing"}
        })
        .addParameter("Crossover", 0.5f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "very low, 100-200Hz"},
            {"20-40", "low, 200-400Hz"},
            {"40-60", "medium, 400-800Hz"},
            {"60-80", "high, 800-1600Hz"},
            {"80-100", "very high, 1600-3200Hz"}
        })
        .addParameter("Phase", 0.5f, 0.0f, 1.0f, "degrees", "linear", {
            {"0-25", "in phase, unified"},
            {"25-50", "slight offset, wider"},
            {"50-75", "90 degrees, swirling"},
            {"75-100", "180 degrees, dramatic"}
        })
        .setTriggerWords({
            "harmonic tremolo", "frequency tremolo", "split tremolo", "brownface",
            "complex tremolo", "vintage tremolo", "phasey", "swirling"
        })
        .setCompatibility({
            {ENGINE_SPRING_REVERB, 0.95f},
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_K_STYLE, 0.85f},
            {ENGINE_CLASSIC_TREMOLO, 0.3f},
            {ENGINE_ROTARY_SPEAKER, 0.4f}
        })
        .setPairsWellWith({
            "reverb", "delay", "overdrive", "compression", "eq"
        })
        .setAvoidWith({
            "other_tremolo", "heavy_modulation", "rotary_speaker"
        })
        .setMoodAdjustments({
            {"weirder", 0.2f}, {"subtler", -0.15f}, {"faster", 0.2f},
            {"slower", -0.2f}, {"more_phase", 0.25f}, {"vintage", 0.1f}
        })
        .build()
    );
    
    // ENGINE 10: Rotary Speaker
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_ROTARY_SPEAKER, "Rotary Speaker", "modulation",
                     "Leslie-style rotating speaker with horn and drum simulation")
        .setSonicTags({
            "rotary", "leslie", "doppler", "swirling", "3D", "spatial",
            "organ", "vintage", "complex-modulation", "stereo-field",
            "mechanical", "authentic", "rich", "dimensional"
        })
        .setEmotionalTags({
            "soulful", "organic", "vintage", "gospel", "bluesy", "spiritual",
            "dynamic", "expressive", "nostalgic", "powerful"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "organ_sounds", "guitar_effects", "vintage_production", "psychedelic_music",
            "gospel_organ", "blues_guitar", "spatial_effects", "stereo_enhancement"
        })
        .setInstrumentTags({
            "organ", "electric_guitar", "electric_piano", "synthesizer", "vocals"
        })
        .setTechnicalProps(0.4f, 0, false, true)  // Requires stereo
        .addParameter("Speed", 0.5f, 0.0f, 1.0f, "", "stepped", {
            {"0-33", "slow/chorale, 0.7Hz"},
            {"33-66", "fast/tremolo, 6.5Hz"},
            {"66-100", "ramping between speeds"}
        })
        .addParameter("Horn/Drum Mix", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "drum heavy, warm"},
            {"20-40", "drum focused"},
            {"40-60", "balanced mix"},
            {"60-80", "horn focused"},
            {"80-100", "horn only, bright"}
        })
        .addParameter("Microphone Distance", 0.3f, 0.0f, 1.0f, "", "linear", {
            {"0-20", "close mic, direct"},
            {"20-40", "near field"},
            {"40-60", "medium distance"},
            {"60-80", "far field, roomy"},
            {"80-100", "ambient, distant"}
        })
        .addParameter("Drive", 0.2f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "clean rotation"},
            {"20-40", "warm overdrive"},
            {"40-60", "tube saturation"},
            {"60-80", "driven hard"},
            {"80-100", "screaming leslie"}
        })
        .addParameter("Stereo Width", 0.8f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "narrow, mono-ish"},
            {"20-40", "focused stereo"},
            {"40-60", "natural width"},
            {"60-80", "wide stereo"},
            {"80-100", "extreme width"}
        })
        .setTriggerWords({
            "rotary", "leslie", "rotating", "organ", "swirl", "doppler",
            "hammond", "b3", "vintage organ", "speaker cabinet"
        })
        .setCompatibility({
            {ENGINE_K_STYLE, 0.9f},
            {ENGINE_SPRING_REVERB, 0.85f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.8f},
            {ENGINE_HARMONIC_TREMOLO, 0.4f},
            {ENGINE_STEREO_CHORUS, 0.3f}
        })
        .setPairsWellWith({
            "overdrive", "reverb", "compression", "eq", "delay"
        })
        .setAvoidWith({
            "other_rotary", "heavy_modulation", "chorus", "tremolo"
        })
        .setMoodAdjustments({
            {"faster", 0.5f}, {"slower", -0.5f}, {"dirtier", 0.2f},
            {"cleaner", -0.2f}, {"wider", 0.15f}, {"more_vintage", 0.1f}
        })
        .build()
    );
    
    // ENGINE 11: Detune Doubler
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_DETUNE_DOUBLER, "Detune Doubler", "modulation",
                     "Pitch-based doubling effect for thickening and widening")
        .setSonicTags({
            "thick", "wide", "doubled", "detuned", "chorus-like", "rich",
            "stereo", "lush", "smooth", "dimensional", "studio-trick",
            "thickening", "fattening", "polished"
        })
        .setEmotionalTags({
            "expansive", "professional", "polished", "modern", "slick",
            "commercial", "radio-ready", "expensive", "produced"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "vocal_thickening", "guitar_doubling", "synth_widening", "mix_enhancement",
            "stereo_imaging", "production_polish", "lead_thickening"
        })
        .setInstrumentTags({
            "vocals", "electric_guitar", "synthesizer", "acoustic_guitar", "strings"
        })
        .setTechnicalProps(0.3f, 256)  // Small latency for pitch shifting
        .addParameter("Detune", 0.15f, 0.0f, 1.0f, "cents", "linear", {
            {"0-10", "micro detune, 0-5 cents"},
            {"10-30", "subtle, 5-15 cents"},
            {"30-50", "noticeable, 15-30 cents"},
            {"50-70", "obvious, 30-50 cents"},
            {"70-90", "extreme, 50-75 cents"},
            {"90-100", "dissonant, 75-100 cents"}
        })
        .addParameter("Voices", 0.5f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "2 voices, simple"},
            {"25-50", "3 voices, fuller"},
            {"50-75", "4 voices, rich"},
            {"75-100", "6 voices, massive"}
        })
        .addParameter("Spread", 0.7f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "center, mono"},
            {"20-40", "narrow stereo"},
            {"40-60", "moderate width"},
            {"60-80", "wide stereo"},
            {"80-100", "extreme width"}
        })
        .addParameter("Delay", 0.3f, 0.0f, 1.0f, "ms", "linear", {
            {"0-20", "tight, 0-10ms"},
            {"20-40", "subtle, 10-20ms"},
            {"40-60", "spacious, 20-30ms"},
            {"60-80", "loose, 30-40ms"},
            {"80-100", "slapback, 40-50ms"}
        })
        .addParameter("Mix", 0.4f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "doubler", "double", "thicken", "widen", "detune", "unison",
            "voices", "spread", "stereo", "fatten"
        })
        .setCompatibility({
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_PLATE_REVERB, 0.85f},
            {ENGINE_STEREO_CHORUS, 0.4f},
            {ENGINE_PITCH_SHIFTER, 0.3f}
        })
        .setPairsWellWith({
            "compression", "eq", "reverb", "delay", "saturation"
        })
        .setAvoidWith({
            "chorus", "heavy_pitch_effects", "other_doublers"
        })
        .setMoodAdjustments({
            {"thicker", 0.2f}, {"wider", 0.15f}, {"tighter", -0.2f},
            {"more_voices", 0.25f}, {"subtler", -0.15f}, {"extreme", 0.3f}
        })
        .build()
    );
    
    // ENGINE 12: Ladder Filter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_LADDER_FILTER, "Ladder Filter", "filter",
                     "Moog-style 4-pole resonant lowpass filter with self-oscillation")
        .setSonicTags({
            "resonant", "squelchy", "moog", "analog", "warm", "musical",
            "self-oscillating", "smooth", "creamy", "vintage", "synth",
            "liquid", "organic", "fat", "juicy"
        })
        .setEmotionalTags({
            "funky", "groovy", "electronic", "retro", "futuristic", "danceable",
            "hypnotic", "psychedelic", "powerful", "expressive"
        })
        .setFrequencyFocus("variable")
        .setUseCases({
            "synth_bass", "filter_sweeps", "electronic_music", "funk_guitar",
            "creative_filtering", "sound_design", "edm_production", "acid_sounds"
        })
        .setInstrumentTags({
            "synthesizer", "bass_guitar", "electric_guitar", "drum_loops", "any"
        })
        .setTechnicalProps(0.25f, 0)
        .addParameter("Cutoff", 0.7f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-10", "sub bass, 20-60Hz"},
            {"10-30", "bass, 60-250Hz"},
            {"30-50", "low mids, 250-1kHz"},
            {"50-70", "mids, 1-4kHz"},
            {"70-90", "highs, 4-10kHz"},
            {"90-100", "air, 10-20kHz"}
        })
        .addParameter("Resonance", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle emphasis"},
            {"20-40", "pronounced peak"},
            {"40-60", "strong resonance"},
            {"60-80", "aggressive, ringing"},
            {"80-95", "near oscillation"},
            {"95-100", "self-oscillation"}
        })
        .addParameter("Drive", 0.2f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "clean, linear"},
            {"20-40", "warm saturation"},
            {"40-60", "obvious distortion"},
            {"60-80", "heavy overdrive"},
            {"80-100", "screaming filter"}
        })
        .addParameter("Envelope Amount", 0.0f, -1.0f, 1.0f, "%", "linear", {
            {"-100--60", "strong negative, reverse"},
            {"-60--20", "subtle negative"},
            {"-20-20", "no envelope"},
            {"20-60", "subtle positive"},
            {"60-100", "strong positive sweep"}
        })
        .addParameter("Mix", 1.0f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "ladder", "moog", "filter", "lowpass", "resonant", "squelch",
            "acid", "303", "sweep", "synth filter"
        })
        .setCompatibility({
            {ENGINE_ENVELOPE_FILTER, 0.85f},
            {ENGINE_K_STYLE, 0.9f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.8f},
            {ENGINE_STATE_VARIABLE_FILTER, 0.4f}
        })
        .setPairsWellWith({
            "distortion", "delay", "reverb", "modulation", "compression"
        })
        .setAvoidWith({
            "other_filters", "extreme_resonance_effects"
        })
        .setMoodAdjustments({
            {"brighter", 0.2f}, {"darker", -0.3f}, {"more_resonant", 0.2f},
            {"cleaner", -0.15f}, {"dirtier", 0.2f}, {"more_movement", 0.3f}
        })
        .build()
    );
    
    // ENGINE 13: Formant Filter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_FORMANT_FILTER, "Formant Filter", "filter",
                     "Vowel-based filter creating vocal-like formant resonances")
        .setSonicTags({
            "vocal", "vowel", "formant", "talk-box", "human", "organic",
            "expressive", "unique", "characterful", "speaking", "singing",
            "resonant", "complex", "animated"
        })
        .setEmotionalTags({
            "expressive", "human", "organic", "playful", "unique", "futuristic",
            "robotic", "alien", "communicative", "animated"
        })
        .setFrequencyFocus("mid")
        .setUseCases({
            "talk_box_effects", "vocal_processing", "synth_animation", "creative_filtering",
            "electronic_music", "sound_design", "funk_guitar", "unique_effects"
        })
        .setInstrumentTags({
            "synthesizer", "electric_guitar", "vocals", "bass", "any"
        })
        .setTechnicalProps(0.3f, 0)
        .addParameter("Vowel", 0.5f, 0.0f, 1.0f, "", "linear", {
            {"0-12", "A - open"},
            {"12-25", "A-E blend"},
            {"25-37", "E - front"},
            {"37-50", "E-I blend"},
            {"50-62", "I - closed"},
            {"62-75", "I-O blend"},
            {"75-87", "O - round"},
            {"87-100", "U - back"}
        })
        .addParameter("Morph", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "static vowel"},
            {"20-40", "slight movement"},
            {"40-60", "morphing vowels"},
            {"60-80", "animated talking"},
            {"80-100", "extreme morphing"}
        })
        .addParameter("Resonance", 0.6f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle formants"},
            {"20-40", "clear vowels"},
            {"40-60", "pronounced effect"},
            {"60-80", "strong resonance"},
            {"80-100", "extreme, synthetic"}
        })
        .addParameter("Gender", 0.5f, 0.0f, 1.0f, "", "linear", {
            {"0-25", "deep male"},
            {"25-40", "male"},
            {"40-60", "neutral"},
            {"60-75", "female"},
            {"75-100", "child/soprano"}
        })
        .addParameter("Mix", 0.7f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "formant", "vowel", "vocal", "talk box", "voice", "speaking",
            "human", "morphing", "animated"
        })
        .setCompatibility({
            {ENGINE_ENVELOPE_FILTER, 0.8f},
            {ENGINE_K_STYLE, 0.85f},
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_VOCAL_FORMANT_FILTER, 0.3f}
        })
        .setPairsWellWith({
            "delay", "reverb", "distortion", "modulation", "pitch_effects"
        })
        .setAvoidWith({
            "other_formant_filters", "heavy_filtering"
        })
        .setMoodAdjustments({
            {"more_vocal", 0.2f}, {"robotic", 0.3f}, {"natural", -0.2f},
            {"animated", 0.25f}, {"static", -0.3f}, {"extreme", 0.3f}
        })
        .build()
    );
    
    // ENGINE 14: Classic Compressor
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_CLASSIC_COMPRESSOR, "Classic Compressor", "dynamics",
                     "VCA-style compressor with smooth gain reduction")
        .setSonicTags({
            "transparent", "smooth", "musical", "vca", "clean", "controlled",
            "punchy", "professional", "studio", "versatile", "precise",
            "balanced", "natural", "polished"
        })
        .setEmotionalTags({
            "professional", "controlled", "polished", "confident", "tight",
            "focused", "powerful", "clean", "modern"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "vocal_compression", "drum_control", "bass_tightening", "mix_glue",
            "dynamic_control", "peak_limiting", "general_compression", "mastering"
        })
        .setInstrumentTags({
            "vocals", "drums", "bass", "full_mix", "any"
        })
        .setTechnicalProps(0.15f, 0)
        .addParameter("Threshold", -12.0f, -40.0f, 0.0f, "dB", "linear", {
            {"-40--30", "very sensitive"},
            {"-30--20", "sensitive"},
            {"-20--10", "moderate"},
            {"-10--5", "light compression"},
            {"-5-0", "peak limiting only"}
        })
        .addParameter("Ratio", 4.0f, 1.0f, 20.0f, ":1", "logarithmic", {
            {"1-2", "gentle compression"},
            {"2-4", "moderate compression"},
            {"4-8", "heavy compression"},
            {"8-12", "strong compression"},
            {"12-20", "limiting"}
        })
        .addParameter("Attack", 10.0f, 0.1f, 100.0f, "ms", "logarithmic", {
            {"0.1-1", "ultra fast, transient control"},
            {"1-5", "fast, drums/percussion"},
            {"5-20", "medium, general use"},
            {"20-50", "slow, vocals/bass"},
            {"50-100", "very slow, gentle"}
        })
        .addParameter("Release", 100.0f, 10.0f, 1000.0f, "ms", "logarithmic", {
            {"10-50", "very fast, pumping"},
            {"50-150", "fast, rhythmic"},
            {"150-300", "medium, natural"},
            {"300-600", "slow, smooth"},
            {"600-1000", "very slow, gentle"}
        })
        .addParameter("Makeup Gain", 0.0f, 0.0f, 24.0f, "dB", "linear", {
            {"0-6", "subtle boost"},
            {"6-12", "moderate gain"},
            {"12-18", "significant boost"},
            {"18-24", "heavy makeup gain"}
        })
        .setTriggerWords({
            "compressor", "compression", "dynamics", "vca", "control", "punch",
            "tighten", "glue", "smooth"
        })
        .setCompatibility({
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_K_STYLE, 0.9f},
            {ENGINE_MASTERING_LIMITER, 0.85f},
            {ENGINE_VINTAGE_OPTO_COMPRESSOR, 0.4f}
        })
        .setPairsWellWith({
            "eq", "saturation", "reverb", "delay", "any"
        })
        .setAvoidWith({
            "other_compressors", "heavy_limiting"
        })
        .setMoodAdjustments({
            {"punchier", -0.2f}, {"smoother", 0.2f}, {"tighter", 0.15f},
            {"more_natural", -0.15f}, {"aggressive", 0.25f}, {"gentle", -0.25f}
        })
        .build()
    );
    
    // ENGINE 15: State Variable Filter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_STATE_VARIABLE_FILTER, "State Variable Filter", "filter",
                     "Multi-mode filter with LP/HP/BP/Notch modes and resonance")
        .setSonicTags({
            "versatile", "multi-mode", "resonant", "clean", "precise", "surgical",
            "transparent", "flexible", "modern", "digital", "controllable",
            "morphing", "smooth", "technical"
        })
        .setEmotionalTags({
            "technical", "precise", "modern", "clean", "futuristic", "clinical",
            "controlled", "versatile", "adaptive"
        })
        .setFrequencyFocus("variable")
        .setUseCases({
            "sound_design", "creative_filtering", "surgical_eq", "electronic_music",
            "mixing", "frequency_sculpting", "resonant_sweeps", "multi_mode_filtering"
        })
        .setInstrumentTags({
            "synthesizer", "drums", "any", "electronic", "samples"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Frequency", 0.5f, 0.0f, 1.0f, "Hz", "logarithmic", ParamRanges::FREQ_RANGES)
        .addParameter("Resonance", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle emphasis"},
            {"20-40", "moderate resonance"},
            {"40-60", "strong peak"},
            {"60-80", "aggressive resonance"},
            {"80-100", "extreme, ringing"}
        })
        .addParameter("Mode", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "lowpass"},
            {"25-50", "highpass"},
            {"50-75", "bandpass"},
            {"75-100", "notch/reject"}
        })
        .addParameter("Slope", 0.5f, 0.0f, 1.0f, "dB/oct", "stepped", {
            {"0-33", "12dB/octave, gentle"},
            {"33-66", "24dB/octave, steep"},
            {"66-100", "48dB/octave, brick wall"}
        })
        .addParameter("Mix", 1.0f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "state variable", "svf", "multimode", "filter", "lowpass", "highpass",
            "bandpass", "notch", "morphing filter"
        })
        .setCompatibility({
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},
            {ENGINE_PARAMETRIC_EQ, 0.85f},
            {ENGINE_K_STYLE, 0.8f},
            {ENGINE_LADDER_FILTER, 0.4f}
        })
        .setPairsWellWith({
            "distortion", "delay", "reverb", "modulation", "compression"
        })
        .setAvoidWith({
            "other_multimode_filters", "heavy_filtering"
        })
        .setMoodAdjustments({
            {"brighter", 0.2f}, {"darker", -0.2f}, {"more_resonant", 0.2f},
            {"cleaner", -0.15f}, {"more_aggressive", 0.25f}, {"surgical", 0.3f}
        })
        .build()
    );
    
    // ENGINE 16: Stereo Chorus
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_STEREO_CHORUS, "Stereo Chorus", "modulation",
                     "Rich stereo chorus with multiple voices and LFO modulation")
        .setSonicTags({
            "lush", "wide", "rich", "shimmering", "detuned", "thick",
            "stereo", "vintage", "80s", "dreamy", "smooth", "polished",
            "swirling", "dimensional", "ensemble"
        })
        .setEmotionalTags({
            "dreamy", "nostalgic", "ethereal", "romantic", "melancholic",
            "expansive", "emotional", "beautiful", "lush"
        })
        .setFrequencyFocus("high-mid")
        .setUseCases({
            "guitar_thickening", "synth_pads", "vocal_widening", "80s_production",
            "clean_guitar", "string_ensemble", "ambient_textures", "stereo_enhancement"
        })
        .setInstrumentTags({
            "electric_guitar", "synthesizer", "electric_piano", "strings", "vocals"
        })
        .setTechnicalProps(0.2f, 0, false, true)
        .addParameter("Rate", 0.3f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "very slow, 0.1-0.5Hz"},
            {"20-40", "slow, 0.5-1Hz"},
            {"40-60", "moderate, 1-3Hz"},
            {"60-80", "fast, 3-6Hz"},
            {"80-100", "very fast, 6-10Hz"}
        })
        .addParameter("Depth", 0.4f, 0.0f, 1.0f, "%", "linear", ParamRanges::MOD_DEPTH_RANGES)
        .addParameter("Voices", 0.5f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "2 voices, simple"},
            {"25-50", "4 voices, rich"},
            {"50-75", "6 voices, lush"},
            {"75-100", "8 voices, massive"}
        })
        .addParameter("Width", 0.8f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "narrow, centered"},
            {"20-40", "moderate width"},
            {"40-60", "wide stereo"},
            {"60-80", "very wide"},
            {"80-100", "extreme width"}
        })
        .addParameter("Mix", 0.35f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "chorus", "ensemble", "thick", "lush", "stereo", "wide",
            "80s", "vintage", "shimmer", "detune"
        })
        .setCompatibility({
            {ENGINE_PLATE_REVERB, 0.9f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.85f},
            {ENGINE_PARAMETRIC_EQ, 0.9f},
            {ENGINE_DETUNE_DOUBLER, 0.4f},
            {ENGINE_ROTARY_SPEAKER, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "delay", "compression", "eq", "overdrive"
        })
        .setAvoidWith({
            "other_chorus", "heavy_modulation", "pitch_effects"
        })
        .setMoodAdjustments({
            {"lusher", 0.2f}, {"subtler", -0.2f}, {"wider", 0.15f},
            {"vintage", 0.1f}, {"modern", -0.1f}, {"dreamier", 0.25f}
        })
        .build()
    );
    
    // ENGINE 17: Spectral Freeze
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_SPECTRAL_FREEZE, "Spectral Freeze", "spectral",
                     "Freezes the spectral content creating infinite sustain drones")
        .setSonicTags({
            "freeze", "infinite", "sustained", "drone", "ambient", "spectral",
            "ethereal", "crystalline", "static", "textural", "experimental",
            "otherworldly", "pad-like", "atmospheric"
        })
        .setEmotionalTags({
            "ethereal", "mystical", "transcendent", "meditative", "cosmic",
            "suspended", "timeless", "otherworldly", "contemplative"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "ambient_music", "drone_creation", "sound_design", "film_scoring",
            "experimental_music", "pad_creation", "texture_layers", "transitions"
        })
        .setInstrumentTags({
            "any", "synthesizer", "guitar", "vocals", "strings", "field_recordings"
        })
        .setTechnicalProps(0.6f, 512)  // FFT processing
        .addParameter("Freeze", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-50", "normal processing"},
            {"50-100", "frozen spectrum"}
        })
        .addParameter("Smooth", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "sharp, metallic"},
            {"20-40", "crisp freeze"},
            {"40-60", "balanced smoothness"},
            {"60-80", "smooth, warm"},
            {"80-100", "very smooth, blurred"}
        })
        .addParameter("Spectral Blur", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "precise frequencies"},
            {"20-40", "slight smearing"},
            {"40-60", "moderate blur"},
            {"60-80", "heavy smearing"},
            {"80-100", "extreme blur"}
        })
        .addParameter("High Damp", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "bright, full spectrum"},
            {"20-40", "slight damping"},
            {"40-60", "moderate damping"},
            {"60-80", "dark, muffled"},
            {"80-100", "very dark"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "freeze", "spectral", "infinite", "sustain", "drone", "ambient",
            "pad", "texture", "experimental"
        })
        .setCompatibility({
            {ENGINE_SHIMMER_REVERB, 0.95f},
            {ENGINE_PLATE_REVERB, 0.85f},
            {ENGINE_GRANULAR_CLOUD, 0.9f},
            {ENGINE_TAPE_ECHO, 0.4f}
        })
        .setPairsWellWith({
            "reverb", "delay", "filters", "modulation", "pitch_shifters"
        })
        .setAvoidWith({
            "heavy_distortion", "transient_shapers", "gates"
        })
        .setMoodAdjustments({
            {"ethereal", 0.2f}, {"darker", 0.3f}, {"brighter", -0.2f},
            {"smoother", 0.15f}, {"more_metallic", -0.25f}, {"ambient", 0.3f}
        })
        .build()
    );
    
    // ENGINE 18: Granular Cloud
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_GRANULAR_CLOUD, "Granular Cloud", "granular",
                     "Granular synthesis creating particle clouds from input audio")
        .setSonicTags({
            "granular", "textural", "cloud", "particles", "experimental",
            "scattered", "fragmented", "atmospheric", "complex", "evolving",
            "chaotic", "crystalline", "pointillistic", "abstract"
        })
        .setEmotionalTags({
            "experimental", "abstract", "futuristic", "mysterious", "alien",
            "fragmented", "disorienting", "fascinating", "complex"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "sound_design", "experimental_music", "ambient_textures", "film_scoring",
            "electronic_music", "glitch_effects", "creative_processing", "texture_generation"
        })
        .setInstrumentTags({
            "any", "synthesizer", "vocals", "field_recordings", "percussion"
        })
        .setTechnicalProps(0.7f, 1024)  // Buffer for grain storage
        .addParameter("Grain Size", 0.3f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "tiny grains, 1-10ms"},
            {"20-40", "small, 10-30ms"},
            {"40-60", "medium, 30-80ms"},
            {"60-80", "large, 80-200ms"},
            {"80-100", "huge, 200-500ms"}
        })
        .addParameter("Density", 0.5f, 0.0f, 1.0f, "grains/sec", "linear", {
            {"0-20", "sparse, 5-20/sec"},
            {"20-40", "moderate, 20-50/sec"},
            {"40-60", "dense, 50-100/sec"},
            {"60-80", "very dense, 100-200/sec"},
            {"80-100", "cloud, 200-500/sec"}
        })
        .addParameter("Position", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "beginning of buffer"},
            {"20-40", "early position"},
            {"40-60", "middle position"},
            {"60-80", "late position"},
            {"80-100", "end of buffer"}
        })
        .addParameter("Spray", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "focused, no scatter"},
            {"20-40", "slight randomization"},
            {"40-60", "moderate scatter"},
            {"60-80", "wide distribution"},
            {"80-100", "chaotic spray"}
        })
        .addParameter("Pitch Spray", 0.2f, 0.0f, 1.0f, "cents", "linear", {
            {"0-20", "no pitch variation"},
            {"20-40", "subtle, ±10 cents"},
            {"40-60", "moderate, ±50 cents"},
            {"60-80", "wide, ±200 cents"},
            {"80-100", "extreme, ±1 octave"}
        })
        .addParameter("Mix", 0.6f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "granular", "grains", "cloud", "particles", "texture", "scatter",
            "experimental", "abstract", "pointillistic"
        })
        .setCompatibility({
            {ENGINE_SPECTRAL_FREEZE, 0.9f},
            {ENGINE_SHIMMER_REVERB, 0.85f},
            {ENGINE_PLATE_REVERB, 0.8f},
            {ENGINE_BIT_CRUSHER, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "delay", "filters", "spectral_effects", "modulation"
        })
        .setAvoidWith({
            "heavy_distortion", "compressors", "transient_shapers"
        })
        .setMoodAdjustments({
            {"more_abstract", 0.3f}, {"denser", 0.2f}, {"sparser", -0.2f},
            {"more_chaotic", 0.25f}, {"tighter", -0.25f}, {"weirder", 0.35f}
        })
        .build()
    );
    
    // ENGINE 19: Analog Ring Modulator
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_ANALOG_RING_MODULATOR, "Analog Ring Modulator", "modulation",
                     "Classic ring modulation creating metallic and bell-like tones")
        .setSonicTags({
            "metallic", "bell-like", "robotic", "inharmonic", "alien",
            "vintage", "experimental", "harsh", "clangorous", "sci-fi",
            "dalek", "synthetic", "dissonant", "unique"
        })
        .setEmotionalTags({
            "alien", "robotic", "futuristic", "unsettling", "mechanical",
            "otherworldly", "cold", "strange", "experimental"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "sci_fi_effects", "experimental_music", "sound_design", "electronic_music",
            "special_effects", "robot_voices", "creative_processing", "vintage_synthesis"
        })
        .setInstrumentTags({
            "synthesizer", "electric_guitar", "vocals", "drums", "any"
        })
        .setTechnicalProps(0.1f, 0)
        .addParameter("Frequency", 0.3f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-10", "sub audio, 1-20Hz"},
            {"10-30", "low, 20-100Hz"},
            {"30-50", "mid, 100-500Hz"},
            {"50-70", "high, 500-2kHz"},
            {"70-90", "very high, 2k-5kHz"},
            {"90-100", "extreme, 5k-10kHz"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle effect"},
            {"20-40", "blended signal"},
            {"40-60", "equal mix"},
            {"60-80", "effect dominant"},
            {"80-100", "pure ring mod"}
        })
        .addParameter("Waveform", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-33", "sine - pure, musical"},
            {"33-66", "triangle - warmer"},
            {"66-100", "square - harsh, digital"}
        })
        .addParameter("Depth", 1.0f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "shallow modulation"},
            {"20-40", "moderate depth"},
            {"40-60", "standard ring mod"},
            {"60-80", "deep modulation"},
            {"80-100", "extreme effect"}
        })
        .setTriggerWords({
            "ring mod", "ring modulator", "metallic", "bell", "robot",
            "dalek", "sci-fi", "alien", "clangorous"
        })
        .setCompatibility({
            {ENGINE_FREQUENCY_SHIFTER, 0.8f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_PLATE_REVERB, 0.75f},
            {ENGINE_BIT_CRUSHER, 0.7f}
        })
        .setPairsWellWith({
            "delay", "reverb", "filters", "distortion", "pitch_effects"
        })
        .setAvoidWith({
            "other_ring_modulators", "heavy_modulation"
        })
        .setMoodAdjustments({
            {"more_metallic", 0.2f}, {"warmer", -0.15f}, {"harsher", 0.25f},
            {"subtler", -0.2f}, {"more_alien", 0.3f}, {"musical", -0.1f}
        })
        .build()
    );
    
    // ENGINE 21: Comb Resonator  
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_COMB_RESONATOR, "Comb Resonator", "filter",
                     "Tuned comb filter creating pitched resonances and metallic tones")
        .setSonicTags({
            "resonant", "metallic", "pitched", "comb", "karplus-strong",
            "plucked", "string-like", "harmonic", "ringing", "tuned",
            "physical-modeling", "percussive", "bright", "crystalline"
        })
        .setEmotionalTags({
            "ethereal", "mysterious", "bell-like", "delicate", "shimmering",
            "meditative", "otherworldly", "pure", "crystalline"
        })
        .setFrequencyFocus("variable")
        .setUseCases({
            "physical_modeling", "karplus_strong", "pitched_effects", "sound_design",
            "ambient_textures", "experimental_music", "percussion_enhancement", "resonant_filtering"
        })
        .setInstrumentTags({
            "percussion", "synthesizer", "guitar", "drums", "any"
        })
        .setTechnicalProps(0.15f, 0)
        .addParameter("Frequency", 0.5f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-15", "sub bass, 20-60Hz"},
            {"15-35", "bass, 60-200Hz"},
            {"35-55", "low mid, 200-600Hz"},
            {"55-75", "mid, 600-2kHz"},
            {"75-90", "high, 2k-6kHz"},
            {"90-100", "very high, 6k-12kHz"}
        })
        .addParameter("Feedback", 0.85f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "short decay, percussive"},
            {"20-40", "medium decay"},
            {"40-60", "long decay, bell-like"},
            {"60-80", "very long, sustained"},
            {"80-95", "near infinite"},
            {"95-100", "self-oscillation"}
        })
        .addParameter("Damping", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "bright, metallic"},
            {"20-40", "slightly damped"},
            {"40-60", "natural damping"},
            {"60-80", "heavily damped"},
            {"80-100", "very dark, muted"}
        })
        .addParameter("Detune", 0.0f, -1.0f, 1.0f, "cents", "linear", {
            {"-100--50", "down 100 cents"},
            {"-50--20", "slightly flat"},
            {"-20-20", "in tune"},
            {"20-50", "slightly sharp"},
            {"50-100", "up 100 cents"}
        })
        .addParameter("Mix", 0.6f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "comb", "resonator", "karplus", "string", "plucked", "metallic",
            "tuned", "pitched", "physical modeling"
        })
        .setCompatibility({
            {ENGINE_PLATE_REVERB, 0.85f},
            {ENGINE_TAPE_ECHO, 0.8f},
            {ENGINE_ENVELOPE_FILTER, 0.75f},
            {ENGINE_STATE_VARIABLE_FILTER, 0.4f}
        })
        .setPairsWellWith({
            "reverb", "delay", "filters", "modulation", "distortion"
        })
        .setAvoidWith({
            "other_comb_filters", "heavy_resonance"
        })
        .setMoodAdjustments({
            {"brighter", -0.2f}, {"darker", 0.2f}, {"more_resonant", 0.15f},
            {"shorter", -0.2f}, {"longer", 0.2f}, {"more_metallic", -0.15f}
        })
        .build()
    );
    
    // ENGINE 22: Pitch Shifter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_PITCH_SHIFTER, "Pitch Shifter", "pitch",
                     "High-quality pitch shifting with formant preservation")
        .setSonicTags({
            "pitch-shift", "transpose", "harmony", "octave", "detune",
            "formant", "clean", "digital", "precise", "polyphonic",
            "transparent", "musical", "studio-quality"
        })
        .setEmotionalTags({
            "transformative", "creative", "playful", "experimental", "magical",
            "otherworldly", "expansive", "imaginative"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "harmony_creation", "vocal_processing", "creative_effects", "pitch_correction",
            "octave_effects", "gender_bending", "instrument_transformation", "sound_design"
        })
        .setInstrumentTags({
            "vocals", "guitar", "bass", "synthesizer", "any"
        })
        .setTechnicalProps(0.4f, 1024)  // FFT-based processing
        .addParameter("Pitch", 0.0f, -24.0f, 24.0f, "semitones", "linear", {
            {"-24--12", "down 2 octaves"},
            {"-12--7", "down 1 octave"},
            {"-7--3", "down fifth/fourth"},
            {"-3-3", "subtle detuning"},
            {"3-7", "up third/fifth"},
            {"7-12", "up 1 octave"},
            {"12-24", "up 2 octaves"}
        })
        .addParameter("Fine Tune", 0.0f, -100.0f, 100.0f, "cents", "linear", {
            {"-100--50", "very flat"},
            {"-50--20", "noticeably flat"},
            {"-20-20", "micro tuning"},
            {"20-50", "noticeably sharp"},
            {"50-100", "very sharp"}
        })
        .addParameter("Formant", 0.0f, -1.0f, 1.0f, "", "linear", {
            {"-100--50", "deep, masculine"},
            {"-50--20", "lower formants"},
            {"-20-20", "preserve formants"},
            {"20-50", "higher formants"},
            {"50-100", "chipmunk, feminine"}
        })
        .addParameter("Quality", 0.7f, 0.0f, 1.0f, "", "stepped", {
            {"0-33", "fast/low quality"},
            {"33-66", "balanced"},
            {"66-100", "high quality"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "pitch", "shift", "transpose", "harmony", "octave", "detune",
            "formant", "gender", "chipmunk"
        })
        .setCompatibility({
            {ENGINE_INTELLIGENT_HARMONIZER, 0.4f},
            {ENGINE_PLATE_REVERB, 0.9f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_DETUNE_DOUBLER, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "delay", "compression", "eq", "modulation"
        })
        .setAvoidWith({
            "other_pitch_shifters", "harmonizers", "heavy_modulation"
        })
        .setMoodAdjustments({
            {"higher", 0.5f}, {"lower", -0.5f}, {"weirder", 0.3f},
            {"natural", -0.1f}, {"extreme", 0.4f}, {"subtle", -0.3f}
        })
        .build()
    );
    
    // ENGINE 23: Phased Vocoder
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_PHASED_VOCODER, "Phased Vocoder", "spectral",
                     "Phase vocoder for time stretching and spectral manipulation")
        .setSonicTags({
            "spectral", "time-stretch", "smearing", "robotic", "ethereal",
            "complex", "futuristic", "abstract", "morphing", "crystalline",
            "digital", "experimental", "transformative"
        })
        .setEmotionalTags({
            "futuristic", "alien", "mysterious", "abstract", "technological",
            "cold", "distant", "ethereal", "surreal"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "time_stretching", "spectral_effects", "sound_design", "experimental_music",
            "ambient_textures", "vocal_processing", "creative_manipulation", "film_scoring"
        })
        .setInstrumentTags({
            "any", "vocals", "synthesizer", "field_recordings", "percussion"
        })
        .setTechnicalProps(0.8f, 2048)  // Large FFT window
        .addParameter("Time Stretch", 1.0f, 0.25f, 4.0f, "x", "logarithmic", {
            {"0.25-0.5", "extreme fast, 4x-2x speed"},
            {"0.5-0.8", "faster, 2x-1.25x"},
            {"0.8-1.2", "near normal speed"},
            {"1.2-2.0", "slower, 0.8x-0.5x"},
            {"2.0-4.0", "extreme slow, 0.5x-0.25x"}
        })
        .addParameter("Pitch Shift", 0.0f, -12.0f, 12.0f, "semitones", "linear", {
            {"-12--7", "down octave"},
            {"-7--3", "down fifth"},
            {"-3-3", "slight shift"},
            {"3-7", "up fifth"},
            {"7-12", "up octave"}
        })
        .addParameter("Spectral Smear", 0.2f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "tight, precise"},
            {"20-40", "slight smearing"},
            {"40-60", "moderate blur"},
            {"60-80", "heavy smearing"},
            {"80-100", "extreme wash"}
        })
        .addParameter("Phase Lock", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "loose phases"},
            {"20-40", "some coherence"},
            {"40-60", "balanced"},
            {"60-80", "tight phases"},
            {"80-100", "phase locked"}
        })
        .addParameter("Mix", 0.7f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "vocoder", "phase vocoder", "time stretch", "spectral", "smear",
            "morph", "abstract", "futuristic"
        })
        .setCompatibility({
            {ENGINE_SPECTRAL_FREEZE, 0.9f},
            {ENGINE_GRANULAR_CLOUD, 0.85f},
            {ENGINE_SHIMMER_REVERB, 0.8f},
            {ENGINE_BIT_CRUSHER, 0.3f}
        })
        .setPairsWellWith({
            "reverb", "delay", "filters", "spectral_effects", "modulation"
        })
        .setAvoidWith({
            "transient_shapers", "gates", "heavy_compression"
        })
        .setMoodAdjustments({
            {"weirder", 0.3f}, {"cleaner", -0.2f}, {"more_abstract", 0.35f},
            {"tighter", 0.2f}, {"more_smeared", 0.25f}, {"robotic", 0.2f}
        })
        .build()
    );
    
    // ENGINE 24: Convolution Reverb
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_CONVOLUTION_REVERB, "Convolution Reverb", "reverb",
                     "Impulse response based reverb for realistic spaces")
        .setSonicTags({
            "realistic", "natural", "spacious", "authentic", "impulse-response",
            "high-quality", "detailed", "immersive", "3D", "pristine",
            "studio", "concert-hall", "cathedral", "room"
        })
        .setEmotionalTags({
            "immersive", "realistic", "spacious", "grand", "natural",
            "expensive", "professional", "cinematic", "authentic"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "realistic_spaces", "film_scoring", "classical_music", "studio_reverb",
            "natural_ambience", "post_production", "immersive_audio", "orchestral_placement"
        })
        .setInstrumentTags({
            "orchestral", "acoustic_instruments", "vocals", "piano", "strings", "any"
        })
        .setTechnicalProps(0.9f, 512)  // CPU intensive
        .addParameter("IR Select", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-20", "small room"},
            {"20-40", "studio"},
            {"40-60", "concert hall"},
            {"60-80", "cathedral"},
            {"80-100", "special spaces"}
        })
        .addParameter("Size", 1.0f, 0.5f, 2.0f, "x", "linear", {
            {"0.5-0.7", "intimate, smaller"},
            {"0.7-0.9", "reduced size"},
            {"0.9-1.1", "natural size"},
            {"1.1-1.5", "enlarged space"},
            {"1.5-2.0", "huge, epic"}
        })
        .addParameter("Pre-Delay", 0.02f, 0.0f, 0.2f, "s", "linear", {
            {"0-0.01", "immediate, close"},
            {"0.01-0.03", "natural distance"},
            {"0.03-0.06", "medium distance"},
            {"0.06-0.1", "far placement"},
            {"0.1-0.2", "very distant"}
        })
        .addParameter("Damping", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "bright, live space"},
            {"20-40", "natural damping"},
            {"40-60", "warm space"},
            {"60-80", "heavily damped"},
            {"80-100", "very dead space"}
        })
        .addParameter("Mix", 0.25f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "convolution", "impulse", "IR", "realistic", "natural", "space",
            "hall", "room", "cathedral", "authentic"
        })
        .setCompatibility({
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_PLATE_REVERB, 0.3f},
            {ENGINE_SPRING_REVERB, 0.3f}
        })
        .setPairsWellWith({
            "eq", "compression", "delay", "saturation", "imaging"
        })
        .setAvoidWith({
            "other_reverbs", "heavy_modulation", "lo-fi_effects"
        })
        .setMoodAdjustments({
            {"bigger", 0.3f}, {"smaller", -0.3f}, {"brighter", -0.2f},
            {"darker", 0.2f}, {"more_distant", 0.15f}, {"closer", -0.15f}
        })
        .build()
    );
    
    // ENGINE 25: Bit Crusher
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_BIT_CRUSHER, "Bit Crusher", "distortion",
                     "Digital distortion through bit depth and sample rate reduction")
        .setSonicTags({
            "digital", "lofi", "crushed", "aliased", "gritty", "harsh",
            "retro", "8-bit", "video-game", "glitchy", "degraded",
            "quantized", "stepped", "crunchy", "artificial"
        })
        .setEmotionalTags({
            "nostalgic", "retro", "aggressive", "digital", "playful",
            "harsh", "raw", "futuristic", "broken"
        })
        .setFrequencyFocus("high")
        .setUseCases({
            "lofi_production", "chiptune", "electronic_music", "creative_distortion",
            "retro_gaming", "glitch_effects", "drum_destruction", "digital_artifacts"
        })
        .setInstrumentTags({
            "drums", "synthesizer", "samples", "any", "electronic"
        })
        .setTechnicalProps(0.1f, 0)
        .addParameter("Bit Depth", 8.0f, 1.0f, 16.0f, "bits", "stepped", {
            {"1-2", "extreme crush, almost noise"},
            {"2-4", "heavy distortion"},
            {"4-6", "crunchy, lofi"},
            {"6-8", "retro digital"},
            {"8-12", "subtle grit"},
            {"12-16", "clean, full quality"}
        })
        .addParameter("Sample Rate", 0.5f, 0.0f, 1.0f, "x", "logarithmic", {
            {"0-10", "extreme aliasing, 1-2kHz"},
            {"10-25", "heavy aliasing, 2-5kHz"},
            {"25-40", "noticeable, 5-10kHz"},
            {"40-60", "moderate, 10-20kHz"},
            {"60-80", "subtle, 20-30kHz"},
            {"80-100", "clean, 30-44kHz"}
        })
        .addParameter("Filter", 0.7f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "very dark, 500Hz"},
            {"20-40", "dark, 1-2kHz"},
            {"40-60", "warm, 2-5kHz"},
            {"60-80", "bright, 5-10kHz"},
            {"80-100", "full range"}
        })
        .addParameter("Mix", 0.6f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "bit crush", "lofi", "8-bit", "digital", "crush", "degrade",
            "retro", "video game", "chiptune", "glitch"
        })
        .setCompatibility({
            {ENGINE_STATE_VARIABLE_FILTER, 0.8f},
            {ENGINE_TAPE_ECHO, 0.75f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.7f},
            {ENGINE_K_STYLE, 0.3f},
            {ENGINE_MULTIBAND_SATURATOR, 0.3f}
        })
        .setPairsWellWith({
            "filters", "delay", "reverb", "eq", "modulation"
        })
        .setAvoidWith({
            "other_distortion", "saturation", "analog_warmth"
        })
        .setMoodAdjustments({
            {"more_lofi", 0.3f}, {"cleaner", -0.3f}, {"darker", 0.2f},
            {"brighter", -0.2f}, {"more_extreme", 0.35f}, {"subtler", -0.25f}
        })
        .build()
    );
    
    // ENGINE 26: Frequency Shifter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_FREQUENCY_SHIFTER, "Frequency Shifter", "modulation",
                     "Linear frequency shifting creating inharmonic and metallic tones")
        .setSonicTags({
            "metallic", "inharmonic", "alien", "dissonant", "unique",
            "experimental", "bell-like", "robotic", "shifting", "weird",
            "psychedelic", "abstract", "non-musical", "special-effect"
        })
        .setEmotionalTags({
            "alien", "unsettling", "futuristic", "strange", "otherworldly",
            "disorienting", "experimental", "cold", "mechanical"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "sound_design", "experimental_music", "special_effects", "sci_fi_sounds",
            "creative_processing", "feedback_elimination", "psychedelic_effects", "alien_voices"
        })
        .setInstrumentTags({
            "synthesizer", "vocals", "drums", "any", "sound_effects"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Shift", 0.0f, -1000.0f, 1000.0f, "Hz", "linear", {
            {"-1000--200", "extreme down shift"},
            {"-200--50", "strong down shift"},
            {"-50--10", "subtle down"},
            {"-10-10", "micro shifting"},
            {"10-50", "subtle up"},
            {"50-200", "strong up shift"},
            {"200-1000", "extreme up shift"}
        })
        .addParameter("Feedback", 0.0f, 0.0f, 1.0f, "%", "linear", {
            {"0", "no feedback"},
            {"1-30", "subtle regeneration"},
            {"30-60", "moderate feedback"},
            {"60-80", "strong feedback"},
            {"80-100", "extreme, chaotic"}
        })
        .addParameter("Direction", 0.5f, 0.0f, 1.0f, "", "stepped", {
            {"0-33", "down only"},
            {"33-66", "up + down"},
            {"66-100", "up only"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "frequency shift", "bode", "metallic", "inharmonic", "alien",
            "shifting", "weird", "experimental"
        })
        .setCompatibility({
            {ENGINE_ANALOG_RING_MODULATOR, 0.8f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_PLATE_REVERB, 0.75f},
            {ENGINE_PITCH_SHIFTER, 0.4f}
        })
        .setPairsWellWith({
            "delay", "reverb", "filters", "ring_mod", "distortion"
        })
        .setAvoidWith({
            "pitch_shifters", "harmonizers", "musical_effects"
        })
        .setMoodAdjustments({
            {"weirder", 0.4f}, {"subtler", -0.3f}, {"more_metallic", 0.25f},
            {"darker", -0.2f}, {"brighter", 0.2f}, {"more_alien", 0.35f}
        })
        .build()
    );
    
    // ENGINE 27: Wave Folder
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_WAVE_FOLDER, "Wave Folder", "distortion",
                     "West Coast synthesis style wavefolding distortion")
        .setSonicTags({
            "folded", "harmonics", "west-coast", "complex", "rich",
            "organic", "dynamic", "responsive", "unique", "synthesis",
            "musical", "warm", "evolving", "animated"
        })
        .setEmotionalTags({
            "organic", "complex", "evolving", "rich", "sophisticated",
            "experimental", "warm", "dynamic", "expressive"
        })
        .setFrequencyFocus("high-mid")
        .setUseCases({
            "west_coast_synthesis", "harmonic_generation", "creative_distortion",
            "synth_processing", "sound_design", "experimental_music", "organic_distortion"
        })
        .setInstrumentTags({
            "synthesizer", "electric_guitar", "bass", "electronic"
        })
        .setTechnicalProps(0.15f, 0)
        .addParameter("Fold", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle folding"},
            {"20-40", "moderate harmonics"},
            {"40-60", "rich folding"},
            {"60-80", "complex harmonics"},
            {"80-100", "extreme folding"}
        })
        .addParameter("Symmetry", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "asymmetric, even harmonics"},
            {"20-40", "slightly asymmetric"},
            {"40-60", "balanced"},
            {"60-80", "slightly symmetric"},
            {"80-100", "symmetric, odd harmonics"}
        })
        .addParameter("Bias", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-30", "negative bias"},
            {"30-70", "centered"},
            {"70-100", "positive bias"}
        })
        .addParameter("Drive", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "gentle input"},
            {"20-40", "moderate drive"},
            {"40-60", "strong input"},
            {"60-80", "hot signal"},
            {"80-100", "extreme overdrive"}
        })
        .addParameter("Mix", 0.7f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "wave folder", "fold", "west coast", "buchla", "serge",
            "harmonics", "organic distortion", "synthesis"
        })
        .setCompatibility({
            {ENGINE_STATE_VARIABLE_FILTER, 0.9f},
            {ENGINE_ENVELOPE_FILTER, 0.85f},
            {ENGINE_SPRING_REVERB, 0.8f},
            {ENGINE_BIT_CRUSHER, 0.3f}
        })
        .setPairsWellWith({
            "filters", "reverb", "delay", "modulation", "envelope_followers"
        })
        .setAvoidWith({
            "heavy_distortion", "bit_crushers", "harsh_effects"
        })
        .setMoodAdjustments({
            {"warmer", 0.1f}, {"brighter", 0.15f}, {"darker", -0.15f},
            {"more_complex", 0.2f}, {"simpler", -0.2f}, {"organic", 0.15f}
        })
        .build()
    );
    
    // ENGINE 28: Shimmer Reverb
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_SHIMMER_REVERB, "Shimmer Reverb", "reverb",
                     "Ethereal reverb with pitch-shifted octave regeneration")
        .setSonicTags({
            "ethereal", "angelic", "shimmering", "octave", "celestial",
            "ambient", "huge", "pitch-shifted", "crystalline", "magical",
            "ascending", "cathedral", "heavenly", "expansive"
        })
        .setEmotionalTags({
            "transcendent", "heavenly", "magical", "ethereal", "uplifting",
            "spiritual", "cosmic", "dreamy", "celestial", "overwhelming"
        })
        .setFrequencyFocus("high")
        .setUseCases({
            "ambient_music", "cinematic_scoring", "worship_music", "ethereal_pads",
            "guitar_atmospheres", "vocal_effects", "new_age_music", "soundscapes"
        })
        .setInstrumentTags({
            "guitar", "synthesizer", "vocals", "strings", "piano", "pads"
        })
        .setTechnicalProps(0.6f, 256)
        .addParameter("Size", 0.7f, 0.0f, 1.0f, "", "linear", {
            {"0-20", "small space"},
            {"20-40", "medium room"},
            {"40-60", "large hall"},
            {"60-80", "cathedral"},
            {"80-100", "infinite space"}
        })
        .addParameter("Shimmer", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0", "no shimmer, normal reverb"},
            {"1-30", "subtle octaves"},
            {"30-60", "prominent shimmer"},
            {"60-80", "strong octave presence"},
            {"80-100", "overwhelming shimmer"}
        })
        .addParameter("Pitch", 12.0f, -24.0f, 24.0f, "semitones", "stepped", {
            {"-24", "down 2 octaves"},
            {"-12", "down 1 octave"},
            {"0", "unison"},
            {"12", "up 1 octave"},
            {"24", "up 2 octaves"}
        })
        .addParameter("Decay", 0.6f, 0.0f, 1.0f, "s", "logarithmic", {
            {"0-20", "short, 1-3s"},
            {"20-40", "medium, 3-6s"},
            {"40-60", "long, 6-12s"},
            {"60-80", "very long, 12-30s"},
            {"80-100", "infinite decay"}
        })
        .addParameter("Damping", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "bright, crystalline"},
            {"20-40", "balanced brightness"},
            {"40-60", "natural damping"},
            {"60-80", "warm, soft"},
            {"80-100", "dark, muffled"}
        })
        .addParameter("Mix", 0.3f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "shimmer", "ethereal", "angelic", "celestial", "octave reverb",
            "ambient", "heavenly", "magical", "crystalline"
        })
        .setCompatibility({
            {ENGINE_SPECTRAL_FREEZE, 0.95f},
            {ENGINE_GRANULAR_CLOUD, 0.9f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_PLATE_REVERB, 0.4f},
            {ENGINE_CONVOLUTION_REVERB, 0.3f}
        })
        .setPairsWellWith({
            "delays", "filters", "modulation", "pitch_effects", "spectral_effects"
        })
        .setAvoidWith({
            "other_reverbs", "heavy_distortion", "transient_shapers"
        })
        .setMoodAdjustments({
            {"more_ethereal", 0.2f}, {"darker", 0.25f}, {"brighter", -0.2f},
            {"bigger", 0.15f}, {"smaller", -0.15f}, {"more_shimmer", 0.3f}
        })
        .build()
    );
    
    // ENGINE 29: Vocal Formant Filter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_VOCAL_FORMANT_FILTER, "Vocal Formant Filter", "filter",
                     "Advanced formant filter with multiple vowel shapes and morphing")
        .setSonicTags({
            "vocal", "formant", "vowel", "human", "organic", "expressive",
            "talk-box", "morphing", "animated", "speaking", "singing",
            "resonant", "articulate", "dynamic"
        })
        .setEmotionalTags({
            "expressive", "human", "communicative", "organic", "playful",
            "animated", "lively", "emotional", "intimate"
        })
        .setFrequencyFocus("mid")
        .setUseCases({
            "vocal_effects", "talk_box", "synth_animation", "creative_filtering",
            "electronic_vocals", "robotic_voices", "funk_guitar", "expressive_leads"
        })
        .setInstrumentTags({
            "vocals", "synthesizer", "electric_guitar", "bass", "any"
        })
        .setTechnicalProps(0.35f, 0)
        .addParameter("Vowel 1", 0.0f, 0.0f, 1.0f, "", "linear", {
            {"0-14", "A - father"},
            {"14-28", "E - bed"},
            {"28-42", "I - beat"},
            {"42-57", "O - boat"},
            {"57-71", "U - boot"},
            {"71-85", "Y - few"},
            {"85-100", "Æ - cat"}
        })
        .addParameter("Vowel 2", 0.5f, 0.0f, 1.0f, "", "linear", {
            {"0-14", "A - father"},
            {"14-28", "E - bed"},
            {"28-42", "I - beat"},
            {"42-57", "O - boat"},
            {"57-71", "U - boot"},
            {"71-85", "Y - few"},
            {"85-100", "Æ - cat"}
        })
        .addParameter("Morph", 0.0f, 0.0f, 1.0f, "%", "linear", {
            {"0", "100% vowel 1"},
            {"1-30", "mostly vowel 1"},
            {"30-70", "blended vowels"},
            {"70-99", "mostly vowel 2"},
            {"100", "100% vowel 2"}
        })
        .addParameter("Resonance", 0.7f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle formants"},
            {"20-40", "clear vowels"},
            {"40-60", "strong character"},
            {"60-80", "very resonant"},
            {"80-100", "extreme, synthetic"}
        })
        .addParameter("Formant Shift", 0.5f, 0.0f, 1.0f, "", "linear", {
            {"0-25", "deep, masculine"},
            {"25-40", "low voice"},
            {"40-60", "neutral"},
            {"60-75", "high voice"},
            {"75-100", "child/cartoon"}
        })
        .addParameter("Mix", 0.8f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "vocal formant", "vowel", "formant", "voice", "talk box",
            "human", "speaking", "morphing", "articulate"
        })
        .setCompatibility({
            {ENGINE_ENVELOPE_FILTER, 0.85f},
            {ENGINE_K_STYLE, 0.8f},
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_FORMANT_FILTER, 0.3f}
        })
        .setPairsWellWith({
            "delay", "reverb", "distortion", "modulation", "envelope_followers"
        })
        .setAvoidWith({
            "other_formant_filters", "heavy_filtering", "extreme_resonance"
        })
        .setMoodAdjustments({
            {"more_human", 0.2f}, {"robotic", -0.2f}, {"expressive", 0.25f},
            {"static", -0.3f}, {"higher_voice", 0.2f}, {"deeper_voice", -0.2f}
        })
        .build()
    );
    
    // ENGINE 30: Transient Shaper
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_TRANSIENT_SHAPER, "Transient Shaper", "dynamics",
                     "Attack and sustain control for reshaping transients")
        .setSonicTags({
            "punchy", "snappy", "tight", "controlled", "dynamic", "percussive",
            "attack", "sustain", "envelope", "shaping", "transparent",
            "surgical", "precise", "impactful"
        })
        .setEmotionalTags({
            "powerful", "controlled", "tight", "aggressive", "precise",
            "punchy", "energetic", "focused", "dynamic"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "drum_enhancement", "percussion_control", "mix_tightening", "attack_emphasis",
            "sustain_control", "dynamic_shaping", "punch_enhancement", "envelope_design"
        })
        .setInstrumentTags({
            "drums", "percussion", "bass", "acoustic_guitar", "piano", "any"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Attack", 0.0f, -1.0f, 1.0f, "", "linear", {
            {"-100--60", "remove attack, soften"},
            {"-60--30", "reduce transients"},
            {"-30-0", "subtle reduction"},
            {"0-30", "subtle boost"},
            {"30-60", "enhance attack"},
            {"60-100", "extreme punch"}
        })
        .addParameter("Sustain", 0.0f, -1.0f, 1.0f, "", "linear", {
            {"-100--60", "gate-like, short"},
            {"-60--30", "reduce sustain"},
            {"-30-0", "subtle reduction"},
            {"0-30", "subtle boost"},
            {"30-60", "enhance body"},
            {"60-100", "massive sustain"}
        })
        .addParameter("Speed", 0.5f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "ultra fast, 0.1-1ms"},
            {"20-40", "fast, 1-5ms"},
            {"40-60", "medium, 5-20ms"},
            {"60-80", "slow, 20-50ms"},
            {"80-100", "very slow, 50-100ms"}
        })
        .addParameter("Output", 0.0f, -12.0f, 12.0f, "dB", "linear", {
            {"-12--6", "strong attenuation"},
            {"-6--3", "moderate cut"},
            {"-3-3", "unity range"},
            {"3-6", "moderate boost"},
            {"6-12", "strong boost"}
        })
        .setTriggerWords({
            "transient", "shaper", "attack", "sustain", "punch", "snap",
            "envelope", "dynamics", "tight", "percussive"
        })
        .setCompatibility({
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_MASTERING_LIMITER, 0.85f},
            {ENGINE_NOISE_GATE, 0.8f}
        })
        .setPairsWellWith({
            "compression", "eq", "saturation", "limiting", "gating"
        })
        .setAvoidWith({
            "heavy_compression", "extreme_limiting", "heavy_reverb"
        })
        .setMoodAdjustments({
            {"punchier", 0.3f}, {"softer", -0.3f}, {"tighter", 0.2f},
            {"looser", -0.2f}, {"more_aggressive", 0.25f}, {"gentler", -0.25f}
        })
        .build()
    );
    
    // ENGINE 31: Dimension Expander
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_DIMENSION_EXPANDER, "Dimension Expander", "spatial",
                     "Stereo field enhancement with psychoacoustic widening")
        .setSonicTags({
            "wide", "spacious", "3D", "dimensional", "stereo", "expansive",
            "immersive", "psychoacoustic", "enhanced", "spatial", "open",
            "airy", "broad", "panoramic"
        })
        .setEmotionalTags({
            "expansive", "open", "free", "spacious", "immersive", "grand",
            "liberating", "atmospheric", "enveloping"
        })
        .setFrequencyFocus("high")
        .setUseCases({
            "stereo_enhancement", "mix_widening", "spatial_effects", "mastering",
            "immersive_audio", "3D_positioning", "width_control", "mono_compatibility"
        })
        .setInstrumentTags({
            "full_mix", "synthesizer", "strings", "pads", "ambient", "any"
        })
        .setTechnicalProps(0.25f, 0, false, true)
        .addParameter("Width", 0.5f, 0.0f, 2.0f, "x", "linear", {
            {"0", "mono collapse"},
            {"0.1-0.5", "narrowed"},
            {"0.5-1.0", "natural width"},
            {"1.0-1.5", "enhanced width"},
            {"1.5-2.0", "extreme width"}
        })
        .addParameter("Bass Mono", 0.3f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0", "no bass mono"},
            {"1-20", "below 50Hz"},
            {"20-40", "below 100Hz"},
            {"40-60", "below 200Hz"},
            {"60-80", "below 300Hz"},
            {"80-100", "below 500Hz"}
        })
        .addParameter("High Freq Width", 0.7f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "minimal HF width"},
            {"20-40", "subtle enhancement"},
            {"40-60", "moderate width"},
            {"60-80", "wide highs"},
            {"80-100", "extreme HF spread"}
        })
        .addParameter("Center Focus", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "weak center"},
            {"20-40", "soft center"},
            {"40-60", "balanced"},
            {"60-80", "strong center"},
            {"80-100", "locked center"}
        })
        .addParameter("Depth", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "flat, no depth"},
            {"20-40", "subtle 3D"},
            {"40-60", "moderate depth"},
            {"60-80", "deep field"},
            {"80-100", "extreme 3D"}
        })
        .setTriggerWords({
            "dimension", "expander", "width", "stereo", "spatial", "3D",
            "widening", "enhancement", "immersive", "panoramic"
        })
        .setCompatibility({
            {ENGINE_MASTERING_LIMITER, 0.95f},
            {ENGINE_PARAMETRIC_EQ, 0.9f},
            {ENGINE_MULTIBAND_SATURATOR, 0.85f},
            {ENGINE_MID_SIDE_PROCESSOR, 0.7f}
        })
        .setPairsWellWith({
            "mastering_chain", "eq", "compression", "reverb", "imaging_tools"
        })
        .setAvoidWith({
            "mono_effects", "heavy_distortion", "aggressive_compression"
        })
        .setMoodAdjustments({
            {"wider", 0.3f}, {"narrower", -0.3f}, {"more_3D", 0.2f},
            {"flatter", -0.2f}, {"more_immersive", 0.25f}, {"tighter", -0.25f}
        })
        .build()
    );
    
    // ENGINE 32: Analog Phaser
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_ANALOG_PHASER, "Analog Phaser", "modulation",
                     "Vintage analog phaser with multiple stages and feedback")
        .setSonicTags({
            "swooshing", "swirling", "phasing", "vintage", "analog", "warm",
            "psychedelic", "70s", "movement", "sweeping", "liquid",
            "jet-plane", "whoosh", "cyclical"
        })
        .setEmotionalTags({
            "psychedelic", "dreamy", "nostalgic", "trippy", "hypnotic",
            "groovy", "retro", "flowing", "mesmerizing"
        })
        .setFrequencyFocus("mid")
        .setUseCases({
            "psychedelic_guitar", "funk_guitar", "vintage_keys", "70s_production",
            "electronic_music", "ambient_textures", "rhythm_guitar", "creative_effects"
        })
        .setInstrumentTags({
            "electric_guitar", "electric_piano", "synthesizer", "bass", "drums"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Rate", 0.3f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-15", "very slow, 0.05-0.2Hz"},
            {"15-35", "slow, 0.2-0.5Hz"},
            {"35-55", "medium, 0.5-2Hz"},
            {"55-75", "fast, 2-5Hz"},
            {"75-90", "very fast, 5-10Hz"},
            {"90-100", "extreme, 10-20Hz"}
        })
        .addParameter("Depth", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MOD_DEPTH_RANGES)
        .addParameter("Feedback", 0.3f, -1.0f, 1.0f, "%", "linear", {
            {"-100--60", "negative, inverted"},
            {"-60--20", "subtle negative"},
            {"-20-20", "minimal feedback"},
            {"20-60", "positive feedback"},
            {"60-90", "strong resonance"},
            {"90-100", "extreme, metallic"}
        })
        .addParameter("Stages", 0.5f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "2 stages, subtle"},
            {"25-50", "4 stages, classic"},
            {"50-75", "6 stages, deep"},
            {"75-100", "8 stages, extreme"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "phaser", "phase", "swoosh", "swirl", "vintage", "analog",
            "psychedelic", "70s", "jet", "whoosh"
        })
        .setCompatibility({
            {ENGINE_K_STYLE, 0.9f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_SPRING_REVERB, 0.8f},
            {ENGINE_STEREO_CHORUS, 0.4f}
        })
        .setPairsWellWith({
            "overdrive", "delay", "reverb", "wah", "compression"
        })
        .setAvoidWith({
            "other_phasers", "heavy_modulation", "chorus", "flanger"
        })
        .setMoodAdjustments({
            {"deeper", 0.2f}, {"subtler", -0.2f}, {"faster", 0.15f},
            {"slower", -0.15f}, {"more_resonant", 0.25f}, {"cleaner", -0.15f}
        })
        .build()
    );
    
    // ENGINE 33: Envelope Filter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_ENVELOPE_FILTER, "Envelope Filter", "filter",
                     "Dynamic filter controlled by input envelope for auto-wah effects")
        .setSonicTags({
            "auto-wah", "envelope", "dynamic", "responsive", "funky", "touch-sensitive",
            "sweeping", "quacky", "vowel-like", "expressive", "animated",
            "triggered", "following", "adaptive"
        })
        .setEmotionalTags({
            "funky", "groovy", "expressive", "playful", "dynamic", "lively",
            "responsive", "energetic", "soulful"
        })
        .setFrequencyFocus("variable")
        .setUseCases({
            "funk_guitar", "bass_effects", "synth_animation", "dynamic_filtering",
            "auto_wah", "envelope_following", "percussive_filtering", "touch_response"
        })
        .setInstrumentTags({
            "electric_guitar", "bass_guitar", "synthesizer", "clavinet", "drums"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Sensitivity", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "low sensitivity"},
            {"20-40", "moderate response"},
            {"40-60", "balanced tracking"},
            {"60-80", "high sensitivity"},
            {"80-100", "hair trigger"}
        })
        .addParameter("Attack", 0.1f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "instant, 0-5ms"},
            {"20-40", "fast, 5-20ms"},
            {"40-60", "medium, 20-50ms"},
            {"60-80", "slow, 50-100ms"},
            {"80-100", "very slow, 100-200ms"}
        })
        .addParameter("Release", 0.3f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "very fast, 10-50ms"},
            {"20-40", "fast, 50-150ms"},
            {"40-60", "medium, 150-300ms"},
            {"60-80", "slow, 300-600ms"},
            {"80-100", "very slow, 600-1000ms"}
        })
        .addParameter("Range", 0.7f, 0.0f, 1.0f, "octaves", "linear", {
            {"0-20", "narrow, 1 octave"},
            {"20-40", "moderate, 2 octaves"},
            {"40-60", "wide, 3 octaves"},
            {"60-80", "very wide, 4 octaves"},
            {"80-100", "extreme, 5 octaves"}
        })
        .addParameter("Q", 0.6f, 0.0f, 1.0f, "", "linear", {
            {"0-20", "broad, smooth"},
            {"20-40", "moderate width"},
            {"40-60", "focused, vocal"},
            {"60-80", "narrow, quacky"},
            {"80-100", "extreme resonance"}
        })
        .addParameter("Mix", 1.0f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "envelope filter", "auto wah", "envelope", "follower", "dynamic filter",
            "funk", "quack", "touch wah", "triggered"
        })
        .setCompatibility({
            {ENGINE_K_STYLE, 0.9f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.85f},
            {ENGINE_ANALOG_PHASER, 0.8f},
            {ENGINE_LADDER_FILTER, 0.4f}
        })
        .setPairsWellWith({
            "overdrive", "compression", "delay", "reverb", "octave_effects"
        })
        .setAvoidWith({
            "other_envelope_filters", "heavy_filtering", "wah_pedals"
        })
        .setMoodAdjustments({
            {"funkier", 0.2f}, {"subtler", -0.2f}, {"more_responsive", 0.15f},
            {"smoother", -0.15f}, {"quackier", 0.25f}, {"mellower", -0.25f}
        })
        .build()
    );
    
    // ENGINE 34: Gated Reverb
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_GATED_REVERB, "Gated Reverb", "reverb",
                     "80s style gated reverb with abrupt cutoff for dramatic effect")
        .setSonicTags({
            "gated", "80s", "dramatic", "punchy", "explosive", "abrupt",
            "non-linear", "drum-reverb", "phil-collins", "stadium",
            "powerful", "controlled", "vintage-digital"
        })
        .setEmotionalTags({
            "dramatic", "powerful", "nostalgic", "bold", "aggressive",
            "explosive", "confident", "retro", "epic"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "drum_production", "80s_sounds", "snare_enhancement", "dramatic_effects",
            "pop_production", "stadium_rock", "electronic_drums", "impact_enhancement"
        })
        .setInstrumentTags({
            "drums", "snare", "toms", "percussion", "synth_drums", "vocals"
        })
        .setTechnicalProps(0.3f, 64)  // Small lookahead for gate
        .addParameter("Room Size", 0.5f, 0.0f, 1.0f, "", "linear", {
            {"0-20", "small booth"},
            {"20-40", "studio room"},
            {"40-60", "large room"},
            {"60-80", "hall"},
            {"80-100", "stadium"}
        })
        .addParameter("Gate Time", 0.3f, 0.0f, 1.0f, "ms", "linear", {
            {"0-20", "very short, 50-100ms"},
            {"20-40", "short, 100-200ms"},
            {"40-60", "medium, 200-350ms"},
            {"60-80", "long, 350-500ms"},
            {"80-100", "very long, 500-800ms"}
        })
        .addParameter("Pre-Delay", 0.1f, 0.0f, 1.0f, "ms", "linear", {
            {"0-20", "immediate, 0-5ms"},
            {"20-40", "tight, 5-15ms"},
            {"40-60", "spacious, 15-30ms"},
            {"60-80", "distant, 30-50ms"},
            {"80-100", "very far, 50-80ms"}
        })
        .addParameter("Gate Slope", 0.7f, 0.0f, 1.0f, "", "linear", {
            {"0-20", "soft fade"},
            {"20-40", "moderate slope"},
            {"40-60", "steep cutoff"},
            {"60-80", "very abrupt"},
            {"80-100", "brick wall"}
        })
        .addParameter("Mix", 0.4f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "gated reverb", "gate", "80s", "phil collins", "drum reverb",
            "non-linear", "abrupt", "dramatic", "explosive"
        })
        .setCompatibility({
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},
            {ENGINE_TRANSIENT_SHAPER, 0.85f},
            {ENGINE_PARAMETRIC_EQ, 0.9f},
            {ENGINE_PLATE_REVERB, 0.3f}
        })
        .setPairsWellWith({
            "compression", "eq", "transient_shaping", "saturation", "delay"
        })
        .setAvoidWith({
            "other_reverbs", "long_delays", "heavy_modulation"
        })
        .setMoodAdjustments({
            {"bigger", 0.2f}, {"tighter", -0.2f}, {"more_dramatic", 0.25f},
            {"subtler", -0.25f}, {"more_80s", 0.15f}, {"modern", -0.15f}
        })
        .build()
    );
    
    // ENGINE 35: Harmonic Exciter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_HARMONIC_EXCITER, "Harmonic Exciter", "enhancement",
                     "Psychoacoustic enhancement adding musical harmonics for presence")
        .setSonicTags({
            "bright", "exciting", "presence", "sparkle", "enhancement", "airy",
            "crisp", "detailed", "harmonic", "psychoacoustic", "sheen",
            "polish", "expensive", "professional"
        })
        .setEmotionalTags({
            "exciting", "energetic", "polished", "professional", "expensive",
            "modern", "crisp", "fresh", "vibrant"
        })
        .setFrequencyFocus("high")
        .setUseCases({
            "mix_enhancement", "vocal_presence", "master_bus", "dull_source_revival",
            "air_addition", "professional_sheen", "broadcast_enhancement", "clarity_boost"
        })
        .setInstrumentTags({
            "vocals", "full_mix", "acoustic_guitar", "drums", "any"
        })
        .setTechnicalProps(0.25f, 0)
        .addParameter("Amount", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle enhancement"},
            {"20-40", "noticeable sparkle"},
            {"40-60", "obvious excitement"},
            {"60-80", "heavy processing"},
            {"80-100", "extreme brightness"}
        })
        .addParameter("Frequency", 0.7f, 0.0f, 1.0f, "kHz", "logarithmic", {
            {"0-20", "low mids, 500Hz-1kHz"},
            {"20-40", "mids, 1-3kHz"},
            {"40-60", "presence, 3-6kHz"},
            {"60-80", "brilliance, 6-10kHz"},
            {"80-100", "air, 10-16kHz"}
        })
        .addParameter("Harmonics", 0.5f, 0.0f, 1.0f, "", "stepped", {
            {"0-33", "2nd harmonic, warm"},
            {"33-66", "2nd + 3rd, balanced"},
            {"66-100", "complex harmonics"}
        })
        .addParameter("Saturation", 0.2f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "clean excitation"},
            {"20-40", "subtle warmth"},
            {"40-60", "noticeable saturation"},
            {"60-80", "heavy coloration"},
            {"80-100", "extreme saturation"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "exciter", "enhancement", "brightness", "sparkle", "presence",
            "air", "sheen", "crisp", "psychoacoustic"
        })
        .setCompatibility({
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_MULTIBAND_SATURATOR, 0.85f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},
            {ENGINE_BIT_CRUSHER, 0.2f}
        })
        .setPairsWellWith({
            "eq", "compression", "limiting", "saturation", "imaging"
        })
        .setAvoidWith({
            "heavy_distortion", "bit_crushing", "lo-fi_effects"
        })
        .setMoodAdjustments({
            {"brighter", 0.2f}, {"darker", -0.3f}, {"more_presence", 0.15f},
            {"warmer", -0.1f}, {"more_air", 0.25f}, {"duller", -0.25f}
        })
        .build()
    );
    
    // ENGINE 36: Feedback Network
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_FEEDBACK_NETWORK, "Feedback Network", "experimental",
                     "Complex feedback matrix creating evolving textures and drones")
        .setSonicTags({
            "feedback", "evolving", "experimental", "ambient", "drone",
            "self-oscillating", "unpredictable", "organic", "complex",
            "generative", "textural", "chaotic", "living", "breathing"
        })
        .setEmotionalTags({
            "mysterious", "evolving", "meditative", "experimental", "organic",
            "unpredictable", "immersive", "hypnotic", "otherworldly"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "ambient_music", "sound_design", "experimental_music", "drone_creation",
            "evolving_textures", "generative_music", "film_scoring", "meditation_sounds"
        })
        .setInstrumentTags({
            "any", "synthesizer", "guitar", "field_recordings", "noise"
        })
        .setTechnicalProps(0.5f, 128)
        .addParameter("Feedback", 0.7f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "stable, controlled"},
            {"20-40", "gentle regeneration"},
            {"40-60", "active feedback"},
            {"60-80", "edge of chaos"},
            {"80-95", "self-oscillating"},
            {"95-100", "runaway feedback"}
        })
        .addParameter("Delay Time", 0.3f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "comb filtering, 1-10ms"},
            {"20-40", "metallic, 10-50ms"},
            {"40-60", "echo-like, 50-200ms"},
            {"60-80", "rhythmic, 200-500ms"},
            {"80-100", "long loops, 500-2000ms"}
        })
        .addParameter("Diffusion", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "discrete echoes"},
            {"20-40", "slight smearing"},
            {"40-60", "moderate diffusion"},
            {"60-80", "heavy smearing"},
            {"80-100", "complete wash"}
        })
        .addParameter("Modulation", 0.2f, 0.0f, 1.0f, "%", "linear", {
            {"0", "static network"},
            {"1-30", "subtle movement"},
            {"30-60", "living texture"},
            {"60-80", "chaotic modulation"},
            {"80-100", "extreme instability"}
        })
        .addParameter("Tone", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "dark, muffled"},
            {"20-40", "warm, vintage"},
            {"40-60", "balanced"},
            {"60-80", "bright, present"},
            {"80-100", "harsh, metallic"}
        })
        .addParameter("Mix", 0.6f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "feedback network", "feedback", "evolving", "drone", "ambient",
            "experimental", "generative", "self-oscillating"
        })
        .setCompatibility({
            {ENGINE_SPECTRAL_FREEZE, 0.9f},
            {ENGINE_GRANULAR_CLOUD, 0.85f},
            {ENGINE_SHIMMER_REVERB, 0.8f},
            {ENGINE_TRANSIENT_SHAPER, 0.2f}
        })
        .setPairsWellWith({
            "filters", "spectral_effects", "reverb", "delay", "modulation"
        })
        .setAvoidWith({
            "transient_processors", "gates", "heavy_compression"
        })
        .setMoodAdjustments({
            {"more_chaotic", 0.2f}, {"more_stable", -0.2f}, {"darker", 0.15f},
            {"brighter", -0.15f}, {"more_evolving", 0.25f}, {"static", -0.3f}
        })
        .build()
    );
    
    // ENGINE 37: Intelligent Harmonizer
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_INTELLIGENT_HARMONIZER, "Intelligent Harmonizer", "pitch",
                     "Scale-aware pitch shifting with intelligent harmony generation")
        .setSonicTags({
            "harmony", "pitch", "intelligent", "musical", "scale-aware",
            "polyphonic", "chord", "interval", "smart", "melodic",
            "harmonic", "voices", "ensemble"
        })
        .setEmotionalTags({
            "musical", "harmonious", "uplifting", "rich", "sophisticated",
            "angelic", "choral", "majestic", "ethereal"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "harmony_creation", "vocal_harmonies", "guitar_harmonies", "chord_generation",
            "orchestral_thickening", "intelligent_doubling", "scale_correction", "ensemble_effects"
        })
        .setInstrumentTags({
            "vocals", "guitar", "synthesizer", "brass", "strings", "any_melodic"
        })
        .setTechnicalProps(0.5f, 1024)
        .addParameter("Interval", 0.5f, 0.0f, 1.0f, "semitones", "linear", {
            {"0", "-24 semitones"},
            {"0.25", "-12 semitones"},
            {"0.5", "unison"},
            {"0.75", "+12 semitones"},
            {"1", "+24 semitones"}
        })
        .addParameter("Key", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-0.083", "C"},
            {"0.083-0.167", "C#/Db"},
            {"0.167-0.25", "D"},
            {"0.25-0.333", "D#/Eb"},
            {"0.333-0.417", "E"},
            {"0.417-0.5", "F"},
            {"0.5-0.583", "F#/Gb"},
            {"0.583-0.667", "G"},
            {"0.667-0.75", "G#/Ab"},
            {"0.75-0.833", "A"},
            {"0.833-0.917", "A#/Bb"},
            {"0.917-1.0", "B"}
        })
        .addParameter("Scale", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-0.1", "Major"},
            {"0.1-0.2", "Minor"},
            {"0.2-0.3", "Dorian"},
            {"0.3-0.4", "Mixolydian"},
            {"0.4-0.5", "Harmonic Minor"},
            {"0.5-0.6", "Melodic Minor"},
            {"0.6-0.7", "Pentatonic Major"},
            {"0.7-0.8", "Pentatonic Minor"},
            {"0.8-0.9", "Blues"},
            {"0.9-1.0", "Chromatic"}
        })
        .addParameter("Voices", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-0.25", "1 voice"},
            {"0.25-0.5", "2 voices - interval + 3rd"},
            {"0.5-0.75", "3 voices - adds 5th"},
            {"0.75-1.0", "4 voices - adds 7th"}
        })
        .addParameter("Spread", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0", "mono center"},
            {"0.3", "narrow stereo"},
            {"0.6", "wide stereo"},
            {"1", "ultra wide"}
        })
        .addParameter("Humanize", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0", "perfect pitch"},
            {"0.3", "subtle variation (±5 cents)"},
            {"0.6", "human-like wobble"},
            {"1", "drunk choir effect"}
        })
        .addParameter("Formant", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0", "no preservation"},
            {"0.5", "balanced spectral envelope"},
            {"1", "full formant preservation"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0", "dry"},
            {"0.5", "50/50"},
            {"1", "wet"}
        })
        .setTriggerWords({
            "harmonizer", "harmony", "intelligent", "scale", "pitch",
            "chord", "interval", "musical", "smart harmony"
        })
        .setCompatibility({
            {ENGINE_PLATE_REVERB, 0.95f},
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.85f},
            {ENGINE_PITCH_SHIFTER, 0.4f}
        })
        .setPairsWellWith({
            "reverb", "delay", "compression", "eq", "chorus"
        })
        .setAvoidWith({
            "other_pitch_shifters", "heavy_distortion", "ring_modulation"
        })
        .setMoodAdjustments({
            {"richer", 0.2f}, {"simpler", -0.2f}, {"higher", 0.15f},
            {"lower", -0.15f}, {"more_complex", 0.25f}, {"cleaner", -0.1f}
        })
        .build()
    );
    
    // ENGINE 38: Parametric EQ
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_PARAMETRIC_EQ, "Parametric EQ", "eq",
                     "5-band parametric equalizer with surgical precision")
        .setSonicTags({
            "precise", "surgical", "transparent", "musical", "clean",
            "professional", "flexible", "corrective", "enhancing", "neutral",
            "studio-grade", "versatile", "accurate"
        })
        .setEmotionalTags({
            "professional", "clean", "controlled", "refined", "polished",
            "transparent", "confident", "precise"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "mixing", "mastering", "corrective_eq", "tonal_shaping", "problem_solving",
            "frequency_carving", "enhancement", "surgical_cuts", "broad_strokes"
        })
        .setInstrumentTags({
            "any", "full_mix", "vocals", "drums", "bass", "guitar"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Low Freq", 0.2f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "20-60Hz"},
            {"20-40", "60-150Hz"},
            {"40-60", "150-300Hz"},
            {"60-80", "300-600Hz"},
            {"80-100", "600-1kHz"}
        })
        .addParameter("Low Gain", 0.0f, -15.0f, 15.0f, "dB", "linear", {
            {"-15--9", "severe cut"},
            {"-9--3", "moderate cut"},
            {"-3-3", "subtle adjustment"},
            {"3-9", "moderate boost"},
            {"9-15", "extreme boost"}
        })
        .addParameter("Mid1 Freq", 0.4f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-25", "200-500Hz"},
            {"25-50", "500-1kHz"},
            {"50-75", "1k-2.5kHz"},
            {"75-100", "2.5k-5kHz"}
        })
        .addParameter("Mid1 Gain", 0.0f, -15.0f, 15.0f, "dB", "linear", {
            {"-15--9", "severe cut"},
            {"-9--3", "moderate cut"},
            {"-3-3", "subtle adjustment"},
            {"3-9", "moderate boost"},
            {"9-15", "extreme boost"}
        })
        .addParameter("Mid1 Q", 0.3f, 0.0f, 1.0f, "", "logarithmic", ParamRanges::Q_RANGES)
        .addParameter("High Freq", 0.7f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "2k-4kHz"},
            {"20-40", "4k-6kHz"},
            {"40-60", "6k-10kHz"},
            {"60-80", "10k-15kHz"},
            {"80-100", "15k-20kHz"}
        })
        .addParameter("High Gain", 0.0f, -15.0f, 15.0f, "dB", "linear", {
            {"-15--9", "severe cut"},
            {"-9--3", "moderate cut"},
            {"-3-3", "subtle adjustment"},
            {"3-9", "moderate boost"},
            {"9-15", "extreme boost"}
        })
        .setTriggerWords({
            "eq", "equalizer", "parametric", "frequency", "tone", "shape",
            "cut", "boost", "surgical", "mix"
        })
        .setCompatibility({
            {ENGINE_CLASSIC_COMPRESSOR, 0.95f},
            {ENGINE_K_STYLE, 0.9f},
            {ENGINE_MASTERING_LIMITER, 0.95f},
            {ENGINE_VINTAGE_CONSOLE_EQ, 0.5f}
        })
        .setPairsWellWith({
            "any", "compression", "saturation", "reverb", "delay"
        })
        .setAvoidWith({
            "other_parametric_eq", "heavy_filtering"
        })
        .setMoodAdjustments({
            {"brighter", 0.1f}, {"darker", -0.1f}, {"warmer", -0.05f},
            {"cleaner", 0.05f}, {"more_presence", 0.1f}, {"muddier", -0.15f}
        })
        .build()
    );
    
    // ENGINE 39: Mastering Limiter
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_MASTERING_LIMITER, "Mastering Limiter", "dynamics",
                     "Transparent brickwall limiter for final stage loudness maximization")
        .setSonicTags({
            "transparent", "loud", "clean", "brickwall", "protective",
            "mastering-grade", "precise", "professional", "final-stage",
            "peak-control", "loudness", "polished"
        })
        .setEmotionalTags({
            "powerful", "professional", "confident", "polished", "competitive",
            "modern", "loud", "impactful", "commercial"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "mastering", "final_limiting", "loudness_maximization", "peak_control",
            "mix_bus", "broadcast_limiting", "streaming_preparation", "cd_mastering"
        })
        .setInstrumentTags({
            "full_mix", "master_bus", "stems", "any"
        })
        .setTechnicalProps(0.4f, 64)  // Lookahead
        .addParameter("Threshold", -3.0f, -30.0f, 0.0f, "dB", "linear", {
            {"-30--20", "heavy limiting"},
            {"-20--10", "moderate limiting"},
            {"-10--5", "gentle limiting"},
            {"-5--2", "peak catching"},
            {"-2-0", "safety limiting"}
        })
        .addParameter("Release", 0.3f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "ultra fast, 1-10ms"},
            {"20-40", "fast, 10-50ms"},
            {"40-60", "medium, 50-200ms"},
            {"60-80", "slow, 200-500ms"},
            {"80-100", "auto-release"}
        })
        .addParameter("Ceiling", -0.3f, -3.0f, 0.0f, "dB", "linear", {
            {"-3.0--2.0", "conservative, -3dB"},
            {"-2.0--1.0", "safe, -2 to -1dB"},
            {"-1.0--0.5", "standard, -0.5dB"},
            {"-0.5--0.2", "aggressive, -0.3dB"},
            {"-0.2-0.0", "maximum, -0.1dB"}
        })
        .addParameter("Character", 0.5f, 0.0f, 1.0f, "", "linear", {
            {"0-20", "transparent"},
            {"20-40", "clean"},
            {"40-60", "smooth"},
            {"60-80", "warm"},
            {"80-100", "aggressive"}
        })
        .addParameter("ISP Mode", 1.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-50", "true peak off"},
            {"50-100", "true peak on"}
        })
        .setTriggerWords({
            "limiter", "mastering", "loudness", "brickwall", "final",
            "peak", "ceiling", "maximizer", "transparent"
        })
        .setCompatibility({
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_MULTIBAND_SATURATOR, 0.9f},
            {ENGINE_DIMENSION_EXPANDER, 0.95f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.7f}
        })
        .setPairsWellWith({
            "eq", "multiband_processing", "stereo_enhancement", "saturation"
        })
        .setAvoidWith({
            "heavy_compression", "other_limiters", "heavy_distortion"
        })
        .setMoodAdjustments({
            {"louder", -0.3f}, {"more_transparent", 0.1f}, {"punchier", -0.1f},
            {"smoother", 0.15f}, {"more_aggressive", -0.2f}, {"cleaner", 0.2f}
        })
        .build()
    );
    
    // ENGINE 40: Noise Gate
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_NOISE_GATE, "Noise Gate", "dynamics",
                     "Advanced noise gate with frequency-conscious detection")
        .setSonicTags({
            "clean", "quiet", "controlled", "gating", "noise-reduction",
            "silence", "precise", "transparent", "surgical", "professional",
            "studio", "cleanup"
        })
        .setEmotionalTags({
            "clean", "professional", "controlled", "precise", "quiet",
            "focused", "tight", "clinical"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "noise_removal", "drum_gating", "vocal_cleanup", "guitar_noise",
            "studio_recording", "live_sound", "podcast_cleanup", "isolation"
        })
        .setInstrumentTags({
            "vocals", "drums", "electric_guitar", "any", "podcasts"
        })
        .setTechnicalProps(0.15f, 0)
        .addParameter("Threshold", -30.0f, -60.0f, 0.0f, "dB", "linear", {
            {"-60--45", "very sensitive"},
            {"-45--30", "sensitive"},
            {"-30--20", "moderate"},
            {"-20--10", "relaxed"},
            {"-10-0", "only loud signals"}
        })
        .addParameter("Range", -40.0f, -80.0f, 0.0f, "dB", "linear", {
            {"-80--60", "complete mute"},
            {"-60--40", "heavy reduction"},
            {"-40--20", "moderate reduction"},
            {"-20--10", "gentle reduction"},
            {"-10-0", "subtle ducking"}
        })
        .addParameter("Attack", 0.1f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "instant, 0-1ms"},
            {"20-40", "fast, 1-5ms"},
            {"40-60", "medium, 5-20ms"},
            {"60-80", "slow, 20-50ms"},
            {"80-100", "very slow, 50-100ms"}
        })
        .addParameter("Hold", 0.2f, 0.0f, 1.0f, "ms", "linear", {
            {"0-20", "no hold, 0-10ms"},
            {"20-40", "short, 10-50ms"},
            {"40-60", "medium, 50-200ms"},
            {"60-80", "long, 200-500ms"},
            {"80-100", "very long, 500-1000ms"}
        })
        .addParameter("Release", 0.3f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "very fast, 10-50ms"},
            {"20-40", "fast, 50-200ms"},
            {"40-60", "medium, 200-500ms"},
            {"60-80", "slow, 500-1000ms"},
            {"80-100", "very slow, 1-2s"}
        })
        .addParameter("Filter Freq", 0.0f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0", "no filtering"},
            {"1-30", "80-200Hz highpass"},
            {"30-60", "200-500Hz"},
            {"60-80", "500-1kHz"},
            {"80-100", "1k-2kHz"}
        })
        .setTriggerWords({
            "gate", "noise gate", "silence", "cleanup", "noise", "mute",
            "quiet", "isolation", "reduction"
        })
        .setCompatibility({
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_TRANSIENT_SHAPER, 0.85f},
            {ENGINE_RODENT_DISTORTION, 0.9f}
        })
        .setPairsWellWith({
            "compression", "eq", "distortion", "reverb", "any"
        })
        .setAvoidWith({
            "heavy_reverb", "long_delays", "ambient_effects"
        })
        .setMoodAdjustments({
            {"tighter", -0.2f}, {"looser", 0.2f}, {"cleaner", -0.15f},
            {"more_natural", 0.15f}, {"more_aggressive", -0.25f}, {"gentler", 0.2f}
        })
        .build()
    );
    
    // ENGINE 41: Vintage Opto Compressor
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_VINTAGE_OPTO_COMPRESSOR, "Vintage Opto Compressor", "dynamics",
                     "Optical compressor with smooth, musical compression characteristics")
        .setSonicTags({
            "smooth", "musical", "optical", "vintage", "warm", "gentle",
            "transparent", "LA-2A", "tube", "creamy", "natural", "singing",
            "effortless", "classic"
        })
        .setEmotionalTags({
            "warm", "smooth", "vintage", "musical", "gentle", "organic",
            "classic", "luxurious", "expensive", "timeless"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "vocal_compression", "bass_smoothing", "mix_glue", "gentle_leveling",
            "vintage_sound", "musical_compression", "broadcasting", "mastering"
        })
        .setInstrumentTags({
            "vocals", "bass", "full_mix", "strings", "brass", "any"
        })
        .setTechnicalProps(0.3f, 0)
        .addParameter("Gain", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle input gain boost"},
            {"20-40", "moderate gain, 6-12dB"},
            {"40-60", "healthy gain, 12-24dB"},
            {"60-80", "high gain, 24-36dB"},
            {"80-100", "maximum gain, 36-40dB"}
        })
        .addParameter("Peak Reduction", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "subtle compression, 1-3dB"},
            {"20-40", "gentle compression, 3-6dB"},
            {"40-60", "moderate compression, 6-10dB"},
            {"60-80", "heavy compression, 10-15dB"},
            {"80-100", "limiting territory, 15-20dB"}
        })
        .addParameter("HF Emphasis", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "flat response, natural"},
            {"25-50", "subtle high-frequency lift"},
            {"50-75", "moderate HF emphasis"},
            {"75-100", "strong high-frequency boost"}
        })
        .addParameter("Output", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-30", "quiet, attenuated output"},
            {"30-60", "moderate output level"},
            {"60-80", "unity gain, matched"},
            {"80-100", "boosted, hot output"}
        })
        .addParameter("Mix", 1.0f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "mostly dry signal"},
            {"25-50", "blend of dry/compressed"},
            {"50-75", "mostly compressed"},
            {"75-100", "fully wet, compressed"}
        })
        .addParameter("Knee", 0.7f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "hard knee, aggressive"},
            {"25-50", "moderate knee, punchy"},
            {"50-75", "soft knee, musical"},
            {"75-100", "very soft knee, smooth"}
        })
        .addParameter("Harmonics", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "clean, transparent"},
            {"25-50", "subtle tube coloration"},
            {"50-75", "vintage tube warmth"},
            {"75-100", "heavy tube saturation"}
        })
        .addParameter("Stereo Link", 1.0f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "independent channels"},
            {"25-50", "loose stereo linking"},
            {"50-75", "moderate stereo link"},
            {"75-100", "tight stereo coupling"}
        })
        .setTriggerWords({
            "opto", "optical", "LA-2A", "vintage compressor", "smooth",
            "musical", "gentle", "tube", "warm compression"
        })
        .setCompatibility({
            {ENGINE_PARAMETRIC_EQ, 0.95f},
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_PLATE_REVERB, 0.85f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.4f}
        })
        .setPairsWellWith({
            "eq", "reverb", "delay", "saturation", "tube_preamp"
        })
        .setAvoidWith({
            "other_compressors", "heavy_limiting", "aggressive_dynamics"
        })
        .setMoodAdjustments({
            {"warmer", 0.15f}, {"smoother", 0.2f}, {"more_vintage", 0.2f},
            {"cleaner", -0.2f}, {"more_aggressive", -0.3f}, {"gentler", 0.15f}
        })
        .build()
    );
    
    // ENGINE 42: Spectral Gate
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_SPECTRAL_GATE, "Spectral Gate", "spectral",
                     "Frequency-dependent gating with 32 independent bands")
        .setSonicTags({
            "spectral", "frequency-selective", "surgical", "noise-removal",
            "restoration", "clean", "precise", "multiband-gate", "fft-based",
            "transparent", "artifact-removal", "intelligent"
        })
        .setEmotionalTags({
            "clean", "precise", "technical", "surgical", "professional",
            "clinical", "transparent", "modern"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "noise_removal", "restoration", "hum_removal", "background_noise",
            "spectral_cleanup", "forensic_audio", "podcast_cleanup", "field_recording_cleanup"
        })
        .setInstrumentTags({
            "any", "vocals", "field_recordings", "restoration", "dialogue"
        })
        .setTechnicalProps(0.6f, 512)  // FFT processing
        .addParameter("Threshold", -40.0f, -80.0f, 0.0f, "dB", "linear", {
            {"-80--60", "very sensitive"},
            {"-60--40", "sensitive"},
            {"-40--20", "moderate"},
            {"-20--10", "relaxed"},
            {"-10-0", "only loud content"}
        })
        .addParameter("Ratio", 0.8f, 0.0f, 1.0f, "", "linear", {
            {"0-20", "gentle reduction"},
            {"20-40", "moderate gating"},
            {"40-60", "strong gating"},
            {"60-80", "heavy gating"},
            {"80-100", "complete removal"}
        })
        .addParameter("Attack", 0.2f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "instant, 0-5ms"},
            {"20-40", "fast, 5-20ms"},
            {"40-60", "medium, 20-50ms"},
            {"60-80", "slow, 50-100ms"},
            {"80-100", "very slow, 100-200ms"}
        })
        .addParameter("Release", 0.4f, 0.0f, 1.0f, "ms", "logarithmic", {
            {"0-20", "very fast, 10-50ms"},
            {"20-40", "fast, 50-200ms"},
            {"40-60", "medium, 200-500ms"},
            {"60-80", "slow, 500-1000ms"},
            {"80-100", "very slow, 1-2s"}
        })
        .addParameter("Frequency Smoothing", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "independent bands"},
            {"20-40", "slight smoothing"},
            {"40-60", "moderate smoothing"},
            {"60-80", "heavy smoothing"},
            {"80-100", "very smooth"}
        })
        .setTriggerWords({
            "spectral gate", "frequency gate", "noise removal", "restoration",
            "cleanup", "spectral", "multiband gate"
        })
        .setCompatibility({
            {ENGINE_PARAMETRIC_EQ, 0.9f},
            {ENGINE_NOISE_GATE, 0.7f},
            {ENGINE_CLASSIC_COMPRESSOR, 0.85f},
            {ENGINE_SPECTRAL_FREEZE, 0.4f}
        })
        .setPairsWellWith({
            "eq", "compression", "de-noising", "restoration_tools"
        })
        .setAvoidWith({
            "heavy_distortion", "bit_crushing", "lo-fi_effects"
        })
        .setMoodAdjustments({
            {"cleaner", -0.2f}, {"more_natural", 0.15f}, {"more_surgical", -0.25f},
            {"gentler", 0.2f}, {"more_aggressive", -0.3f}, {"transparent", 0.1f}
        })
        .build()
    );
    
    // ENGINE 43: Chaos Generator
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_CHAOS_GENERATOR, "Chaos Generator", "experimental",
                     "Controlled randomness using chaos theory and strange attractors")
        .setSonicTags({
            "chaotic", "random", "unpredictable", "generative", "experimental",
            "strange-attractor", "nonlinear", "evolving", "complex", "organic",
            "mathematical", "fractal", "emergent", "self-organizing"
        })
        .setEmotionalTags({
            "unpredictable", "experimental", "mysterious", "complex", "fascinating",
            "alien", "organic", "evolving", "mathematical"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "sound_design", "experimental_music", "generative_music", "modulation_source",
            "creative_effects", "ambient_textures", "sci_fi_sounds", "chaos_modulation"
        })
        .setInstrumentTags({
            "synthesizer", "any", "noise", "modular", "experimental"
        })
        .setTechnicalProps(0.3f, 0)
        .addParameter("System", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-20", "Lorenz attractor"},
            {"20-40", "Rossler system"},
            {"40-60", "Henon map"},
            {"60-80", "Chua circuit"},
            {"80-100", "Logistic map"}
        })
        .addParameter("Rate", 0.5f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "very slow, 0.01-0.1Hz"},
            {"20-40", "slow, 0.1-1Hz"},
            {"40-60", "moderate, 1-10Hz"},
            {"60-80", "fast, 10-100Hz"},
            {"80-100", "audio rate, 100-1000Hz"}
        })
        .addParameter("Chaos", 0.7f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "periodic, stable"},
            {"20-40", "edge of chaos"},
            {"40-60", "chaotic regime"},
            {"60-80", "highly chaotic"},
            {"80-100", "extreme chaos"}
        })
        .addParameter("Dimension", 0.5f, 0.0f, 1.0f, "", "linear", {
            {"0-33", "X output only"},
            {"33-66", "X + Y mixed"},
            {"66-100", "X + Y + Z (3D)"}
        })
        .addParameter("Smooth", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "raw chaos"},
            {"20-40", "slight smoothing"},
            {"40-60", "moderate smooth"},
            {"60-80", "heavy smoothing"},
            {"80-100", "very smooth"}
        })
        .addParameter("Mix", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "chaos", "random", "generative", "strange attractor", "lorenz",
            "experimental", "unpredictable", "evolving"
        })
        .setCompatibility({
            {ENGINE_SPECTRAL_FREEZE, 0.85f},
            {ENGINE_GRANULAR_CLOUD, 0.8f},
            {ENGINE_ANALOG_RING_MODULATOR, 0.75f},
            {ENGINE_TRANSIENT_SHAPER, 0.2f}
        })
        .setPairsWellWith({
            "filters", "delays", "reverbs", "spectral_effects", "modulation"
        })
        .setAvoidWith({
            "precise_effects", "surgical_tools", "gates"
        })
        .setMoodAdjustments({
            {"more_chaotic", 0.2f}, {"more_stable", -0.2f}, {"weirder", 0.3f},
            {"smoother", 0.15f}, {"more_extreme", 0.35f}, {"subtler", -0.25f}
        })
        .build()
    );
    
    // ENGINE 44: Buffer Repeat
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_BUFFER_REPEAT, "Buffer Repeat", "glitch",
                     "Real-time buffer capture and repeat for glitch effects")
        .setSonicTags({
            "glitch", "stutter", "repeat", "buffer", "chop", "slice",
            "rhythmic", "electronic", "digital", "precise", "locked",
            "beat-repeat", "frozen", "looping"
        })
        .setEmotionalTags({
            "glitchy", "electronic", "modern", "rhythmic", "mechanical",
            "precise", "digital", "futuristic", "robotic"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "glitch_effects", "electronic_music", "beat_repeat", "stutter_effects",
            "edm_production", "live_performance", "dj_effects", "rhythmic_effects"
        })
        .setInstrumentTags({
            "drums", "any", "electronic", "vocals", "full_mix"
        })
        .setTechnicalProps(0.2f, 0)
        .addParameter("Division", 0.25f, 0.0f, 1.0f, "", "stepped", {
            {"0-12", "1/32"},
            {"12-25", "1/16"},
            {"25-37", "1/8"},
            {"37-50", "1/4"},
            {"50-62", "1/2"},
            {"62-75", "1 bar"},
            {"75-87", "2 bars"},
            {"87-100", "4 bars"}
        })
        .addParameter("Repeats", 0.5f, 0.0f, 1.0f, "", "stepped", {
            {"0-12", "1 repeat"},
            {"12-25", "2 repeats"},
            {"25-37", "3 repeats"},
            {"37-50", "4 repeats"},
            {"50-62", "6 repeats"},
            {"62-75", "8 repeats"},
            {"75-87", "12 repeats"},
            {"87-100", "16 repeats"}
        })
        .addParameter("Trigger", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-50", "manual/off"},
            {"50-100", "capture/repeat"}
        })
        .addParameter("Pitch", 0.0f, -12.0f, 12.0f, "semitones", "stepped", {
            {"-12", "down octave"},
            {"-7", "down fifth"},
            {"-5", "down fourth"},
            {"0", "original pitch"},
            {"5", "up fourth"},
            {"7", "up fifth"},
            {"12", "up octave"}
        })
        .addParameter("Decay", 0.0f, 0.0f, 1.0f, "%", "linear", {
            {"0", "no decay"},
            {"1-30", "slight fade"},
            {"30-60", "moderate decay"},
            {"60-80", "fast decay"},
            {"80-100", "very fast fade"}
        })
        .addParameter("Mix", 1.0f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "buffer", "repeat", "glitch", "stutter", "beat repeat", "chop",
            "slice", "loop", "freeze"
        })
        .setCompatibility({
            {ENGINE_BIT_CRUSHER, 0.85f},
            {ENGINE_STATE_VARIABLE_FILTER, 0.8f},
            {ENGINE_TAPE_ECHO, 0.75f},
            {ENGINE_GRANULAR_CLOUD, 0.4f}
        })
        .setPairsWellWith({
            "filters", "distortion", "delay", "reverb", "bit_crushing"
        })
        .setAvoidWith({
            "time_based_effects", "heavy_reverb", "long_delays"
        })
        .setMoodAdjustments({
            {"more_glitchy", 0.25f}, {"smoother", -0.3f}, {"more_rhythmic", 0.2f},
            {"more_chaotic", 0.3f}, {"tighter", -0.2f}, {"more_extreme", 0.35f}
        })
        .build()
    );
    
    // ENGINE 45: Vintage Console EQ
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_VINTAGE_CONSOLE_EQ, "Vintage Console EQ", "eq",
                     "Classic British console EQ with musical curves and transformer color")
        .setSonicTags({
            "vintage", "console", "british", "musical", "colored", "warm",
            "transformer", "neve-style", "thick", "punchy", "analog",
            "classic", "fat", "expensive"
        })
        .setEmotionalTags({
            "vintage", "warm", "expensive", "classic", "luxurious",
            "professional", "musical", "rich", "legendary"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "mixing", "vintage_sound", "console_emulation", "musical_eq",
            "character_eq", "drum_enhancement", "vocal_warmth", "transformer_color"
        })
        .setInstrumentTags({
            "drums", "vocals", "bass", "any", "full_mix"
        })
        .setTechnicalProps(0.3f, 0)
        .addParameter("Low Gain", 0.0f, -15.0f, 15.0f, "dB", "linear", {
            {"-15--9", "heavy cut, tightens low end"},
            {"-9--3", "moderate cut, cleans up mud"},
            {"-3-3", "subtle adjustment, natural"},
            {"3-9", "moderate boost, warm full"},
            {"9-15", "heavy boost, thick powerful"}
        })
        .addParameter("Low Freq", 0.3f, 0.0f, 1.0f, "%", "logarithmic", {
            {"0-25", "60Hz, deep sub bass"},
            {"25-50", "100Hz, fundamental bass"},
            {"50-75", "200Hz, low midrange"},
            {"75-100", "400Hz, muddy region"}
        })
        .addParameter("Mid Gain", 0.0f, -15.0f, 15.0f, "dB", "linear", {
            {"-15--9", "heavy cut, scooped sound"},
            {"-9--3", "moderate cut, removes harshness"},
            {"-3-3", "subtle adjustment, balanced"},
            {"3-9", "moderate boost, presence"},
            {"9-15", "heavy boost, aggressive forward"}
        })
        .addParameter("Mid Freq", 0.5f, 0.0f, 1.0f, "%", "logarithmic", {
            {"0-25", "500Hz, low-mid body"},
            {"25-50", "1kHz, vocal presence"},
            {"50-75", "2kHz, attack/clarity"},
            {"75-100", "4kHz, bite/edge"}
        })
        .addParameter("Mid Q", 0.7f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "very wide, musical"},
            {"25-50", "wide, gentle shaping"},
            {"50-75", "moderate, focused"},
            {"75-100", "narrow, surgical"}
        })
        .addParameter("High Gain", 0.0f, -15.0f, 15.0f, "dB", "linear", {
            {"-15--9", "heavy cut, dull/warm"},
            {"-9--3", "moderate cut, smooth"},
            {"-3-3", "subtle adjustment, natural"},
            {"3-9", "moderate boost, airy/bright"},
            {"9-15", "heavy boost, crisp/sparkle"}
        })
        .addParameter("High Freq", 0.7f, 0.0f, 1.0f, "%", "logarithmic", {
            {"0-25", "6kHz, presence region"},
            {"25-50", "8kHz, clarity/definition"},
            {"50-75", "12kHz, air/openness"},
            {"75-100", "16kHz, sparkle/sheen"}
        })
        .addParameter("Drive", 0.2f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "clean, transparent"},
            {"20-40", "subtle console warmth"},
            {"40-60", "obvious transformer color"},
            {"60-80", "heavy saturation, thick"},
            {"80-100", "driven hard, compressed"}
        })
        .addParameter("Console Type", 0.0f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "Neve style, warm punchy"},
            {"25-50", "API style, clean aggressive"},
            {"50-75", "SSL style, modern precise"},
            {"75-100", "Trident style, musical colored"}
        })
        .addParameter("Vintage", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-25", "modern, clean response"},
            {"25-50", "slight vintage character"},
            {"50-75", "obvious vintage coloration"},
            {"75-100", "heavily aged, maximum vintage"}
        })
        .setTriggerWords({
            "console eq", "vintage eq", "british eq", "neve", "musical eq",
            "transformer", "colored eq", "analog eq"
        })
        .setCompatibility({
            {ENGINE_VINTAGE_OPTO_COMPRESSOR, 0.95f},
            {ENGINE_VINTAGE_TUBE_PREAMP, 0.9f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_PARAMETRIC_EQ, 0.5f}
        })
        .setPairsWellWith({
            "vintage_compression", "tape_effects", "tube_saturation", "analog_gear"
        })
        .setAvoidWith({
            "digital_eq", "surgical_eq", "transparent_effects"
        })
        .setMoodAdjustments({
            {"warmer", 0.2f}, {"more_vintage", 0.15f}, {"cleaner", -0.2f},
            {"more_colored", 0.25f}, {"more_transparent", -0.25f}, {"punchier", 0.15f}
        })
        .build()
    );
    
    // ENGINE 46: Mid-Side Processor
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_MID_SIDE_PROCESSOR, "Mid-Side Processor", "spatial",
                     "Mid-side processing for surgical stereo field control")
        .setSonicTags({
            "mid-side", "M/S", "stereo", "surgical", "width-control",
            "center-sides", "imaging", "spatial", "precise", "mastering",
            "professional", "transparent", "control"
        })
        .setEmotionalTags({
            "precise", "professional", "technical", "controlled", "surgical",
            "sophisticated", "clean", "modern"
        })
        .setFrequencyFocus("full")
        .setUseCases({
            "mastering", "stereo_control", "width_adjustment", "problem_solving",
            "mono_compatibility", "surgical_processing", "mix_enhancement", "spatial_control"
        })
        .setInstrumentTags({
            "full_mix", "master_bus", "stereo_sources", "any"
        })
        .setTechnicalProps(0.2f, 0, false, true)
        .addParameter("Mid Gain", 0.0f, -12.0f, 12.0f, "dB", "linear", {
            {"-12--6", "heavy cut"},
            {"-6--3", "moderate cut"},
            {"-3-3", "unity range"},
            {"3-6", "moderate boost"},
            {"6-12", "heavy boost"}
        })
        .addParameter("Side Gain", 0.0f, -12.0f, 12.0f, "dB", "linear", {
            {"-12--6", "narrow/mono"},
            {"-6--3", "reduced width"},
            {"-3-3", "natural width"},
            {"3-6", "enhanced width"},
            {"6-12", "extreme width"}
        })
        .addParameter("Mid EQ", 0.0f, -10.0f, 10.0f, "dB", "linear", {
            {"-10--5", "dark center"},
            {"-5--2", "warm center"},
            {"-2-2", "neutral"},
            {"2-5", "bright center"},
            {"5-10", "crisp center"}
        })
        .addParameter("Side EQ", 0.0f, -10.0f, 10.0f, "dB", "linear", {
            {"-10--5", "dark sides"},
            {"-5--2", "warm sides"},
            {"-2-2", "neutral"},
            {"2-5", "bright sides"},
            {"5-10", "airy sides"}
        })
        .addParameter("Bass Mono", 0.3f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0", "no bass mono"},
            {"1-25", "below 80Hz"},
            {"25-50", "below 150Hz"},
            {"50-75", "below 250Hz"},
            {"75-100", "below 400Hz"}
        })
        .setTriggerWords({
            "mid side", "M/S", "mid-side", "stereo control", "width",
            "imaging", "spatial", "mastering"
        })
        .setCompatibility({
            {ENGINE_MASTERING_LIMITER, 0.95f},
            {ENGINE_PARAMETRIC_EQ, 0.9f},
            {ENGINE_DIMENSION_EXPANDER, 0.7f},
            {ENGINE_MULTIBAND_SATURATOR, 0.85f}
        })
        .setPairsWellWith({
            "mastering_chain", "eq", "compression", "limiting", "imaging"
        })
        .setAvoidWith({
            "mono_effects", "heavy_modulation", "time_based_effects"
        })
        .setMoodAdjustments({
            {"wider", 0.2f}, {"narrower", -0.2f}, {"center_focused", -0.15f},
            {"sides_enhanced", 0.15f}, {"more_mono", -0.3f}, {"more_stereo", 0.3f}
        })
        .build()
    );
    
    // ENGINE 47: Vintage Tube Preamp
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_VINTAGE_TUBE_PREAMP, "Vintage Tube Preamp", "saturation",
                     "Tube preamp emulation with authentic harmonic saturation")
        .setSonicTags({
            "tube", "valve", "warm", "vintage", "harmonic", "saturated",
            "analog", "creamy", "thick", "musical", "expensive", "classic",
            "smooth", "rich", "euphonic"
        })
        .setEmotionalTags({
            "warm", "vintage", "luxurious", "expensive", "classic",
            "nostalgic", "organic", "musical", "rich"
        })
        .setFrequencyFocus("low-mid")
        .setUseCases({
            "warming_up", "vintage_color", "harmonic_enhancement", "analog_warmth",
            "tube_saturation", "mix_glue", "character_addition", "expensive_sound"
        })
        .setInstrumentTags({
            "vocals", "bass", "drums", "full_mix", "any"
        })
        .setTechnicalProps(0.25f, 0)
        .addParameter("Drive", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "clean, subtle warmth"},
            {"20-40", "gentle saturation"},
            {"40-60", "obvious tube color"},
            {"60-80", "heavy saturation"},
            {"80-100", "overdriven tubes"}
        })
        .addParameter("Tube Type", 0.33f, 0.0f, 1.0f, "", "stepped", {
            {"0-33", "12AX7 - classic"},
            {"33-66", "12AT7 - cleaner"},
            {"66-100", "6L6 - power tube"}
        })
        .addParameter("Bias", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "cold bias, edgy"},
            {"20-40", "cool, crisp"},
            {"40-60", "optimal bias"},
            {"60-80", "warm bias"},
            {"80-100", "hot bias, compressed"}
        })
        .addParameter("Low Frequency", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "thin, no bass"},
            {"20-40", "controlled lows"},
            {"40-60", "balanced response"},
            {"60-80", "enhanced bass"},
            {"80-100", "huge bottom end"}
        })
        .addParameter("High Frequency", 0.7f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "dark, vintage"},
            {"20-40", "warm highs"},
            {"40-60", "balanced treble"},
            {"60-80", "bright, airy"},
            {"80-100", "crisp, modern"}
        })
        .addParameter("Output", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-40", "attenuated"},
            {"40-60", "unity gain"},
            {"60-100", "boosted output"}
        })
        .setTriggerWords({
            "tube", "valve", "preamp", "vintage", "warm", "saturation",
            "analog", "harmonic", "12AX7", "classic"
        })
        .setCompatibility({
            {ENGINE_VINTAGE_OPTO_COMPRESSOR, 0.95f},
            {ENGINE_VINTAGE_CONSOLE_EQ, 0.9f},
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_K_STYLE, 0.6f},
            {ENGINE_MULTIBAND_SATURATOR, 0.5f}
        })
        .setPairsWellWith({
            "vintage_gear", "tape_effects", "analog_eq", "opto_compression"
        })
        .setAvoidWith({
            "digital_distortion", "bit_crushers", "harsh_effects"
        })
        .setMoodAdjustments({
            {"warmer", 0.2f}, {"cleaner", -0.2f}, {"more_vintage", 0.25f},
            {"modern", -0.15f}, {"thicker", 0.15f}, {"brighter", 0.1f}
        })
        .build()
    );
    
    // ENGINE 48: Spring Reverb
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_SPRING_REVERB, "Spring Reverb", "reverb",
                     "Physical model of vintage spring reverb tanks")
        .setSonicTags({
            "spring", "metallic", "vintage", "surf", "twangy", "boingy",
            "mechanical", "resonant", "characteristic", "amp-reverb",
            "guitar-amp", "splashy", "unique", "physical-model"
        })
        .setEmotionalTags({
            "vintage", "nostalgic", "surf", "retro", "garage", "raw",
            "authentic", "characterful", "lo-fi"
        })
        .setFrequencyFocus("mid")
        .setUseCases({
            "surf_guitar", "vintage_production", "amp_reverb", "rockabilly",
            "garage_rock", "retro_sounds", "character_reverb", "lo_fi_reverb"
        })
        .setInstrumentTags({
            "electric_guitar", "organ", "vocals", "snare", "synthesizer"
        })
        .setTechnicalProps(0.35f, 0)
        .addParameter("Tank Type", 0.25f, 0.0f, 1.0f, "", "stepped", {
            {"0-25", "short tank, bright"},
            {"25-50", "medium tank, balanced"},
            {"50-75", "long tank, dark"},
            {"75-100", "dual tank, complex"}
        })
        .addParameter("Decay", 0.4f, 0.0f, 1.0f, "s", "linear", {
            {"0-20", "very short, 0.5-1s"},
            {"20-40", "short, 1-2s"},
            {"40-60", "medium, 2-3s"},
            {"60-80", "long, 3-5s"},
            {"80-100", "very long, 5-8s"}
        })
        .addParameter("Tension", 0.5f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "loose, dark"},
            {"20-40", "relaxed"},
            {"40-60", "normal tension"},
            {"60-80", "tight, bright"},
            {"80-100", "very tight, metallic"}
        })
        .addParameter("Drip", 0.3f, 0.0f, 1.0f, "%", "linear", {
            {"0", "no drip"},
            {"1-30", "subtle splash"},
            {"30-60", "moderate drip"},
            {"60-80", "obvious splash"},
            {"80-100", "extreme drip"}
        })
        .addParameter("Tone", 0.6f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "very dark"},
            {"20-40", "warm"},
            {"40-60", "balanced"},
            {"60-80", "bright"},
            {"80-100", "very bright"}
        })
        .addParameter("Mix", 0.25f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "spring", "reverb", "vintage", "surf", "amp reverb", "tank",
            "twang", "boing", "drip", "splash"
        })
        .setCompatibility({
            {ENGINE_K_STYLE, 0.95f},
            {ENGINE_CLASSIC_TREMOLO, 0.9f},
            {ENGINE_TAPE_ECHO, 0.85f},
            {ENGINE_PLATE_REVERB, 0.3f},
            {ENGINE_CONVOLUTION_REVERB, 0.3f}
        })
        .setPairsWellWith({
            "overdrive", "tremolo", "tape_echo", "vintage_effects", "compression"
        })
        .setAvoidWith({
            "other_reverbs", "modern_effects", "pristine_delays"
        })
        .setMoodAdjustments({
            {"more_vintage", 0.2f}, {"brighter", 0.15f}, {"darker", -0.15f},
            {"more_drip", 0.25f}, {"cleaner", -0.2f}, {"more_surf", 0.3f}
        })
        .build()
    );
    
    // ENGINE 49: Resonant Chorus
    registry.registerEngine(
        MetadataBuilder()
        .setBasicInfo(ENGINE_RESONANT_CHORUS, "Resonant Chorus", "modulation",
                     "Chorus with resonant filter for unique swirling textures")
        .setSonicTags({
            "resonant", "filtered", "swirling", "unique", "complex",
            "textured", "evolving", "liquid", "bubbly", "psychedelic",
            "animated", "colorful", "rich", "dimensional"
        })
        .setEmotionalTags({
            "dreamy", "psychedelic", "ethereal", "mysterious", "colorful",
            "evolving", "liquid", "mesmerizing", "otherworldly"
        })
        .setFrequencyFocus("variable")
        .setUseCases({
            "psychedelic_production", "creative_modulation", "unique_textures",
            "ambient_music", "experimental_effects", "synth_processing", "guitar_effects"
        })
        .setInstrumentTags({
            "synthesizer", "electric_guitar", "electric_piano", "pads", "any"
        })
        .setTechnicalProps(0.35f, 0)
        .addParameter("Rate", 0.3f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "very slow, 0.05-0.2Hz"},
            {"20-40", "slow, 0.2-0.5Hz"},
            {"40-60", "moderate, 0.5-2Hz"},
            {"60-80", "fast, 2-5Hz"},
            {"80-100", "very fast, 5-10Hz"}
        })
        .addParameter("Depth", 0.5f, 0.0f, 1.0f, "%", "linear", ParamRanges::MOD_DEPTH_RANGES)
        .addParameter("Resonance", 0.6f, 0.0f, 1.0f, "%", "linear", {
            {"0-20", "no resonance"},
            {"20-40", "subtle peak"},
            {"40-60", "moderate resonance"},
            {"60-80", "strong resonance"},
            {"80-100", "self-oscillating"}
        })
        .addParameter("Filter Freq", 0.5f, 0.0f, 1.0f, "Hz", "logarithmic", {
            {"0-20", "low, 100-300Hz"},
            {"20-40", "low-mid, 300-800Hz"},
            {"40-60", "mid, 800-2kHz"},
            {"60-80", "high-mid, 2k-5kHz"},
            {"80-100", "high, 5k-10kHz"}
        })
        .addParameter("Feedback", 0.3f, -1.0f, 1.0f, "%", "linear", {
            {"-100--50", "negative, hollow"},
            {"-50--20", "slight negative"},
            {"-20-20", "minimal feedback"},
            {"20-50", "positive feedback"},
            {"50-80", "strong regeneration"},
            {"80-100", "extreme feedback"}
        })
        .addParameter("Mix", 0.4f, 0.0f, 1.0f, "%", "linear", ParamRanges::MIX_RANGES)
        .setTriggerWords({
            "resonant chorus", "filtered chorus", "unique", "swirling",
            "psychedelic", "bubbly", "liquid", "textured"
        })
        .setCompatibility({
            {ENGINE_TAPE_ECHO, 0.9f},
            {ENGINE_SHIMMER_REVERB, 0.85f},
            {ENGINE_ANALOG_PHASER, 0.7f},
            {ENGINE_STEREO_CHORUS, 0.4f}
        })
        .setPairsWellWith({
            "delay", "reverb", "filters", "distortion", "pitch_effects"
        })
        .setAvoidWith({
            "other_chorus", "heavy_modulation", "similar_resonant_effects"
        })
        .setMoodAdjustments({
            {"weirder", 0.25f}, {"subtler", -0.2f}, {"more_resonant", 0.2f},
            {"cleaner", -0.15f}, {"more_psychedelic", 0.3f}, {"tamer", -0.25f}
        })
        .build()
    );
    
    // Initialize the registry with all metadata
    DBG("Initialized metadata for all 50 engines");
}
}