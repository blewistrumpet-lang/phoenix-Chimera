/**
 * Test All Engines - Command Line Test Runner
 * Runs comprehensive tests on all Chimera engines and generates reports
 */

#include <JuceHeader.h>
#include "EngineTestSuite.h"
#include "EngineFactory.h"
#include <iostream>
#include <iomanip>

class TestRunner : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Chimera Engine Tester"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    
    void initialise(const juce::String& commandLine) override {
        // Parse command line arguments
        juce::StringArray args;
        args.addTokens(commandLine, true);
        
        bool quickMode = args.contains("--quick");
        bool htmlReport = args.contains("--html");
        bool jsonReport = args.contains("--json");
        juce::String category = "";
        
        // Check for category filter
        for (int i = 0; i < args.size() - 1; ++i) {
            if (args[i] == "--category") {
                category = args[i + 1];
            }
        }
        
        // Create test suite
        EngineTestSuite suite;
        
        // Set up progress callbacks
        suite.onProgress = [](int current, int total, const juce::String& engineName) {
            std::cout << "\r[" << current << "/" << total << "] Testing: " 
                     << std::left << std::setw(30) << engineName.toStdString() << std::flush;
        };
        
        suite.onEngineComplete = [](const EngineTestProtocols::EngineTestReport& report) {
            std::cout << " - " << (report.overallPass ? "✓ PASSED" : "✗ FAILED") 
                     << " (CPU: " << std::fixed << std::setprecision(2) 
                     << report.cpuUsage << "%)" << std::endl;
        };
        
        suite.onTestComplete = [this](const EngineTestSuite::TestSummary& summary) {
            std::cout << "\n" << std::string(60, '=') << std::endl;
            std::cout << "TEST COMPLETE" << std::endl;
            std::cout << std::string(60, '=') << std::endl;
            std::cout << "Total Engines: " << summary.totalEngines << std::endl;
            std::cout << "Passed: " << summary.passedEngines << std::endl;
            std::cout << "Failed: " << summary.failedEngines << std::endl;
            std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) 
                     << summary.getPassRate() << "%" << std::endl;
            std::cout << "Average CPU: " << std::fixed << std::setprecision(2) 
                     << summary.averageCPU << "%" << std::endl;
            std::cout << std::string(60, '=') << std::endl;
        };
        
        // Run tests
        std::cout << "Starting Chimera Engine Tests..." << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        if (category.isNotEmpty()) {
            std::cout << "Testing category: " << category << std::endl;
            suite.runCategoryTests(category);
        } else if (quickMode) {
            std::cout << "Running quick tests (basic only)..." << std::endl;
            // Test just a few engines for quick validation
            suite.runQuickTest(ENGINE_K_STYLE);
            suite.runQuickTest(ENGINE_PLATE_REVERB);
            suite.runQuickTest(ENGINE_DIGITAL_CHORUS);
        } else {
            std::cout << "Running comprehensive tests on all engines..." << std::endl;
            suite.runAllEngineTests();
        }
        
        // Generate reports
        juce::File reportDir = juce::File::getCurrentWorkingDirectory().getChildFile("test_reports");
        reportDir.createDirectory();
        
        auto timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
        
        // Always generate text report
        juce::File textReport = reportDir.getChildFile("test_report_" + timestamp + ".txt");
        suite.generateTextReport(textReport);
        std::cout << "\nText report saved to: " << textReport.getFullPathName() << std::endl;
        
        if (htmlReport) {
            juce::File htmlFile = reportDir.getChildFile("test_report_" + timestamp + ".html");
            suite.generateHTMLReport(htmlFile);
            std::cout << "HTML report saved to: " << htmlFile.getFullPathName() << std::endl;
            
            // Open in browser
            htmlFile.startAsProcess();
        }
        
        if (jsonReport) {
            juce::File jsonFile = reportDir.getChildFile("test_report_" + timestamp + ".json");
            suite.generateJSONReport(jsonFile);
            std::cout << "JSON report saved to: " << jsonFile.getFullPathName() << std::endl;
        }
        
        // Exit with appropriate code
        int exitCode = suite.getLastTestSummary().failedEngines > 0 ? 1 : 0;
        quit();
        setApplicationReturnValue(exitCode);
    }
    
    void shutdown() override {
        std::cout << "Test runner shutdown." << std::endl;
    }
};

// Create the application instance
START_JUCE_APPLICATION(TestRunner)