/**
 * Simple Engine Test Runner
 * Can be compiled as part of the plugin or standalone
 */

#include "EngineFactory.h"
#include "TestSignalGenerator.h"
#include "AudioMeasurements.h"
#include "EngineTestProtocols.h"
#include "EngineTestSuite.h"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    std::cout << "=========================================\n";
    std::cout << "Chimera Engine Test Suite v1.0\n";
    std::cout << "=========================================\n\n";
    
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juce_init;
    
    // Create test suite
    EngineTestSuite suite;
    
    // Setup callbacks
    suite.onProgress = [](int current, int total, const juce::String& engineName) {
        std::cout << "[" << current << "/" << total << "] Testing: " 
                 << engineName.toStdString() << "..." << std::flush;
    };
    
    suite.onEngineComplete = [](const EngineTestProtocols::EngineTestReport& report) {
        std::cout << " " << (report.overallPass ? "✓ PASSED" : "✗ FAILED") << std::endl;
        
        if (!report.overallPass) {
            for (const auto& result : report.results) {
                if (!result.passed) {
                    std::cout << "  ⚠ " << result.testName.toStdString() 
                             << ": " << result.notes.toStdString() << std::endl;
                }
            }
        }
    };
    
    // Test specific engines that we know exist
    std::cout << "Testing core engines...\n";
    std::cout << "-----------------------\n";
    
    // Test a selection of engines from different categories
    std::vector<std::pair<int, juce::String>> engines = {
        // Dynamics
        {15, "Classic Compressor"},
        {44, "Mastering Limiter"},
        
        // Filters  
        {16, "State Variable Filter"},
        {100, "Ladder Filter"},
        
        // Delays & Reverbs
        {2, "Tape Echo"},
        {3, "Plate Reverb"},
        
        // Modulation
        {6, "Classic Tremolo"},
        {17, "Stereo Chorus"},
        
        // Distortion
        {1, "K-Style Overdrive"},
        {4, "Rodent Distortion"},
        {5, "Muff Fuzz"}
    };
    
    int passed = 0;
    int failed = 0;
    
    for (const auto& [id, name] : engines) {
        std::cout << "Testing " << name.toStdString() << "... " << std::flush;
        
        auto engine = EngineFactory::createEngine(id);
        if (engine) {
            engine->prepareToPlay(48000.0, 512);
            auto report = EngineTestProtocols::runBasicTests(engine.get(), id);
            
            if (report.overallPass) {
                std::cout << "✓ PASSED";
                passed++;
            } else {
                std::cout << "✗ FAILED";
                failed++;
                
                // Show failed tests
                for (const auto& result : report.results) {
                    if (!result.passed) {
                        std::cout << "\n  ⚠ " << result.testName.toStdString();
                    }
                }
            }
            
            std::cout << " (CPU: " << report.cpuUsage << "%)" << std::endl;
        } else {
            std::cout << "✗ Could not create engine" << std::endl;
            failed++;
        }
    }
    
    std::cout << "\n=========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "-----------\n";
    std::cout << "Tested: " << (passed + failed) << " engines\n";
    std::cout << "Passed: " << passed << "\n";
    std::cout << "Failed: " << failed << "\n";
    std::cout << "Pass Rate: " << (passed * 100.0 / (passed + failed)) << "%\n";
    std::cout << "=========================================\n";
    
    // Generate simple HTML report
    std::ofstream html("engine_test_report.html");
    html << "<!DOCTYPE html><html><head><title>Chimera Engine Test Report</title>";
    html << "<style>body{font-family:Arial;margin:20px;} ";
    html << ".passed{color:green;} .failed{color:red;}</style></head><body>";
    html << "<h1>Chimera Engine Test Report</h1>";
    html << "<p>Date: " << juce::Time::getCurrentTime().toString(true, true).toStdString() << "</p>";
    html << "<h2>Summary</h2>";
    html << "<p>Tested: " << (passed + failed) << " engines<br>";
    html << "Passed: <span class='passed'>" << passed << "</span><br>";
    html << "Failed: <span class='failed'>" << failed << "</span><br>";
    html << "Pass Rate: " << (passed * 100.0 / (passed + failed)) << "%</p>";
    html << "</body></html>";
    html.close();
    
    std::cout << "\nHTML report saved to: engine_test_report.html\n";
    
    return failed > 0 ? 1 : 0;
}