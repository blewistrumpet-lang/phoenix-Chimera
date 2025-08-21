// Parameter Debug Test for Chimera Phoenix
// This test helps diagnose why UI parameters aren't controlling what they should

#include <iostream>
#include <map>
#include <string>

// Simulate what should happen when you turn a knob

void debugParameterFlow() {
    std::cout << "\n=== PARAMETER FLOW DEBUGGING ===\n" << std::endl;
    
    // When you turn the first knob in slot 1:
    std::cout << "1. UI Knob Turned: Slot 1, Knob 1" << std::endl;
    std::cout << "2. JUCE Parameter ID: 'slot1_param1'" << std::endl;
    std::cout << "3. Value sent to updateEngineParameters: params[0] = knob_value" << std::endl;
    std::cout << "4. Engine receives in updateParameters: index 0 = knob_value" << std::endl;
    std::cout << "5. Engine's getParameterName(0) returns: 'Drive' (or whatever)" << std::endl;
    std::cout << "6. Engine internally maps: index 0 -> m_drive parameter" << std::endl;
    
    std::cout << "\n=== COMMON ISSUES TO CHECK ===\n" << std::endl;
    
    std::cout << "Issue 1: ENGINE NOT LOADED" << std::endl;
    std::cout << "  - Check if dropdown shows 'None' (engine ID 0)" << std::endl;
    std::cout << "  - ENGINE_NONE is a bypass, parameters won't do anything" << std::endl;
    
    std::cout << "\nIssue 2: SLOT BYPASSED" << std::endl;
    std::cout << "  - Check if slot bypass is engaged" << std::endl;
    std::cout << "  - Bypassed slots don't process audio or respond to parameters" << std::endl;
    
    std::cout << "\nIssue 3: MIX PARAMETER AT 0%" << std::endl;
    std::cout << "  - Some engines have mix at different indices" << std::endl;
    std::cout << "  - If mix is 0%, you only hear dry signal" << std::endl;
    
    std::cout << "\nIssue 4: PARAMETER RANGE ISSUES" << std::endl;
    std::cout << "  - UI shows 0-100% but engine expects different range" << std::endl;
    std::cout << "  - Some parameters might need to be at extreme values to hear effect" << std::endl;
    
    std::cout << "\n=== SPECIFIC ENGINE CHECK ===\n" << std::endl;
    
    // Example for K-Style Overdrive (Engine ID 15)
    std::cout << "K-Style Overdrive Parameter Mapping:" << std::endl;
    std::cout << "  Param 0 (UI Knob 1): 'Drive' -> controls distortion amount" << std::endl;
    std::cout << "  Param 1 (UI Knob 2): 'Tone' -> controls tone/filter" << std::endl;
    std::cout << "  Param 2 (UI Knob 3): 'Level' -> controls output level" << std::endl;
    std::cout << "  Param 3 (UI Knob 4): 'Mix' -> controls dry/wet blend" << std::endl;
    
    std::cout << "\n=== DEBUGGING STEPS ===\n" << std::endl;
    std::cout << "1. Select K-Style Overdrive in Slot 1" << std::endl;
    std::cout << "2. Set Knob 1 (Drive) to maximum" << std::endl;
    std::cout << "3. Set Knob 4 (Mix) to maximum" << std::endl;
    std::cout << "4. Play audio - you should hear distortion" << std::endl;
    std::cout << "5. If no effect, check:" << std::endl;
    std::cout << "   - Is slot bypassed?" << std::endl;
    std::cout << "   - Is engine actually loaded? (not 'None')" << std::endl;
    std::cout << "   - Is audio routing correct in DAW?" << std::endl;
}

int main() {
    debugParameterFlow();
    
    std::cout << "\n=== PARAMETER VALUES TO CHECK IN LOGIC ===\n" << std::endl;
    std::cout << "1. Open plugin window" << std::endl;
    std::cout << "2. Check which engine is selected in each slot" << std::endl;
    std::cout << "3. Verify slot is not bypassed" << std::endl;
    std::cout << "4. Try these test values:" << std::endl;
    std::cout << "   - Select 'Tape Echo' in Slot 1" << std::endl;
    std::cout << "   - Set Delay Time (param 1) to 50%" << std::endl;
    std::cout << "   - Set Feedback (param 2) to 30%" << std::endl;
    std::cout << "   - Set Mix (param 5) to 50%" << std::endl;
    std::cout << "   - You should hear delayed repeats" << std::endl;
    
    return 0;
}