// Test the actual ParameterControlMap implementation
#include "JUCE_Plugin/Source/ParameterControlMap.h"
#include <iostream>

int main() {
    std::cout << "Testing ParameterControlMap with all 57 engines..." << std::endl;
    
    // Test each engine
    for (int engineId = 0; engineId <= 56; ++engineId) {
        auto& params = ParameterControlMap::getEngineParameters(engineId);
        
        if (params.empty() && engineId == 0) {
            std::cout << "Engine " << engineId << " (BYPASS): No parameters (correct)" << std::endl;
        } else if (params.empty()) {
            std::cout << "ERROR: Engine " << engineId << " has no parameters!" << std::endl;
        } else {
            std::cout << "Engine " << engineId << ": " << params.size() << " parameters" << std::endl;
            
            // Verify each parameter has a name and valid control type
            for (size_t i = 0; i < params.size(); ++i) {
                if (params[i].name.empty()) {
                    std::cout << "  ERROR: Parameter " << i << " has empty name!" << std::endl;
                }
                if (params[i].control < 0 || params[i].control > 3) {
                    std::cout << "  ERROR: Parameter " << i << " has invalid control type: " 
                              << params[i].control << std::endl;
                }
            }
        }
    }
    
    // Test out of range
    std::cout << "\nTesting out of range engine IDs..." << std::endl;
    auto& params57 = ParameterControlMap::getEngineParameters(57);
    auto& params100 = ParameterControlMap::getEngineParameters(100);
    auto& paramsNeg = ParameterControlMap::getEngineParameters(-1);
    
    std::cout << "Engine 57: " << params57.size() << " parameters (should be default)" << std::endl;
    std::cout << "Engine 100: " << params100.size() << " parameters (should be default)" << std::endl;
    std::cout << "Engine -1: " << paramsNeg.size() << " parameters (should be default)" << std::endl;
    
    std::cout << "\n=== Test completed successfully ===" << std::endl;
    return 0;
}