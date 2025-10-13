/**
 * CRITICAL TEST: Does setSlotEngine() actually load engines?
 * 
 * Previous agents found that Trinity calls setSlotEngine() directly,
 * but the user reports "no engines are being loaded."
 * 
 * This test will prove definitively whether setSlotEngine() works.
 * 
 * BASED ON WORKING test_combo_box_flow.cpp - extending it to test setSlotEngine() directly
 */
#include <iostream>
#include <JuceHeader.h>
#include "JUCE_Plugin/Source/PluginProcessor.h"

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void testEngineRetrieval(ChimeraAudioProcessor& processor, int slot, int expectedEngineID, const std::string& context) {
    std::cout << "\n--- " << context << " ---" << std::endl;
    
    // Test 1: getEngineIDForSlot() - what the UI uses
    int reportedEngineID = processor.getEngineIDForSlot(slot);
    std::cout << "getEngineIDForSlot(" << slot << ") reports: " << reportedEngineID << std::endl;
    
    // Test 2: Direct engine instance check - the actual truth
    auto& engine = processor.getEngine(slot);
    bool engineExists = (engine != nullptr);
    std::cout << "Actual engine instance exists: " << (engineExists ? "YES" : "NO") << std::endl;
    
    if (engineExists) {
        std::cout << "Engine name: " << engine->getName().toStdString() << std::endl;
        std::cout << "Engine address: " << std::hex << engine.get() << std::dec << std::endl;
        std::cout << "Engine parameters: " << engine->getNumParameters() << std::endl;
    }
    
    // Test 3: Parameter value check - what actually drives the system
    auto& apvts = processor.getValueTreeState();
    auto* param = apvts.getRawParameterValue("slot" + juce::String(slot + 1) + "_engine");
    if (param) {
        float paramValue = param->load();
        std::cout << "Raw parameter value: " << paramValue << std::endl;
        
        // Convert to choice index and engine ID to verify mapping
        auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(
            apvts.getParameter("slot" + juce::String(slot + 1) + "_engine"));
        if (choiceParam) {
            int choiceIndex = choiceParam->getIndex();
            int mappedEngineID = ChimeraAudioProcessor::choiceIndexToEngineID(choiceIndex);
            std::cout << "Choice index: " << choiceIndex << " -> Engine ID: " << mappedEngineID << std::endl;
        }
    }
    
    // Result analysis
    bool testPassed = (reportedEngineID == expectedEngineID) && 
                     (expectedEngineID == 0 ? !engineExists : engineExists);
    
    std::cout << "\nRESULT: " << (testPassed ? "✅ PASS" : "❌ FAIL") << std::endl;
    if (!testPassed) {
        std::cout << "Expected engine ID: " << expectedEngineID << std::endl;
        std::cout << "Got engine ID: " << reportedEngineID << std::endl;
        std::cout << "Expected engine exists: " << (expectedEngineID != 0 ? "YES" : "NO") << std::endl;
        std::cout << "Actually engine exists: " << (engineExists ? "YES" : "NO") << std::endl;
    }
    
    return;
}

int main() {
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    printSeparator("CRITICAL TEST: setSlotEngine() Engine Loading Verification");
    
    std::cout << "Testing whether setSlotEngine() actually creates and stores engine instances..." << std::endl;
    
    // Create processor
    ChimeraAudioProcessor processor;
    
    // Test initial state - should be empty
    printSeparator("Phase 1: Initial State Verification");
    std::cout << "Checking that all slots start empty..." << std::endl;
    
    for (int slot = 0; slot < 6; ++slot) {
        testEngineRetrieval(processor, slot, 0, "Slot " + std::to_string(slot) + " initial state");
    }
    
    // Test setSlotEngine() with various engines
    printSeparator("Phase 2: setSlotEngine() Testing");
    
    struct TestCase {
        int slot;
        int engineID;
        std::string engineName;
    };
    
    std::vector<TestCase> testCases = {
        {0, 1, "Vintage Opto Compressor"},
        {1, 22, "K-Style Overdrive"}, 
        {2, 39, "Plate Reverb"},
        {3, 12, "Intelligent Harmonizer"},
        {4, 5, "Multi-Band Compressor"}
    };
    
    std::cout << "Testing setSlotEngine() with 5 different engines..." << std::endl;
    
    for (const auto& test : testCases) {
        std::cout << "\n>>> Calling setSlotEngine(" << test.slot << ", " << test.engineID << ") for " << test.engineName << std::endl;
        
        // THE CRITICAL CALL: Does this actually work?
        processor.setSlotEngine(test.slot, test.engineID);
        
        // Verify immediately
        testEngineRetrieval(processor, test.slot, test.engineID, 
                          "After setSlotEngine(" + std::to_string(test.slot) + ", " + std::to_string(test.engineID) + ")");
    }
    
    // Test edge cases
    printSeparator("Phase 3: Edge Case Testing");
    
    std::cout << "\nTesting edge cases..." << std::endl;
    
    // Test setting to ENGINE_NONE (0)
    std::cout << "\n>>> Testing setSlotEngine(0, 0) - clearing engine" << std::endl;
    processor.setSlotEngine(0, 0);
    testEngineRetrieval(processor, 0, 0, "After clearing slot 0 with ENGINE_NONE");
    
    // Test invalid slot (should be ignored)
    std::cout << "\n>>> Testing setSlotEngine(-1, 1) - invalid slot" << std::endl;
    processor.setSlotEngine(-1, 1);
    std::cout << "Invalid slot call completed (should be ignored)" << std::endl;
    
    // Test invalid engine ID
    std::cout << "\n>>> Testing setSlotEngine(1, 999) - invalid engine ID" << std::endl;
    processor.setSlotEngine(1, 999);
    testEngineRetrieval(processor, 1, 0, "After invalid engine ID 999 (should default to ENGINE_NONE)");
    
    // Final comprehensive verification
    printSeparator("Phase 4: Final State Verification");
    
    std::cout << "Final state of all slots:" << std::endl;
    for (int slot = 0; slot < 6; ++slot) {
        int engineID = processor.getEngineIDForSlot(slot);
        auto& engine = processor.getEngine(slot);
        std::cout << "Slot " << slot << ": Engine ID " << engineID 
                  << ", Instance: " << (engine ? "EXISTS" : "NULL");
        if (engine) {
            std::cout << " (" << engine->getName().toStdString() << ")";
        }
        std::cout << std::endl;
    }
    
    // CRITICAL ANALYSIS
    printSeparator("CRITICAL ANALYSIS: Does setSlotEngine() Work?");
    
    bool foundWorkingEngine = false;
    bool foundBrokenEngine = false;
    
    for (int slot = 0; slot < 6; ++slot) {
        int engineID = processor.getEngineIDForSlot(slot);
        auto& engine = processor.getEngine(slot);
        
        if (engineID != 0) {
            if (engine) {
                foundWorkingEngine = true;
                std::cout << "✅ Slot " << slot << ": Engine ID " << engineID << " HAS working instance" << std::endl;
            } else {
                foundBrokenEngine = true;
                std::cout << "❌ Slot " << slot << ": Engine ID " << engineID << " but NO instance!" << std::endl;
            }
        }
    }
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "FINAL VERDICT:" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    if (foundWorkingEngine && !foundBrokenEngine) {
        std::cout << "✅ SUCCESS: setSlotEngine() DOES create and store engine instances!" << std::endl;
        std::cout << "✅ The engine loading system is working correctly." << std::endl;
        std::cout << "✅ Both getEngineIDForSlot() and actual instances are consistent." << std::endl;
        std::cout << "\nConclusion: The problem is NOT in setSlotEngine()." << std::endl;
        std::cout << "Look for issues in:" << std::endl;
        std::cout << "  - Trinity's engine ID mapping" << std::endl;
        std::cout << "  - Parameter synchronization" << std::endl;
        std::cout << "  - UI update mechanisms" << std::endl;
    } else if (foundBrokenEngine) {
        std::cout << "❌ CRITICAL FAILURE: setSlotEngine() sets IDs but doesn't create instances!" << std::endl;
        std::cout << "❌ There's a disconnect between parameter setting and engine creation." << std::endl;
        std::cout << "❌ The loadEngine() call in parameterChanged() may be failing." << std::endl;
        std::cout << "\nThis is a fundamental bug in the engine loading system." << std::endl;
    } else {
        std::cout << "⚠️  INCONCLUSIVE: No engines were loaded during this test." << std::endl;
        std::cout << "⚠️  This could indicate a problem with setSlotEngine() itself." << std::endl;
        std::cout << "⚠️  Check if the parameter setting mechanism is working." << std::endl;
    }
    
    std::cout << std::string(60, '=') << std::endl;
    
    return foundBrokenEngine ? 1 : 0;
}