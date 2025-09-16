#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

void testMusicalInput(EngineBase* reverb, const std::string& name) {
    std::cout << "\n" << name << ":" << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    reverb->prepareToPlay(44100, 512);
    reverb->reset();
    
    // Set realistic musical parameters
    std::map<int, float> params;
    for (int i = 0; i < reverb->getNumParameters(); ++i) {
        juce::String paramName = reverb->getParameterName(i).toLowerCase();
        if (paramName.contains("mix")) {
            params[i] = 0.3f;  // 30% wet - typical musical setting
        } else if (paramName.contains("size") || paramName.contains("room")) {
            params[i] = 0.6f;  // Medium room
        } else if (paramName.contains("time")) {
            params[i] = 0.5f;  // Medium decay
        } else if (paramName.contains("damp")) {
            params[i] = 0.4f;  // Moderate damping
        } else {
            params[i] = 0.5f;  // Default middle
        }
    }
    reverb->updateParameters(params);
    
    juce::AudioBuffer<float> buffer(2, 512);
    
    // Test 1: Musical chord (C major triad)
    std::cout << "  Test 1: C Major Chord" << std::endl;
    float frequencies[] = {261.63f, 329.63f, 392.0f}; // C4, E4, G4
    
    // Play chord for 1 second
    int chordBlocks = 86; // ~1 second
    float maxDuringChord = 0.0f;
    
    for (int block = 0; block < chordBlocks; ++block) {
        for (int i = 0; i < 512; ++i) {
            float sample = 0.0f;
            for (float freq : frequencies) {
                sample += 0.15f * std::sin(2.0f * M_PI * freq * (block * 512 + i) / 44100.0f);
            }
            // Add envelope
            float envelope = 1.0f;
            if (block < 5) envelope = block / 5.0f; // Fade in
            if (block > 80) envelope = (86 - block) / 6.0f; // Fade out
            sample *= envelope;
            
            buffer.setSample(0, i, sample);
            buffer.setSample(1, i, sample);
        }
        reverb->process(buffer);
        
        for (int i = 0; i < 512; ++i) {
            maxDuringChord = std::max(maxDuringChord, std::abs(buffer.getSample(0, i)));
        }
    }
    
    std::cout << "    Max during chord: " << std::fixed << std::setprecision(3) << maxDuringChord;
    if (maxDuringChord > 1.0f) {
        std::cout << " ✗ (clipping)";
    } else if (maxDuringChord < 0.1f) {
        std::cout << " ⚠ (too quiet)";
    } else {
        std::cout << " ✓";
    }
    std::cout << std::endl;
    
    // Measure tail decay
    std::cout << "    Tail decay: ";
    std::vector<float> tailLevels;
    for (int block = 0; block < 100; ++block) { // ~1.2 seconds of tail
        buffer.clear();
        reverb->process(buffer);
        
        float blockMax = 0.0f;
        for (int i = 0; i < 512; ++i) {
            blockMax = std::max(blockMax, std::abs(buffer.getSample(0, i)));
        }
        tailLevels.push_back(blockMax);
        
        if (blockMax < maxDuringChord * 0.001f) { // -60dB point
            float tailTime = block * 512.0f / 44100.0f;
            std::cout << tailTime << "s ✓" << std::endl;
            break;
        }
    }
    
    if (tailLevels.back() >= maxDuringChord * 0.001f) {
        std::cout << ">1.2s ";
        if (tailLevels.back() < tailLevels[0] * 0.1f) {
            std::cout << "✓ (decaying)" << std::endl;
        } else {
            std::cout << "⚠ (slow decay)" << std::endl;
        }
    }
    
    // Test 2: Percussive hit (snare-like)
    std::cout << "  Test 2: Percussive Hit" << std::endl;
    reverb->reset();
    
    // Create snare-like burst
    for (int i = 0; i < 64; ++i) { // Short burst
        float noise = (rand() / (float)RAND_MAX * 2.0f - 1.0f) * 0.5f;
        float envelope = std::exp(-i * 0.1f);
        float sample = noise * envelope;
        buffer.setSample(0, i, sample);
        buffer.setSample(1, i, sample);
    }
    for (int i = 64; i < 512; ++i) {
        buffer.setSample(0, i, 0.0f);
        buffer.setSample(1, i, 0.0f);
    }
    
    reverb->process(buffer);
    
    float percussiveResponse = 0.0f;
    for (int i = 0; i < 512; ++i) {
        percussiveResponse = std::max(percussiveResponse, std::abs(buffer.getSample(0, i)));
    }
    
    std::cout << "    Initial response: " << percussiveResponse;
    if (percussiveResponse > 1.0f) {
        std::cout << " ✗ (clipping)";
    } else if (percussiveResponse < 0.05f) {
        std::cout << " ⚠ (too quiet)";
    } else {
        std::cout << " ✓";
    }
    std::cout << std::endl;
    
    // Check tail character
    buffer.clear();
    reverb->process(buffer);
    float tailCharacter = 0.0f;
    for (int i = 0; i < 512; ++i) {
        tailCharacter += std::abs(buffer.getSample(0, i));
    }
    tailCharacter /= 512.0f;
    
    std::cout << "    Tail density: " << std::scientific << tailCharacter;
    if (tailCharacter > 1e-6f) {
        std::cout << " ✓ (present)" << std::endl;
    } else {
        std::cout << " ⚠ (absent)" << std::endl;
    }
}

int main() {
    std::cout << "\n============================================" << std::endl;
    std::cout << "    MUSICAL REVERB VALIDATION" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "\nTesting with realistic musical input..." << std::endl;
    
    PlateReverb plate;
    testMusicalInput(&plate, "PlateReverb");
    
    ShimmerReverb shimmer;
    testMusicalInput(&shimmer, "ShimmerReverb");
    
    SpringReverb spring;
    testMusicalInput(&spring, "SpringReverb");
    
    GatedReverb gated;
    testMusicalInput(&gated, "GatedReverb");
    
    ConvolutionReverb conv;
    testMusicalInput(&conv, "ConvolutionReverb");
    
    std::cout << "\n============================================" << std::endl;
    std::cout << "All reverbs tested with:" << std::endl;
    std::cout << "  • C major chord (musical content)" << std::endl;
    std::cout << "  • Percussive hit (transient response)" << std::endl;
    std::cout << "  • 30% wet mix (typical setting)" << std::endl;
    std::cout << "============================================" << std::endl;
    
    return 0;
}