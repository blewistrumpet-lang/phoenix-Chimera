#pragma once
#include <JuceHeader.h>
#include "EngineBase.h"
#include "EngineTestProtocols.h"
#include "EngineFactory.h"
#include <vector>
#include <memory>

/**
 * Automated Engine Test Suite
 * Comprehensive testing system for all Chimera engines
 */
class EngineTestSuite {
public:
    EngineTestSuite();
    
    // Test execution
    void runAllEngineTests();
    void runQuickTest(int engineID);
    void runCategoryTests(const juce::String& category);
    
    // Results management
    struct TestSummary {
        int totalEngines;
        int passedEngines;
        int failedEngines;
        float averageCPU;
        std::vector<EngineTestProtocols::EngineTestReport> reports;
        
        float getPassRate() const { 
            return totalEngines > 0 ? (float)passedEngines / totalEngines * 100.0f : 0.0f; 
        }
    };
    
    TestSummary getLastTestSummary() const { return m_lastSummary; }
    
    // Report generation
    void generateHTMLReport(const juce::File& outputFile);
    void generateTextReport(const juce::File& outputFile);
    void generateJSONReport(const juce::File& outputFile);
    
    // Callbacks for progress reporting
    std::function<void(int current, int total, const juce::String& engineName)> onProgress;
    std::function<void(const EngineTestProtocols::EngineTestReport& report)> onEngineComplete;
    std::function<void(const TestSummary& summary)> onTestComplete;
    
private:
    TestSummary m_lastSummary;
    
    // Test a single engine
    EngineTestProtocols::EngineTestReport testEngine(int engineID);
    
    // HTML report helpers
    juce::String generateHTMLHeader();
    juce::String generateHTMLEngineReport(const EngineTestProtocols::EngineTestReport& report);
    juce::String generateHTMLSummary(const TestSummary& summary);
    juce::String generateHTMLFooter();
    
    // Color coding for results
    juce::String getColorForResult(bool passed);
    juce::String getColorForValue(float value, float min, float max);
};