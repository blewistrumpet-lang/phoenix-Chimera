#include <iostream>
#include <vector>
#include <iomanip>
#include "JUCE_Plugin/JuceLibraryCode/JuceHeader.h"
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

int findMixParam(EngineBase* engine) {
    if (!engine) return -1;
    
    int numParams = engine->getNumParameters();
    
    // First pass: exact "mix" match
    for (int i = 0; i < numParams; ++i) {
        juce::String paramName = engine->getParameterName(i).toLowerCase();
        if (paramName == "mix") {
            return i;
        }
    }
    
    // Second pass: contains "mix"
    for (int i = 0; i < numParams; ++i) {
        juce::String paramName = engine->getParameterName(i).toLowerCase();
        if (paramName.contains("mix")) {
            return i;
        }
    }
    
    // Third pass: wet/dry/blend/amount
    for (int i = 0; i < numParams; ++i) {
        juce::String paramName = engine->getParameterName(i).toLowerCase();
        if (paramName.contains("wet") || 
            paramName.contains("dry") || 
            paramName.contains("blend") ||
            paramName.contains("amount")) {
            return i;
        }
    }
    
    return -1;
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════╗\n";
    std::cout << "║    FINDING CORRECT MIX PARAMETER INDICES       ║\n";
    std::cout << "╚════════════════════════════════════════════════╝\n\n";
    
    std::cout << "// Corrected getMixParameterIndex function:\n";
    std::cout << "int getMixParameterIndex(int engineId) {\n";
    std::cout << "    switch (engineId) {\n";
    
    for (int engineId = 1; engineId <= 56; ++engineId) {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            std::cout << "        case " << engineId << ": return -1; // Failed to create engine\n";
            continue;
        }
        
        std::string engineName = engine->getName().toStdString();
        int mixIndex = findMixParam(engine.get());
        
        if (mixIndex >= 0) {
            std::string paramName = engine->getParameterName(mixIndex).toStdString();
            std::cout << "        case " << engineId << ": return " << mixIndex 
                      << "; // " << engineName << " - \"" << paramName << "\"\n";
        } else {
            std::cout << "        case " << engineId << ": return -1; // " 
                      << engineName << " - No mix parameter\n";
        }
    }
    
    std::cout << "        default: return -1;\n";
    std::cout << "    }\n";
    std::cout << "}\n\n";
    
    // Also generate the corrected constant definitions
    std::cout << "// Engine ID constants (for reference):\n";
    std::cout << "#define ENGINE_VCA_COMPRESSOR 1\n";
    std::cout << "#define ENGINE_OPTO_COMPRESSOR 2\n";
    std::cout << "#define ENGINE_TRANSIENT_SHAPER 3\n";
    std::cout << "#define ENGINE_NOISE_GATE 4\n";
    std::cout << "#define ENGINE_MASTERING_LIMITER 5\n";
    std::cout << "#define ENGINE_DYNAMIC_EQ 6\n";
    std::cout << "#define ENGINE_PARAMETRIC_EQ 7\n";
    std::cout << "#define ENGINE_VINTAGE_CONSOLE_EQ 8\n";
    std::cout << "#define ENGINE_LADDER_FILTER 9\n";
    std::cout << "#define ENGINE_STATE_VARIABLE_FILTER 10\n";
    std::cout << "#define ENGINE_FORMANT_FILTER 11\n";
    std::cout << "#define ENGINE_ENVELOPE_FILTER 12\n";
    std::cout << "#define ENGINE_COMB_RESONATOR 13\n";
    std::cout << "#define ENGINE_VOCAL_FORMANT 14\n";
    std::cout << "#define ENGINE_VINTAGE_TUBE 15\n";
    std::cout << "#define ENGINE_WAVE_FOLDER 16\n";
    std::cout << "#define ENGINE_HARMONIC_EXCITER 17\n";
    std::cout << "#define ENGINE_BIT_CRUSHER 18\n";
    std::cout << "#define ENGINE_MULTIBAND_SATURATOR 19\n";
    std::cout << "#define ENGINE_MUFF_FUZZ 20\n";
    std::cout << "#define ENGINE_RODENT_DISTORTION 21\n";
    std::cout << "#define ENGINE_K_STYLE 22\n";
    std::cout << "#define ENGINE_DIGITAL_CHORUS 23\n";
    std::cout << "#define ENGINE_RESONANT_CHORUS 24\n";
    std::cout << "#define ENGINE_ANALOG_PHASER 25\n";
    std::cout << "#define ENGINE_RING_MODULATOR 26\n";
    std::cout << "#define ENGINE_FREQUENCY_SHIFTER 27\n";
    std::cout << "#define ENGINE_HARMONIC_TREMOLO 28\n";
    std::cout << "#define ENGINE_CLASSIC_TREMOLO 29\n";
    std::cout << "#define ENGINE_ROTARY_SPEAKER 30\n";
    std::cout << "#define ENGINE_PITCH_SHIFTER 31\n";
    std::cout << "#define ENGINE_DETUNE_DOUBLER 32\n";
    std::cout << "#define ENGINE_INTELLIGENT_HARMONIZER 33\n";
    std::cout << "#define ENGINE_TAPE_ECHO 34\n";
    std::cout << "#define ENGINE_DIGITAL_DELAY 35\n";
    std::cout << "#define ENGINE_MAGNETIC_DRUM_ECHO 36\n";
    std::cout << "#define ENGINE_BUCKET_BRIGADE_DELAY 37\n";
    std::cout << "#define ENGINE_BUFFER_REPEAT 38\n";
    std::cout << "#define ENGINE_PLATE_REVERB 39\n";
    std::cout << "#define ENGINE_SPRING_REVERB 40\n";
    std::cout << "#define ENGINE_CONVOLUTION_REVERB 41\n";
    std::cout << "#define ENGINE_SHIMMER_REVERB 42\n";
    std::cout << "#define ENGINE_GATED_REVERB 43\n";
    std::cout << "#define ENGINE_STEREO_WIDENER 44\n";
    std::cout << "#define ENGINE_STEREO_IMAGER 45\n";
    std::cout << "#define ENGINE_DIMENSION_EXPANDER 46\n";
    std::cout << "#define ENGINE_SPECTRAL_FREEZE 47\n";
    std::cout << "#define ENGINE_SPECTRAL_GATE 48\n";
    std::cout << "#define ENGINE_PHASED_VOCODER 49\n";
    std::cout << "#define ENGINE_GRANULAR_CLOUD 50\n";
    std::cout << "#define ENGINE_CHAOS_GENERATOR 51\n";
    std::cout << "#define ENGINE_FEEDBACK_NETWORK 52\n";
    std::cout << "#define ENGINE_MID_SIDE_PROCESSOR 53\n";
    std::cout << "#define ENGINE_GAIN_UTILITY 54\n";
    std::cout << "#define ENGINE_MONO_MAKER 55\n";
    std::cout << "#define ENGINE_PHASE_ALIGN 56\n";
    
    return 0;
}