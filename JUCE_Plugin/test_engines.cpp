#include <iostream>
#include <memory>
#include "Source/ParameterDefinitions.h"
#include "Source/EngineFactory.h"

int main() {
    std::cout << "Testing ChimeraPhoenix Engines...\n\n";
    
    // Test all engine IDs
    int engineIds[] = {
        ENGINE_BYPASS, ENGINE_K_STYLE, ENGINE_TAPE_ECHO, ENGINE_PLATE_REVERB,
        ENGINE_RODENT_DISTORTION, ENGINE_MUFF_FUZZ, ENGINE_CLASSIC_TREMOLO,
        ENGINE_MAGNETIC_DRUM_ECHO, ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_DIGITAL_DELAY,
        ENGINE_HARMONIC_TREMOLO, ENGINE_ROTARY_SPEAKER, ENGINE_DETUNE_DOUBLER,
        ENGINE_LADDER_FILTER, ENGINE_FORMANT_FILTER, ENGINE_CLASSIC_COMPRESSOR,
        ENGINE_STATE_VARIABLE_FILTER, ENGINE_STEREO_CHORUS, ENGINE_SPECTRAL_FREEZE,
        ENGINE_GRANULAR_CLOUD, ENGINE_ANALOG_RING_MODULATOR, ENGINE_MULTIBAND_SATURATOR,
        ENGINE_COMB_RESONATOR, ENGINE_PITCH_SHIFTER, ENGINE_PHASED_VOCODER,
        ENGINE_CONVOLUTION_REVERB, ENGINE_BIT_CRUSHER, ENGINE_FREQUENCY_SHIFTER,
        ENGINE_WAVE_FOLDER, ENGINE_SHIMMER_REVERB, ENGINE_VOCAL_FORMANT_FILTER,
        ENGINE_TRANSIENT_SHAPER, ENGINE_DIMENSION_EXPANDER
    };
    
    const char* engineNames[] = {
        "Bypass", "K-Style Overdrive", "Tape Echo", "Plate Reverb",
        "Rodent Distortion", "Muff Fuzz", "Classic Tremolo",
        "Magnetic Drum Echo", "Bucket Brigade Delay", "Digital Delay",
        "Harmonic Tremolo", "Rotary Speaker", "Detune Doubler",
        "Ladder Filter", "Formant Filter", "Classic Compressor",
        "State Variable Filter", "Stereo Chorus", "Spectral Freeze",
        "Granular Cloud", "Analog Ring Modulator", "Multiband Saturator",
        "Comb Resonator", "Pitch Shifter", "Phased Vocoder",
        "Convolution Reverb", "Bit Crusher", "Frequency Shifter",
        "Wave Folder", "Shimmer Reverb", "Vocal Formant Filter",
        "Transient Shaper", "Dimension Expander"
    };
    
    int totalEngines = sizeof(engineIds) / sizeof(engineIds[0]);
    int successCount = 0;
    
    for (int i = 0; i < totalEngines; i++) {
        std::cout << "Engine " << (i+1) << "/" << totalEngines << " - " << engineNames[i] << " (ID: " << engineIds[i] << "): ";
        
        try {
            auto engine = EngineFactory::createEngine(engineIds[i]);
            if (engine) {
                std::cout << "✓ Created successfully - " << engine->getName().toStdString() << std::endl;
                successCount++;
            } else {
                std::cout << "✗ Failed to create" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "✗ Exception: " << e.what() << std::endl;
        }
    }
    
    std::cout << "\nSummary: " << successCount << "/" << totalEngines << " engines created successfully." << std::endl;
    
    return 0;
}