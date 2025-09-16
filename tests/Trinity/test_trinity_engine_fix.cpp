#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PluginProcessor.h"

class TestHarness {
public:
    void runTests() {
        testEngineSelection();
        testTrinityResponse();
    }
    
private:
    void testEngineSelection() {
        std::cout << "\n=== Testing Engine Selection ===" << std::endl;
        
        ChimeraAudioProcessor processor;
        
        // Test setting various engines
        struct TestCase {
            int engineID;
            const char* name;
        };
        
        TestCase tests[] = {
            {0, "Bypass"},
            {15, "BitCrusher"},
            {56, "Phase Align"},
            {25, "Some middle engine"}
        };
        
        for (const auto& test : tests) {
            std::cout << "Setting engine " << test.engineID << " (" << test.name << ")..." << std::endl;
            
            // Set engine on slot 0
            processor.setSlotEngine(0, test.engineID);
            
            // Get the parameter to verify
            auto* param = processor.getValueTreeState().getParameter("slot1_engine");
            if (param) {
                auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param);
                if (choiceParam) {
                    int currentChoice = choiceParam->getIndex();
                    std::cout << "  Current choice index: " << currentChoice << std::endl;
                    
                    // Verify it matches
                    if (currentChoice == test.engineID) {
                        std::cout << "  ✓ Correct!" << std::endl;
                    } else {
                        std::cout << "  ✗ ERROR: Expected " << test.engineID << " but got " << currentChoice << std::endl;
                    }
                }
            }
        }
    }
    
    void testTrinityResponse() {
        std::cout << "\n=== Testing Trinity Response Parsing ===" << std::endl;
        
        // Simulate a Trinity response with engine IDs
        juce::var trinityResponse;
        auto params = new juce::DynamicObject();
        
        // Set slot1 to BitCrusher (ID 15)
        params->setProperty("slot1_engine", 15.0f);
        params->setProperty("slot1_param1", 0.7f);
        
        // Set slot2 to Phase Align (ID 56)  
        params->setProperty("slot2_engine", 56.0f);
        params->setProperty("slot2_param1", 0.3f);
        
        trinityResponse = juce::var(params);
        
        std::cout << "Trinity response:" << std::endl;
        std::cout << "  slot1_engine: 15 (BitCrusher)" << std::endl;
        std::cout << "  slot2_engine: 56 (Phase Align)" << std::endl;
        
        // Test the conversion that would happen in applyTrinityPresetFromParameters
        float slot1_engine = trinityResponse.getProperty("slot1_engine", 0.0f);
        float slot2_engine = trinityResponse.getProperty("slot2_engine", 0.0f);
        
        std::cout << "\nRaw values from Trinity:" << std::endl;
        std::cout << "  slot1_engine raw: " << slot1_engine << std::endl;
        std::cout << "  slot2_engine raw: " << slot2_engine << std::endl;
        
        // Simulate what AudioParameterChoice::convertTo0to1 would do
        // For 57 choices (0-56), the normalized value is choiceIndex / 56.0
        float normalized1 = slot1_engine / 56.0f;
        float normalized2 = slot2_engine / 56.0f;
        
        std::cout << "\nNormalized values for setValueNotifyingHost:" << std::endl;
        std::cout << "  slot1_engine normalized: " << normalized1 << std::endl;
        std::cout << "  slot2_engine normalized: " << normalized2 << std::endl;
        
        // Verify conversion back
        int recovered1 = static_cast<int>(normalized1 * 56.0f + 0.5f);
        int recovered2 = static_cast<int>(normalized2 * 56.0f + 0.5f);
        
        std::cout << "\nRecovered engine IDs:" << std::endl;
        std::cout << "  slot1_engine recovered: " << recovered1;
        if (recovered1 == 15) std::cout << " ✓"; else std::cout << " ✗";
        std::cout << std::endl;
        
        std::cout << "  slot2_engine recovered: " << recovered2;
        if (recovered2 == 56) std::cout << " ✓"; else std::cout << " ✗";
        std::cout << std::endl;
    }
};

int main() {
    std::cout << "Trinity Engine Fix Test" << std::endl;
    std::cout << "========================" << std::endl;
    
    TestHarness harness;
    harness.runTests();
    
    return 0;
}