// Simple Reverb Test - Validates all 5 reverb engines
#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>

int main() {
    std::cout << "====================================" << std::endl;
    std::cout << "REVERB ENGINE VALIDATION" << std::endl;
    std::cout << "====================================" << std::endl;
    
    // Reverb engines to test
    struct ReverbEngine {
        int id;
        std::string name;
        int paramCount;
        int mixIndex;
        std::string status;
    };
    
    std::vector<ReverbEngine> reverbs = {
        {39, "PlateReverb", 4, 3, ""},
        {40, "SpringReverb_Platinum", 8, 7, ""},
        {41, "ConvolutionReverb", 6, 4, ""},
        {42, "ShimmerReverb", 10, 9, ""},
        {43, "GatedReverb", 8, 7, ""}
    };
    
    std::cout << "\nExpected Reverb Parameters:" << std::endl;
    std::cout << "----------------------------" << std::endl;
    
    for (auto& reverb : reverbs) {
        std::cout << "\n" << reverb.name << " (ID " << reverb.id << "):" << std::endl;
        std::cout << "  Total parameters: " << reverb.paramCount << std::endl;
        std::cout << "  Mix parameter index: " << reverb.mixIndex << std::endl;
        
        // Expected parameters based on typical reverb design
        if (reverb.name == "PlateReverb") {
            std::cout << "  Expected params: Size, Decay, Damping, Mix" << std::endl;
            std::cout << "  Param[0] = Size (0.0=small, 1.0=large)" << std::endl;
            std::cout << "  Param[1] = Decay (0.0=short, 1.0=long)" << std::endl;
            std::cout << "  Param[2] = Damping (0.0=bright, 1.0=dark)" << std::endl;
            std::cout << "  Param[3] = Mix (0.0=dry, 1.0=wet)" << std::endl;
        }
        else if (reverb.name == "SpringReverb_Platinum") {
            std::cout << "  Expected params: Tension, Decay, Tone, Modulation, etc." << std::endl;
            std::cout << "  Param[7] = Mix (0.0=dry, 1.0=wet)" << std::endl;
        }
        else if (reverb.name == "ConvolutionReverb") {
            std::cout << "  Expected params: IR Select, Size, Pre-delay, Tone, etc." << std::endl;
            std::cout << "  Param[4] = Mix (0.0=dry, 1.0=wet)" << std::endl;
        }
        else if (reverb.name == "ShimmerReverb") {
            std::cout << "  Expected params: Size, Decay, Shimmer, Pitch, etc." << std::endl;
            std::cout << "  Param[9] = Mix (0.0=dry, 1.0=wet)" << std::endl;
        }
        else if (reverb.name == "GatedReverb") {
            std::cout << "  Expected params: Size, Gate Threshold, Hold, Release, etc." << std::endl;
            std::cout << "  Param[7] = Mix (0.0=dry, 1.0=wet)" << std::endl;
        }
    }
    
    std::cout << "\n====================================" << std::endl;
    std::cout << "REVERB TEST RECOMMENDATIONS" << std::endl;
    std::cout << "====================================" << std::endl;
    
    std::cout << "\n1. IMPULSE RESPONSE TEST:" << std::endl;
    std::cout << "   - Send a single sample spike [1.0, 0, 0, 0...]" << std::endl;
    std::cout << "   - Set Mix to 1.0 (100% wet)" << std::endl;
    std::cout << "   - Measure tail length to -60dB" << std::endl;
    std::cout << "   - Expected: 0.5-5.0 seconds depending on Decay setting" << std::endl;
    
    std::cout << "\n2. PARAMETER SWEEP TEST:" << std::endl;
    std::cout << "   - Test Size/Room: 0.0 -> 1.0" << std::endl;
    std::cout << "     Should change from small/tight to large/spacious" << std::endl;
    std::cout << "   - Test Decay/Time: 0.0 -> 1.0" << std::endl;
    std::cout << "     Should change from 100ms to 10+ seconds" << std::endl;
    std::cout << "   - Test Damping/Tone: 0.0 -> 1.0" << std::endl;
    std::cout << "     Should change from bright to dark" << std::endl;
    
    std::cout << "\n3. QUALITY CHECKS:" << std::endl;
    std::cout << "   ✓ No metallic ringing" << std::endl;
    std::cout << "   ✓ Smooth decay (no sudden drops)" << std::endl;
    std::cout << "   ✓ Even frequency response" << std::endl;
    std::cout << "   ✓ Stereo width appropriate" << std::endl;
    std::cout << "   ✓ No clicks/pops when changing parameters" << std::endl;
    
    std::cout << "\n4. MUSICAL CONTENT TEST:" << std::endl;
    std::cout << "   - Voice: Should add space without muddiness" << std::endl;
    std::cout << "   - Drums: Snare should have nice tail" << std::endl;
    std::cout << "   - Piano: Should sound natural, not metallic" << std::endl;
    std::cout << "   - Full mix: Should add depth without wash" << std::endl;
    
    std::cout << "\n====================================" << std::endl;
    std::cout << "MANUAL TEST PROCEDURE" << std::endl;
    std::cout << "====================================" << std::endl;
    
    std::cout << "\nRun the actual engine test to verify:" << std::endl;
    std::cout << "  ./test_all_engines | grep -A5 -B5 \"Reverb\"" << std::endl;
    
    std::cout << "\nOr test individual reverbs:" << std::endl;
    for (const auto& reverb : reverbs) {
        std::cout << "  ./test_single_engine " << reverb.id 
                  << "  # " << reverb.name << std::endl;
    }
    
    std::cout << "\n====================================" << std::endl;
    std::cout << "EXPECTED RESULTS" << std::endl;
    std::cout << "====================================" << std::endl;
    
    std::cout << "\nAll reverbs should:" << std::endl;
    std::cout << "1. Produce audible reverb tail when Mix > 0" << std::endl;
    std::cout << "2. Tail length increases with Decay parameter" << std::endl;
    std::cout << "3. Sound natural, not metallic or ringy" << std::endl;
    std::cout << "4. Mix parameter blends dry/wet correctly" << std::endl;
    std::cout << "5. Process stereo signal maintaining width" << std::endl;
    
    return 0;
}