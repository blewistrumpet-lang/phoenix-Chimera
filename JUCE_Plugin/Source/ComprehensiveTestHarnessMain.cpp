#include "ComprehensiveTestHarness.h"
#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>

using namespace ChimeraTestHarness;

void printUsage(const std::string& programName) {
    std::cout << "Chimera Phoenix Comprehensive Test Harness\n";
    std::cout << "==========================================\n\n";
    std::cout << "Usage: " << programName << " [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --help                Show this help message\n";
    std::cout << "  --engine ID           Test only the specified engine ID (0-56)\n";
    std::cout << "  --sample-rate RATE    Set sample rate (default: 48000)\n";
    std::cout << "  --block-size SIZE     Set block size (default: 512)\n";
    std::cout << "  --duration SECONDS    Set test duration per signal (default: 2.0)\n";
    std::cout << "  --sweep-steps STEPS   Set parameter sweep steps (default: 20)\n";
    std::cout << "  --verbose             Enable verbose output\n";
    std::cout << "  --parallel            Enable parallel testing (default: true)\n";
    std::cout << "  --sequential          Disable parallel testing\n";
    std::cout << "  --max-threads NUM     Set maximum concurrent threads\n";
    std::cout << "  --output-dir DIR      Set output directory for reports (default: .)\n";
    std::cout << "  --html-report FILE    Generate HTML report (default: test_report.html)\n";
    std::cout << "  --json-report FILE    Generate JSON report (default: test_report.json)\n";
    std::cout << "  --csv-report FILE     Generate CSV report (default: test_report.csv)\n";
    std::cout << "  --summary-report FILE Generate summary report (default: test_summary.txt)\n";
    std::cout << "  --detailed-report FILE Generate detailed report (default: test_detailed.txt)\n";
    std::cout << "  --no-reports          Skip report generation\n\n";
    std::cout << "Examples:\n";
    std::cout << "  " << programName << "                    # Test all engines with default settings\n";
    std::cout << "  " << programName << " --engine 15        # Test only engine 15 (Vintage Tube)\n";
    std::cout << "  " << programName << " --verbose --parallel # Test all engines with verbose output\n";
    std::cout << "  " << programName << " --sample-rate 96000 --block-size 256 # Custom audio settings\n\n";
}

void printBanner() {
    std::cout << R"(
 ╔═══════════════════════════════════════════════════════════════════════╗
 ║                  CHIMERA PHOENIX TEST HARNESS v1.0                   ║
 ║                     Comprehensive Engine Testing                      ║
 ╚═══════════════════════════════════════════════════════════════════════╝
)" << std::endl;
}

struct TestConfig {
    int singleEngineId = -1;  // -1 means test all engines
    double sampleRate = 48000.0;
    int blockSize = 512;
    float testDuration = 2.0f;
    int sweepSteps = 20;
    bool verbose = false;
    bool parallel = true;
    int maxThreads = 0;  // 0 means use hardware concurrency
    std::string outputDir = ".";
    std::string htmlReport = "test_report.html";
    std::string jsonReport = "test_report.json";
    std::string csvReport = "test_report.csv";
    std::string summaryReport = "test_summary.txt";
    std::string detailedReport = "test_detailed.txt";
    bool generateReports = true;
};

TestConfig parseCommandLine(int argc, char* argv[]) {
    TestConfig config;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            exit(0);
        }
        else if (arg == "--engine" && i + 1 < argc) {
            config.singleEngineId = std::stoi(argv[++i]);
            if (config.singleEngineId < 0 || config.singleEngineId >= ENGINE_COUNT) {
                std::cerr << "Error: Engine ID must be between 0 and " << (ENGINE_COUNT - 1) << std::endl;
                exit(1);
            }
        }
        else if (arg == "--sample-rate" && i + 1 < argc) {
            config.sampleRate = std::stod(argv[++i]);
            if (config.sampleRate <= 0) {
                std::cerr << "Error: Sample rate must be positive" << std::endl;
                exit(1);
            }
        }
        else if (arg == "--block-size" && i + 1 < argc) {
            config.blockSize = std::stoi(argv[++i]);
            if (config.blockSize <= 0 || config.blockSize > 8192) {
                std::cerr << "Error: Block size must be between 1 and 8192" << std::endl;
                exit(1);
            }
        }
        else if (arg == "--duration" && i + 1 < argc) {
            config.testDuration = std::stof(argv[++i]);
            if (config.testDuration <= 0) {
                std::cerr << "Error: Test duration must be positive" << std::endl;
                exit(1);
            }
        }
        else if (arg == "--sweep-steps" && i + 1 < argc) {
            config.sweepSteps = std::stoi(argv[++i]);
            if (config.sweepSteps < 2 || config.sweepSteps > 100) {
                std::cerr << "Error: Sweep steps must be between 2 and 100" << std::endl;
                exit(1);
            }
        }
        else if (arg == "--verbose") {
            config.verbose = true;
        }
        else if (arg == "--parallel") {
            config.parallel = true;
        }
        else if (arg == "--sequential") {
            config.parallel = false;
        }
        else if (arg == "--max-threads" && i + 1 < argc) {
            config.maxThreads = std::stoi(argv[++i]);
            if (config.maxThreads < 1) {
                std::cerr << "Error: Max threads must be at least 1" << std::endl;
                exit(1);
            }
        }
        else if (arg == "--output-dir" && i + 1 < argc) {
            config.outputDir = argv[++i];
        }
        else if (arg == "--html-report" && i + 1 < argc) {
            config.htmlReport = argv[++i];
        }
        else if (arg == "--json-report" && i + 1 < argc) {
            config.jsonReport = argv[++i];
        }
        else if (arg == "--csv-report" && i + 1 < argc) {
            config.csvReport = argv[++i];
        }
        else if (arg == "--summary-report" && i + 1 < argc) {
            config.summaryReport = argv[++i];
        }
        else if (arg == "--detailed-report" && i + 1 < argc) {
            config.detailedReport = argv[++i];
        }
        else if (arg == "--no-reports") {
            config.generateReports = false;
        }
        else {
            std::cerr << "Error: Unknown option '" << arg << "'" << std::endl;
            std::cerr << "Use --help for usage information" << std::endl;
            exit(1);
        }
    }
    
    return config;
}

std::string getFullPath(const std::string& outputDir, const std::string& filename) {
    if (outputDir == "." || outputDir.empty()) {
        return filename;
    }
    return outputDir + "/" + filename;
}

void printTestConfiguration(const TestConfig& config) {
    std::cout << "Test Configuration:\n";
    std::cout << "  Target: ";
    if (config.singleEngineId >= 0) {
        std::cout << "Engine #" << config.singleEngineId << " (" << getEngineTypeName(config.singleEngineId) << ")\n";
    } else {
        std::cout << "All " << ENGINE_COUNT << " engines\n";
    }
    std::cout << "  Sample Rate: " << config.sampleRate << " Hz\n";
    std::cout << "  Block Size: " << config.blockSize << " samples\n";
    std::cout << "  Test Duration: " << config.testDuration << " seconds per test\n";
    std::cout << "  Parameter Sweep Steps: " << config.sweepSteps << "\n";
    std::cout << "  Parallel Testing: " << (config.parallel ? "Enabled" : "Disabled") << "\n";
    if (config.parallel && config.maxThreads > 0) {
        std::cout << "  Max Threads: " << config.maxThreads << "\n";
    }
    std::cout << "  Verbose Output: " << (config.verbose ? "Enabled" : "Disabled") << "\n";
    if (config.generateReports) {
        std::cout << "  Output Directory: " << config.outputDir << "\n";
    } else {
        std::cout << "  Report Generation: Disabled\n";
    }
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    try {
        printBanner();
        
        // Parse command line arguments
        TestConfig config = parseCommandLine(argc, argv);
        
        printTestConfiguration(config);
        
        // Create and configure test harness
        ComprehensiveTestHarness harness;
        harness.setSampleRate(config.sampleRate);
        harness.setBlockSize(config.blockSize);
        harness.setTestDuration(config.testDuration);
        harness.setNumParameterSweepSteps(config.sweepSteps);
        harness.setVerboseOutput(config.verbose);
        harness.setParallelTesting(config.parallel);
        
        if (config.maxThreads > 0) {
            harness.setMaxConcurrentTests(config.maxThreads);
        }
        
        // Start testing
        auto startTime = std::chrono::high_resolution_clock::now();
        
        TestSuiteResults results;
        
        if (config.singleEngineId >= 0) {
            // Test single engine
            std::cout << "Starting test of engine #" << config.singleEngineId << "...\n\n";
            
            auto engineResult = harness.testSingleEngine(config.singleEngineId);
            results.engineResults.push_back(std::move(engineResult));
            results.totalEngines = 1;
        } else {
            // Test all engines
            std::cout << "Starting comprehensive test of all " << ENGINE_COUNT << " engines...\n\n";
            results = harness.testAllEngines();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        results.totalExecutionTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        // Calculate final summary
        results.calculateSummary();
        
        // Print results to console
        std::cout << "\n";
        harness.printSummaryToConsole(results);
        
        // Generate reports if requested
        if (config.generateReports) {
            std::cout << "Generating reports...\n";
            
            try {
                harness.generateSummaryReport(results, getFullPath(config.outputDir, config.summaryReport));
                harness.generateDetailedReport(results, getFullPath(config.outputDir, config.detailedReport));
                harness.generateHTMLReport(results, getFullPath(config.outputDir, config.htmlReport));
                harness.generateJSONReport(results, getFullPath(config.outputDir, config.jsonReport));
                harness.generateCSVReport(results, getFullPath(config.outputDir, config.csvReport));
                
                std::cout << "Reports generated in: " << config.outputDir << "\n";
                std::cout << "  Summary: " << config.summaryReport << "\n";
                std::cout << "  Detailed: " << config.detailedReport << "\n";
                std::cout << "  HTML: " << config.htmlReport << "\n";
                std::cout << "  JSON: " << config.jsonReport << "\n";
                std::cout << "  CSV: " << config.csvReport << "\n";
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to generate some reports: " << e.what() << std::endl;
            }
        }
        
        // Determine exit code based on results
        int exitCode = 0;
        
        if (results.failedEngines > 0) {
            exitCode = 3;  // Engines failed to create
        } else if (results.enginesWithCriticalIssues > 0) {
            exitCode = 2;  // Critical issues found
        } else if (results.enginesWithErrors > 0) {
            exitCode = 1;  // Errors found
        }
        // Exit code 0 means all tests passed or only warnings
        
        std::cout << "\nTest harness completed with exit code: " << exitCode << "\n";
        
        if (exitCode > 0) {
            std::cout << "Issues found that require attention. Check the reports for details.\n";
            
            // Show quick summary of what needs fixing
            auto problematic = results.getProblematicEngines();
            if (!problematic.empty()) {
                std::cout << "\nTOP PRIORITY FIXES:\n";
                for (size_t i = 0; i < std::min(size_t(3), problematic.size()); ++i) {
                    const auto& engine = problematic[i];
                    std::cout << "  " << (i+1) << ". Engine #" << engine.engineID << " (" << engine.engineName << ")\n";
                    
                    auto recs = engine.getPrioritizedRecommendations();
                    if (!recs.empty()) {
                        std::cout << "     → " << recs[0] << "\n";
                    }
                }
            }
        } else {
            std::cout << "All engines passed basic functionality tests!\n";
        }
        
        return exitCode;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 4;  // Fatal error exit code
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception occurred" << std::endl;
        return 4;
    }
}

// Additional utility function to validate engine factory
namespace {
    void validateEngineFactory() {
        std::cout << "Validating Engine Factory...\n";
        
        int workingEngines = 0;
        int failedEngines = 0;
        
        for (int i = 0; i < ENGINE_COUNT; ++i) {
            try {
                auto engine = EngineFactory::createEngine(i);
                if (engine) {
                    workingEngines++;
                    if (i < 10) {  // Only show first 10 for brevity
                        std::cout << "  ✓ Engine #" << std::setw(2) << i << ": " 
                                 << getEngineTypeName(i) << "\n";
                    }
                } else {
                    failedEngines++;
                    std::cout << "  ✗ Engine #" << std::setw(2) << i << ": " 
                             << getEngineTypeName(i) << " (failed to create)\n";
                }
            } catch (...) {
                failedEngines++;
                std::cout << "  ✗ Engine #" << std::setw(2) << i << ": " 
                         << getEngineTypeName(i) << " (exception during creation)\n";
            }
        }
        
        if (workingEngines > 10) {
            std::cout << "  ... and " << (workingEngines - 10) << " more engines created successfully\n";
        }
        
        std::cout << "Engine Factory Validation: " << workingEngines << " working, " 
                 << failedEngines << " failed\n\n";
        
        if (failedEngines > 0) {
            std::cout << "Warning: Some engines failed to create. Tests will continue with working engines.\n\n";
        }
    }
}