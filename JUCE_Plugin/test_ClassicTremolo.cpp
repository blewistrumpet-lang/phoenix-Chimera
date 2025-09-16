#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <complex>
#include "JUCE_Plugin/Source/ClassicTremolo.h"

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

int main() {
    std::cout << "Testing ClassicTremolo\n";
    
    ClassicTremolo tremolo;
    float sampleRate = 48000.0f;
    int blockSize = 512;
    
    tremolo.prepareToPlay(sampleRate, blockSize);
    
    // Test modulation
    std::map<int, float> params;
    params[0] = 0.5f;  // Rate
    params[1] = 0.8f;  // Depth
    params[7] = 1.0f;  // Mix
    tremolo.updateParameters(params);
    
    // Generate test signal
    std::vector<float> testSignal(blockSize);
    for (int i = 0; i < blockSize; ++i) {
        testSignal[i] = 0.7f * sin(2.0f * M_PI * 440.0f * i / sampleRate);
    }
    
    // Process multiple blocks to detect tremolo
    std::vector<float> amplitudes;
    for (int block = 0; block < 20; ++block) {
        juce::AudioBuffer<float> buffer(1, blockSize);
        buffer.copyFrom(0, 0, testSignal.data(), blockSize);
        tremolo.process(buffer);
        
        float rms = 0;
        for (int i = 0; i < blockSize; ++i) {
            float sample = buffer.getSample(0, i);
            rms += sample * sample;
        }
        rms = sqrt(rms / blockSize);
        amplitudes.push_back(rms);
    }
    
    // Calculate modulation depth
    float minAmp = *std::min_element(amplitudes.begin(), amplitudes.end());
    float maxAmp = *std::max_element(amplitudes.begin(), amplitudes.end());
    float modDepth = (maxAmp - minAmp) / (maxAmp + minAmp) * 100;
    
    std::cout << "Modulation depth: " << modDepth << "%\n";
    
    // Test THD
    juce::AudioBuffer<float> buffer(1, blockSize);
    buffer.copyFrom(0, 0, testSignal.data(), blockSize);
    tremolo.process(buffer);
    
    std::vector<float> output(blockSize);
    for (int i = 0; i < blockSize; ++i) {
        output[i] = buffer.getSample(0, i);
    }
    
    std::vector<std::complex<float>> spectrum;
    computeFFT(output, spectrum);
    
    int fundamentalBin = static_cast<int>(440.0f * blockSize / sampleRate);
    float fundamental = std::abs(spectrum[fundamentalBin]);
    float harmonics = 0;
    for (int h = 2; h <= 5; ++h) {
        if (fundamentalBin * h < blockSize/2) {
            harmonics += pow(std::abs(spectrum[fundamentalBin * h]), 2);
        }
    }
    float thd = sqrt(harmonics) / fundamental * 100;
    
    std::cout << "THD: " << thd << "%\n";
    
    float qualityScore = 100;
    if (modDepth < 10) qualityScore -= 30;
    qualityScore -= thd * 3;
    qualityScore = std::max(0.0f, std::min(100.0f, qualityScore));
    
    std::cout << "Quality Score: " << qualityScore << "/100\n";
    
    std::string grade;
    if (qualityScore >= 90) grade = "A";
    else if (qualityScore >= 80) grade = "B";
    else if (qualityScore >= 70) grade = "C";
    else grade = "D";
    
    std::cout << "Grade: " << grade << "\n";
    
    return 0;
}
