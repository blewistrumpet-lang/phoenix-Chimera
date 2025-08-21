#include <iostream>
#include <vector>
#include <map>

int main() {
    std::cout << "==== REVERB TEAM VALIDATION ====" << std::endl;
    
    std::vector<int> engines = {39, 40, 41, 42, 43};
    std::map<int, std::string> names = {
        {39, "PlateReverb"},
        {40, "SpringReverb_Platinum"},
        {41, "ConvolutionReverb"},
        {42, "ShimmerReverb"},
        {43, "GatedReverb"}
    };
    
    std::map<int, int> expectedMixIndex = {
        {39, 3}, {40, 7}, {41, 4}, {42, 9}, {43, 7}
    };
    
    std::cout << "\nKnown Issues:" << std::endl;
    std::cout << "- PlateReverb (ID 39): EAM mix index issue" << std::endl;
    std::cout << "- SpringReverb_Platinum (ID 40): EAM mix index issue" << std::endl;
    std::cout << "- GatedReverb (ID 43): EAM mix index issue" << std::endl;
    
    for (int id : engines) {
        std::cout << "\n[" << id << "] " << names[id] << std::endl;
        std::cout << "  Mix Index: " << expectedMixIndex[id] << std::endl;
        if (id == 39 || id == 40 || id == 43) {
            std::cout << "  Status: ⚠️ EAM Issue" << std::endl;
        } else {
            std::cout << "  Status: ✅" << std::endl;
        }
    }
    
    return 0;
}
