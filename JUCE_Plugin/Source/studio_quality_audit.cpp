// Studio Quality Audit for all DSP Engines
// This audit checks each engine for professional studio-grade requirements

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <filesystem>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

struct QualityCheck {
    bool hasDenormalGuard = false;
    bool hasScrubBuffer = false;
    bool hasDCBlocker = false;
    bool hasSafeFloatCheck = false;
    bool hasParameterSmoothing = false;
    bool hasSampleRatePrep = false;
    bool hasBufferBoundsCheck = false;
    bool hasProperReset = false;
    bool usesWorkBuffers = false;
    bool hasThreadSafety = false;
    bool hasMetering = false;
    bool hasOversampling = false;
    int totalScore = 0;
    std::vector<std::string> issues;
    std::vector<std::string> strengths;
};

class EngineAuditor {
public:
    void auditEngine(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) return;
        
        std::string filename = fs::path(filePath).filename().string();
        
        // Skip non-engine files
        if (filename.find("Test") != std::string::npos ||
            filename.find("Validator") != std::string::npos ||
            filename.find("Generator") != std::string::npos ||
            filename.find("Harness") != std::string::npos ||
            filename.find("Metadata") != std::string::npos ||
            filename.find("Factory") != std::string::npos ||
            filename.find("_OLD") != std::string::npos ||
            filename == "EngineBase.h" ||
            filename == "DspEngineUtilities.h") {
            return;
        }
        
        // Only process .cpp engine files
        if (filename.substr(filename.length() - 4) != ".cpp") return;
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        QualityCheck check;
        std::string engineName = filename.substr(0, filename.length() - 4);
        
        // Check for critical DSP safety features
        check.hasDenormalGuard = content.find("DenormalGuard") != std::string::npos;
        check.hasScrubBuffer = content.find("scrubBuffer") != std::string::npos;
        check.hasDCBlocker = content.find("DCBlocker") != std::string::npos ||
                            content.find("dcBlocker") != std::string::npos;
        
        // Check for safe float handling
        check.hasSafeFloatCheck = content.find("isnan") != std::string::npos ||
                                  content.find("isinf") != std::string::npos ||
                                  content.find("safeFloat") != std::string::npos;
        
        // Check for parameter smoothing
        check.hasParameterSmoothing = content.find("Smoother") != std::string::npos ||
                                      content.find("smoothing") != std::string::npos ||
                                      content.find("ramp") != std::string::npos;
        
        // Check for sample rate preparation
        check.hasSampleRatePrep = content.find("prepareToPlay") != std::string::npos &&
                                  content.find("sampleRate") != std::string::npos;
        
        // Check for buffer bounds checking
        check.hasBufferBoundsCheck = content.find("getNumSamples") != std::string::npos &&
                                     (content.find("std::min") != std::string::npos ||
                                      content.find("clamp") != std::string::npos);
        
        // Check for proper reset
        check.hasProperReset = content.find("void reset()") != std::string::npos;
        
        // Check for work buffers (efficiency)
        check.usesWorkBuffers = content.find("workBuffer") != std::string::npos ||
                               content.find("m_buffer") != std::string::npos ||
                               content.find("tempBuffer") != std::string::npos;
        
        // Check for thread safety
        check.hasThreadSafety = content.find("std::atomic") != std::string::npos ||
                               content.find("memory_order") != std::string::npos;
        
        // Check for metering
        check.hasMetering = content.find("getRMS") != std::string::npos ||
                           content.find("getPeak") != std::string::npos ||
                           content.find("getLevel") != std::string::npos ||
                           content.find("meter") != std::string::npos;
        
        // Check for oversampling
        check.hasOversampling = content.find("oversample") != std::string::npos ||
                               content.find("Oversampl") != std::string::npos;
        
        // Calculate score and identify issues
        calculateScore(check, engineName);
        
        // Store results
        results[engineName] = check;
    }
    
    void calculateScore(QualityCheck& check, const std::string& engineName) {
        int maxScore = 100;
        check.totalScore = 0;
        
        // Critical safety (40 points)
        if (check.hasDenormalGuard) {
            check.totalScore += 10;
            check.strengths.push_back("Has denormal protection");
        } else {
            check.issues.push_back("CRITICAL: Missing DenormalGuard");
        }
        
        if (check.hasScrubBuffer) {
            check.totalScore += 10;
            check.strengths.push_back("Has NaN/Inf scrubbing");
        } else {
            check.issues.push_back("CRITICAL: Missing scrubBuffer()");
        }
        
        if (check.hasDCBlocker) {
            check.totalScore += 10;
            check.strengths.push_back("Has DC blocking");
        } else if (needsDCBlocker(engineName)) {
            check.issues.push_back("WARNING: May need DC blocker");
        }
        
        if (check.hasSafeFloatCheck) {
            check.totalScore += 10;
            check.strengths.push_back("Has float safety checks");
        } else {
            check.issues.push_back("WARNING: No explicit float safety");
        }
        
        // Quality features (30 points)
        if (check.hasParameterSmoothing) {
            check.totalScore += 10;
            check.strengths.push_back("Has parameter smoothing");
        } else {
            check.issues.push_back("WARNING: No parameter smoothing");
        }
        
        if (check.hasSampleRatePrep) {
            check.totalScore += 10;
            check.strengths.push_back("Properly prepares sample rate");
        } else {
            check.issues.push_back("WARNING: Sample rate prep unclear");
        }
        
        if (check.hasBufferBoundsCheck) {
            check.totalScore += 10;
            check.strengths.push_back("Has buffer bounds checking");
        } else {
            check.issues.push_back("WARNING: No explicit bounds checking");
        }
        
        // Professional features (30 points)
        if (check.hasProperReset) {
            check.totalScore += 10;
            check.strengths.push_back("Has reset function");
        } else {
            check.issues.push_back("Missing reset() function");
        }
        
        if (check.usesWorkBuffers) {
            check.totalScore += 5;
            check.strengths.push_back("Uses work buffers");
        }
        
        if (check.hasThreadSafety) {
            check.totalScore += 5;
            check.strengths.push_back("Has thread-safe operations");
        }
        
        if (check.hasMetering) {
            check.totalScore += 5;
            check.strengths.push_back("Has metering");
        }
        
        if (check.hasOversampling && needsOversampling(engineName)) {
            check.totalScore += 5;
            check.strengths.push_back("Has oversampling");
        }
    }
    
    bool needsDCBlocker(const std::string& engineName) {
        // Engines that typically need DC blocking
        return engineName.find("Compressor") != std::string::npos ||
               engineName.find("Distortion") != std::string::npos ||
               engineName.find("Saturator") != std::string::npos ||
               engineName.find("Exciter") != std::string::npos ||
               engineName.find("Overdrive") != std::string::npos ||
               engineName.find("Fuzz") != std::string::npos ||
               engineName.find("Filter") != std::string::npos;
    }
    
    bool needsOversampling(const std::string& engineName) {
        // Engines that benefit from oversampling
        return engineName.find("Distortion") != std::string::npos ||
               engineName.find("Saturator") != std::string::npos ||
               engineName.find("Exciter") != std::string::npos ||
               engineName.find("Overdrive") != std::string::npos ||
               engineName.find("BitCrusher") != std::string::npos ||
               engineName.find("WaveFolder") != std::string::npos;
    }
    
    void generateReport() {
        std::ofstream report("studio_quality_audit_report.md");
        
        report << "# Studio Quality Audit Report\n\n";
        report << "## Summary\n\n";
        
        // Categorize engines by quality score
        std::vector<std::pair<std::string, QualityCheck>> critical;
        std::vector<std::pair<std::string, QualityCheck>> needsWork;
        std::vector<std::pair<std::string, QualityCheck>> good;
        std::vector<std::pair<std::string, QualityCheck>> excellent;
        
        for (const auto& [name, check] : results) {
            if (check.totalScore < 40) {
                critical.push_back({name, check});
            } else if (check.totalScore < 60) {
                needsWork.push_back({name, check});
            } else if (check.totalScore < 80) {
                good.push_back({name, check});
            } else {
                excellent.push_back({name, check});
            }
        }
        
        report << "- **Critical Issues**: " << critical.size() << " engines\n";
        report << "- **Needs Work**: " << needsWork.size() << " engines\n";
        report << "- **Good Quality**: " << good.size() << " engines\n";
        report << "- **Excellent**: " << excellent.size() << " engines\n\n";
        
        // Critical engines that need immediate attention
        if (!critical.empty()) {
            report << "## 🔴 CRITICAL - Needs Immediate Fix\n\n";
            for (const auto& [name, check] : critical) {
                report << "### " << name << " (Score: " << check.totalScore << "/100)\n";
                report << "**Issues:**\n";
                for (const auto& issue : check.issues) {
                    report << "- " << issue << "\n";
                }
                report << "\n";
            }
        }
        
        // Engines that need work
        if (!needsWork.empty()) {
            report << "## 🟡 Needs Improvement\n\n";
            for (const auto& [name, check] : needsWork) {
                report << "### " << name << " (Score: " << check.totalScore << "/100)\n";
                report << "**Issues:**\n";
                for (const auto& issue : check.issues) {
                    report << "- " << issue << "\n";
                }
                if (!check.strengths.empty()) {
                    report << "**Strengths:**\n";
                    for (const auto& strength : check.strengths) {
                        report << "- " << strength << "\n";
                    }
                }
                report << "\n";
            }
        }
        
        // Good quality engines
        if (!good.empty()) {
            report << "## 🟢 Good Quality\n\n";
            for (const auto& [name, check] : good) {
                report << "### " << name << " (Score: " << check.totalScore << "/100)\n";
                if (!check.issues.empty()) {
                    report << "**Minor Issues:**\n";
                    for (const auto& issue : check.issues) {
                        report << "- " << issue << "\n";
                    }
                }
                report << "\n";
            }
        }
        
        // Excellent engines
        if (!excellent.empty()) {
            report << "## ⭐ Excellent - Studio Ready\n\n";
            for (const auto& [name, check] : excellent) {
                report << "- **" << name << "** (Score: " << check.totalScore << "/100)\n";
            }
        }
        
        report << "\n## Recommendations\n\n";
        report << "1. **Immediate Priority**: Fix all CRITICAL issues in red engines\n";
        report << "2. **Add DenormalGuard and scrubBuffer to all engines**\n";
        report << "3. **Implement parameter smoothing where missing**\n";
        report << "4. **Add DC blocking to dynamics and distortion engines**\n";
        report << "5. **Consider oversampling for non-linear processors**\n";
        
        report.close();
        std::cout << "Report generated: studio_quality_audit_report.md\n";
    }
    
private:
    std::map<std::string, QualityCheck> results;
};

int main() {
    EngineAuditor auditor;
    
    std::cout << "Starting Studio Quality Audit...\n";
    
    // Scan all engine files
    for (const auto& entry : fs::directory_iterator(".")) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            if (path.substr(path.length() - 4) == ".cpp") {
                auditor.auditEngine(path);
            }
        }
    }
    
    auditor.generateReport();
    
    return 0;
}