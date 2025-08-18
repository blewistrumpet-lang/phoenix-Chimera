#include "ComprehensiveTestHarness.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace ChimeraTestHarness {

    // Report generation methods
    void ComprehensiveTestHarness::generateSummaryReport(const TestSuiteResults& results, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            logMessage("Failed to create summary report: " + filename);
            return;
        }
        
        file << "CHIMERA PHOENIX COMPREHENSIVE TEST HARNESS - SUMMARY REPORT\n";
        file << std::string(80, '=') << "\n";
        file << "Generated: " << std::chrono::system_clock::now() << "\n";
        file << "Test Duration: " << ReportUtils::formatDuration(results.totalExecutionTime) << "\n\n";
        
        // Summary statistics
        file << "SUMMARY STATISTICS\n";
        file << std::string(40, '-') << "\n";
        file << std::setw(30) << std::left << "Total Engines Tested:" << results.totalEngines << "\n";
        file << std::setw(30) << std::left << "Working Engines:" << results.workingEngines << "\n";
        file << std::setw(30) << std::left << "Failed to Create:" << results.failedEngines << "\n";
        file << std::setw(30) << std::left << "Critical Issues:" << results.enginesWithCriticalIssues << "\n";
        file << std::setw(30) << std::left << "Errors:" << results.enginesWithErrors << "\n";
        file << std::setw(30) << std::left << "Warnings:" << results.enginesWithWarnings << "\n";
        file << std::setw(30) << std::left << "Average Score:" << ReportUtils::formatScore(results.averageScore) << "\n";
        file << std::setw(30) << std::left << "Average CPU Usage:" << ReportUtils::formatPercentage(results.averageCpuUsage) << "\n";
        file << std::setw(30) << std::left << "Worst CPU Usage:" << ReportUtils::formatPercentage(results.worstCpuUsage) << "\n\n";
        
        // Quick overview table
        file << "QUICK OVERVIEW\n";
        file << std::string(40, '-') << "\n";
        file << std::setw(4) << "ID" << std::setw(25) << "Engine Name" 
             << std::setw(8) << "Score" << std::setw(8) << "CPU%" 
             << std::setw(12) << "Status" << "\n";
        file << std::string(57, '-') << "\n";
        
        for (const auto& engine : results.engineResults) {
            if (!engine.engineCreated) {
                file << std::setw(4) << engine.engineID 
                     << std::setw(25) << engine.engineName.substr(0, 24)
                     << std::setw(8) << "FAIL"
                     << std::setw(8) << "N/A"
                     << std::setw(12) << "NO CREATE" << "\n";
                continue;
            }
            
            std::string status;
            if (engine.criticalIssues > 0) status = "CRITICAL";
            else if (engine.errorIssues > 0) status = "ERROR";
            else if (engine.warningIssues > 0) status = "WARNING";
            else status = "PASS";
            
            file << std::setw(4) << engine.engineID 
                 << std::setw(25) << engine.engineName.substr(0, 24)
                 << std::setw(8) << ReportUtils::formatScore(engine.overallScore)
                 << std::setw(8) << ReportUtils::formatPercentage(engine.avgCpuUsage)
                 << std::setw(12) << status << "\n";
        }
        
        // Problematic engines section
        auto problematic = results.getProblematicEngines();
        if (!problematic.empty()) {
            file << "\nPROBLEMATIC ENGINES (PRIORITY ORDER)\n";
            file << std::string(40, '-') << "\n";
            
            for (size_t i = 0; i < std::min(size_t(15), problematic.size()); ++i) {
                const auto& engine = problematic[i];
                file << std::setw(3) << engine.engineID << ": " << engine.engineName << "\n";
                
                if (engine.criticalIssues > 0) {
                    file << "    Critical Issues: " << engine.criticalIssues << "\n";
                }
                if (engine.errorIssues > 0) {
                    file << "    Errors: " << engine.errorIssues << "\n";
                }
                if (engine.warningIssues > 0) {
                    file << "    Warnings: " << engine.warningIssues << "\n";
                }
                
                // Show top 3 recommendations
                auto recommendations = engine.getPrioritizedRecommendations();
                if (!recommendations.empty()) {
                    file << "    Top Recommendations:\n";
                    for (size_t j = 0; j < std::min(size_t(3), recommendations.size()); ++j) {
                        file << "    - " << recommendations[j] << "\n";
                    }
                }
                file << "\n";
            }
        }
        
        // Performance insights
        file << "PERFORMANCE INSIGHTS\n";
        file << std::string(40, '-') << "\n";
        
        // Find performance outliers
        std::vector<std::pair<float, std::string>> cpuRanking;
        for (const auto& engine : results.engineResults) {
            if (engine.engineCreated) {
                cpuRanking.push_back({engine.maxCpuUsage, engine.engineName});
            }
        }
        std::sort(cpuRanking.rbegin(), cpuRanking.rend());
        
        file << "Highest CPU Usage Engines:\n";
        for (size_t i = 0; i < std::min(size_t(5), cpuRanking.size()); ++i) {
            file << "  " << (i+1) << ". " << cpuRanking[i].second 
                 << " (" << ReportUtils::formatPercentage(cpuRanking[i].first) << ")\n";
        }
        
        file.close();
        logMessage("Summary report generated: " + filename);
    }
    
    void ComprehensiveTestHarness::generateDetailedReport(const TestSuiteResults& results, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            logMessage("Failed to create detailed report: " + filename);
            return;
        }
        
        file << "CHIMERA PHOENIX COMPREHENSIVE TEST HARNESS - DETAILED REPORT\n";
        file << std::string(80, '=') << "\n";
        file << "Generated: " << std::chrono::system_clock::now() << "\n";
        file << "Test Configuration:\n";
        file << "  Sample Rate: " << m_sampleRate << " Hz\n";
        file << "  Block Size: " << m_blockSize << " samples\n";
        file << "  Test Duration: " << m_testDuration << " seconds\n";
        file << "  Parameter Sweep Steps: " << m_parameterSweepSteps << "\n\n";
        
        // Detailed results for each engine
        for (const auto& engine : results.engineResults) {
            file << std::string(80, '=') << "\n";
            file << "ENGINE #" << engine.engineID << ": " << engine.engineName << "\n";
            file << std::string(80, '=') << "\n";
            
            if (!engine.engineCreated) {
                file << "STATUS: FAILED TO CREATE ENGINE INSTANCE\n";
                file << "This is a critical issue that prevents any testing.\n";
                file << "Recommendations:\n";
                file << "- Check EngineFactory implementation\n";
                file << "- Verify engine class exists and compiles correctly\n";
                file << "- Check for missing dependencies or includes\n\n";
                continue;
            }
            
            file << "Overall Score: " << ReportUtils::formatScore(engine.overallScore) << "\n";
            file << "Test Duration: " << ReportUtils::formatDuration(engine.totalTestTime) << "\n";
            file << "Average CPU Usage: " << ReportUtils::formatPercentage(engine.avgCpuUsage) << "\n";
            file << "Peak CPU Usage: " << ReportUtils::formatPercentage(engine.maxCpuUsage) << "\n";
            file << "Average Latency: " << engine.avgLatencyMs << " ms\n";
            file << "Peak Latency: " << engine.maxLatencyMs << " ms\n\n";
            
            // Issue summary
            if (engine.criticalIssues > 0 || engine.errorIssues > 0 || engine.warningIssues > 0) {
                file << "ISSUES FOUND:\n";
                if (engine.criticalIssues > 0) file << "  Critical Issues: " << engine.criticalIssues << "\n";
                if (engine.errorIssues > 0) file << "  Errors: " << engine.errorIssues << "\n";
                if (engine.warningIssues > 0) file << "  Warnings: " << engine.warningIssues << "\n";
                file << "\n";
            }
            
            // Test category details
            auto writeTestCategory = [&](const TestCategory& category) {
                file << category.name << ":\n";
                file << "  Overall Score: " << ReportUtils::formatScore(category.overallScore) << "\n";
                file << "  All Tests Passed: " << (category.allPassed ? "YES" : "NO") << "\n";
                
                for (const auto& test : category.results) {
                    file << "  - " << test.testName << ": ";
                    if (test.passed) {
                        file << "PASS (" << ReportUtils::formatScore(test.score) << ")";
                    } else {
                        file << "FAIL (" << ReportUtils::severityToString(test.severity) << ")";
                    }
                    file << "\n";
                    
                    if (!test.message.empty()) {
                        file << "    Message: " << test.message << "\n";
                    }
                    
                    if (!test.details.empty()) {
                        file << "    Details: " << test.details << "\n";
                    }
                    
                    if (!test.metrics.empty()) {
                        file << "    Metrics: ";
                        bool first = true;
                        for (const auto& [key, value] : test.metrics) {
                            if (!first) file << ", ";
                            file << key << "=" << value;
                            first = false;
                        }
                        file << "\n";
                    }
                    
                    if (!test.recommendations.empty()) {
                        file << "    Recommendations:\n";
                        for (const auto& rec : test.recommendations) {
                            file << "      * " << rec << "\n";
                        }
                    }
                }
                file << "\n";
            };
            
            writeTestCategory(engine.parameterSweepTests);
            writeTestCategory(engine.safetyTests);
            writeTestCategory(engine.audioQualityTests);
            writeTestCategory(engine.performanceTests);
            writeTestCategory(engine.stabilityTests);
            
            // Prioritized recommendations for this engine
            auto recommendations = engine.getPrioritizedRecommendations();
            if (!recommendations.empty()) {
                file << "PRIORITIZED RECOMMENDATIONS:\n";
                for (size_t i = 0; i < recommendations.size(); ++i) {
                    file << "  " << (i+1) << ". " << recommendations[i] << "\n";
                }
                file << "\n";
            }
        }
        
        file.close();
        logMessage("Detailed report generated: " + filename);
    }
    
    void ComprehensiveTestHarness::generateHTMLReport(const TestSuiteResults& results, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            logMessage("Failed to create HTML report: " + filename);
            return;
        }
        
        // HTML header with CSS
        file << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Chimera Phoenix - Comprehensive Test Report</title>
    <style>
        body { 
            font-family: 'Segoe UI', Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background-color: #f5f5f5; 
        }
        .container { 
            max-width: 1200px; 
            margin: 0 auto; 
            background-color: white; 
            padding: 20px; 
            border-radius: 10px; 
            box-shadow: 0 4px 6px rgba(0,0,0,0.1);
        }
        h1 { 
            color: #2c3e50; 
            text-align: center; 
            border-bottom: 3px solid #3498db; 
            padding-bottom: 10px; 
        }
        h2 { 
            color: #34495e; 
            border-bottom: 2px solid #ecf0f1; 
            padding-bottom: 5px; 
        }
        .summary { 
            background: linear-gradient(135deg, #3498db, #2980b9); 
            color: white; 
            padding: 20px; 
            border-radius: 10px; 
            margin-bottom: 20px; 
        }
        .summary-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); 
            gap: 15px; 
        }
        .summary-item { 
            background: rgba(255,255,255,0.2); 
            padding: 15px; 
            border-radius: 8px; 
        }
        .summary-item h3 { 
            margin: 0 0 10px 0; 
            font-size: 18px; 
        }
        .summary-item .value { 
            font-size: 24px; 
            font-weight: bold; 
        }
        table { 
            width: 100%; 
            border-collapse: collapse; 
            margin: 20px 0; 
            background: white;
        }
        th, td { 
            border: 1px solid #ddd; 
            padding: 12px; 
            text-align: left; 
        }
        th { 
            background: #34495e; 
            color: white; 
            font-weight: bold;
        }
        tr:nth-child(even) { 
            background-color: #f8f9fa; 
        }
        tr:hover { 
            background-color: #e8f4f8; 
        }
        .status { 
            padding: 4px 8px; 
            border-radius: 4px; 
            color: white; 
            font-weight: bold; 
            text-align: center;
        }
        .status-pass { background-color: #27ae60; }
        .status-warning { background-color: #f39c12; }
        .status-error { background-color: #e74c3c; }
        .status-critical { background-color: #c0392b; }
        .status-no-create { background-color: #7f8c8d; }
        .progress-bar { 
            width: 100%; 
            background-color: #ecf0f1; 
            border-radius: 10px; 
            overflow: hidden; 
        }
        .progress-fill { 
            height: 20px; 
            border-radius: 10px; 
            background: linear-gradient(90deg, #e74c3c 0%, #f39c12 25%, #f1c40f 50%, #2ecc71 100%);
            transition: width 0.3s ease;
        }
        .engine-details { 
            margin: 20px 0; 
            border: 1px solid #ddd; 
            border-radius: 8px; 
            overflow: hidden;
        }
        .engine-header { 
            background: #34495e; 
            color: white; 
            padding: 15px; 
            font-weight: bold; 
            font-size: 18px;
        }
        .engine-content { 
            padding: 15px; 
        }
        .test-category { 
            margin: 15px 0; 
            border-left: 4px solid #3498db; 
            padding-left: 15px;
        }
        .test-item { 
            margin: 8px 0; 
            padding: 8px; 
            background: #f8f9fa; 
            border-radius: 4px;
        }
        .recommendations { 
            background: #fff3cd; 
            border: 1px solid #ffeaa7; 
            border-radius: 8px; 
            padding: 15px; 
            margin: 15px 0;
        }
        .recommendations h4 { 
            color: #856404; 
            margin-top: 0; 
        }
        .recommendations ul { 
            margin: 0; 
            padding-left: 20px; 
        }
        .recommendations li { 
            margin: 5px 0; 
            color: #856404; 
        }
        .chart-container { 
            margin: 20px 0; 
            text-align: center; 
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Chimera Phoenix - Comprehensive Test Report</h1>
        <p style="text-align: center; color: #7f8c8d;">Generated: )" << std::chrono::system_clock::now() << R"(</p>
)";
        
        // Summary section
        file << R"(        <div class="summary">
            <h2 style="color: white; border-bottom: 2px solid rgba(255,255,255,0.3);">Test Summary</h2>
            <div class="summary-grid">
                <div class="summary-item">
                    <h3>Total Engines</h3>
                    <div class="value">)" << results.totalEngines << R"(</div>
                </div>
                <div class="summary-item">
                    <h3>Working Engines</h3>
                    <div class="value">)" << results.workingEngines << R"(</div>
                </div>
                <div class="summary-item">
                    <h3>Failed to Create</h3>
                    <div class="value">)" << results.failedEngines << R"(</div>
                </div>
                <div class="summary-item">
                    <h3>Average Score</h3>
                    <div class="value">)" << std::fixed << std::setprecision(1) << results.averageScore << R"(%</div>
                </div>
                <div class="summary-item">
                    <h3>Critical Issues</h3>
                    <div class="value">)" << results.enginesWithCriticalIssues << R"(</div>
                </div>
                <div class="summary-item">
                    <h3>Average CPU</h3>
                    <div class="value">)" << std::fixed << std::setprecision(2) << results.averageCpuUsage << R"(%</div>
                </div>
            </div>
        </div>
)";
        
        // Main results table
        file << R"(        <h2>Engine Test Results</h2>
        <table>
            <thead>
                <tr>
                    <th>Engine ID</th>
                    <th>Engine Name</th>
                    <th>Score</th>
                    <th>CPU Usage</th>
                    <th>Issues</th>
                    <th>Status</th>
                </tr>
            </thead>
            <tbody>)";
        
        for (const auto& engine : results.engineResults) {
            file << "<tr>";
            file << "<td>" << engine.engineID << "</td>";
            file << "<td>" << ReportUtils::escapeHTML(engine.engineName) << "</td>";
            
            if (!engine.engineCreated) {
                file << "<td>N/A</td>";
                file << "<td>N/A</td>";
                file << "<td>Cannot create</td>";
                file << R"(<td><span class="status status-no-create">NO CREATE</span></td>)";
            } else {
                file << "<td>";
                file << "<div class=\"progress-bar\">";
                file << "<div class=\"progress-fill\" style=\"width: " << engine.overallScore << "%\"></div>";
                file << "</div>";
                file << std::fixed << std::setprecision(1) << engine.overallScore << "%";
                file << "</td>";
                
                file << "<td>" << std::fixed << std::setprecision(2) << engine.avgCpuUsage << "%</td>";
                
                file << "<td>";
                if (engine.criticalIssues > 0) file << engine.criticalIssues << " critical ";
                if (engine.errorIssues > 0) file << engine.errorIssues << " errors ";
                if (engine.warningIssues > 0) file << engine.warningIssues << " warnings";
                if (engine.criticalIssues == 0 && engine.errorIssues == 0 && engine.warningIssues == 0) {
                    file << "None";
                }
                file << "</td>";
                
                file << "<td>";
                if (engine.criticalIssues > 0) {
                    file << R"(<span class="status status-critical">CRITICAL</span>)";
                } else if (engine.errorIssues > 0) {
                    file << R"(<span class="status status-error">ERROR</span>)";
                } else if (engine.warningIssues > 0) {
                    file << R"(<span class="status status-warning">WARNING</span>)";
                } else {
                    file << R"(<span class="status status-pass">PASS</span>)";
                }
                file << "</td>";
            }
            
            file << "</tr>\n";
        }
        
        file << R"(            </tbody>
        </table>)";
        
        // Problematic engines section
        auto problematic = results.getProblematicEngines();
        if (!problematic.empty()) {
            file << R"(        <h2>Problematic Engines (Priority Order)</h2>)";
            
            for (size_t i = 0; i < std::min(size_t(10), problematic.size()); ++i) {
                const auto& engine = problematic[i];
                
                file << R"(        <div class="engine-details">
            <div class="engine-header">
                Engine #)" << engine.engineID << ": " << ReportUtils::escapeHTML(engine.engineName) << R"(
            </div>
            <div class="engine-content">
                <p><strong>Score:</strong> )" << std::fixed << std::setprecision(1) << engine.overallScore << R"(%</p>
                <p><strong>Issues:</strong> )";
                
                if (engine.criticalIssues > 0) file << engine.criticalIssues << " critical, ";
                if (engine.errorIssues > 0) file << engine.errorIssues << " errors, ";
                if (engine.warningIssues > 0) file << engine.warningIssues << " warnings";
                
                file << R"(</p>)";
                
                auto recommendations = engine.getPrioritizedRecommendations();
                if (!recommendations.empty()) {
                    file << R"(                <div class="recommendations">
                    <h4>Top Recommendations:</h4>
                    <ul>)";
                    
                    for (size_t j = 0; j < std::min(size_t(5), recommendations.size()); ++j) {
                        file << "<li>" << ReportUtils::escapeHTML(recommendations[j]) << "</li>";
                    }
                    
                    file << R"(                    </ul>
                </div>)";
                }
                
                file << R"(            </div>
        </div>)";
            }
        }
        
        // Performance insights
        file << R"(        <h2>Performance Insights</h2>
        <div class="chart-container">
            <h3>Top 5 CPU Usage Engines</h3>
            <table style="max-width: 600px; margin: 0 auto;">
                <thead>
                    <tr>
                        <th>Rank</th>
                        <th>Engine</th>
                        <th>CPU Usage</th>
                    </tr>
                </thead>
                <tbody>)";
        
        std::vector<std::pair<float, std::string>> cpuRanking;
        for (const auto& engine : results.engineResults) {
            if (engine.engineCreated) {
                cpuRanking.push_back({engine.maxCpuUsage, engine.engineName});
            }
        }
        std::sort(cpuRanking.rbegin(), cpuRanking.rend());
        
        for (size_t i = 0; i < std::min(size_t(5), cpuRanking.size()); ++i) {
            file << "<tr>";
            file << "<td>" << (i+1) << "</td>";
            file << "<td>" << ReportUtils::escapeHTML(cpuRanking[i].second) << "</td>";
            file << "<td>" << std::fixed << std::setprecision(2) << cpuRanking[i].first << "%</td>";
            file << "</tr>";
        }
        
        file << R"(                </tbody>
            </table>
        </div>
    </div>
</body>
</html>)";
        
        file.close();
        logMessage("HTML report generated: " + filename);
    }
    
    void ComprehensiveTestHarness::generateJSONReport(const TestSuiteResults& results, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            logMessage("Failed to create JSON report: " + filename);
            return;
        }
        
        file << "{\n";
        file << "  \"testSuite\": \"Chimera Phoenix Comprehensive Test Harness\",\n";
        file << "  \"version\": \"1.0\",\n";
        file << "  \"timestamp\": \"" << std::chrono::system_clock::now() << "\",\n";
        file << "  \"configuration\": {\n";
        file << "    \"sampleRate\": " << m_sampleRate << ",\n";
        file << "    \"blockSize\": " << m_blockSize << ",\n";
        file << "    \"testDuration\": " << m_testDuration << ",\n";
        file << "    \"parameterSweepSteps\": " << m_parameterSweepSteps << "\n";
        file << "  },\n";
        file << "  \"summary\": {\n";
        file << "    \"totalEngines\": " << results.totalEngines << ",\n";
        file << "    \"workingEngines\": " << results.workingEngines << ",\n";
        file << "    \"failedEngines\": " << results.failedEngines << ",\n";
        file << "    \"enginesWithCriticalIssues\": " << results.enginesWithCriticalIssues << ",\n";
        file << "    \"enginesWithErrors\": " << results.enginesWithErrors << ",\n";
        file << "    \"enginesWithWarnings\": " << results.enginesWithWarnings << ",\n";
        file << "    \"averageScore\": " << std::fixed << std::setprecision(2) << results.averageScore << ",\n";
        file << "    \"averageCpuUsage\": " << results.averageCpuUsage << ",\n";
        file << "    \"worstCpuUsage\": " << results.worstCpuUsage << ",\n";
        file << "    \"totalExecutionTimeMs\": " << results.totalExecutionTime.count() << "\n";
        file << "  },\n";
        file << "  \"engines\": [\n";
        
        for (size_t i = 0; i < results.engineResults.size(); ++i) {
            const auto& engine = results.engineResults[i];
            
            file << "    {\n";
            file << "      \"id\": " << engine.engineID << ",\n";
            file << "      \"name\": \"" << engine.engineName << "\",\n";
            file << "      \"engineCreated\": " << (engine.engineCreated ? "true" : "false") << ",\n";
            file << "      \"overallScore\": " << std::fixed << std::setprecision(2) << engine.overallScore << ",\n";
            file << "      \"allTestsPassed\": " << (engine.allTestsPassed ? "true" : "false") << ",\n";
            file << "      \"testDurationMs\": " << engine.totalTestTime.count() << ",\n";
            file << "      \"performance\": {\n";
            file << "        \"avgCpuUsage\": " << engine.avgCpuUsage << ",\n";
            file << "        \"maxCpuUsage\": " << engine.maxCpuUsage << ",\n";
            file << "        \"avgLatencyMs\": " << engine.avgLatencyMs << ",\n";
            file << "        \"maxLatencyMs\": " << engine.maxLatencyMs << "\n";
            file << "      },\n";
            file << "      \"issues\": {\n";
            file << "        \"critical\": " << engine.criticalIssues << ",\n";
            file << "        \"errors\": " << engine.errorIssues << ",\n";
            file << "        \"warnings\": " << engine.warningIssues << "\n";
            file << "      },\n";
            file << "      \"testCategories\": {\n";
            file << "        \"parameterSweep\": {\n";
            file << "          \"score\": " << engine.parameterSweepTests.overallScore << ",\n";
            file << "          \"allPassed\": " << (engine.parameterSweepTests.allPassed ? "true" : "false") << ",\n";
            file << "          \"testCount\": " << engine.parameterSweepTests.results.size() << "\n";
            file << "        },\n";
            file << "        \"safety\": {\n";
            file << "          \"score\": " << engine.safetyTests.overallScore << ",\n";
            file << "          \"allPassed\": " << (engine.safetyTests.allPassed ? "true" : "false") << ",\n";
            file << "          \"testCount\": " << engine.safetyTests.results.size() << "\n";
            file << "        },\n";
            file << "        \"audioQuality\": {\n";
            file << "          \"score\": " << engine.audioQualityTests.overallScore << ",\n";
            file << "          \"allPassed\": " << (engine.audioQualityTests.allPassed ? "true" : "false") << ",\n";
            file << "          \"testCount\": " << engine.audioQualityTests.results.size() << "\n";
            file << "        },\n";
            file << "        \"performance\": {\n";
            file << "          \"score\": " << engine.performanceTests.overallScore << ",\n";
            file << "          \"allPassed\": " << (engine.performanceTests.allPassed ? "true" : "false") << ",\n";
            file << "          \"testCount\": " << engine.performanceTests.results.size() << "\n";
            file << "        },\n";
            file << "        \"stability\": {\n";
            file << "          \"score\": " << engine.stabilityTests.overallScore << ",\n";
            file << "          \"allPassed\": " << (engine.stabilityTests.allPassed ? "true" : "false") << ",\n";
            file << "          \"testCount\": " << engine.stabilityTests.results.size() << "\n";
            file << "        }\n";
            file << "      }\n";
            file << "    }";
            
            if (i < results.engineResults.size() - 1) {
                file << ",";
            }
            file << "\n";
        }
        
        file << "  ]\n";
        file << "}\n";
        
        file.close();
        logMessage("JSON report generated: " + filename);
    }
    
    void ComprehensiveTestHarness::generateCSVReport(const TestSuiteResults& results, const std::string& filename) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            logMessage("Failed to create CSV report: " + filename);
            return;
        }
        
        // CSV header
        file << "EngineID,EngineName,EngineCreated,OverallScore,AllTestsPassed,";
        file << "ParameterSweepScore,SafetyScore,AudioQualityScore,PerformanceScore,StabilityScore,";
        file << "CriticalIssues,ErrorIssues,WarningIssues,";
        file << "AvgCpuUsage,MaxCpuUsage,AvgLatencyMs,MaxLatencyMs,TestDurationMs\n";
        
        // Data rows
        for (const auto& engine : results.engineResults) {
            file << engine.engineID << ",";
            file << "\"" << engine.engineName << "\",";
            file << (engine.engineCreated ? "TRUE" : "FALSE") << ",";
            file << std::fixed << std::setprecision(2) << engine.overallScore << ",";
            file << (engine.allTestsPassed ? "TRUE" : "FALSE") << ",";
            file << engine.parameterSweepTests.overallScore << ",";
            file << engine.safetyTests.overallScore << ",";
            file << engine.audioQualityTests.overallScore << ",";
            file << engine.performanceTests.overallScore << ",";
            file << engine.stabilityTests.overallScore << ",";
            file << engine.criticalIssues << ",";
            file << engine.errorIssues << ",";
            file << engine.warningIssues << ",";
            file << engine.avgCpuUsage << ",";
            file << engine.maxCpuUsage << ",";
            file << engine.avgLatencyMs << ",";
            file << engine.maxLatencyMs << ",";
            file << engine.totalTestTime.count() << "\n";
        }
        
        file.close();
        logMessage("CSV report generated: " + filename);
    }

} // namespace ChimeraTestHarness