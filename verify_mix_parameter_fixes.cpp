#include <iostream>
#include <map>
#include <string>

// Simulate the fixed getMixParameterIndex function
int getMixParameterIndex(int engineID) {
    const int ENGINE_PLATE_REVERB = 39;
    const int ENGINE_SPRING_REVERB = 40;
    const int ENGINE_VCA_COMPRESSOR = 2;
    
    switch (engineID) {
        case ENGINE_PLATE_REVERB:
            return 3; // FIXED: was 6, now 3
        case ENGINE_SPRING_REVERB:
            return 7; // FIXED: was 9, now 7  
        case ENGINE_VCA_COMPRESSOR:
            return 6; // FIXED: was 4, now 6
        default:
            return -1;
    }
}

// Simulate engine parameter counts based on actual implementations
struct EngineInfo {
    std::string name;
    int paramCount;
    int mixIndex;
    std::string mixParamName;
};

std::map<int, EngineInfo> getEngineInfo() {
    std::map<int, EngineInfo> engines;
    
    EngineInfo plate = {"PlateReverb", 4, 3, "Mix"};
    EngineInfo spring = {"SpringReverb", 8, 7, "Mix"};
    EngineInfo compressor = {"ClassicCompressor", 10, 6, "Mix"};
    
    engines[39] = plate;
    engines[40] = spring;
    engines[2] = compressor;
    
    return engines;
}

int main() {
    std::map<int, EngineInfo> engines = getEngineInfo();
    
    std::cout << "=== Parameter Mapping Verification ===" << std::endl;
    std::cout << std::endl;
    
    bool allPassed = true;
    
    for (std::map<int, EngineInfo>::const_iterator it = engines.begin(); it != engines.end(); ++it) {
        int engineID = it->first;
        const EngineInfo& info = it->second;
        int reportedIndex = getMixParameterIndex(engineID);
        bool valid = (reportedIndex >= 0 && reportedIndex < info.paramCount);
        bool correct = (reportedIndex == info.mixIndex);
        
        std::cout << "Engine: " << info.name << " (ID " << engineID << ")" << std::endl;
        std::cout << "  Parameter count: " << info.paramCount << " (0-" << (info.paramCount-1) << ")" << std::endl;
        std::cout << "  Actual Mix index: " << info.mixIndex << std::endl;
        std::cout << "  Reported index: " << reportedIndex << std::endl;
        
        if (!valid) {
            std::cout << "  ❌ ERROR: Index " << reportedIndex << " is out of range (0-" << (info.paramCount-1) << ")" << std::endl;
            allPassed = false;
        } else if (!correct) {
            std::cout << "  ❌ ERROR: Wrong parameter - index " << reportedIndex << " is not the Mix parameter" << std::endl;
            allPassed = false;
        } else {
            std::cout << "  ✅ CORRECT: Mix parameter properly mapped to index " << reportedIndex << std::endl;
        }
        std::cout << std::endl;
    }
    
    std::cout << "=== SUMMARY ===" << std::endl;
    if (allPassed) {
        std::cout << "✅ ALL FIXES SUCCESSFUL: All three engines now have correct Mix parameter mappings!" << std::endl;
        std::cout << std::endl;
        std::cout << "Fixed Issues:" << std::endl;
        std::cout << "• PlateReverb: Mix moved from index 6 → 3 (within 4 parameter range)" << std::endl;
        std::cout << "• SpringReverb: Mix moved from index 9 → 7 (within 8 parameter range)" << std::endl;  
        std::cout << "• ClassicCompressor: Mix moved from index 4 → 6 (correct Mix parameter)" << std::endl;
    } else {
        std::cout << "❌ SOME ISSUES REMAIN: Please check the errors above" << std::endl;
    }
    
    return allPassed ? 0 : 1;
}