#include <iostream>
#include <cmath>

void testParameterNormalization() {
    std::cout << "Testing AudioParameterChoice normalization:" << std::endl;
    std::cout << "===========================================" << std::endl;
    
    // AudioParameterChoice with 57 choices (0-56)
    const int numChoices = 57;
    
    // Test problem engine IDs
    int testEngines[] = {0, 15, 52, 55, 56};
    const char* names[] = {"Bypass", "BitCrusher", "Engine 52", "Engine 55", "Phase Align"};
    
    for (int i = 0; i < 5; i++) {
        int engineID = testEngines[i];
        
        // What convertTo0to1 should do: value / (numChoices - 1)
        float normalizedCorrect = static_cast<float>(engineID) / (numChoices - 1);
        
        // What might be happening if it's broken: value / numChoices
        float normalizedWrong = static_cast<float>(engineID) / numChoices;
        
        // What value would convert back to
        int recoveredCorrect = static_cast<int>(normalizedCorrect * (numChoices - 1) + 0.5f);
        int recoveredWrong = static_cast<int>(normalizedWrong * (numChoices - 1) + 0.5f);
        
        std::cout << "\n" << names[i] << " (ID " << engineID << "):" << std::endl;
        std::cout << "  Correct normalized: " << normalizedCorrect 
                  << " -> recovers to " << recoveredCorrect << std::endl;
        std::cout << "  Wrong normalized:   " << normalizedWrong 
                  << " -> recovers to " << recoveredWrong;
        
        if (recoveredWrong == 56) {
            std::cout << " <- THIS WOULD CAUSE PHASE ALIGN!";
        }
        std::cout << std::endl;
    }
    
    std::cout << "\nIf Trinity sends raw engine IDs as parameter values:" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    // What happens if we pass engine ID directly as parameter value
    for (int i = 0; i < 5; i++) {
        float rawValue = static_cast<float>(testEngines[i]);
        
        // AudioParameterChoice expects 0-1, but we're passing 0-56
        // It would clamp to 1.0 for anything > 1.0
        float clamped = (rawValue > 1.0f) ? 1.0f : rawValue;
        
        // Then convert back to choice index
        int resultingChoice = static_cast<int>(clamped * (numChoices - 1) + 0.5f);
        
        std::cout << names[i] << " (ID " << testEngines[i] << "):" << std::endl;
        std::cout << "  Raw value: " << rawValue;
        if (rawValue > 1.0f) {
            std::cout << " -> clamped to 1.0";
        }
        std::cout << " -> choice " << resultingChoice;
        if (resultingChoice == 56) {
            std::cout << " = PHASE ALIGN!";
        }
        std::cout << std::endl;
    }
}

int main() {
    testParameterNormalization();
    return 0;
}