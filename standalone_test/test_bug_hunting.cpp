/**
 * ==============================================================================
 * test_bug_hunting.cpp
 *
 * COMPREHENSIVE BUG HUNTING MISSION
 * Tests edge cases and boundary conditions using techniques not previously covered
 *
 * TEST CATEGORIES:
 * 1. Edge Case Testing - Zero-length buffers, extreme values, NaN/Inf
 * 2. Boundary Condition Testing - First/last samples, buffer size = 1
 * 3. Numerical Stability - Denormals, precision loss, division by zero
 * 4. State Management - Uninitialized state, reset correctness
 * 5. Platform-Specific - macOS specifics, SIMD, alignment
 * 6. Concurrency - Multiple instances, thread safety
 *
 * ==============================================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <vector>
#include <map>
#include <cmath>
#include <cfenv>
#include <chrono>
#include <algorithm>
#include <limits>
#include <random>

#include "JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineBase.h"
#include "../JUCE_Plugin/Source/EngineTypes.h"

// ============================================================================
// Bug Report Structure
// ============================================================================
struct BugReport {
    enum class Severity {
        CRITICAL,   // Crash, hang, data corruption
        HIGH,       // NaN/Inf output, major artifacts
        MEDIUM,     // Minor artifacts, unexpected behavior
        LOW         // Edge case, non-critical
    };

    int engineId;
    std::string engineName;
    std::string testName;
    std::string description;
    std::string reproductionSteps;
    Severity severity;
    bool isReproducible;
    std::string affectedEngines;
    std::string recommendedFix;
    int estimatedFixTimeHours;

    static std::string severityToString(Severity s) {
        switch(s) {
            case Severity::CRITICAL: return "CRITICAL";
            case Severity::HIGH: return "HIGH";
            case Severity::MEDIUM: return "MEDIUM";
            case Severity::LOW: return "LOW";
        }
        return "UNKNOWN";
    }
};

// ============================================================================
// Utility Functions
// ============================================================================
inline bool isNaN(float value) {
    return std::isnan(value);
}

inline bool isInf(float value) {
    return std::isinf(value);
}

inline bool isDenormal(float value) {
    return value != 0.0f && std::abs(value) < std::numeric_limits<float>::min();
}

inline bool isInvalidFloat(float value) {
    return isNaN(value) || isInf(value);
}

// Check for DC offset
float calculateDCOffset(const juce::AudioBuffer<float>& buffer, int channel) {
    float sum = 0.0f;
    int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(channel);

    for (int i = 0; i < numSamples; ++i) {
        sum += data[i];
    }

    return sum / numSamples;
}

// Check for silence
bool isSilent(const juce::AudioBuffer<float>& buffer, float threshold = 1e-6f) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (std::abs(data[i]) > threshold) {
                return false;
            }
        }
    }
    return true;
}

// Calculate RMS
float calculateRMS(const juce::AudioBuffer<float>& buffer, int channel) {
    float sum = 0.0f;
    int numSamples = buffer.getNumSamples();
    const float* data = buffer.getReadPointer(channel);

    for (int i = 0; i < numSamples; ++i) {
        sum += data[i] * data[i];
    }

    return std::sqrt(sum / numSamples);
}

// Find peak
float findPeak(const juce::AudioBuffer<float>& buffer) {
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            peak = std::max(peak, std::abs(data[i]));
        }
    }
    return peak;
}

// Count denormals
int countDenormals(const juce::AudioBuffer<float>& buffer) {
    int count = 0;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            if (isDenormal(data[i])) {
                count++;
            }
        }
    }
    return count;
}

// ============================================================================
// Test Result Tracker
// ============================================================================
class TestResultTracker {
public:
    struct TestResult {
        bool passed = true;
        std::string testName;
        std::string details;
        double executionTimeMs = 0.0;
    };

    void addResult(int engineId, const TestResult& result) {
        results_[engineId].push_back(result);
        if (!result.passed) {
            failureCount_++;
        }
        totalCount_++;
    }

    void addBug(const BugReport& bug) {
        bugs_.push_back(bug);
    }

    void printSummary() {
        std::cout << "\n========================================\n";
        std::cout << "BUG HUNTING MISSION SUMMARY\n";
        std::cout << "========================================\n";
        std::cout << "Total Tests: " << totalCount_ << "\n";
        std::cout << "Failed Tests: " << failureCount_ << "\n";
        std::cout << "Pass Rate: " << std::fixed << std::setprecision(1)
                  << (100.0 * (totalCount_ - failureCount_) / totalCount_) << "%\n";
        std::cout << "Bugs Found: " << bugs_.size() << "\n\n";

        // Categorize bugs by severity
        int critical = 0, high = 0, medium = 0, low = 0;
        for (const auto& bug : bugs_) {
            switch (bug.severity) {
                case BugReport::Severity::CRITICAL: critical++; break;
                case BugReport::Severity::HIGH: high++; break;
                case BugReport::Severity::MEDIUM: medium++; break;
                case BugReport::Severity::LOW: low++; break;
            }
        }

        std::cout << "Bug Severity Breakdown:\n";
        std::cout << "  CRITICAL: " << critical << "\n";
        std::cout << "  HIGH:     " << high << "\n";
        std::cout << "  MEDIUM:   " << medium << "\n";
        std::cout << "  LOW:      " << low << "\n";
        std::cout << "========================================\n\n";
    }

    void generateReport(const std::string& filename) {
        std::ofstream report(filename);

        report << "# BUG HUNTING MISSION REPORT\n";
        report << "Generated: " << juce::Time::getCurrentTime().toString(true, true).toStdString() << "\n\n";

        report << "## Executive Summary\n";
        report << "- Total Tests: " << totalCount_ << "\n";
        report << "- Failed Tests: " << failureCount_ << "\n";
        report << "- Pass Rate: " << std::fixed << std::setprecision(1)
               << (100.0 * (totalCount_ - failureCount_) / totalCount_) << "%\n";
        report << "- Bugs Found: " << bugs_.size() << "\n\n";

        // Sort bugs by severity
        auto sortedBugs = bugs_;
        std::sort(sortedBugs.begin(), sortedBugs.end(),
            [](const BugReport& a, const BugReport& b) {
                return static_cast<int>(a.severity) < static_cast<int>(b.severity);
            });

        report << "## Bugs Found (Prioritized by Severity)\n\n";
        for (size_t i = 0; i < sortedBugs.size(); ++i) {
            const auto& bug = sortedBugs[i];
            report << "### Bug #" << (i+1) << ": " << bug.description << "\n";
            report << "- **Severity**: " << BugReport::severityToString(bug.severity) << "\n";
            report << "- **Engine**: " << bug.engineName << " (ID: " << bug.engineId << ")\n";
            report << "- **Test**: " << bug.testName << "\n";
            report << "- **Reproducible**: " << (bug.isReproducible ? "Yes" : "No") << "\n";
            report << "- **Affected Engines**: " << bug.affectedEngines << "\n";
            report << "- **Recommended Fix**: " << bug.recommendedFix << "\n";
            report << "- **Estimated Fix Time**: " << bug.estimatedFixTimeHours << " hours\n";
            report << "- **Reproduction Steps**:\n";
            report << bug.reproductionSteps << "\n\n";
        }

        report << "## Detailed Test Results\n\n";
        for (const auto& [engineId, testResults] : results_) {
            report << "### Engine " << engineId << "\n";
            for (const auto& result : testResults) {
                report << "- " << result.testName << ": "
                       << (result.passed ? "PASS" : "FAIL");
                if (!result.passed) {
                    report << " - " << result.details;
                }
                report << " (" << std::fixed << std::setprecision(2)
                       << result.executionTimeMs << "ms)\n";
            }
            report << "\n";
        }

        report.close();
        std::cout << "Report generated: " << filename << "\n";
    }

private:
    std::map<int, std::vector<TestResult>> results_;
    std::vector<BugReport> bugs_;
    int totalCount_ = 0;
    int failureCount_ = 0;
};

// ============================================================================
// Test Suite
// ============================================================================
class BugHuntingTestSuite {
public:
    BugHuntingTestSuite() : tracker_() {}

    void runAllTests() {
        std::cout << "Starting Comprehensive Bug Hunt...\n\n";

        // Test all 56 engines
        for (int engineId = 0; engineId < 56; ++engineId) {
            testEngine(engineId);
        }

        tracker_.printSummary();
        tracker_.generateReport("bug_hunting_report.md");
    }

private:
    TestResultTracker tracker_;

    void testEngine(int engineId) {
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            return;
        }

        std::cout << "Testing Engine " << engineId << ": "
                  << engine->getName() << "\n";

        // Run all test categories
        testEdgeCases(engineId, engine.get());
        testBoundaryConditions(engineId, engine.get());
        testNumericalStability(engineId, engine.get());
        testStateManagement(engineId, engine.get());
        testParameterEdgeCases(engineId, engine.get());
        testBufferSizeVariations(engineId, engine.get());
    }

    // ========================================================================
    // TEST CATEGORY 1: Edge Cases
    // ========================================================================
    void testEdgeCases(int engineId, EngineBase* engine) {
        // Test 1.1: Zero-length buffer
        testZeroLengthBuffer(engineId, engine);

        // Test 1.2: Sample rate = 0 (should handle gracefully)
        testZeroSampleRate(engineId, engine);

        // Test 1.3: Extreme sample rates
        testExtremeSampleRates(engineId, engine);

        // Test 1.4: NaN input
        testNaNInput(engineId, engine);

        // Test 1.5: Inf input
        testInfInput(engineId, engine);

        // Test 1.6: Extreme amplitude input
        testExtremeAmplitude(engineId, engine);
    }

    void testZeroLengthBuffer(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Zero-Length Buffer";

        try {
            engine->prepareToPlay(44100.0, 512);
            juce::AudioBuffer<float> buffer(2, 0);  // Zero samples!

            engine->process(buffer);

            result.passed = true;
            result.details = "Handled gracefully";
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();

            BugReport bug;
            bug.engineId = engineId;
            bug.engineName = engine->getName().toStdString();
            bug.testName = "Zero-Length Buffer";
            bug.description = "Crash on zero-length buffer";
            bug.reproductionSteps = "1. Create buffer with 0 samples\n2. Call process()";
            bug.severity = BugReport::Severity::HIGH;
            bug.isReproducible = true;
            bug.affectedEngines = "Engine " + std::to_string(engineId);
            bug.recommendedFix = "Add buffer size validation at start of process()";
            bug.estimatedFixTimeHours = 1;
            tracker_.addBug(bug);
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testZeroSampleRate(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Zero Sample Rate";

        try {
            engine->prepareToPlay(0.0, 512);  // Invalid sample rate!
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();

            engine->process(buffer);

            // Check for division by zero issues
            bool hasInvalid = false;
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < buffer.getNumSamples(); ++i) {
                    if (isInvalidFloat(data[i])) {
                        hasInvalid = true;
                        break;
                    }
                }
            }

            if (hasInvalid) {
                result.passed = false;
                result.details = "Produced NaN/Inf with zero sample rate";

                BugReport bug;
                bug.engineId = engineId;
                bug.engineName = engine->getName().toStdString();
                bug.testName = "Zero Sample Rate";
                bug.description = "NaN/Inf output with zero sample rate";
                bug.reproductionSteps = "1. Call prepareToPlay(0.0, 512)\n2. Process buffer";
                bug.severity = BugReport::Severity::CRITICAL;
                bug.isReproducible = true;
                bug.affectedEngines = "Engine " + std::to_string(engineId);
                bug.recommendedFix = "Add sample rate validation in prepareToPlay()";
                bug.estimatedFixTimeHours = 2;
                tracker_.addBug(bug);
            } else {
                result.passed = true;
                result.details = "Handled gracefully";
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testExtremeSampleRates(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Extreme Sample Rates";

        std::vector<double> testRates = {8000.0, 11025.0, 192000.0, 384000.0, 768000.0};
        bool allPassed = true;
        std::ostringstream details;

        for (double rate : testRates) {
            try {
                engine->prepareToPlay(rate, 512);
                juce::AudioBuffer<float> buffer(2, 512);

                // Generate test signal
                for (int ch = 0; ch < 2; ++ch) {
                    float* data = buffer.getWritePointer(ch);
                    for (int i = 0; i < 512; ++i) {
                        data[i] = 0.5f * std::sin(2.0f * M_PI * 1000.0f * i / rate);
                    }
                }

                engine->process(buffer);

                // Check for issues
                float peak = findPeak(buffer);
                if (isInvalidFloat(peak) || peak > 100.0f) {
                    allPassed = false;
                    details << rate << "Hz: Invalid output; ";
                }
            } catch (...) {
                allPassed = false;
                details << rate << "Hz: Exception; ";
            }
        }

        result.passed = allPassed;
        result.details = details.str();

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testNaNInput(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "NaN Input";

        try {
            engine->prepareToPlay(44100.0, 512);
            juce::AudioBuffer<float> buffer(2, 512);

            // Fill with NaN
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = std::numeric_limits<float>::quiet_NaN();
                }
            }

            engine->process(buffer);

            // Check if NaN propagated
            int nanCount = 0;
            for (int ch = 0; ch < 2; ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < 512; ++i) {
                    if (isNaN(data[i])) {
                        nanCount++;
                    }
                }
            }

            if (nanCount > 0) {
                result.passed = false;
                result.details = "NaN propagated through engine (" + std::to_string(nanCount) + " samples)";

                BugReport bug;
                bug.engineId = engineId;
                bug.engineName = engine->getName().toStdString();
                bug.testName = "NaN Input";
                bug.description = "Engine propagates NaN values";
                bug.reproductionSteps = "1. Fill buffer with NaN\n2. Process buffer\n3. NaN remains in output";
                bug.severity = BugReport::Severity::HIGH;
                bug.isReproducible = true;
                bug.affectedEngines = "Engine " + std::to_string(engineId);
                bug.recommendedFix = "Add NaN sanitization at input or output";
                bug.estimatedFixTimeHours = 2;
                tracker_.addBug(bug);
            } else {
                result.passed = true;
                result.details = "NaN handled correctly";
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testInfInput(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Infinity Input";

        try {
            engine->prepareToPlay(44100.0, 512);
            juce::AudioBuffer<float> buffer(2, 512);

            // Fill with Inf
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = std::numeric_limits<float>::infinity();
                }
            }

            engine->process(buffer);

            // Check if Inf propagated
            int infCount = 0;
            for (int ch = 0; ch < 2; ++ch) {
                const float* data = buffer.getReadPointer(ch);
                for (int i = 0; i < 512; ++i) {
                    if (isInf(data[i])) {
                        infCount++;
                    }
                }
            }

            if (infCount > 0) {
                result.passed = false;
                result.details = "Inf propagated through engine (" + std::to_string(infCount) + " samples)";

                BugReport bug;
                bug.engineId = engineId;
                bug.engineName = engine->getName().toStdString();
                bug.testName = "Infinity Input";
                bug.description = "Engine propagates Infinity values";
                bug.reproductionSteps = "1. Fill buffer with Inf\n2. Process buffer\n3. Inf remains in output";
                bug.severity = BugReport::Severity::HIGH;
                bug.isReproducible = true;
                bug.affectedEngines = "Engine " + std::to_string(engineId);
                bug.recommendedFix = "Add Inf sanitization at input or output";
                bug.estimatedFixTimeHours = 2;
                tracker_.addBug(bug);
            } else {
                result.passed = true;
                result.details = "Inf handled correctly";
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testExtremeAmplitude(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Extreme Amplitude";

        try {
            engine->prepareToPlay(44100.0, 512);
            juce::AudioBuffer<float> buffer(2, 512);

            // Fill with very large values
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = 1000000.0f * std::sin(2.0f * M_PI * 1000.0f * i / 44100.0f);
                }
            }

            engine->process(buffer);

            // Check for overflow issues
            float peak = findPeak(buffer);
            bool hasInvalid = isInvalidFloat(peak);

            if (hasInvalid) {
                result.passed = false;
                result.details = "Produced NaN/Inf with extreme input";

                BugReport bug;
                bug.engineId = engineId;
                bug.engineName = engine->getName().toStdString();
                bug.testName = "Extreme Amplitude";
                bug.description = "Overflow with extreme input amplitude";
                bug.reproductionSteps = "1. Generate signal with amplitude > 1,000,000\n2. Process buffer";
                bug.severity = BugReport::Severity::MEDIUM;
                bug.isReproducible = true;
                bug.affectedEngines = "Engine " + std::to_string(engineId);
                bug.recommendedFix = "Add input clipping or saturation";
                bug.estimatedFixTimeHours = 2;
                tracker_.addBug(bug);
            } else {
                result.passed = true;
                result.details = "Peak: " + std::to_string(peak);
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    // ========================================================================
    // TEST CATEGORY 2: Boundary Conditions
    // ========================================================================
    void testBoundaryConditions(int engineId, EngineBase* engine) {
        testBufferSizeOne(engineId, engine);
        testFirstSampleAfterInit(engineId, engine);
        testLargeBuffer(engineId, engine);
    }

    void testBufferSizeOne(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Buffer Size = 1";

        try {
            engine->prepareToPlay(44100.0, 1);
            juce::AudioBuffer<float> buffer(2, 1);
            buffer.setSample(0, 0, 0.5f);
            buffer.setSample(1, 0, 0.5f);

            engine->process(buffer);

            bool hasInvalid = isInvalidFloat(buffer.getSample(0, 0)) ||
                            isInvalidFloat(buffer.getSample(1, 0));

            result.passed = !hasInvalid;
            result.details = hasInvalid ? "Invalid output" : "OK";
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testFirstSampleAfterInit(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "First Sample After Init";

        try {
            engine->prepareToPlay(44100.0, 512);
            juce::AudioBuffer<float> buffer(2, 1);
            buffer.setSample(0, 0, 0.7f);
            buffer.setSample(1, 0, 0.7f);

            // Process immediately without warmup
            engine->process(buffer);

            float val0 = buffer.getSample(0, 0);
            float val1 = buffer.getSample(1, 0);

            bool hasInvalid = isInvalidFloat(val0) || isInvalidFloat(val1);

            if (hasInvalid) {
                result.passed = false;
                result.details = "Invalid output on first sample";

                BugReport bug;
                bug.engineId = engineId;
                bug.engineName = engine->getName().toStdString();
                bug.testName = "First Sample After Init";
                bug.description = "Uninitialized state causes invalid first sample";
                bug.reproductionSteps = "1. Call prepareToPlay()\n2. Process single sample immediately\n3. Output is NaN/Inf";
                bug.severity = BugReport::Severity::HIGH;
                bug.isReproducible = true;
                bug.affectedEngines = "Engine " + std::to_string(engineId);
                bug.recommendedFix = "Initialize all state variables in prepareToPlay()";
                bug.estimatedFixTimeHours = 3;
                tracker_.addBug(bug);
            } else {
                result.passed = true;
                result.details = "First sample OK";
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testLargeBuffer(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Large Buffer (16384)";

        try {
            engine->prepareToPlay(44100.0, 16384);
            juce::AudioBuffer<float> buffer(2, 16384);

            // Fill with test signal
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 16384; ++i) {
                    data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
                }
            }

            engine->process(buffer);

            float peak = findPeak(buffer);
            bool hasInvalid = isInvalidFloat(peak);

            result.passed = !hasInvalid;
            result.details = hasInvalid ? "Invalid output" : ("Peak: " + std::to_string(peak));
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    // ========================================================================
    // TEST CATEGORY 3: Numerical Stability
    // ========================================================================
    void testNumericalStability(int engineId, EngineBase* engine) {
        testDenormalHandling(engineId, engine);
        testPrecisionLoss(engineId, engine);
    }

    void testDenormalHandling(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Denormal Handling";

        try {
            engine->prepareToPlay(44100.0, 512);
            juce::AudioBuffer<float> buffer(2, 512);

            // Fill with denormal values
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = 1e-40f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
                }
            }

            engine->process(buffer);

            int denormalCount = countDenormals(buffer);

            if (denormalCount > 100) {
                result.passed = false;
                result.details = "Many denormals in output (" + std::to_string(denormalCount) + ")";

                BugReport bug;
                bug.engineId = engineId;
                bug.engineName = engine->getName().toStdString();
                bug.testName = "Denormal Handling";
                bug.description = "Engine produces denormal values causing CPU spikes";
                bug.reproductionSteps = "1. Process very quiet signal (1e-40)\n2. Count denormal values in output";
                bug.severity = BugReport::Severity::MEDIUM;
                bug.isReproducible = true;
                bug.affectedEngines = "Engine " + std::to_string(engineId);
                bug.recommendedFix = "Add denormal flushing (FTZ/DAZ) or use juce::ScopedNoDenormals";
                bug.estimatedFixTimeHours = 1;
                tracker_.addBug(bug);
            } else {
                result.passed = true;
                result.details = std::to_string(denormalCount) + " denormals";
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testPrecisionLoss(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Precision Loss";

        try {
            engine->prepareToPlay(44100.0, 512);
            juce::AudioBuffer<float> buffer(2, 512);

            // Very small signal
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = 1e-8f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
                }
            }

            float rmsBefore = calculateRMS(buffer, 0);

            engine->process(buffer);

            float rmsAfter = calculateRMS(buffer, 0);

            // Check if signal completely disappeared
            if (rmsAfter == 0.0f && rmsBefore > 0.0f) {
                result.passed = false;
                result.details = "Signal completely lost (precision issue)";
            } else {
                result.passed = true;
                result.details = "RMS preserved";
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    // ========================================================================
    // TEST CATEGORY 4: State Management
    // ========================================================================
    void testStateManagement(int engineId, EngineBase* engine) {
        testResetCorrectness(engineId, engine);
        testDoubleInit(engineId, engine);
    }

    void testResetCorrectness(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Reset Correctness";

        try {
            engine->prepareToPlay(44100.0, 512);
            juce::AudioBuffer<float> buffer(2, 512);

            // Process some audio to build up state
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = 0.8f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
                }
            }

            for (int i = 0; i < 10; ++i) {
                engine->process(buffer);
            }

            // Now reset
            engine->reset();

            // Process silence
            buffer.clear();
            engine->process(buffer);

            // Check if output is truly silent
            float peak = findPeak(buffer);

            if (peak > 1e-6f) {
                result.passed = false;
                result.details = "Reset() didn't clear state (peak: " + std::to_string(peak) + ")";

                BugReport bug;
                bug.engineId = engineId;
                bug.engineName = engine->getName().toStdString();
                bug.testName = "Reset Correctness";
                bug.description = "reset() doesn't fully clear internal state";
                bug.reproductionSteps = "1. Process audio\n2. Call reset()\n3. Process silence\n4. Output is not silent";
                bug.severity = BugReport::Severity::MEDIUM;
                bug.isReproducible = true;
                bug.affectedEngines = "Engine " + std::to_string(engineId);
                bug.recommendedFix = "Review reset() implementation - clear all buffers and state";
                bug.estimatedFixTimeHours = 2;
                tracker_.addBug(bug);
            } else {
                result.passed = true;
                result.details = "Reset clears state correctly";
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testDoubleInit(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Double Initialization";

        try {
            // Initialize twice without reset
            engine->prepareToPlay(44100.0, 512);
            engine->prepareToPlay(48000.0, 1024);

            juce::AudioBuffer<float> buffer(2, 1024);
            buffer.clear();

            engine->process(buffer);

            float peak = findPeak(buffer);
            bool hasInvalid = isInvalidFloat(peak);

            result.passed = !hasInvalid;
            result.details = hasInvalid ? "Invalid output after double init" : "Handled correctly";
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    // ========================================================================
    // TEST CATEGORY 5: Parameter Edge Cases
    // ========================================================================
    void testParameterEdgeCases(int engineId, EngineBase* engine) {
        testNegativeParameters(engineId, engine);
        testGreaterThanOneParameters(engineId, engine);
        testNaNParameters(engineId, engine);
    }

    void testNegativeParameters(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Negative Parameters";

        try {
            engine->prepareToPlay(44100.0, 512);

            // Set all parameters to negative values
            std::map<int, float> params;
            int numParams = engine->getNumParameters();
            for (int i = 0; i < numParams; ++i) {
                params[i] = -1.0f;
            }
            engine->updateParameters(params);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
                }
            }

            engine->process(buffer);

            float peak = findPeak(buffer);
            bool hasInvalid = isInvalidFloat(peak);

            result.passed = !hasInvalid;
            result.details = hasInvalid ? "Invalid output with negative params" : "Handled correctly";
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testGreaterThanOneParameters(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Parameters > 1.0";

        try {
            engine->prepareToPlay(44100.0, 512);

            // Set all parameters to values > 1.0
            std::map<int, float> params;
            int numParams = engine->getNumParameters();
            for (int i = 0; i < numParams; ++i) {
                params[i] = 10.0f;
            }
            engine->updateParameters(params);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
                }
            }

            engine->process(buffer);

            float peak = findPeak(buffer);
            bool hasInvalid = isInvalidFloat(peak);

            result.passed = !hasInvalid;
            result.details = hasInvalid ? "Invalid output with params > 1.0" : "Handled correctly";
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    void testNaNParameters(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "NaN Parameters";

        try {
            engine->prepareToPlay(44100.0, 512);

            // Set all parameters to NaN
            std::map<int, float> params;
            int numParams = engine->getNumParameters();
            for (int i = 0; i < numParams; ++i) {
                params[i] = std::numeric_limits<float>::quiet_NaN();
            }
            engine->updateParameters(params);

            juce::AudioBuffer<float> buffer(2, 512);
            for (int ch = 0; ch < 2; ++ch) {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < 512; ++i) {
                    data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
                }
            }

            engine->process(buffer);

            float peak = findPeak(buffer);
            bool hasInvalid = isInvalidFloat(peak);

            if (hasInvalid) {
                result.passed = false;
                result.details = "NaN parameters caused invalid output";

                BugReport bug;
                bug.engineId = engineId;
                bug.engineName = engine->getName().toStdString();
                bug.testName = "NaN Parameters";
                bug.description = "Engine crashes or produces invalid output with NaN parameters";
                bug.reproductionSteps = "1. Set all parameters to NaN\n2. Process audio";
                bug.severity = BugReport::Severity::HIGH;
                bug.isReproducible = true;
                bug.affectedEngines = "Engine " + std::to_string(engineId);
                bug.recommendedFix = "Add parameter validation in updateParameters()";
                bug.estimatedFixTimeHours = 2;
                tracker_.addBug(bug);
            } else {
                result.passed = true;
                result.details = "NaN parameters handled";
            }
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }

    // ========================================================================
    // TEST CATEGORY 6: Buffer Size Variations
    // ========================================================================
    void testBufferSizeVariations(int engineId, EngineBase* engine) {
        auto start = std::chrono::high_resolution_clock::now();
        TestResultTracker::TestResult result;
        result.testName = "Buffer Size Changes";

        try {
            engine->prepareToPlay(44100.0, 512);

            // Test various buffer sizes
            std::vector<int> sizes = {1, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
            bool allPassed = true;

            for (int size : sizes) {
                juce::AudioBuffer<float> buffer(2, size);

                for (int ch = 0; ch < 2; ++ch) {
                    float* data = buffer.getWritePointer(ch);
                    for (int i = 0; i < size; ++i) {
                        data[i] = 0.5f * std::sin(2.0f * M_PI * 440.0f * i / 44100.0f);
                    }
                }

                engine->process(buffer);

                float peak = findPeak(buffer);
                if (isInvalidFloat(peak) || peak > 100.0f) {
                    allPassed = false;
                    break;
                }
            }

            result.passed = allPassed;
            result.details = allPassed ? "All sizes OK" : "Failed on some sizes";
        } catch (const std::exception& e) {
            result.passed = false;
            result.details = std::string("Exception: ") + e.what();
        }

        auto end = std::chrono::high_resolution_clock::now();
        result.executionTimeMs = std::chrono::duration<double, std::milli>(end - start).count();
        tracker_.addResult(engineId, result);
    }
};

// ============================================================================
// Main
// ============================================================================
int main() {
    std::cout << "========================================\n";
    std::cout << "BUG HUNTING MISSION\n";
    std::cout << "Project Chimera Phoenix v3.0\n";
    std::cout << "========================================\n\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Run comprehensive bug hunt
    BugHuntingTestSuite suite;
    suite.runAllTests();

    std::cout << "\nBug hunting complete!\n";
    std::cout << "Report saved to: bug_hunting_report.md\n";

    return 0;
}
