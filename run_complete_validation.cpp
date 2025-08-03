// Master validation runner for all Chimera engines
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <map>
#include <chrono>
#include <cmath>
#include <algorithm>
#include "JUCE_Plugin/Source/ComprehensiveEngineValidator.h"
#include "JUCE_Plugin/Source/EngineTestAgents.h"
#include "JUCE_Plugin/Source/ParameterSweepTest.h"
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/EngineTypes.h"

// Engine definitions for testing
struct EngineDefinition {
    int id;
    std::string name;
    std::string category;
};

std::vector<EngineDefinition> getAllEngines() {
    return {
        {ENGINE_K_STYLE, "K-Style Overdrive", "Distortion"},
        {ENGINE_TAPE_ECHO, "Tape Echo", "Time-Based"},
        {ENGINE_PLATE_REVERB, "Plate Reverb", "Time-Based"},
        {ENGINE_RODENT_DISTORTION, "Rodent Distortion", "Distortion"},
        {ENGINE_MUFF_FUZZ, "Muff Fuzz", "Distortion"},
        {ENGINE_CLASSIC_TREMOLO, "Classic Tremolo", "Modulation"},
        {ENGINE_MAGNETIC_DRUM_ECHO, "Magnetic Drum Echo", "Time-Based"},
        {ENGINE_BUCKET_BRIGADE_DELAY, "Bucket Brigade Delay", "Time-Based"},
        {ENGINE_DIGITAL_DELAY, "Digital Delay", "Time-Based"},
        {ENGINE_HARMONIC_TREMOLO, "Harmonic Tremolo", "Modulation"},
        {ENGINE_ROTARY_SPEAKER, "Rotary Speaker", "Modulation"},
        {ENGINE_DETUNE_DOUBLER, "Detune Doubler", "Spectral"},
        {ENGINE_LADDER_FILTER, "Ladder Filter", "Filter"},
        {ENGINE_FORMANT_FILTER, "Formant Filter", "Filter"},
        {ENGINE_VCA_COMPRESSOR, "Classic Compressor", "Dynamics"},
        {ENGINE_STATE_VARIABLE_FILTER, "State Variable Filter", "Filter"},
        {ENGINE_DIGITAL_CHORUS, "Stereo Chorus", "Modulation"},
        {ENGINE_SPECTRAL_FREEZE, "Spectral Freeze", "Spectral"},
        {ENGINE_GRANULAR_CLOUD, "Granular Cloud", "Spectral"},
        {ENGINE_RING_MODULATOR, "Analog Ring Modulator", "Modulation"},
        {ENGINE_MULTIBAND_SATURATOR, "Multiband Saturator", "Distortion"},
        {ENGINE_COMB_RESONATOR, "Comb Resonator", "Filter"},
        {ENGINE_PITCH_SHIFTER, "Pitch Shifter", "Spectral"},
        {ENGINE_PHASED_VOCODER, "Phased Vocoder", "Spectral"},
        {ENGINE_CONVOLUTION_REVERB, "Convolution Reverb", "Time-Based"},
        {ENGINE_BIT_CRUSHER, "Bit Crusher", "Distortion"},
        {ENGINE_FREQUENCY_SHIFTER, "Frequency Shifter", "Spectral"},
        {ENGINE_WAVE_FOLDER, "Wave Folder", "Distortion"},
        {ENGINE_SHIMMER_REVERB, "Shimmer Reverb", "Time-Based"},
        {ENGINE_VOCAL_FORMANT, "Vocal Formant Filter", "Filter"},
        {ENGINE_TRANSIENT_SHAPER, "Transient Shaper", "Dynamics"},
        {ENGINE_DIMENSION_EXPANDER, "Dimension Expander", "Modulation"},
        {ENGINE_ANALOG_PHASER, "Analog Phaser", "Modulation"},
        {ENGINE_ENVELOPE_FILTER, "Envelope Filter", "Filter"},
        {ENGINE_GATED_REVERB, "Gated Reverb", "Time-Based"},
        {ENGINE_HARMONIC_EXCITER, "Harmonic Exciter", "Distortion"},
        {ENGINE_FEEDBACK_NETWORK, "Feedback Network", "Time-Based"},
        {ENGINE_INTELLIGENT_HARMONIZER, "Intelligent Harmonizer", "Spectral"},
        {ENGINE_PARAMETRIC_EQ, "Parametric EQ", "Filter"},
        {ENGINE_MASTERING_LIMITER, "Mastering Limiter", "Dynamics"},
        {ENGINE_NOISE_GATE, "Noise Gate", "Dynamics"},
        {ENGINE_OPTO_COMPRESSOR, "Vintage Opto Compressor", "Dynamics"},
        {ENGINE_SPECTRAL_GATE, "Spectral Gate", "Dynamics"},
        {ENGINE_CHAOS_GENERATOR, "Chaos Generator", "Spectral"},
        {ENGINE_BUFFER_REPEAT, "Buffer Repeat", "Time-Based"},
        {ENGINE_VINTAGE_CONSOLE_EQ, "Vintage Console EQ", "Filter"},
        {ENGINE_MID_SIDE_PROCESSOR, "Mid/Side Processor", "Spectral"},
        {ENGINE_VINTAGE_TUBE, "Vintage Tube Preamp", "Distortion"},
        {ENGINE_SPRING_REVERB, "Spring Reverb", "Time-Based"},
        {ENGINE_RESONANT_CHORUS, "Resonant Chorus", "Modulation"},
        {ENGINE_STEREO_WIDENER, "Stereo Widener", "Spectral"},
        {ENGINE_STEREO_IMAGER, "Stereo Imager", "Spectral"},
        {ENGINE_DYNAMIC_EQ, "Dynamic EQ", "Filter"}
    };
}

// Test result aggregation
struct AggregatedResults {
    std::map<std::string, std::vector<ComprehensiveEngineValidator::ValidationResult>> byCategory;
    int totalEngines = 0;
    int passedEngines = 0;
    int failedEngines = 0;
    int warningEngines = 0;
    float averageScore = 0;
    float averageCPU = 0;
    float averageTHD = 0;
    std::vector<std::string> criticalIssues;
    std::vector<std::string> recommendations;
};

AggregatedResults analyzeResults(const std::vector<ComprehensiveEngineValidator::ValidationResult>& results) {
    AggregatedResults agg;
    agg.totalEngines = results.size();
    
    float totalScore = 0;
    float totalCPU = 0;
    float totalTHD = 0;
    int thdCount = 0;
    
    for (const auto& result : results) {
        // Categorize
        std::string category = "Unknown";
        for (const auto& engine : getAllEngines()) {
            if (engine.id == result.engineID) {
                category = engine.category;
                break;
            }
        }
        agg.byCategory[category].push_back(result);
        
        // Count pass/fail
        if (result.overallStatus == ComprehensiveEngineValidator::TestStatus::PASSED) {
            agg.passedEngines++;
        } else if (result.overallStatus == ComprehensiveEngineValidator::TestStatus::FAILED) {
            agg.failedEngines++;
            agg.criticalIssues.push_back(result.engineName + ": " + result.notes);
        } else {
            agg.warningEngines++;
        }
        
        // Aggregate metrics
        totalScore += result.overallScore;
        
        // Extract CPU and THD from quality metrics
        for (const auto& metric : result.qualityMetrics) {
            if (metric.name == "CPU Usage") {
                totalCPU += metric.value;
            } else if (metric.name == "THD") {
                totalTHD += metric.value;
                thdCount++;
            }
        }
    }
    
    agg.averageScore = totalScore / agg.totalEngines;
    agg.averageCPU = totalCPU / agg.totalEngines;
    if (thdCount > 0) {
        agg.averageTHD = totalTHD / thdCount;
    }
    
    // Generate recommendations
    if (agg.failedEngines > 0) {
        agg.recommendations.push_back("Critical: " + std::to_string(agg.failedEngines) + " engines are failing tests");
    }
    if (agg.averageCPU > 20) {
        agg.recommendations.push_back("Performance: Average CPU usage is high (" + 
                                     std::to_string(agg.averageCPU) + "%)");
    }
    if (agg.averageScore < 70) {
        agg.recommendations.push_back("Quality: Overall quality score needs improvement");
    }
    
    return agg;
}

void generateMasterReport(const AggregatedResults& agg, const std::string& filename) {
    std::ofstream html(filename);
    
    html << "<!DOCTYPE html>\n<html>\n<head>\n";
    html << "<title>Chimera Engine Complete Validation Report</title>\n";
    html << "<script src='https://cdn.plot.ly/plotly-latest.min.js'></script>\n";
    html << "<style>\n";
    html << "body { font-family: 'Segoe UI', Arial, sans-serif; margin: 0; padding: 0; background: #f0f2f5; }\n";
    html << ".header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 40px; text-align: center; }\n";
    html << ".container { max-width: 1400px; margin: 0 auto; padding: 20px; }\n";
    html << ".summary-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr)); gap: 20px; margin: 30px 0; }\n";
    html << ".card { background: white; border-radius: 10px; padding: 20px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }\n";
    html << ".stat-card { text-align: center; }\n";
    html << ".stat-value { font-size: 3em; font-weight: bold; margin: 10px 0; }\n";
    html << ".stat-label { color: #666; text-transform: uppercase; font-size: 0.9em; }\n";
    html << ".pass { color: #10b981; }\n";
    html << ".fail { color: #ef4444; }\n";
    html << ".warning { color: #f59e0b; }\n";
    html << ".category-section { margin: 40px 0; }\n";
    html << ".category-header { background: #f8f9fa; padding: 15px; border-left: 4px solid #667eea; margin: 20px 0; }\n";
    html << "table { width: 100%; border-collapse: collapse; background: white; }\n";
    html << "th { background: #667eea; color: white; padding: 12px; text-align: left; }\n";
    html << "td { padding: 10px; border-bottom: 1px solid #e5e7eb; }\n";
    html << "tr:hover { background: #f9fafb; }\n";
    html << ".progress-bar { width: 100%; height: 30px; background: #e5e7eb; border-radius: 15px; overflow: hidden; }\n";
    html << ".progress-fill { height: 100%; background: linear-gradient(90deg, #10b981, #059669); }\n";
    html << ".issue-list { background: #fef2f2; border: 1px solid #fecaca; border-radius: 5px; padding: 15px; margin: 20px 0; }\n";
    html << ".recommendation-list { background: #fefce8; border: 1px solid #fde68a; border-radius: 5px; padding: 15px; margin: 20px 0; }\n";
    html << "</style>\n</head>\n<body>\n";
    
    // Header
    html << "<div class='header'>\n";
    html << "<h1>🔬 Chimera Engine Complete Validation Report</h1>\n";
    html << "<p>Comprehensive Testing of All " << agg.totalEngines << " Audio Engines</p>\n";
    html << "<p>Generated: " << __DATE__ << " " << __TIME__ << "</p>\n";
    html << "</div>\n";
    
    html << "<div class='container'>\n";
    
    // Executive Summary
    html << "<div class='card'>\n";
    html << "<h2>Executive Summary</h2>\n";
    
    float passRate = (agg.passedEngines * 100.0f / agg.totalEngines);
    html << "<div class='progress-bar'><div class='progress-fill' style='width: " << passRate << "%;'></div></div>\n";
    html << "<p style='text-align: center; margin-top: 10px;'>" << std::fixed << std::setprecision(1) << passRate << "% Pass Rate</p>\n";
    
    html << "<div class='summary-grid'>\n";
    html << "<div class='stat-card'><div class='stat-value'>" << agg.totalEngines << "</div><div class='stat-label'>Total Engines</div></div>\n";
    html << "<div class='stat-card'><div class='stat-value pass'>" << agg.passedEngines << "</div><div class='stat-label'>Passed</div></div>\n";
    html << "<div class='stat-card'><div class='stat-value fail'>" << agg.failedEngines << "</div><div class='stat-label'>Failed</div></div>\n";
    html << "<div class='stat-card'><div class='stat-value warning'>" << agg.warningEngines << "</div><div class='stat-label'>Warnings</div></div>\n";
    html << "<div class='stat-card'><div class='stat-value'>" << std::fixed << std::setprecision(1) << agg.averageScore << "%</div><div class='stat-label'>Avg Score</div></div>\n";
    html << "<div class='stat-card'><div class='stat-value'>" << std::fixed << std::setprecision(1) << agg.averageCPU << "%</div><div class='stat-label'>Avg CPU</div></div>\n";
    html << "</div>\n";
    html << "</div>\n";
    
    // Critical Issues
    if (!agg.criticalIssues.empty()) {
        html << "<div class='issue-list'>\n";
        html << "<h3>⚠️ Critical Issues</h3>\n";
        html << "<ul>\n";
        for (const auto& issue : agg.criticalIssues) {
            html << "<li>" << issue << "</li>\n";
        }
        html << "</ul>\n";
        html << "</div>\n";
    }
    
    // Recommendations
    if (!agg.recommendations.empty()) {
        html << "<div class='recommendation-list'>\n";
        html << "<h3>💡 Recommendations</h3>\n";
        html << "<ul>\n";
        for (const auto& rec : agg.recommendations) {
            html << "<li>" << rec << "</li>\n";
        }
        html << "</ul>\n";
        html << "</div>\n";
    }
    
    // Results by Category
    html << "<div class='card'>\n";
    html << "<h2>Results by Category</h2>\n";
    
    for (const auto& [category, results] : agg.byCategory) {
        html << "<div class='category-section'>\n";
        html << "<div class='category-header'><h3>" << category << " (" << results.size() << " engines)</h3></div>\n";
        
        html << "<table>\n";
        html << "<tr><th>Engine</th><th>Status</th><th>Score</th><th>Tests Passed</th><th>Issues</th></tr>\n";
        
        for (const auto& result : results) {
            html << "<tr>\n";
            html << "<td><strong>" << result.engineName << "</strong></td>\n";
            
            // Status
            html << "<td class='";
            if (result.overallStatus == ComprehensiveEngineValidator::TestStatus::PASSED) {
                html << "pass'>✓ PASSED";
            } else if (result.overallStatus == ComprehensiveEngineValidator::TestStatus::FAILED) {
                html << "fail'>✗ FAILED";
            } else {
                html << "warning'>⚠ WARNING";
            }
            html << "</td>\n";
            
            // Score
            html << "<td>" << std::fixed << std::setprecision(1) << result.overallScore << "%</td>\n";
            
            // Tests Passed
            int testsPassed = 0;
            int totalTests = 0;
            for (const auto& test : result.functionalityTests) {
                totalTests++;
                if (test.status == ComprehensiveEngineValidator::TestStatus::PASSED) testsPassed++;
            }
            for (const auto& test : result.parameterTests) {
                totalTests++;
                if (test.status == ComprehensiveEngineValidator::TestStatus::PASSED) testsPassed++;
            }
            html << "<td>" << testsPassed << "/" << totalTests << "</td>\n";
            
            // Issues
            html << "<td style='font-size: 0.9em; color: #666;'>";
            if (result.notes.empty()) {
                html << "No issues";
            } else {
                html << result.notes;
            }
            html << "</td>\n";
            
            html << "</tr>\n";
        }
        
        html << "</table>\n";
        html << "</div>\n";
    }
    
    html << "</div>\n";
    
    // Charts
    html << "<div class='card'>\n";
    html << "<h2>Visual Analysis</h2>\n";
    
    // Category breakdown pie chart
    html << "<div id='categoryChart' style='width: 100%; height: 400px;'></div>\n";
    html << "<script>\n";
    html << "var categoryData = [{\n";
    html << "  values: [";
    bool first = true;
    for (const auto& [category, results] : agg.byCategory) {
        if (!first) html << ", ";
        html << results.size();
        first = false;
    }
    html << "],\n";
    html << "  labels: [";
    first = true;
    for (const auto& [category, results] : agg.byCategory) {
        if (!first) html << ", ";
        html << "'" << category << "'";
        first = false;
    }
    html << "],\n";
    html << "  type: 'pie',\n";
    html << "  hole: .4\n";
    html << "}];\n";
    html << "var layout = { title: 'Engines by Category' };\n";
    html << "Plotly.newPlot('categoryChart', categoryData, layout);\n";
    html << "</script>\n";
    
    html << "</div>\n";
    
    // Footer
    html << "<div style='text-align: center; padding: 40px; color: #666;'>\n";
    html << "<p>Chimera Audio Engine Validation System v1.0</p>\n";
    html << "<p>© 2024 Chimera Audio - Comprehensive Testing Suite</p>\n";
    html << "</div>\n";
    
    html << "</div>\n"; // container
    html << "</body>\n</html>\n";
    
    html.close();
}

int main() {
    std::cout << "========================================\n";
    std::cout << "Chimera Engine Complete Validation\n";
    std::cout << "========================================\n\n";
    
    // Create validator
    ComprehensiveEngineValidator::EngineValidator validator;
    validator.setOutputDirectory("validation_reports");
    
    // Get all engines
    auto engines = getAllEngines();
    std::cout << "Testing " << engines.size() << " engines...\n\n";
    
    // Test each engine
    std::vector<ComprehensiveEngineValidator::ValidationResult> allResults;
    
    for (const auto& engine : engines) {
        std::cout << "Testing " << std::setw(30) << std::left << engine.name << ": ";
        std::cout.flush();
        
        // Run comprehensive validation
        auto result = validator.validateEngine(
            engine.id, 
            ComprehensiveEngineValidator::ValidationLevel::COMPREHENSIVE
        );
        
        // Show quick status
        if (result.overallStatus == ComprehensiveEngineValidator::TestStatus::PASSED) {
            std::cout << "✓ PASSED";
        } else if (result.overallStatus == ComprehensiveEngineValidator::TestStatus::FAILED) {
            std::cout << "✗ FAILED";
        } else {
            std::cout << "⚠ WARNING";
        }
        std::cout << " (Score: " << std::fixed << std::setprecision(1) << result.overallScore << "%)\n";
        
        allResults.push_back(result);
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Analyzing Results...\n";
    std::cout << "========================================\n";
    
    // Analyze results
    auto aggregated = analyzeResults(allResults);
    
    // Print summary
    std::cout << "\nSummary:\n";
    std::cout << "--------\n";
    std::cout << "Total Engines: " << aggregated.totalEngines << "\n";
    std::cout << "Passed: " << aggregated.passedEngines << "\n";
    std::cout << "Failed: " << aggregated.failedEngines << "\n";
    std::cout << "Warnings: " << aggregated.warningEngines << "\n";
    std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) 
              << (aggregated.passedEngines * 100.0f / aggregated.totalEngines) << "%\n";
    std::cout << "Average Score: " << aggregated.averageScore << "%\n";
    std::cout << "Average CPU: " << aggregated.averageCPU << "%\n";
    
    // Generate master report
    std::cout << "\nGenerating comprehensive report...\n";
    generateMasterReport(aggregated, "complete_validation_report.html");
    
    // Save detailed reports
    validator.saveReports(allResults);
    
    std::cout << "\n========================================\n";
    std::cout << "Validation Complete!\n";
    std::cout << "========================================\n";
    std::cout << "\nReports saved:\n";
    std::cout << "  - complete_validation_report.html (Master Report)\n";
    std::cout << "  - validation_reports/ (Detailed Engine Reports)\n";
    
    return 0;
}