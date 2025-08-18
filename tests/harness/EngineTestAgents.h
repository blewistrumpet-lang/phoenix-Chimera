#pragma once

#include <JuceHeader.h>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include "EngineBase.h"
#include "TestSignalGenerator.h"
#include "AudioMeasurements.h"
#include "EngineTypes.h"

/**
 * Specialized Test Agents for Different Effect Categories
 * 
 * Each test agent knows how to properly test its specific effect type,
 * generating appropriate test signals and measuring relevant characteristics.
 */

namespace EngineTestAgents {

    // Base test result structure
    struct TestResult {
        bool passed = false;
        std::string testName;
        std::string description;
        float measuredValue = 0.0f;
        float expectedRange[2] = {0.0f, 0.0f}; // min, max
        std::string units;
        std::vector<float> measurements; // For plotting
        std::string failureReason;
        
        bool isInRange() const {
            return measuredValue >= expectedRange[0] && measuredValue <= expectedRange[1];
        }
    };

    struct EngineTestSuite {
        std::string engineName;
        int engineType;
        std::vector<TestResult> results;
        bool overallPassed = false;
        float processingTime = 0.0f; // milliseconds
        
        void calculateOverallResult() {
            overallPassed = true;
            for (const auto& result : results) {
                if (!result.passed) {
                    overallPassed = false;
                    break;
                }
            }
        }
    };

    // Base test agent interface
    class TestAgentBase {
    public:
        virtual ~TestAgentBase() = default;
        virtual EngineTestSuite runTests(std::unique_ptr<EngineBase> engine, double sampleRate = 44100.0) = 0;
        virtual std::string getAgentName() const = 0;
        
    protected:
        // Common helper functions
        TestResult createBasicFunctionTest(EngineBase* engine, double sampleRate);
        TestResult createParameterResponseTest(EngineBase* engine, int paramIndex, double sampleRate);
        TestResult createSilenceTest(EngineBase* engine, double sampleRate);
        TestResult createLatencyTest(EngineBase* engine, double sampleRate);
        bool detectProcessingActivity(const juce::AudioBuffer<float>& input, const juce::AudioBuffer<float>& output);
    };

    // Dynamics Effects Test Agent (Compressors, Limiters, Gates)
    class DynamicsTestAgent : public TestAgentBase {
    public:
        EngineTestSuite runTests(std::unique_ptr<EngineBase> engine, double sampleRate = 44100.0) override;
        std::string getAgentName() const override { return "Dynamics"; }
        
    private:
        TestResult testGainReduction(EngineBase* engine, double sampleRate);
        TestResult testThresholdResponse(EngineBase* engine, double sampleRate);
        TestResult testAttackRelease(EngineBase* engine, double sampleRate);
        TestResult testRatioResponse(EngineBase* engine, double sampleRate);
        TestResult testMakeupGain(EngineBase* engine, double sampleRate);
        TestResult testKneeResponse(EngineBase* engine, double sampleRate);
    };

    // Filter/EQ Effects Test Agent
    class FilterTestAgent : public TestAgentBase {
    public:
        EngineTestSuite runTests(std::unique_ptr<EngineBase> engine, double sampleRate = 44100.0) override;
        std::string getAgentName() const override { return "Filter/EQ"; }
        
    private:
        TestResult testFrequencyResponse(EngineBase* engine, double sampleRate);
        TestResult testCutoffSweep(EngineBase* engine, double sampleRate);
        TestResult testResonanceEffect(EngineBase* engine, double sampleRate);
        TestResult testFilterStability(EngineBase* engine, double sampleRate);
        TestResult testGainResponse(EngineBase* engine, double sampleRate);
        TestResult testQualityFactor(EngineBase* engine, double sampleRate);
    };

    // Time-based Effects Test Agent (Reverbs, Delays)
    class TimeBasedTestAgent : public TestAgentBase {
    public:
        EngineTestSuite runTests(std::unique_ptr<EngineBase> engine, double sampleRate = 44100.0) override;
        std::string getAgentName() const override { return "Time-based"; }
        
    private:
        TestResult testImpulseResponse(EngineBase* engine, double sampleRate);
        TestResult testDelayTime(EngineBase* engine, double sampleRate);
        TestResult testFeedbackResponse(EngineBase* engine, double sampleRate);
        TestResult testDecayTime(EngineBase* engine, double sampleRate);
        TestResult testDryWetMix(EngineBase* engine, double sampleRate);
        TestResult testEchoClarity(EngineBase* engine, double sampleRate);
    };

    // Modulation Effects Test Agent (Chorus, Phaser, Tremolo)
    class ModulationTestAgent : public TestAgentBase {
    public:
        EngineTestSuite runTests(std::unique_ptr<EngineBase> engine, double sampleRate = 44100.0) override;
        std::string getAgentName() const override { return "Modulation"; }
        
    private:
        TestResult testLFORate(EngineBase* engine, double sampleRate);
        TestResult testModulationDepth(EngineBase* engine, double sampleRate);
        TestResult testStereoWidth(EngineBase* engine, double sampleRate);
        TestResult testModulationShape(EngineBase* engine, double sampleRate);
        TestResult testPhaseResponse(EngineBase* engine, double sampleRate);
        TestResult testChorusVoices(EngineBase* engine, double sampleRate);
    };

    // Distortion Effects Test Agent (Overdrive, Fuzz, Saturation)
    class DistortionTestAgent : public TestAgentBase {
    public:
        EngineTestSuite runTests(std::unique_ptr<EngineBase> engine, double sampleRate = 44100.0) override;
        std::string getAgentName() const override { return "Distortion"; }
        
    private:
        TestResult testHarmonicGeneration(EngineBase* engine, double sampleRate);
        TestResult testTHDMeasurement(EngineBase* engine, double sampleRate);
        TestResult testSaturationCurve(EngineBase* engine, double sampleRate);
        TestResult testDriveResponse(EngineBase* engine, double sampleRate);
        TestResult testToneShaping(EngineBase* engine, double sampleRate);
        TestResult testOverdriveCharacter(EngineBase* engine, double sampleRate);
    };

    // Spectral Effects Test Agent (Pitch Shift, Vocoder, Spectral)
    class SpectralTestAgent : public TestAgentBase {
    public:
        EngineTestSuite runTests(std::unique_ptr<EngineBase> engine, double sampleRate = 44100.0) override;
        std::string getAgentName() const override { return "Spectral"; }
        
    private:
        TestResult testFrequencyShifting(EngineBase* engine, double sampleRate);
        TestResult testPitchShifting(EngineBase* engine, double sampleRate);
        TestResult testFormantPreservation(EngineBase* engine, double sampleRate);
        TestResult testSpectralResolution(EngineBase* engine, double sampleRate);
        TestResult testArtifactLevel(EngineBase* engine, double sampleRate);
        TestResult testTransientHandling(EngineBase* engine, double sampleRate);
    };

    // Factory function to create appropriate test agent based on engine type
    class TestAgentFactory {
    public:
        static std::unique_ptr<TestAgentBase> createTestAgent(int engineType);
        static std::string getEffectCategoryName(int engineType);
        
    private:
        static bool isDynamicsEffect(int engineType);
        static bool isFilterEffect(int engineType);
        static bool isTimeBasedEffect(int engineType);
        static bool isModulationEffect(int engineType);
        static bool isDistortionEffect(int engineType);
        static bool isSpectralEffect(int engineType);
    };

    // Utility functions for test validation
    namespace TestUtils {
        bool validateParameterRange(float value, float min, float max);
        float calculatePercentageChange(float original, float modified);
        std::vector<float> generateParameterSweep(float min, float max, int steps);
        juce::AudioBuffer<float> generateTestSignalForCategory(const std::string& category, double sampleRate);
        bool isSignificantChange(float before, float after, float threshold = 0.05f);
        std::string formatMeasurement(float value, const std::string& units, int precision = 2);
    }
}