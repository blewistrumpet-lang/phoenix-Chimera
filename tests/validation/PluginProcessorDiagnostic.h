#pragma once
#include "PluginProcessor.h"
#include "EngineDiagnostic.h"

/**
 * Plugin Processor Diagnostic Integration
 * 
 * This adds diagnostic capabilities directly to the PluginProcessor
 * for debugging engine processing in real-time.
 */
class PluginProcessorDiagnostic {
public:
    // Add this to your PluginProcessor class as a method
    static void addDiagnosticToProcessor(ChimeraAudioProcessor& processor) {
        // This would be called from the constructor or prepareToPlay
        // to set up diagnostic hooks
    }
    
    // Run engine diagnostics - call this from wherever you want to debug
    static void runDiagnostics(ChimeraAudioProcessor& processor) {
        auto results = EngineDiagnostic::runComprehensiveTest(
            processor.getSampleRate(), 
            512 // test block size
        );
        
        EngineDiagnostic::printResults(results);
    }
    
    // Test specific engines currently loaded in the processor
    static void testActiveEngines(ChimeraAudioProcessor& processor) {
        std::vector<EngineDiagnostic::DiagnosticResult> results;
        
        // Create test buffer
        juce::AudioBuffer<float> testBuffer(2, 512);
        EngineDiagnostic::generateTestTone(testBuffer, 1000.0f, 0.5f, processor.getSampleRate());
        
        std::cout << "Testing currently active engines in processor...\n";
        
        // Test each slot that has an active engine
        for (int slot = 0; slot < 4; ++slot) { // Assuming 4 slots
            // Get engine from processor - this would need to be adapted
            // based on your actual PluginProcessor implementation
            
            std::cout << "Slot " << (slot + 1) << ": ";
            
            // You would need to add a method to get the engine from a slot
            // auto engine = processor.getEngineForSlot(slot);
            // if (engine) {
            //     auto result = EngineDiagnostic::testEngine(testBuffer, 
            //         "Slot" + std::to_string(slot + 1), engine, {});
            //     results.push_back(result);
            //     std::cout << result.engineName << " - " 
            //               << (result.isProcessing ? "WORKING" : "NOT WORKING") << "\n";
            // } else {
                std::cout << "Empty\n";
            // }
        }
        
        if (!results.empty()) {
            EngineDiagnostic::printResults(results);
        }
    }
};