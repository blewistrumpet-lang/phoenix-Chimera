#!/bin/bash

echo "Testing all Modulation engines..."

# List of modulation engines
engines=(
    "AnalogPhaser"
    "ClassicTremolo"
    "HarmonicTremolo"
    "ResonantChorus"
    "RotarySpeaker"
    "StereoChorus"
)

# Create test files for each engine
for engine in "${engines[@]}"; do
    if [ "$engine" != "AnalogPhaser" ]; then  # Skip AnalogPhaser as we already have it
        echo "Creating test for $engine..."
        
        cat > "test_${engine}.cpp" << EOF
#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <complex>
#include <numeric>
#include <algorithm>
#include <chrono>
#include <random>
#include "JUCE_Plugin/Source/${engine}.h"

// Analysis functions
float calculateTHD(const std::vector<std::complex<float>>& spectrum, int fundamentalBin) {
    float fundamental = std::abs(spectrum[fundamentalBin]);
    float harmonics = 0;
    for (int h = 2; h <= 5; ++h) {
        int bin = fundamentalBin * h;
        if (bin < spectrum.size() / 2) {
            harmonics += std::pow(std::abs(spectrum[bin]), 2);
        }
    }
    return sqrt(harmonics) / (fundamental + 1e-10f);
}

void computeFFT(const std::vector<float>& signal, std::vector<std::complex<float>>& spectrum) {
    int N = signal.size();
    spectrum.resize(N);
    for (int k = 0; k < N; ++k) {
        std::complex<float> sum(0, 0);
        for (int n = 0; n < N; ++n) {
            float angle = -2.0f * M_PI * k * n / N;
            sum += signal[n] * std::complex<float>(cos(angle), sin(angle));
        }
        spectrum[k] = sum;
    }
}

// Test signal generator
std::vector<float> generateTestTone(float sampleRate, int samples) {
    std::vector<float> signal(samples);
    for (int i = 0; i < samples; ++i) {
        signal[i] = 0.7f * sin(2.0f * M_PI * 440.0f * i / sampleRate);
    }
    return signal;
}

int main() {
    std::cout << "================================================================================\\n";
    std::cout << "                   ENGINE ANALYSIS REPORT                                  \\n";
    std::cout << "================================================================================\\n\\n";
    std::cout << "Engine: ${engine}\\n";
    std::cout << "Analysis Date: " << __DATE__ << "\\n";
    std::cout << "Framework: Comprehensive Engine Analyzer v1.0\\n\\n";
    
    ${engine} effect;
    float sampleRate = 48000.0f;
    int blockSize = 512;
    
    effect.prepareToPlay(sampleRate, blockSize);
    
    std::cout << "PARAMETER TEST:\\n";
    std::cout << "==============\\n\\n";
    
    // Test all parameters
    for (int param = 0; param < 8; ++param) {
        juce::String paramName = effect.getParameterName(param);
        if (paramName.isEmpty()) continue;
        
        std::cout << paramName.toStdString() << " (Index " << param << "): ";
        
        std::map<int, float> params;
        params[param] = 0.5f;
        effect.updateParameters(params);
        
        auto testSignal = generateTestTone(sampleRate, blockSize);
        juce::AudioBuffer<float> buffer(1, blockSize);
        buffer.copyFrom(0, 0, testSignal.data(), blockSize);
        
        effect.process(buffer);
        
        float rms = 0;
        for (int i = 0; i < blockSize; ++i) {
            float sample = buffer.getSample(0, i);
            rms += sample * sample;
        }
        rms = sqrt(rms / blockSize);
        
        std::cout << "RMS=" << std::fixed << std::setprecision(3) << rms << "\\n";
    }
    
    // Test modulation depth
    std::cout << "\\nMODULATION TEST:\\n";
    std::cout << "================\\n";
    
    std::map<int, float> params;
    params[0] = 0.5f;  // Rate
    params[1] = 0.7f;  // Depth  
    params[7] = 1.0f;  // Mix
    effect.updateParameters(params);
    
    // Process multiple blocks
    std::vector<float> modProfile;
    auto testSignal = generateTestTone(sampleRate, blockSize);
    
    for (int block = 0; block < 10; ++block) {
        juce::AudioBuffer<float> buffer(1, blockSize);
        buffer.copyFrom(0, 0, testSignal.data(), blockSize);
        effect.process(buffer);
        
        float avgMag = 0;
        for (int i = 0; i < blockSize; ++i) {
            avgMag += std::abs(buffer.getSample(0, i));
        }
        avgMag /= blockSize;
        modProfile.push_back(avgMag);
    }
    
    float minMag = *std::min_element(modProfile.begin(), modProfile.end());
    float maxMag = *std::max_element(modProfile.begin(), modProfile.end());
    float modDepth = (maxMag - minMag) / (maxMag + minMag) * 100;
    
    std::cout << "Modulation depth: " << modDepth << "%\\n";
    
    // THD test
    juce::AudioBuffer<float> buffer(1, blockSize);
    buffer.copyFrom(0, 0, testSignal.data(), blockSize);
    effect.process(buffer);
    
    std::vector<float> output(blockSize);
    for (int i = 0; i < blockSize; ++i) {
        output[i] = buffer.getSample(0, i);
    }
    
    std::vector<std::complex<float>> spectrum;
    computeFFT(output, spectrum);
    int fundamentalBin = static_cast<int>(440.0f * blockSize / sampleRate);
    float thd = calculateTHD(spectrum, fundamentalBin) * 100;
    
    std::cout << "THD: " << thd << "%\\n";
    
    // Score
    float qualityScore = 100 - thd * 5 - (modDepth < 5 ? 30 : 0);
    qualityScore = std::max(0.0f, std::min(100.0f, qualityScore));
    
    std::cout << "\\nQuality Score: " << qualityScore << "/100\\n";
    
    std::string grade;
    if (qualityScore >= 90) grade = "A";
    else if (qualityScore >= 80) grade = "B";
    else if (qualityScore >= 70) grade = "C";
    else grade = "D";
    
    std::cout << "Grade: " << grade << "\\n";
    
    std::cout << "\\n================================================================================\\n";
    
    return 0;
}
EOF
    fi
done

# Compile and run tests
for engine in "${engines[@]}"; do
    echo ""
    echo "================================"
    echo "Testing $engine..."
    echo "================================"
    
    if [ -f "test_${engine}.cpp" ]; then
        g++ -std=c++17 -o "test_${engine}" "test_${engine}.cpp" \
            -I/Users/Branden/JUCE/modules \
            -IJUCE_Plugin/Source \
            -IJUCE_Plugin/JuceLibraryCode \
            -framework CoreAudio -framework CoreFoundation -framework Accelerate \
            -framework AudioToolbox -framework AudioUnit -framework CoreAudioKit \
            -framework CoreMIDI -framework Cocoa -framework Carbon \
            -framework QuartzCore -framework IOKit -framework Security \
            -framework WebKit -framework Metal -framework MetalKit \
            -DDEBUG=1 -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
            -DJUCE_MODULE_AVAILABLE_juce_core=1 \
            -DJUCE_MODULE_AVAILABLE_juce_audio_basics=1 \
            -DJUCE_MODULE_AVAILABLE_juce_events=1 \
            -DJUCE_STANDALONE_APPLICATION=1 \
            JUCE_Plugin/Builds/MacOSX/build/Debug/libChimeraPhoenix.a 2>&1
        
        if [ $? -eq 0 ]; then
            ./test_${engine} > "Reports/${engine}_Results.txt" 2>&1
            echo "Results saved to Reports/${engine}_Results.txt"
            
            # Extract key results
            grep "Quality Score:" "Reports/${engine}_Results.txt" | tail -1
            grep "Grade:" "Reports/${engine}_Results.txt" | tail -1
        else
            echo "Compilation failed for $engine"
        fi
    fi
done

echo ""
echo "All modulation engine tests complete!"