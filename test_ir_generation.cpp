#include <iostream>
#include <vector>
#include <cmath>

std::vector<float> generateConcertHall(double sampleRate, float lengthSeconds) {
    int numSamples = static_cast<int>(sampleRate * lengthSeconds);
    std::vector<float> ir(numSamples);
    
    // Early reflections pattern for concert hall
    const float earlyTimes[] = {0.015f, 0.022f, 0.035f, 0.045f, 0.058f, 0.072f, 0.089f, 0.108f};
    const float earlyGains[] = {0.7f, 0.65f, 0.6f, 0.55f, 0.5f, 0.45f, 0.4f, 0.35f};
    
    for (int i = 0; i < 8; i++) {
        int pos = static_cast<int>(earlyTimes[i] * sampleRate);
        if (pos < numSamples) {
            ir[pos] = earlyGains[i] * (i % 2 ? 1.0f : -0.8f);
        }
    }
    
    // Late reverb tail with exponential decay
    float rt60 = 2.8f; // Concert hall RT60
    float decayRate = -3.0f / rt60;
    
    for (int i = static_cast<int>(0.1f * sampleRate); i < numSamples; i++) {
        float time = i / static_cast<float>(sampleRate);
        float envelope = std::exp(decayRate * time);
        
        // Add diffuse reverb using noise
        float noise = (std::sin(i * 0.00137f) + std::sin(i * 0.00213f) + 
                      std::sin(i * 0.00317f)) / 3.0f;
        
        // Frequency-dependent decay
        float dampingFactor = 1.0f - (time / lengthSeconds) * 0.7f;
        ir[i] += noise * envelope * dampingFactor * 0.3f;
    }
    
    return ir;
}

int main() {
    std::cout << "Testing IR Generation\n";
    std::cout << "====================\n";
    
    auto ir = generateConcertHall(44100, 3.0f);
    
    std::cout << "\nIR size: " << ir.size() << " samples\n";
    
    // Check early reflections
    std::cout << "\nEarly reflections (first 5000 samples):\n";
    int count = 0;
    for (int i = 0; i < 5000 && i < ir.size(); i++) {
        if (std::abs(ir[i]) > 0.01f) {
            std::cout << "  Sample " << i << " (" << (i/44.1f) << "ms): " << ir[i] << "\n";
            count++;
        }
    }
    
    // Check overall energy
    float energy = 0;
    for (float sample : ir) {
        energy += sample * sample;
    }
    
    std::cout << "\nTotal IR energy: " << energy << "\n";
    std::cout << "RMS: " << std::sqrt(energy / ir.size()) << "\n";
    
    if (energy > 0.1f) {
        std::cout << "\n✓ IR has significant energy\n";
    } else {
        std::cout << "\n✗ IR has very low energy\n";
    }
    
    return 0;
}
