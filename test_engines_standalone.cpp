// Standalone test runner for Chimera engines
// Compile with: clang++ -std=c++17 test_engines_standalone.cpp -o test_engines \
//   -I ~/JUCE/modules -framework CoreAudio -framework CoreMIDI -framework AudioToolbox \
//   -framework Accelerate -framework CoreFoundation -framework AppKit

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1

// Include necessary JUCE modules inline
#include "~/JUCE/modules/juce_core/juce_core.cpp"
#include "~/JUCE/modules/juce_audio_basics/juce_audio_basics.cpp"
#include "~/JUCE/modules/juce_audio_processors/juce_audio_processors.cpp"
#include "~/JUCE/modules/juce_graphics/juce_graphics.cpp"
#include "~/JUCE/modules/juce_gui_basics/juce_gui_basics.cpp"
#include "~/JUCE/modules/juce_events/juce_events.cpp"
#include "~/JUCE/modules/juce_data_structures/juce_data_structures.cpp"

// Now include our test files
#include "JUCE_Plugin/Source/TestSignalGenerator.cpp"
#include "JUCE_Plugin/Source/AudioMeasurements.cpp"
#include "JUCE_Plugin/Source/EngineTestProtocols.cpp"
#include "JUCE_Plugin/Source/EngineTestSuite.cpp"
#include "JUCE_Plugin/Source/EngineFactory.cpp"

// Include all engines
#include "JUCE_Plugin/Source/KStyleOverdrive.cpp"
#include "JUCE_Plugin/Source/TapeEcho.cpp"
#include "JUCE_Plugin/Source/PlateReverb.cpp"
#include "JUCE_Plugin/Source/RodentDistortion.cpp"
#include "JUCE_Plugin/Source/MuffFuzz.cpp"
#include "JUCE_Plugin/Source/ClassicTremolo.cpp"
#include "JUCE_Plugin/Source/MagneticDrumEcho.cpp"
#include "JUCE_Plugin/Source/BucketBrigadeDelay.cpp"
#include "JUCE_Plugin/Source/DigitalDelay.cpp"
#include "JUCE_Plugin/Source/HarmonicTremolo.cpp"
#include "JUCE_Plugin/Source/RotarySpeaker.cpp"
#include "JUCE_Plugin/Source/DetuneDoubler.cpp"
#include "JUCE_Plugin/Source/LadderFilter.cpp"
#include "JUCE_Plugin/Source/FormantFilter.cpp"
#include "JUCE_Plugin/Source/ClassicCompressor.cpp"
#include "JUCE_Plugin/Source/StateVariableFilter.cpp"
#include "JUCE_Plugin/Source/StereoChorus.cpp"
#include "JUCE_Plugin/Source/SpectralFreeze.cpp"
#include "JUCE_Plugin/Source/GranularCloud.cpp"
#include "JUCE_Plugin/Source/AnalogRingModulator.cpp"
#include "JUCE_Plugin/Source/MultibandSaturator.cpp"
#include "JUCE_Plugin/Source/CombResonator.cpp"
#include "JUCE_Plugin/Source/PitchShifter.cpp"
#include "JUCE_Plugin/Source/PhasedVocoder.cpp"
#include "JUCE_Plugin/Source/ConvolutionReverb.cpp"
#include "JUCE_Plugin/Source/BitCrusher.cpp"
#include "JUCE_Plugin/Source/FrequencyShifter.cpp"
#include "JUCE_Plugin/Source/WaveFolder.cpp"
#include "JUCE_Plugin/Source/ShimmerReverb.cpp"
#include "JUCE_Plugin/Source/VocalFormantFilter.cpp"
#include "JUCE_Plugin/Source/TransientShaper.cpp"
#include "JUCE_Plugin/Source/DimensionExpander.cpp"
#include "JUCE_Plugin/Source/AnalogPhaser.cpp"
#include "JUCE_Plugin/Source/EnvelopeFilter.cpp"
#include "JUCE_Plugin/Source/GatedReverb.cpp"
#include "JUCE_Plugin/Source/HarmonicExciter.cpp"
#include "JUCE_Plugin/Source/FeedbackNetwork.cpp"
#include "JUCE_Plugin/Source/IntelligentHarmonizer.cpp"
#include "JUCE_Plugin/Source/ParametricEQ.cpp"
#include "JUCE_Plugin/Source/MasteringLimiter.cpp"
#include "JUCE_Plugin/Source/NoiseGate.cpp"
#include "JUCE_Plugin/Source/VintageOptoCompressor.cpp"
#include "JUCE_Plugin/Source/SpectralGate.cpp"
#include "JUCE_Plugin/Source/ChaosGenerator.cpp"
#include "JUCE_Plugin/Source/BufferRepeat.cpp"
#include "JUCE_Plugin/Source/VintageConsoleEQ.cpp"
#include "JUCE_Plugin/Source/MidSideProcessor.cpp"
#include "JUCE_Plugin/Source/VintageTubePreamp.cpp"
#include "JUCE_Plugin/Source/SpringReverb.cpp"
#include "JUCE_Plugin/Source/ResonantChorus.cpp"
#include "JUCE_Plugin/Source/StereoWidener.cpp"
#include "JUCE_Plugin/Source/DynamicEQ.cpp"
#include "JUCE_Plugin/Source/StereoImager.cpp"

#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;
    
    std::cout << "=========================================\n";
    std::cout << "Chimera Engine Test Suite v1.0\n";  
    std::cout << "=========================================\n\n";
    
    // Create test suite
    EngineTestSuite suite;
    
    // Setup progress callbacks
    suite.onProgress = [](int current, int total, const juce::String& engineName) {
        std::cout << "\r[" << std::setw(3) << current << "/" << std::setw(3) << total << "] "
                 << "Testing: " << std::left << std::setw(30) << engineName.toStdString() 
                 << std::flush;
    };
    
    suite.onEngineComplete = [](const EngineTestProtocols::EngineTestReport& report) {
        std::cout << " " << (report.overallPass ? "✓ PASS" : "✗ FAIL")
                 << " (CPU: " << std::fixed << std::setprecision(2) 
                 << report.cpuUsage << "%)" << std::endl;
    };
    
    // Run all tests
    std::cout << "Running comprehensive tests on all engines...\n";
    std::cout << "---------------------------------------------\n";
    suite.runAllEngineTests();
    
    // Get summary
    auto summary = suite.getLastTestSummary();
    
    // Print summary
    std::cout << "\n=========================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "=========================================\n";
    std::cout << "Total Engines: " << summary.totalEngines << std::endl;
    std::cout << "Passed: " << summary.passedEngines << std::endl;
    std::cout << "Failed: " << summary.failedEngines << std::endl;
    std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) 
             << summary.getPassRate() << "%" << std::endl;
    std::cout << "Average CPU: " << std::fixed << std::setprecision(2)
             << summary.averageCPU << "%" << std::endl;
    
    // Generate reports
    juce::File reportDir = juce::File::getCurrentWorkingDirectory().getChildFile("test_reports");
    reportDir.createDirectory();
    
    auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    
    // Generate HTML report
    juce::File htmlReport = reportDir.getChildFile("test_report_" + timestamp + ".html");
    suite.generateHTMLReport(htmlReport);
    std::cout << "\nHTML report saved to: " << htmlReport.getFullPathName() << std::endl;
    
    // Generate text report
    juce::File textReport = reportDir.getChildFile("test_report_" + timestamp + ".txt");
    suite.generateTextReport(textReport);
    std::cout << "Text report saved to: " << textReport.getFullPathName() << std::endl;
    
    // Open HTML report in browser
    htmlReport.startAsProcess();
    
    return summary.failedEngines > 0 ? 1 : 0;
}