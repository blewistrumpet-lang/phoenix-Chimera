#include "EngineQualityTest.h"
#include "EngineFactory.h"
#include "ParameterDefinitions.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

QualityTestRunner::QualityTestRunner() {
    // Configure test settings
    m_tester.setSampleRate(48000.0);
    m_tester.setBlockSize(512);
    m_tester.setTestDuration(1.0f);
    m_tester.setVerbose(false);
}

void QualityTestRunner::runAllEngineTests() {
    std::cout << "================================\n";
    std::cout << "Chimera Phoenix Quality Test Suite\n";
    std::cout << "Testing all 50 engines...\n";
    std::cout << "================================\n\n";
    
    // Test all engines
    for (int engineType = 0; engineType < 50; ++engineType) {
        runEngineTest(engineType);
    }
    
    // Print summary
    printSummary();
    
    // Generate reports
    generateHTMLReport("test_results.html");
    generateJSONReport("test_results.json");
}

void QualityTestRunner::runEngineTest(int engineType) {
    // Create engine
    auto engine = EngineFactory::createEngine(engineType);
    if (!engine) {
        std::cout << "Failed to create engine type " << engineType << "\n";
        return;
    }
    
    std::string engineName = engine->getName().toStdString();
    std::cout << "Testing Engine #" << engineType << ": " << engineName << "... ";
    std::cout.flush();
    
    // Run tests
    TestResults results = m_tester.runAllTests(engine, engineType);
    
    // Update counters
    m_totalTests++;
    if (results.passed) {
        m_passedTests++;
        std::cout << "PASSED";
    } else {
        m_failedTests++;
        std::cout << "FAILED";
    }
    
    if (results.warningTests > 0) {
        m_warningTests += results.warningTests;
        std::cout << " (with " << results.warningTests << " warnings)";
    }
    
    std::cout << " - Score: " << int(results.overallScore) << "%\n";
    
    // Store results
    m_allResults.push_back(results);
    
    // Print failures if any
    if (!results.passed) {
        std::cout << "  Failed tests:\n";
        
        auto printFailures = [](const std::vector<TestResult>& tests) {
            for (const auto& test : tests) {
                if (!test.passed) {
                    std::cout << "    - " << test.testName << ": " << test.message << "\n";
                }
            }
        };
        
        printFailures(results.audioQuality.getAllTests());
        printFailures(results.functionality.getAllTests());
        printFailures(results.dspQuality.getAllTests());
        printFailures(results.boutiqueQuality.getAllTests());
        printFailures(results.engineSpecific.getAllTests());
    }
}

void QualityTestRunner::runTestSuite(const std::string& suiteName) {
    std::cout << "Running test suite: " << suiteName << "\n";
    
    if (suiteName == "audio_quality") {
        for (int i = 0; i < 50; ++i) {
            auto engine = EngineFactory::createEngine(i);
            if (engine) {
                AudioQualityResults results = m_tester.testAudioQuality(engine.get());
                std::cout << "Engine " << i << " audio quality: " 
                         << (results.allPassed() ? "PASS" : "FAIL") << "\n";
            }
        }
    } else if (suiteName == "performance") {
        for (int i = 0; i < 50; ++i) {
            auto engine = EngineFactory::createEngine(i);
            if (engine) {
                PerformanceMetrics metrics = m_tester.benchmarkPerformance(engine.get());
                std::cout << "Engine " << i << " CPU usage: " 
                         << metrics.cpuUsagePercent << "%\n";
            }
        }
    } else if (suiteName == "boutique") {
        for (int i = 0; i < 50; ++i) {
            auto engine = EngineFactory::createEngine(i);
            if (engine) {
                BoutiqueQualityResults results = m_tester.testBoutiqueFeatures(engine.get());
                std::cout << "Engine " << i << " boutique features: "
                         << (results.allPassed() ? "PASS" : "FAIL") << "\n";
            }
        }
    }
}

void QualityTestRunner::printSummary() {
    std::cout << "\n================================\n";
    std::cout << "TEST SUMMARY\n";
    std::cout << "================================\n";
    std::cout << "Total Engines Tested: " << m_totalTests << "\n";
    std::cout << "Passed: " << m_passedTests << "\n";
    std::cout << "Failed: " << m_failedTests << "\n";
    std::cout << "Warnings: " << m_warningTests << "\n";
    
    float passRate = (m_totalTests > 0) ? 
                     (float(m_passedTests) / float(m_totalTests) * 100.0f) : 0.0f;
    std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) 
              << passRate << "%\n";
    
    // Performance summary
    float avgCPU = 0.0f;
    float maxCPU = 0.0f;
    for (const auto& result : m_allResults) {
        avgCPU += result.performance.cpuUsagePercent;
        maxCPU = std::max(maxCPU, result.performance.cpuUsagePercent);
    }
    if (!m_allResults.empty()) {
        avgCPU /= m_allResults.size();
    }
    
    std::cout << "\nPerformance Summary:\n";
    std::cout << "Average CPU Usage: " << avgCPU << "%\n";
    std::cout << "Maximum CPU Usage: " << maxCPU << "%\n";
    
    // Quality summary
    int boutiquePassCount = 0;
    for (const auto& result : m_allResults) {
        if (result.boutiqueQuality.allPassed()) {
            boutiquePassCount++;
        }
    }
    
    std::cout << "\nBoutique Quality Summary:\n";
    std::cout << "Engines with all boutique features: " << boutiquePassCount << "/50\n";
    
    // List failed engines
    if (m_failedTests > 0) {
        std::cout << "\nFailed Engines:\n";
        for (const auto& result : m_allResults) {
            if (!result.passed) {
                std::cout << "  - " << result.engineName 
                         << " (Score: " << int(result.overallScore) << "%)\n";
            }
        }
    }
    
    std::cout << "\nExit Code: " << getExitCode() << "\n";
}

void QualityTestRunner::generateHTMLReport(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "<!DOCTYPE html>\n";
    file << "<html>\n<head>\n";
    file << "<title>Chimera Phoenix Engine Quality Test Report</title>\n";
    file << "<style>\n";
    file << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    file << "table { border-collapse: collapse; width: 100%; margin: 20px 0; }\n";
    file << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    file << "th { background-color: #4CAF50; color: white; }\n";
    file << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
    file << ".passed { color: green; font-weight: bold; }\n";
    file << ".failed { color: red; font-weight: bold; }\n";
    file << ".warning { color: orange; font-weight: bold; }\n";
    file << ".summary { background-color: #e7f3ff; padding: 15px; margin: 20px 0; border-radius: 5px; }\n";
    file << ".engine-details { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }\n";
    file << "</style>\n";
    file << "</head>\n<body>\n";
    
    // Header
    file << "<h1>Chimera Phoenix Engine Quality Test Report</h1>\n";
    file << "<p>Generated: " << std::chrono::system_clock::now() << "</p>\n";
    
    // Summary
    file << "<div class='summary'>\n";
    file << "<h2>Test Summary</h2>\n";
    file << "<p>Total Engines: " << m_totalTests << "</p>\n";
    file << "<p>Passed: <span class='passed'>" << m_passedTests << "</span></p>\n";
    file << "<p>Failed: <span class='failed'>" << m_failedTests << "</span></p>\n";
    file << "<p>Pass Rate: " << std::fixed << std::setprecision(1) 
         << (float(m_passedTests) / float(m_totalTests) * 100.0f) << "%</p>\n";
    file << "</div>\n";
    
    // Results table
    file << "<h2>Engine Test Results</h2>\n";
    file << "<table>\n";
    file << "<tr><th>Engine</th><th>Audio Quality</th><th>Functionality</th>";
    file << "<th>DSP Quality</th><th>Boutique</th><th>Performance</th>";
    file << "<th>Overall Score</th><th>Status</th></tr>\n";
    
    for (const auto& result : m_allResults) {
        file << "<tr>\n";
        file << "<td>" << result.engineName << "</td>\n";
        file << "<td class='" << (result.audioQuality.allPassed() ? "passed" : "failed") << "'>" 
             << (result.audioQuality.allPassed() ? "PASS" : "FAIL") << "</td>\n";
        file << "<td class='" << (result.functionality.allPassed() ? "passed" : "failed") << "'>" 
             << (result.functionality.allPassed() ? "PASS" : "FAIL") << "</td>\n";
        file << "<td class='" << (result.dspQuality.allPassed() ? "passed" : "failed") << "'>" 
             << (result.dspQuality.allPassed() ? "PASS" : "FAIL") << "</td>\n";
        file << "<td class='" << (result.boutiqueQuality.allPassed() ? "passed" : "failed") << "'>" 
             << (result.boutiqueQuality.allPassed() ? "PASS" : "FAIL") << "</td>\n";
        file << "<td>" << std::fixed << std::setprecision(1) 
             << result.performance.cpuUsagePercent << "%</td>\n";
        file << "<td>" << int(result.overallScore) << "%</td>\n";
        file << "<td class='" << (result.passed ? "passed" : "failed") << "'>" 
             << (result.passed ? "PASSED" : "FAILED") << "</td>\n";
        file << "</tr>\n";
    }
    
    file << "</table>\n";
    
    // Detailed results for failed engines
    file << "<h2>Failed Engine Details</h2>\n";
    for (const auto& result : m_allResults) {
        if (!result.passed) {
            file << "<div class='engine-details'>\n";
            file << "<h3>" << result.engineName << "</h3>\n";
            file << "<p>Overall Score: " << int(result.overallScore) << "%</p>\n";
            
            // List failed tests
            file << "<h4>Failed Tests:</h4>\n<ul>\n";
            
            auto listFailedTests = [&file](const std::vector<TestResult>& tests) {
                for (const auto& test : tests) {
                    if (!test.passed) {
                        file << "<li><strong>" << test.testName << "</strong>: " 
                             << test.message << "</li>\n";
                    }
                }
            };
            
            listFailedTests(result.audioQuality.getAllTests());
            listFailedTests(result.functionality.getAllTests());
            listFailedTests(result.dspQuality.getAllTests());
            listFailedTests(result.boutiqueQuality.getAllTests());
            listFailedTests(result.engineSpecific.getAllTests());
            
            file << "</ul>\n";
            
            // Recommendations
            if (!result.recommendations.empty()) {
                file << "<h4>Recommendations:</h4>\n<ul>\n";
                for (const auto& rec : result.recommendations) {
                    file << "<li>" << rec << "</li>\n";
                }
                file << "</ul>\n";
            }
            
            file << "</div>\n";
        }
    }
    
    file << "</body>\n</html>\n";
    file.close();
    
    std::cout << "HTML report generated: " << filename << "\n";
}

void QualityTestRunner::generateJSONReport(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "{\n";
    file << "  \"testSuite\": \"Chimera Phoenix Quality Tests\",\n";
    file << "  \"timestamp\": \"" << std::chrono::system_clock::now() << "\",\n";
    file << "  \"summary\": {\n";
    file << "    \"totalEngines\": " << m_totalTests << ",\n";
    file << "    \"passed\": " << m_passedTests << ",\n";
    file << "    \"failed\": " << m_failedTests << ",\n";
    file << "    \"passRate\": " << std::fixed << std::setprecision(2) 
         << (float(m_passedTests) / float(m_totalTests) * 100.0f) << "\n";
    file << "  },\n";
    file << "  \"engines\": [\n";
    
    for (size_t i = 0; i < m_allResults.size(); ++i) {
        const auto& result = m_allResults[i];
        file << "    {\n";
        file << "      \"name\": \"" << result.engineName << "\",\n";
        file << "      \"type\": " << result.engineType << ",\n";
        file << "      \"passed\": " << (result.passed ? "true" : "false") << ",\n";
        file << "      \"score\": " << result.overallScore << ",\n";
        file << "      \"audioQuality\": " << (result.audioQuality.allPassed() ? "true" : "false") << ",\n";
        file << "      \"functionality\": " << (result.functionality.allPassed() ? "true" : "false") << ",\n";
        file << "      \"dspQuality\": " << (result.dspQuality.allPassed() ? "true" : "false") << ",\n";
        file << "      \"boutiqueQuality\": " << (result.boutiqueQuality.allPassed() ? "true" : "false") << ",\n";
        file << "      \"cpuUsage\": " << result.performance.cpuUsagePercent << ",\n";
        file << "      \"latency\": " << result.performance.processingLatencyMs << "\n";
        file << "    }";
        if (i < m_allResults.size() - 1) file << ",";
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";
    file.close();
    
    std::cout << "JSON report generated: " << filename << "\n";
}

// Main function for standalone test runner
int main(int argc, char* argv[]) {
    QualityTestRunner runner;
    
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--engine" && argc > 2) {
            int engineType = std::stoi(argv[2]);
            runner.runEngineTest(engineType);
        } else if (arg == "--suite" && argc > 2) {
            runner.runTestSuite(argv[2]);
        } else if (arg == "--help") {
            std::cout << "Usage:\n";
            std::cout << "  QualityTestRunner              - Run all tests\n";
            std::cout << "  QualityTestRunner --engine N   - Test engine N\n";
            std::cout << "  QualityTestRunner --suite NAME - Run test suite\n";
            std::cout << "Available suites: audio_quality, performance, boutique\n";
            return 0;
        }
    } else {
        runner.runAllEngineTests();
    }
    
    return runner.getExitCode();
}