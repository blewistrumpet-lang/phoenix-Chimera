#include <iostream>
#include <cmath>
#include <vector>

// Simple Freeverb test to validate algorithm
class SimpleFreeverb {
    static constexpr int numCombs = 8;
    static constexpr float fixedGain = 0.015f;
    
    struct Comb {
        std::vector<float> buffer;
        int size;
        int pos = 0;
        float damp = 0;
        float dampState = 0;
        float feedback = 0;
        
        void init(int sz) {
            size = sz;
            buffer.resize(size, 0.0f);
            pos = 0;
        }
        
        float process(float input) {
            float output = buffer[pos];
            dampState = output * (1.0f - damp) + dampState * damp;
            buffer[pos] = input + dampState * feedback;
            pos = (pos + 1) % size;
            return output;
        }
    };
    
    Comb combs[numCombs];
    
public:
    void init(double sampleRate) {
        const int combTunings[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        for (int i = 0; i < numCombs; i++) {
            combs[i].init(combTunings[i]);
            combs[i].feedback = 0.84f;
            combs[i].damp = 0.2f;
        }
    }
    
    float process(float input) {
        float output = 0;
        for (int i = 0; i < numCombs; i++) {
            output += combs[i].process(input * fixedGain);
        }
        return output;
    }
};

int main() {
    SimpleFreeverb reverb;
    reverb.init(44100);
    
    // Process impulse
    float output = reverb.process(1.0f);
    std::cout << "First sample after impulse: " << output << "\n";
    
    // Process tail
    float totalEnergy = 0;
    for (int i = 0; i < 1000; i++) {
        output = reverb.process(0.0f);
        totalEnergy += output * output;
        if (i < 10) {
            std::cout << "Sample " << i << ": " << output << "\n";
        }
    }
    
    std::cout << "Total tail energy: " << totalEnergy << "\n";
    
    return 0;
}
