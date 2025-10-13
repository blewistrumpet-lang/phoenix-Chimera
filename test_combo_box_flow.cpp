/**
 * Test to prove whether setting combo box actually loads engines
 */
#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PluginProcessor.h"

int main() {
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    std::cout << "\n=== TEST: Does ComboBox -> Parameter -> Engine Loading Work? ===\n" << std::endl;
    
    // Create processor
    ChimeraAudioProcessor processor;
    
    // Get the AudioProcessorValueTreeState
    auto& apvts = processor.getValueTreeState();
    
    // Check initial state
    std::cout << "Initial engine in slot 0: " << processor.getEngineIDForSlot(0) << std::endl;
    
    // Test 1: Set parameter directly
    std::cout << "\nTest 1: Setting parameter 'slot1_engine' to choice index 22 (K-Style Overdrive)" << std::endl;
    if (auto* param = apvts.getParameter("slot1_engine")) {
        float normalizedValue = 22.0f / 56.0f;  // 57 choices, index 22
        param->setValueNotifyingHost(normalizedValue);
        std::cout << "  Parameter set to normalized value: " << normalizedValue << std::endl;
    }
    
    // Check if engine loaded
    int engineId = processor.getEngineIDForSlot(0);
    std::cout << "  Engine in slot 0 after parameter change: " << engineId << std::endl;
    
    if (engineId == 22) {
        std::cout << "  ✅ SUCCESS: Engine loaded via parameter change!" << std::endl;
    } else {
        std::cout << "  ❌ FAILURE: Engine NOT loaded (expected 22, got " << engineId << ")" << std::endl;
    }
    
    // Test 2: CRITICAL TEST - Does setSlotEngine() work directly?
    std::cout << "\nTest 2: CRITICAL TEST - Direct setSlotEngine() call" << std::endl;
    std::cout << "  Trinity calls setSlotEngine() directly. Testing this now..." << std::endl;
    
    // Reset slot first
    if (auto* param = apvts.getParameter("slot1_engine")) {
        param->setValueNotifyingHost(0);
    }
    
    // Test setSlotEngine() with different engines
    struct TestCase {
        int slot;
        int engineID;
        std::string name;
    };
    
    std::vector<TestCase> tests = {
        {0, 1, "Vintage Opto Compressor"},
        {1, 39, "Plate Reverb"},
        {2, 12, "Intelligent Harmonizer"}
    };
    
    bool allTestsPassed = true;
    
    for (const auto& test : tests) {
        std::cout << "\n>>> Testing setSlotEngine(" << test.slot << ", " << test.engineID << ") - " << test.name << std::endl;
        
        // CRITICAL CALL: Does this work?
        processor.setSlotEngine(test.slot, test.engineID);
        
        // Verify it worked
        int resultEngineID = processor.getEngineIDForSlot(test.slot);
        auto& engine = processor.getEngine(test.slot);
        
        std::cout << "  Result: getEngineIDForSlot(" << test.slot << ") = " << resultEngineID << std::endl;
        std::cout << "  Engine instance: " << (engine ? "EXISTS" : "NULL") << std::endl;
        if (engine) {
            std::cout << "  Engine name: " << engine->getName().toStdString() << std::endl;
        }
        
        bool testPassed = (resultEngineID == test.engineID) && (engine != nullptr);
        std::cout << "  Status: " << (testPassed ? "✅ PASS" : "❌ FAIL") << std::endl;
        
        if (!testPassed) {
            allTestsPassed = false;
            std::cout << "  ERROR: Expected engine ID " << test.engineID << ", got " << resultEngineID << std::endl;
            std::cout << "  ERROR: Expected engine instance to exist, got " << (engine ? "EXISTS" : "NULL") << std::endl;
        }
    }
    
    // Test 3: What happens when Trinity preset callback sets combo box?
    std::cout << "\nTest 3: Simulating what Trinity preset does with combo box" << std::endl;
    std::cout << "  In PluginEditorFull, Trinity callback does:" << std::endl;
    std::cout << "    engineSelectors[i].setSelectedId(choiceIndex + 1);" << std::endl;
    std::cout << "  This SHOULD trigger ComboBoxAttachment to update parameter" << std::endl;
    std::cout << "  Which SHOULD call parameterChanged() and load engine" << std::endl;
    
    std::cout << "\n=== CONCLUSION ===\n" << std::endl;
    
    // Analyze both parameter system and setSlotEngine() results
    bool parameterSystemWorks = (engineId == 22);
    
    std::cout << "Parameter System Test: " << (parameterSystemWorks ? "✅ PASS" : "❌ FAIL") << std::endl;
    std::cout << "setSlotEngine() Tests: " << (allTestsPassed ? "✅ PASS" : "❌ FAIL") << std::endl;
    
    if (parameterSystemWorks && allTestsPassed) {
        std::cout << "\n🎉 EXCELLENT NEWS: Both systems work perfectly!" << std::endl;
        std::cout << "✅ Parameter changes DO load engines correctly." << std::endl;
        std::cout << "✅ setSlotEngine() DOES create and store engines properly." << std::endl;
        std::cout << "✅ Trinity's direct setSlotEngine() calls SHOULD work." << std::endl;
        std::cout << "\n🔍 Since setSlotEngine() works, the issue must be elsewhere:" << std::endl;
        std::cout << "   - Trinity's engine ID mapping might be wrong" << std::endl;
        std::cout << "   - Trinity might not be calling setSlotEngine() at all" << std::endl;
        std::cout << "   - UI synchronization issues after engine loading" << std::endl;
        std::cout << "   - Parameter value validation or conversion errors" << std::endl;
    } else if (!parameterSystemWorks && !allTestsPassed) {
        std::cout << "\n❌ CRITICAL FAILURE: Both systems are broken!" << std::endl;
        std::cout << "This indicates a fundamental problem in the engine loading architecture." << std::endl;
    } else if (parameterSystemWorks && !allTestsPassed) {
        std::cout << "\n⚠️  MIXED RESULTS: Parameter system works but setSlotEngine() fails!" << std::endl;
        std::cout << "This suggests setSlotEngine() has a bug in parameter conversion or validation." << std::endl;
    } else {
        std::cout << "\n⚠️  MIXED RESULTS: setSlotEngine() works but parameter system fails!" << std::endl;
        std::cout << "This suggests parameterChanged() has issues with AudioParameterChoice handling." << std::endl;
    }
    
    return 0;
}