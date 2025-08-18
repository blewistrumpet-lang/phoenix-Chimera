#include "EngineTestSuite.h"
#include "EngineTypes.h"
#include "ParameterDefinitions.h"
#include <sstream>
#include <iomanip>

EngineTestSuite::EngineTestSuite() {
    m_lastSummary = TestSummary{0, 0, 0, 0.0f, {}};
}

void EngineTestSuite::runAllEngineTests() {
    m_lastSummary = TestSummary{0, 0, 0, 0.0f, {}};
    
    // Test all valid engine IDs
    std::vector<int> engineIDs;
    
    // Add all engine IDs from EngineTypes.h
    for (int id = 0; id < ENGINE_COUNT; ++id) {
        // Try to create the engine to see if it's valid
        auto engine = EngineFactory::createEngine(id);
        if (engine) {
            engineIDs.push_back(id);
        }
    }
    
    m_lastSummary.totalEngines = engineIDs.size();
    float totalCPU = 0.0f;
    
    for (size_t i = 0; i < engineIDs.size(); ++i) {
        int engineID = engineIDs[i];
        
        // Report progress
        if (onProgress) {
            auto engine = EngineFactory::createEngine(engineID);
            onProgress(i + 1, engineIDs.size(), engine->getName());
        }
        
        // Run test
        auto report = testEngine(engineID);
        m_lastSummary.reports.push_back(report);
        
        // Update stats
        if (report.overallPass) {
            m_lastSummary.passedEngines++;
        } else {
            m_lastSummary.failedEngines++;
        }
        totalCPU += report.cpuUsage;
        
        // Report completion
        if (onEngineComplete) {
            onEngineComplete(report);
        }
    }
    
    m_lastSummary.averageCPU = totalCPU / engineIDs.size();
    
    // Final callback
    if (onTestComplete) {
        onTestComplete(m_lastSummary);
    }
}

void EngineTestSuite::runQuickTest(int engineID) {
    m_lastSummary = TestSummary{1, 0, 0, 0.0f, {}};
    
    auto report = testEngine(engineID);
    m_lastSummary.reports.push_back(report);
    
    if (report.overallPass) {
        m_lastSummary.passedEngines = 1;
    } else {
        m_lastSummary.failedEngines = 1;
    }
    
    m_lastSummary.averageCPU = report.cpuUsage;
    
    if (onEngineComplete) {
        onEngineComplete(report);
    }
    
    if (onTestComplete) {
        onTestComplete(m_lastSummary);
    }
}

void EngineTestSuite::runCategoryTests(const juce::String& category) {
    m_lastSummary = TestSummary{0, 0, 0, 0.0f, {}};
    
    std::vector<int> engineIDs;
    
    // Filter engines by category
    if (category == "Dynamics") {
        engineIDs = {ENGINE_VCA_COMPRESSOR, ENGINE_OPTO_COMPRESSOR, 
                    ENGINE_MASTERING_LIMITER, ENGINE_NOISE_GATE};
    } else if (category == "Filters") {
        engineIDs = {ENGINE_LADDER_FILTER, ENGINE_STATE_VARIABLE_FILTER,
                    ENGINE_FORMANT_FILTER, ENGINE_ENVELOPE_FILTER,
                    ENGINE_PARAMETRIC_EQ, ENGINE_VINTAGE_CONSOLE_EQ, ENGINE_DYNAMIC_EQ};
    } else if (category == "Delays") {
        engineIDs = {ENGINE_TAPE_ECHO, ENGINE_DIGITAL_DELAY,
                    ENGINE_BUCKET_BRIGADE_DELAY, ENGINE_MAGNETIC_DRUM_ECHO};
    } else if (category == "Reverbs") {
        engineIDs = {ENGINE_PLATE_REVERB, ENGINE_CONVOLUTION_REVERB,
                    ENGINE_SHIMMER_REVERB, ENGINE_GATED_REVERB,
                    ENGINE_SPRING_REVERB, ENGINE_FEEDBACK_NETWORK};
    } else if (category == "Modulation") {
        engineIDs = {ENGINE_DIGITAL_CHORUS, ENGINE_ANALOG_PHASER,
                    ENGINE_CLASSIC_TREMOLO, ENGINE_HARMONIC_TREMOLO,
                    ENGINE_ROTARY_SPEAKER, ENGINE_RESONANT_CHORUS,
                    ENGINE_DETUNE_DOUBLER};
    } else if (category == "Distortion") {
        engineIDs = {ENGINE_K_STYLE, ENGINE_RODENT_DISTORTION,
                    ENGINE_MUFF_FUZZ, ENGINE_VINTAGE_TUBE,
                    ENGINE_MULTIBAND_SATURATOR, ENGINE_WAVE_FOLDER,
                    ENGINE_BIT_CRUSHER};
    }
    
    m_lastSummary.totalEngines = engineIDs.size();
    float totalCPU = 0.0f;
    
    for (size_t i = 0; i < engineIDs.size(); ++i) {
        int engineID = engineIDs[i];
        
        if (onProgress) {
            auto engine = EngineFactory::createEngine(engineID);
            if (engine) {
                onProgress(i + 1, engineIDs.size(), engine->getName());
            }
        }
        
        auto report = testEngine(engineID);
        m_lastSummary.reports.push_back(report);
        
        if (report.overallPass) {
            m_lastSummary.passedEngines++;
        } else {
            m_lastSummary.failedEngines++;
        }
        totalCPU += report.cpuUsage;
        
        if (onEngineComplete) {
            onEngineComplete(report);
        }
    }
    
    if (m_lastSummary.totalEngines > 0) {
        m_lastSummary.averageCPU = totalCPU / m_lastSummary.totalEngines;
    }
    
    if (onTestComplete) {
        onTestComplete(m_lastSummary);
    }
}

EngineTestProtocols::EngineTestReport EngineTestSuite::testEngine(int engineID) {
    auto engine = EngineFactory::createEngine(engineID);
    
    if (!engine) {
        EngineTestProtocols::EngineTestReport report;
        report.engineName = "Unknown";
        report.engineID = engineID;
        report.overallPass = false;
        report.addResult("Engine Creation", false, 0.0f, 0.0f, 0.0f, "Failed to create engine");
        return report;
    }
    
    // Prepare the engine
    engine->prepareToPlay(48000.0, 512);
    
    // Run comprehensive test
    return EngineTestProtocols::runComprehensiveTest(engine.get(), engineID);
}

void EngineTestSuite::generateHTMLReport(const juce::File& outputFile) {
    std::stringstream html;
    
    html << generateHTMLHeader();
    html << generateHTMLSummary(m_lastSummary);
    
    // Individual engine reports
    html << "<h2>Detailed Results</h2>\n";
    for (const auto& report : m_lastSummary.reports) {
        html << generateHTMLEngineReport(report);
    }
    
    html << generateHTMLFooter();
    
    outputFile.replaceWithText(html.str());
}

void EngineTestSuite::generateTextReport(const juce::File& outputFile) {
    std::stringstream text;
    
    text << "CHIMERA ENGINE TEST REPORT\n";
    text << "==========================\n\n";
    
    text << "Summary:\n";
    text << "--------\n";
    text << "Total Engines: " << m_lastSummary.totalEngines << "\n";
    text << "Passed: " << m_lastSummary.passedEngines << "\n";
    text << "Failed: " << m_lastSummary.failedEngines << "\n";
    text << "Pass Rate: " << std::fixed << std::setprecision(1) 
         << m_lastSummary.getPassRate() << "%\n";
    text << "Average CPU: " << std::fixed << std::setprecision(2) 
         << m_lastSummary.averageCPU << "%\n\n";
    
    // Individual results
    for (const auto& report : m_lastSummary.reports) {
        text << "Engine: " << report.engineName.toStdString() 
             << " (ID: " << report.engineID << ")\n";
        text << "Status: " << (report.overallPass ? "PASSED" : "FAILED") << "\n";
        text << "CPU Usage: " << std::fixed << std::setprecision(2) 
             << report.cpuUsage << "%\n";
        
        for (const auto& result : report.results) {
            text << "  " << result.testName.toStdString() << ": ";
            text << (result.passed ? "PASS" : "FAIL");
            text << " (" << result.notes.toStdString() << ")\n";
        }
        text << "\n";
    }
    
    outputFile.replaceWithText(text.str());
}

void EngineTestSuite::generateJSONReport(const juce::File& outputFile) {
    juce::DynamicObject::Ptr root = new juce::DynamicObject();
    
    // Summary
    juce::DynamicObject::Ptr summary = new juce::DynamicObject();
    summary->setProperty("totalEngines", m_lastSummary.totalEngines);
    summary->setProperty("passedEngines", m_lastSummary.passedEngines);
    summary->setProperty("failedEngines", m_lastSummary.failedEngines);
    summary->setProperty("passRate", m_lastSummary.getPassRate());
    summary->setProperty("averageCPU", m_lastSummary.averageCPU);
    root->setProperty("summary", summary.get());
    
    // Engine reports
    juce::Array<juce::var> engines;
    for (const auto& report : m_lastSummary.reports) {
        juce::DynamicObject::Ptr engine = new juce::DynamicObject();
        engine->setProperty("name", report.engineName);
        engine->setProperty("id", report.engineID);
        engine->setProperty("passed", report.overallPass);
        engine->setProperty("cpuUsage", report.cpuUsage);
        engine->setProperty("latency", report.latency);
        
        juce::Array<juce::var> tests;
        for (const auto& result : report.results) {
            juce::DynamicObject::Ptr test = new juce::DynamicObject();
            test->setProperty("name", result.testName);
            test->setProperty("passed", result.passed);
            test->setProperty("value", result.measuredValue);
            test->setProperty("min", result.expectedMin);
            test->setProperty("max", result.expectedMax);
            test->setProperty("notes", result.notes);
            tests.add(test.get());
        }
        engine->setProperty("tests", tests);
        engines.add(engine.get());
    }
    root->setProperty("engines", engines);
    
    // Write JSON
    juce::var json(root.get());
    outputFile.replaceWithText(juce::JSON::toString(json, true));
}

juce::String EngineTestSuite::generateHTMLHeader() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <title>Chimera Engine Test Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        h1 { color: #333; border-bottom: 3px solid #007acc; padding-bottom: 10px; }
        h2 { color: #555; margin-top: 30px; }
        .summary { background: white; padding: 20px; border-radius: 8px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .engine-report { background: white; padding: 15px; margin: 15px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .passed { color: #28a745; font-weight: bold; }
        .failed { color: #dc3545; font-weight: bold; }
        .warning { color: #ffc107; }
        table { width: 100%; border-collapse: collapse; margin: 10px 0; }
        th { background: #007acc; color: white; padding: 10px; text-align: left; }
        td { padding: 8px; border-bottom: 1px solid #ddd; }
        tr:hover { background: #f9f9f9; }
        .progress-bar { width: 200px; height: 20px; background: #e0e0e0; border-radius: 10px; overflow: hidden; }
        .progress-fill { height: 100%; background: linear-gradient(90deg, #28a745, #007acc); }
        .stat-card { display: inline-block; background: white; padding: 15px; margin: 10px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .stat-value { font-size: 2em; font-weight: bold; color: #007acc; }
        .stat-label { color: #666; margin-top: 5px; }
    </style>
</head>
<body>
    <h1>🎵 Chimera Engine Test Report</h1>
    <p>Generated: )" + juce::Time::getCurrentTime().toString(true, true) + R"(</p>
)";
}

juce::String EngineTestSuite::generateHTMLSummary(const TestSummary& summary) {
    std::stringstream html;
    
    html << "<div class='summary'>\n";
    html << "<h2>Test Summary</h2>\n";
    
    html << "<div style='text-align: center;'>\n";
    
    // Stat cards
    html << "<div class='stat-card'>\n";
    html << "<div class='stat-value'>" << summary.totalEngines << "</div>\n";
    html << "<div class='stat-label'>Total Engines</div>\n";
    html << "</div>\n";
    
    html << "<div class='stat-card'>\n";
    html << "<div class='stat-value' style='color: #28a745;'>" << summary.passedEngines << "</div>\n";
    html << "<div class='stat-label'>Passed</div>\n";
    html << "</div>\n";
    
    html << "<div class='stat-card'>\n";
    html << "<div class='stat-value' style='color: #dc3545;'>" << summary.failedEngines << "</div>\n";
    html << "<div class='stat-label'>Failed</div>\n";
    html << "</div>\n";
    
    html << "<div class='stat-card'>\n";
    html << "<div class='stat-value'>" << std::fixed << std::setprecision(1) 
         << summary.getPassRate() << "%</div>\n";
    html << "<div class='stat-label'>Pass Rate</div>\n";
    html << "</div>\n";
    
    html << "<div class='stat-card'>\n";
    html << "<div class='stat-value'>" << std::fixed << std::setprecision(2) 
         << summary.averageCPU << "%</div>\n";
    html << "<div class='stat-label'>Avg CPU</div>\n";
    html << "</div>\n";
    
    html << "</div>\n"; // center div
    
    // Progress bar
    html << "<div style='margin: 20px auto; width: 400px;'>\n";
    html << "<div class='progress-bar'>\n";
    html << "<div class='progress-fill' style='width: " 
         << summary.getPassRate() << "%;'></div>\n";
    html << "</div>\n";
    html << "</div>\n";
    
    html << "</div>\n"; // summary div
    
    return html.str();
}

juce::String EngineTestSuite::generateHTMLEngineReport(const EngineTestProtocols::EngineTestReport& report) {
    std::stringstream html;
    
    html << "<div class='engine-report'>\n";
    html << "<h3>" << report.engineName.toStdString() 
         << " <span style='color: #666;'>(ID: " << report.engineID << ")</span></h3>\n";
    
    html << "<p>Status: <span class='" 
         << (report.overallPass ? "passed'>PASSED" : "failed'>FAILED")
         << "</span></p>\n";
    
    html << "<p>CPU Usage: " << std::fixed << std::setprecision(2) 
         << report.cpuUsage << "%";
    if (report.cpuUsage > 5.0f) {
        html << " <span class='warning'>⚠️ High CPU</span>";
    }
    html << "</p>\n";
    
    // Test results table
    html << "<table>\n";
    html << "<tr><th>Test</th><th>Result</th><th>Value</th><th>Expected</th><th>Notes</th></tr>\n";
    
    for (const auto& result : report.results) {
        html << "<tr>\n";
        html << "<td>" << result.testName.toStdString() << "</td>\n";
        html << "<td class='" << (result.passed ? "passed" : "failed") << "'>"
             << (result.passed ? "✓ PASS" : "✗ FAIL") << "</td>\n";
        html << "<td>" << std::fixed << std::setprecision(2) 
             << result.measuredValue << "</td>\n";
        
        if (result.expectedMin != 0 || result.expectedMax != 0) {
            html << "<td>" << result.expectedMin << " - " << result.expectedMax << "</td>\n";
        } else {
            html << "<td>-</td>\n";
        }
        
        html << "<td>" << result.notes.toStdString() << "</td>\n";
        html << "</tr>\n";
    }
    
    html << "</table>\n";
    html << "</div>\n";
    
    return html.str();
}

juce::String EngineTestSuite::generateHTMLFooter() {
    return R"(
    <footer style='margin-top: 50px; padding: 20px; text-align: center; color: #666;'>
        <p>Chimera Audio Engine Test Suite v1.0</p>
        <p>© 2024 Chimera Audio - All Rights Reserved</p>
    </footer>
</body>
</html>
)";
}

juce::String EngineTestSuite::getColorForResult(bool passed) {
    return passed ? "#28a745" : "#dc3545";
}

juce::String EngineTestSuite::getColorForValue(float value, float min, float max) {
    if (value < min) return "#dc3545"; // Red
    if (value > max) return "#ffc107"; // Yellow
    return "#28a745"; // Green
}