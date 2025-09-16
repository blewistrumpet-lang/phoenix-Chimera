#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>
#include <memory>
#include "JUCE_Plugin/Source/BitCrusher.h"
#include "JUCE_Plugin/Source/KStyleOverdrive.h"
#include "JUCE_Plugin/Source/HarmonicExciter.h"
#include "JUCE_Plugin/Source/RodentDistortion.h"
#include "JUCE_Plugin/Source/MultibandSaturator.h"
#include "JUCE_Plugin/Source/MuffFuzz.h"

void printTestHeader(const std::string& name) {
    std::cout << "\n╔════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║ " << std::left << std::setw(50) << name << " ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════╝" << std::endl;
}

void testDistortionEngine(const std::string& name, std::unique_ptr<EngineBase> engine) {
    printTestHeader(name);
    
    // Prepare engine
    engine->prepareToPlay(44100.0, 512);
    
    // Create test signal
    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch) {
        float* data = buffer.getWritePointer(ch);
        for (int i = 0; i < 512; ++i) {
            data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }
    }
    
    // Measure input
    float inputPeak = buffer.getMagnitude(0, 512);
    float inputRMS = buffer.getRMSLevel(0, 0, 512);
    
    // Process
    engine->process(buffer);
    
    // Measure output
    float outputPeak = buffer.getMagnitude(0, 512);
    float outputRMS = buffer.getRMSLevel(0, 0, 512);
    
    // Check for audio output
    bool hasAudio = outputPeak > 0.001f;
    bool hasDistortion = outputRMS != inputRMS;
    
    // Check for issues
    bool hasNaN = false;
    bool hasClipping = false;
    float maxSample = 0.0f;
    
    for (int ch = 0; ch < 2; ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < 512; ++i) {
            if (!std::isfinite(data[i])) {
                hasNaN = true;
            }
            float absSample = std::abs(data[i]);
            maxSample = std::max(maxSample, absSample);
            if (absSample > 0.99f) {
                hasClipping = true;
            }
        }
    }
    
    // Print results
    std::cout << "│ Input Peak:     " << std::fixed << std::setprecision(4) << inputPeak << std::endl;
    std::cout << "│ Output Peak:    " << outputPeak;
    if (hasAudio) {
        std::cout << " ✓ HAS AUDIO";
    } else {
        std::cout << " ✗ NO AUDIO";
    }
    std::cout << std::endl;
    
    std::cout << "│ Input RMS:      " << inputRMS << std::endl;
    std::cout << "│ Output RMS:     " << outputRMS;
    if (hasDistortion) {
        std::cout << " ✓ PROCESSING";
    } else {
        std::cout << " ✗ NO CHANGE";
    }
    std::cout << std::endl;
    
    std::cout << "│ Max Sample:     " << maxSample << std::endl;
    std::cout << "│ Status:         ";
    
    if (hasNaN) {
        std::cout << "✗ HAS NaN/Inf";
    } else if (!hasAudio) {
        std::cout << "✗ NO OUTPUT";
    } else if (hasClipping) {
        std::cout << "⚠ CLIPPING";
    } else {
        std::cout << "✓ WORKING";
    }
    std::cout << std::endl;
    
    // Test parameter changes
    std::cout << "│" << std::endl;
    std::cout << "│ Testing Parameters:" << std::endl;
    
    // Test with max drive/gain
    std::map<int, float> params;
    params[0] = 1.0f; // Usually drive/gain
    engine->updateParameters(params);
    
    // Create new test signal
    juce::AudioBuffer<float> driveBuffer(2, 256);
    for (int ch = 0; ch < 2; ++ch) {
        float* data = driveBuffer.getWritePointer(ch);
        for (int i = 0; i < 256; ++i) {
            data[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
        }
    }
    
    float driveInputPeak = driveBuffer.getMagnitude(0, 256);
    engine->process(driveBuffer);
    float driveOutputPeak = driveBuffer.getMagnitude(0, 256);
    
    std::cout << "│ Max Drive Test: " << driveInputPeak << " → " << driveOutputPeak;
    if (driveOutputPeak > driveInputPeak * 0.5f) {
        std::cout << " ✓";
    } else {
        std::cout << " ✗";
    }
    std::cout << std::endl;
}

void testRodentModes() {
    std::cout << "\n┌────────────────────────────────────────────────────┐" << std::endl;
    std::cout << "│ RODENT DISTORTION MODE TEST                       │" << std::endl;
    std::cout << "└────────────────────────────────────────────────────┘" << std::endl;
    
    auto rodent = std::make_unique<RodentDistortion>();
    rodent->prepareToPlay(44100.0, 256);
    
    const char* modes[] = {"RAT", "Tube Screamer", "Big Muff", "Fuzz Face"};
    
    for (int mode = 0; mode < 4; ++mode) {
        std::map<int, float> params;
        params[6] = mode * 0.25f; // Distortion type parameter
        rodent->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 256);
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < 256; ++i) {
                data[i] = 0.3f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
            }
        }
        
        try {
            float inputPeak = buffer.getMagnitude(0, 256);
            rodent->process(buffer);
            float outputPeak = buffer.getMagnitude(0, 256);
            
            std::cout << "│ Mode " << mode << " (" << modes[mode] << "): ";
            std::cout << inputPeak << " → " << outputPeak;
            std::cout << " ✓ NO CRASH" << std::endl;
        } catch (...) {
            std::cout << "│ Mode " << mode << " (" << modes[mode] << "): ✗ CRASHED!" << std::endl;
        }
    }
}

void testMultibandDrives() {
    std::cout << "\n┌────────────────────────────────────────────────────┐" << std::endl;
    std::cout << "│ MULTIBAND SATURATOR BAND TEST                     │" << std::endl;
    std::cout << "└────────────────────────────────────────────────────┘" << std::endl;
    
    auto multiband = std::make_unique<MultibandSaturator>();
    multiband->prepareToPlay(44100.0, 256);
    
    const char* bands[] = {"Low (100Hz)", "Mid (1kHz)", "High (5kHz)"};
    float frequencies[] = {100.0f, 1000.0f, 5000.0f};
    
    for (int band = 0; band < 3; ++band) {
        std::map<int, float> params;
        params[0] = (band == 0) ? 1.0f : 0.0f; // Low drive
        params[1] = (band == 1) ? 1.0f : 0.0f; // Mid drive
        params[2] = (band == 2) ? 1.0f : 0.0f; // High drive
        multiband->updateParameters(params);
        
        juce::AudioBuffer<float> buffer(2, 256);
        for (int ch = 0; ch < 2; ++ch) {
            float* data = buffer.getWritePointer(ch);
            for (int i = 0; i < 256; ++i) {
                data[i] = 0.3f * std::sin(2.0f * M_PI * frequencies[band] * i / 44100.0f);
            }
        }
        
        float inputPeak = buffer.getMagnitude(0, 256);
        multiband->process(buffer);
        float outputPeak = buffer.getMagnitude(0, 256);
        
        std::cout << "│ " << bands[band] << " Band: ";
        std::cout << inputPeak << " → " << outputPeak;
        
        float ratio = outputPeak / inputPeak;
        if (ratio > 0.8f) {
            std::cout << " ✓ WORKING (ratio: " << ratio << ")" << std::endl;
        } else {
            std::cout << " ✗ WEAK (ratio: " << ratio << ")" << std::endl;
        }
    }
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║        DISTORTION ENGINE VERIFICATION PROOF        ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════╝" << std::endl;
    
    // Test all distortion engines
    testDistortionEngine("BitCrusher", std::make_unique<BitCrusher>());
    testDistortionEngine("K-Style Overdrive", std::make_unique<KStyleOverdrive>());
    testDistortionEngine("Harmonic Exciter", std::make_unique<HarmonicExciter>());
    testDistortionEngine("Rodent Distortion", std::make_unique<RodentDistortion>());
    testDistortionEngine("Multiband Saturator", std::make_unique<MultibandSaturator>());
    testDistortionEngine("Muff Fuzz", std::make_unique<MuffFuzz>());
    
    // Special tests
    testRodentModes();
    testMultibandDrives();
    
    std::cout << "\n╔════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                    TEST COMPLETE                   ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════╝" << std::endl;
    
    return 0;
}