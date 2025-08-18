// TEST TO PROVE MIX PARAMETER BUG EXISTS
// This test demonstrates that HarmonicExciter doesn't process audio
// because Mix parameter is at wrong index

#include <iostream>
#include <cmath>

// Simulating the actual code structure

// From HarmonicExciter_Platinum.h
enum HarmonicExciterParams {
    HE_Frequency = 0,
    HE_Drive = 1,
    HE_Harmonics = 2,
    HE_Clarity = 3,
    HE_Warmth = 4,
    HE_Presence = 5,
    HE_Color = 6,
    HE_Mix = 7  // <-- ACTUAL MIX INDEX IS 7
};

// From PluginProcessor.cpp getMixParameterIndex
int getMixParameterIndex(int engineID) {
    const int ENGINE_HARMONIC_EXCITER = 17;
    
    switch(engineID) {
        case ENGINE_HARMONIC_EXCITER:
            // BUG: This case is MISSING from the actual code!
            // So it falls through to default: return 3
            break;
        default:
            return 3;  // <-- WRONG! Returns 3 instead of 7
    }
    return 3;
}

// Simulating the parameter update and process
class HarmonicExciterSim {
public:
    float mixAmount = 0.0f;
    
    void updateParameters(float params[15]) {
        // Engine expects mix at index 7
        mixAmount = params[HE_Mix];
        std::cout << "Mix parameter at index 7 = " << mixAmount << std::endl;
    }
    
    bool process() {
        // From actual HarmonicExciter_Platinum.cpp
        if (mixAmount < 0.001f) {
            std::cout << "EARLY RETURN - No processing!" << std::endl;
            return false;
        }
        std::cout << "Processing audio normally" << std::endl;
        return true;
    }
};

int main() {
    std::cout << "=== DEMONSTRATING MIX PARAMETER BUG ===" << std::endl;
    std::cout << std::endl;
    
    // Simulate what happens in the plugin
    float params[15] = {0};
    
    // Plugin tries to set mix at wrong index
    int wrongMixIndex = getMixParameterIndex(17);  // Returns 3
    std::cout << "Plugin thinks Mix is at index: " << wrongMixIndex << std::endl;
    std::cout << "Actual Mix parameter is at index: " << HE_Mix << std::endl;
    std::cout << std::endl;
    
    // Plugin sets mix at wrong location
    params[wrongMixIndex] = 1.0f;  // Sets index 3 to 1.0
    std::cout << "Plugin sets params[" << wrongMixIndex << "] = 1.0" << std::endl;
    std::cout << "But params[" << HE_Mix << "] = " << params[HE_Mix] << std::endl;
    std::cout << std::endl;
    
    // Engine processes with these parameters
    HarmonicExciterSim engine;
    engine.updateParameters(params);
    
    std::cout << "Result: ";
    bool processed = engine.process();
    
    std::cout << std::endl;
    std::cout << "=== PROOF OF BUG ===" << std::endl;
    std::cout << "1. HarmonicExciter expects Mix at index 7" << std::endl;
    std::cout << "2. getMixParameterIndex doesn't handle ENGINE_HARMONIC_EXCITER" << std::endl;
    std::cout << "3. Returns default value 3 instead of 7" << std::endl;
    std::cout << "4. Mix stays at 0.0, causing early return" << std::endl;
    std::cout << "5. Engine never processes audio!" << std::endl;
    
    return processed ? 0 : 1;
}