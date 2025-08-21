#include <iostream>
#include <vector>
#include <map>

int main() {
    std::cout << "==== DYNAMICS TEAM VALIDATION ====" << std::endl;
    
    // Dynamics engines: IDs 1-6
    std::vector<int> engines = {1, 2, 3, 4, 5, 6};
    std::map<int, std::string> names = {
        {1, "VintageOptoCompressor_Platinum"},
        {2, "ClassicCompressor"},
        {3, "TransientShaper_Platinum"},
        {4, "NoiseGate_Platinum"},
        {5, "MasteringLimiter_Platinum"},
        {6, "DynamicEQ"}
    };
    
    std::map<int, int> expectedParams = {
        {1, 10}, {2, 10}, {3, 10}, {4, 8}, {5, 8}, {6, 8}
    };
    
    std::map<int, int> expectedMixIndex = {
        {1, 5}, {2, 6}, {3, 9}, {4, 6}, {5, 5}, {6, 6}
    };
    
    std::cout << "\nKnown Issues:" << std::endl;
    std::cout << "- ClassicCompressor (ID 2): EAM mix index issue" << std::endl;
    std::cout << "- DynamicEQ (ID 6): EAM mix index issue" << std::endl;
    
    for (int id : engines) {
        std::cout << "\n[" << id << "] " << names[id] << std::endl;
        std::cout << "  Parameters: " << expectedParams[id] << std::endl;
        std::cout << "  Mix Index: " << expectedMixIndex[id] << std::endl;
    }
    
    return 0;
}
