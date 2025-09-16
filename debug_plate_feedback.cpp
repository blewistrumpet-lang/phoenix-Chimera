#include <iostream>
#include <iomanip>
#include "JUCE_Plugin/Source/PlateReverb.h"

int main() {
    PlateReverb reverb;
    reverb.prepareToPlay(44100, 512);
    reverb.reset();
    
    // Set to full wet, moderate size
    std::map<int, float> params;
    params[0] = 0.5f; // Size
    params[1] = 0.5f; // Damping  
    params[2] = 0.0f; // Predelay
    params[3] = 1.0f; // Mix (full wet)
    reverb.updateParameters(params);
    
    // Let params settle
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    for (int i = 0; i < 10; ++i) {
        reverb.process(buffer);
    }
    
    // Send impulse
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    std::cout << "Block | Max Sample | Energy | Status" << std::endl;
    std::cout << "------+------------+--------+-------" << std::endl;
    
    float prevEnergy = 0.0f;
    int growthCount = 0;
    
    for (int block = 0; block < 50; ++block) {
        reverb.process(buffer);
        
        float maxSample = 0.0f;
        float energy = 0.0f;
        for (int i = 0; i < 512; ++i) {
            float sample = buffer.getSample(0, i);
            maxSample = std::max(maxSample, std::abs(sample));
            energy += sample * sample;
        }
        energy /= 512.0f;
        
        std::string status = "OK";
        if (energy > prevEnergy * 1.1f && block > 5) {
            status = "GROWING";
            growthCount++;
        }
        
        std::cout << std::setw(5) << block << " | "
                  << std::fixed << std::setprecision(6) << std::setw(10) << maxSample << " | "
                  << std::scientific << std::setprecision(2) << energy << " | "
                  << status << std::endl;
        
        prevEnergy = energy;
        buffer.clear(); // Clear for next iteration
        
        if (maxSample > 10.0f || !std::isfinite(maxSample)) {
            std::cout << "ERROR: Output exploded!" << std::endl;
            break;
        }
    }
    
    std::cout << "\nGrowth events: " << growthCount << std::endl;
    if (growthCount > 2) {
        std::cout << "UNSTABLE: Feedback is growing" << std::endl;
    } else {
        std::cout << "STABLE: Normal decay" << std::endl;
    }
    
    return 0;
}
