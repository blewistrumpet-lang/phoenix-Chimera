#include "EngineMetadata.h"
#include "ParameterDefinitions.h"

// Initialize all engine metadata with rich, thoughtful descriptions
void initializeEngineMetadata() {
    auto& registry = EngineMetadataRegistry::getInstance();
    
    // ENGINE 0: K-Style Overdrive
    {
        EngineMetadata meta;
        meta.engineId = ENGINE_K_STYLE;
        meta.name = "K-Style Overdrive";
        meta.category = "distortion";
        meta.description = "Smooth tube-style overdrive with asymmetric clipping for even harmonics";
        
        // Sonic characteristics - what it SOUNDS like
        meta.sonicTags = {
            "warm", "smooth", "creamy", "musical", "tube-like", "harmonic-rich",
            "midrange-focused", "touch-sensitive", "dynamic", "analog-warmth",
            "gentle-compression", "singing-sustain", "bluesy", "vintage-voiced"
        };
        
        // Emotional impact - what it FEELS like
        meta.emotionalTags = {
            "confident", "aggressive", "passionate", "gritty", "soulful",
            "expressive", "powerful", "raw", "organic", "human", "responsive"
        };
        
        meta.frequencyFocus = "midrange"; // Where it has most impact
        
        // Real-world use cases
        meta.typicalUseCases = {
            "guitar_lead", "guitar_rhythm", "bass_warmth", "drum_saturation",
            "vocal_grit", "mix_glue", "analog_warmth", "tube_emulation"
        };
        
        meta.instrumentTags = {
            "electric_guitar", "bass_guitar", "synthesizer", "organ",
            "electric_piano", "drums", "male_vocals"
        };
        
        // Technical properties for smart routing
        meta.cpuComplexity = 0.15f; // Very efficient
        meta.latencySamples = 0;
        meta.supportsSidechain = false;
        meta.requiresStereo = false;
        
        // Parameter personalities - this is crucial for AI
        meta.parameters.push_back({
            "Drive",
            0.3f,  // Default 30% - warm but not distorted
            0.0f,  // Min
            1.0f,  // Max
            "%",   // Unit
            "logarithmic", // More control in lower range
            {
                {"0-10", "clean boost, no clipping"},
                {"10-25", "edge of breakup, touch sensitive"},
                {"25-40", "warm overdrive, singing sustain"},
                {"40-60", "saturated overdrive, compressed"},
                {"60-80", "heavy overdrive, harmonically rich"},
                {"80-100", "fuzz territory, heavily compressed"}
            }
        });
        
        meta.parameters.push_back({
            "Tone",
            0.5f,  // Default 50% - balanced
            0.0f,
            1.0f,
            "%",
            "linear",
            {
                {"0-20", "dark, vintage, muffled"},
                {"20-40", "warm, reduced presence"},
                {"40-60", "balanced, natural"},
                {"60-80", "bright, modern, cutting"},
                {"80-100", "aggressive highs, fizzy"}
            }
        });
        
        meta.parameters.push_back({
            "Output",
            0.5f,  // Unity gain by default
            0.0f,
            1.0f,
            "%",
            "linear",
            {
                {"0-40", "attenuated, quieter than input"},
                {"40-60", "unity gain, level matched"},
                {"60-100", "boosted, driving next stage"}
            }
        });
        
        // AI guidance - what prompts should trigger this
        meta.triggerWords = {
            "overdrive", "drive", "warm", "tube", "valve", "k-style", "boutique",
            "smooth", "creamy", "bluesy", "lead", "sustain", "breakup", "edge",
            "grit", "saturation", "analog", "vintage", "classic"
        };
        
        // Compatibility scoring - how well it plays with others
        meta.compatibilityScores = {
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},    // Great before compression
            {ENGINE_PARAMETRIC_EQ, 0.95f},         // EQ shapes the tone beautifully
            {ENGINE_TAPE_ECHO, 0.85f},             // Classic combination
            {ENGINE_PLATE_REVERB, 0.8f},           // Adds space to driven tone
            {ENGINE_VINTAGE_TUBE_PREAMP, 0.6f},    // Can be too much saturation
            {ENGINE_BIT_CRUSHER, 0.3f},            // Conflicting distortion types
            {ENGINE_RODENT_DISTORTION, 0.2f}       // Don't stack similar effects
        };
        
        // What it pairs well with
        meta.pairsWellWith = {
            "compression", "eq", "reverb", "delay", "modulation", "wah"
        };
        
        // What to avoid
        meta.avoidWith = {
            "other_distortion", "bit_crusher", "heavy_fuzz"
        };
        
        // Mood adjustments - how to tweak for different vibes
        meta.moodAdjustments = {
            {"warmer", 0.1f},        // Increase drive slightly
            {"cleaner", -0.2f},      // Reduce drive
            {"brighter", 0.2f},      // Increase tone
            {"darker", -0.2f},       // Reduce tone
            {"aggressive", 0.3f},    // More drive
            {"subtle", -0.15f},      // Less drive
            {"vintage", -0.1f}       // Darker tone
        };
        
        registry.registerEngine(meta);
    }
    
    // ENGINE 1: Tape Echo
    {
        EngineMetadata meta;
        meta.engineId = ENGINE_TAPE_ECHO;
        meta.name = "Tape Echo";
        meta.category = "delay";
        meta.description = "Authentic tape delay with wow, flutter, saturation, and self-oscillation";
        
        meta.sonicTags = {
            "warm", "analog", "vintage", "wobbly", "saturated", "feedback-capable",
            "lo-fi", "degraded", "modulated", "unstable", "characterful",
            "tape-saturation", "wow-and-flutter", "dub-capable", "self-oscillating"
        };
        
        meta.emotionalTags = {
            "nostalgic", "dreamy", "hypnotic", "psychedelic", "mysterious",
            "spacious", "floating", "ethereal", "retro", "experimental",
            "meditative", "trippy"
        };
        
        meta.frequencyFocus = "full";
        
        meta.typicalUseCases = {
            "vocal_throw", "guitar_ambience", "dub_effects", "psychedelic_production",
            "vintage_emulation", "space_creation", "rhythmic_delays", "ambient_washes"
        };
        
        meta.instrumentTags = {
            "vocals", "electric_guitar", "synthesizer", "drums", "percussion",
            "bass", "keys", "saxophone"
        };
        
        meta.cpuComplexity = 0.25f;
        meta.latencySamples = 0;
        
        meta.parameters.push_back({
            "Delay Time",
            0.375f,  // 375ms - dotted eighth at 120 BPM
            0.0f,
            1.0f,
            "ms",
            "logarithmic",
            {
                {"0-5", "comb filter, metallic"},
                {"5-15", "doubling, thickening"},
                {"15-50", "slapback echo, rockabilly"},
                {"50-150", "short echo, rhythmic"},
                {"150-400", "medium delay, musical"},
                {"400-800", "long delay, ambient"},
                {"800-1000", "very long, ethereal"}
            }
        });
        
        meta.parameters.push_back({
            "Feedback",
            0.35f,  // Some repeats but stable
            0.0f,
            1.0f,
            "%",
            "linear",
            {
                {"0-20", "single repeat, no feedback"},
                {"20-40", "few repeats, decaying"},
                {"40-60", "multiple repeats, musical"},
                {"60-75", "many repeats, building"},
                {"75-85", "near oscillation, dub"},
                {"85-95", "self-oscillation, careful!"},
                {"95-100", "runaway feedback, chaos"}
            }
        });
        
        meta.parameters.push_back({
            "Wow & Flutter",
            0.25f,  // Vintage character without nausea
            0.0f,
            1.0f,
            "%",
            "linear",
            {
                {"0-10", "pristine, no modulation"},
                {"10-30", "subtle vintage character"},
                {"30-50", "noticeable tape wobble"},
                {"50-70", "heavy tape degradation"},
                {"70-90", "extreme warping"},
                {"90-100", "broken tape machine"}
            }
        });
        
        meta.parameters.push_back({
            "Saturation",
            0.3f,  // Warm but not distorted
            0.0f,
            1.0f,
            "%",
            "linear",
            {
                {"0-20", "clean repeats"},
                {"20-40", "warm tape compression"},
                {"40-60", "noticeable saturation"},
                {"60-80", "heavy tape distortion"},
                {"80-100", "overdriven tape"}
            }
        });
        
        meta.parameters.push_back({
            "Mix",
            0.35f,  // Audible but not overpowering
            0.0f,
            1.0f,
            "%",
            "linear",
            {
                {"0-30", "subtle ambience"},
                {"30-50", "balanced delay"},
                {"50-70", "prominent effect"},
                {"70-100", "delay-dominated"}
            }
        });
        
        meta.triggerWords = {
            "tape", "echo", "delay", "vintage", "analog", "wow", "flutter",
            "space", "dub", "feedback", "oscillation", "rockabilly", "slapback",
            "psychedelic", "60s", "70s", "retro", "degraded", "lo-fi"
        };
        
        meta.compatibilityScores = {
            {ENGINE_PLATE_REVERB, 0.95f},         // Classic studio pairing
            {ENGINE_SPRING_REVERB, 0.9f},          // Surf rock heaven
            {ENGINE_K_STYLE, 0.85f},               // Drive into delay
            {ENGINE_VINTAGE_TUBE_PREAMP, 0.9f},    // Warm vintage chain
            {ENGINE_SPECTRAL_FREEZE, 0.4f},        // Conflicting time effects
            {ENGINE_DIGITAL_DELAY, 0.3f}           // Redundant delays
        };
        
        meta.moodAdjustments = {
            {"spacier", 0.2f},       // More feedback
            {"cleaner", -0.15f},     // Less saturation
            {"weirder", 0.3f},       // More wow/flutter
            {"tighter", -0.2f},      // Less delay time
            {"dubby", 0.25f},        // More feedback
            {"vintage", 0.15f}       // More wow/flutter
        };
        
        registry.registerEngine(meta);
    }
    
    // ENGINE 2: Plate Reverb
    {
        EngineMetadata meta;
        meta.engineId = ENGINE_PLATE_REVERB;
        meta.name = "Plate Reverb";
        meta.category = "reverb";
        meta.description = "EMT 140 style plate reverb with dense, smooth reflections";
        
        meta.sonicTags = {
            "smooth", "dense", "metallic", "bright", "studio-classic", "lush",
            "shimmering", "professional", "vintage-studio", "even-decay",
            "no-early-reflections", "instant-density", "frequency-balanced"
        };
        
        meta.emotionalTags = {
            "polished", "expensive", "professional", "dreamy", "ethereal",
            "floating", "angelic", "sophisticated", "classy", "timeless"
        };
        
        meta.frequencyFocus = "high-mid";
        
        meta.typicalUseCases = {
            "vocal_sweetening", "drum_ambience", "mix_glue", "string_sections",
            "piano_space", "lead_instruments", "professional_mixing", "studio_sheen"
        };
        
        meta.instrumentTags = {
            "vocals", "drums", "strings", "piano", "brass", "acoustic_guitar"
        };
        
        meta.cpuComplexity = 0.45f;
        meta.latencySamples = 0;
        
        meta.parameters.push_back({
            "Size",
            0.5f,  // Medium plate
            0.0f,
            1.0f,
            "",
            "linear",
            {
                {"0-20", "tiny plate, metallic"},
                {"20-40", "small studio plate"},
                {"40-60", "medium plate, balanced"},
                {"60-80", "large plate, spacious"},
                {"80-100", "huge plate, cavernous"}
            }
        });
        
        meta.parameters.push_back({
            "Decay",
            0.4f,  // 2-3 seconds, musical
            0.0f,
            1.0f,
            "seconds",
            "logarithmic",
            {
                {"0-20", "very short, 0.5-1s"},
                {"20-40", "short, 1-2s"},
                {"40-60", "medium, 2-4s"},
                {"60-80", "long, 4-8s"},
                {"80-100", "very long, 8-20s"}
            }
        });
        
        meta.parameters.push_back({
            "Damping",
            0.3f,  // Some high frequency absorption
            0.0f,
            1.0f,
            "%",
            "linear",
            {
                {"0-20", "bright, metallic"},
                {"20-40", "balanced brightness"},
                {"40-60", "natural damping"},
                {"60-80", "warm, vintage"},
                {"80-100", "dark, muffled"}
            }
        });
        
        meta.parameters.push_back({
            "Pre-Delay",
            0.1f,  // 10ms for clarity
            0.0f,
            1.0f,
            "ms",
            "linear",
            {
                {"0-10", "instant, no separation"},
                {"10-30", "subtle separation"},
                {"30-60", "clear separation"},
                {"60-100", "distant placement"}
            }
        });
        
        meta.parameters.push_back({
            "Mix",
            0.25f,  // Subtle but present
            0.0f,
            1.0f,
            "%",
            "linear",
            {
                {"0-20", "subtle ambience"},
                {"20-40", "noticeable space"},
                {"40-60", "balanced reverb"},
                {"60-80", "reverb-forward"},
                {"80-100", "washed out"}
            }
        });
        
        meta.triggerWords = {
            "plate", "reverb", "space", "room", "ambience", "studio", "emt",
            "vintage", "smooth", "lush", "professional", "polish", "sheen",
            "density", "tail"
        };
        
        meta.compatibilityScores = {
            {ENGINE_CLASSIC_COMPRESSOR, 0.9f},     // Compress before reverb
            {ENGINE_PARAMETRIC_EQ, 0.95f},         // Shape the reverb
            {ENGINE_TAPE_ECHO, 0.9f},              // Delay into reverb
            {ENGINE_SPRING_REVERB, 0.2f},          // Don't stack reverbs
            {ENGINE_CONVOLUTION_REVERB, 0.1f}      // Redundant reverbs
        };
        
        meta.moodAdjustments = {
            {"bigger", 0.2f},        // Increase size
            {"smaller", -0.2f},      // Decrease size
            {"brighter", -0.15f},    // Less damping
            {"darker", 0.2f},        // More damping
            {"wetter", 0.15f},       // More mix
            {"tighter", -0.1f}       // Less decay
        };
        
        registry.registerEngine(meta);
    }
    
    // ENGINE 3: Rodent Distortion (RAT-style)
    {
        EngineMetadata meta;
        meta.engineId = ENGINE_RODENT_DISTORTION;
        meta.name = "Rodent Distortion";
        meta.category = "distortion";
        meta.description = "Aggressive op-amp distortion with signature filter curve";
        
        meta.sonicTags = {
            "aggressive", "fuzzy", "compressed", "mid-scooped", "tight-bass",
            "cutting", "harsh", "industrial", "metallic", "gnarly",
            "saturated", "clipping", "raw", "abrasive"
        };
        
        meta.emotionalTags = {
            "angry", "aggressive", "rebellious", "intense", "fierce",
            "brutal", "uncompromising", "edgy", "dangerous", "wild"
        };
        
        meta.frequencyFocus = "high-mid";
        
        meta.typicalUseCases = {
            "metal_guitar", "punk_guitar", "aggressive_bass", "industrial_synth",
            "drum_destruction", "lo-fi_production", "noise_music"
        };
        
        meta.instrumentTags = {
            "electric_guitar", "bass_guitar", "synthesizer", "drum_machines"
        };
        
        meta.cpuComplexity = 0.2f;
        meta.latencySamples = 0;
        
        // Detailed parameters...
        meta.parameters.push_back({
            "Distortion",
            0.4f,  // Moderate distortion
            0.0f,
            1.0f,
            "%",
            "logarithmic",
            {
                {"0-20", "light clipping"},
                {"20-40", "moderate distortion"},
                {"40-60", "heavy distortion"},
                {"60-80", "extreme saturation"},
                {"80-100", "total annihilation"}
            }
        });
        
        meta.triggerWords = {
            "rat", "rodent", "distortion", "fuzz", "aggressive", "metal",
            "punk", "harsh", "industrial", "gnarly", "fierce"
        };
        
        registry.registerEngine(meta);
    }
    
    // Continue with remaining 46 engines...
    // Each should have this level of detail and thought
    
    // The key is that EVERY parameter range description helps the AI understand
    // not just WHAT the parameter does, but HOW it affects the sound emotionally
    // and musically at different settings.
}