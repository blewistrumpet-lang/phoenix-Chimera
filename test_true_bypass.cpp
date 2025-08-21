// Test to verify true bypass at pitch=0.5, formant=0.5
#include <iostream>
#include <cmath>

int main() {
    // When pitch parameter = 0.500
    float pitchParam = 0.500f;
    float formantParam = 0.500f;
    
    // Pitch conversion
    float semitones = (pitchParam - 0.5f) * 48.0f;
    float pitchRatio = std::pow(2.0f, semitones / 12.0f);
    
    std::cout << "Pitch parameter: " << pitchParam << std::endl;
    std::cout << "Semitones: " << semitones << std::endl;
    std::cout << "Pitch ratio: " << pitchRatio << std::endl;
    std::cout << "Should bypass: " << (std::abs(pitchRatio - 1.0f) < 0.001f) << std::endl;
    
    std::cout << "\nFormant parameter: " << formantParam << std::endl;
    std::cout << "Formant is neutral: " << (std::abs(formantParam - 0.5f) < 0.001f) << std::endl;
    
    // Check bypass condition
    bool shouldBypass = (std::abs(pitchRatio - 1.0f) < 0.001f) && 
                       (std::abs(formantParam - 0.5f) < 0.001f);
    
    std::cout << "\nFull bypass (no processing): " << shouldBypass << std::endl;
    
    return 0;
}