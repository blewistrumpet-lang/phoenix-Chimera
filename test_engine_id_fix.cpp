#include <iostream>
#include <cassert>

// Simulating the conversion logic
int engineIDToChoiceIndex(int engineID) {
    // Direct 1:1 mapping since engine IDs are 0-56 and dropdown indices are 0-56
    return engineID;
}

int choiceIndexToEngineID(int choiceIndex) {
    // Direct 1:1 mapping
    return choiceIndex;
}

void testConversions() {
    std::cout << "Testing engine ID conversions..." << std::endl;
    
    // Test all engine IDs
    for (int engineID = 0; engineID <= 56; ++engineID) {
        int choiceIndex = engineIDToChoiceIndex(engineID);
        int backToEngineID = choiceIndexToEngineID(choiceIndex);
        
        assert(engineID == backToEngineID);
        
        if (engineID == 0) {
            std::cout << "Engine ID " << engineID << " (Bypass) -> Choice Index " << choiceIndex << std::endl;
        } else if (engineID == 56) {
            std::cout << "Engine ID " << engineID << " (Phase Align) -> Choice Index " << choiceIndex << std::endl;
        }
    }
    
    // Specific tests for problematic values
    std::cout << "\nSpecific tests:" << std::endl;
    
    // Test Phase Align
    int phaseAlignID = 56;
    int phaseAlignChoice = engineIDToChoiceIndex(phaseAlignID);
    std::cout << "Phase Align: Engine ID " << phaseAlignID << " -> Choice Index " << phaseAlignChoice << std::endl;
    assert(phaseAlignChoice == 56);
    
    // Test normalized value for AudioParameterChoice
    // With 57 total choices (0-56), normalized value for choice 56 would be 56/56 = 1.0
    float normalizedValue = static_cast<float>(phaseAlignChoice) / 56.0f;
    std::cout << "Normalized value for Phase Align: " << normalizedValue << std::endl;
    
    // Test some other engines
    int bitcrusherID = 15; // BitCrusher
    int bitcrusherChoice = engineIDToChoiceIndex(bitcrusherID);
    std::cout << "BitCrusher: Engine ID " << bitcrusherID << " -> Choice Index " << bitcrusherChoice << std::endl;
    assert(bitcrusherChoice == 15);
    
    std::cout << "\nAll tests passed!" << std::endl;
}

int main() {
    testConversions();
    return 0;
}