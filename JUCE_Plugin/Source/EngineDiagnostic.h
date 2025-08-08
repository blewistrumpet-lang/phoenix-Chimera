#pragma once
#include <JuceHeader.h>
#include <map>
#include <string>

/**
 * Engine Diagnostic Tool
 * 
 * This utility helps debug engine processing by measuring:
 * - Input vs output levels
 * - Whether audio is being modified
 * - Parameter settings and their effects
 * - Wet/dry mix behavior
 */
class EngineDiagnostic {
public:
    struct DiagnosticResult {
        std::string engineName;
        float inputRMS = 0.0f;
        float outputRMS = 0.0f;
        float inputPeak = 0.0f;
        float outputPeak = 0.0f;
        float gainChange_dB = 0.0f;
        bool audioModified = false;
        float mixValue = 0.0f;
        std::map<int, float> parameters;
        bool isProcessing = false;
        
        std::string toString() const;
    };
    
    // Test specific engines with known parameters
    static DiagnosticResult testPlateReverb(juce::AudioBuffer<float>& testBuffer);
    static DiagnosticResult testClassicCompressor(juce::AudioBuffer<float>& testBuffer);
    static DiagnosticResult testRodentDistortion(juce::AudioBuffer<float>& testBuffer);
    
    // Generic engine test
    static DiagnosticResult testEngine(juce::AudioBuffer<float>& testBuffer, 
                                     const std::string& engineName,
                                     class EngineBase* engine,
                                     const std::map<int, float>& testParams);
    
    // Audio analysis utilities
    static float calculateRMS(const juce::AudioBuffer<float>& buffer);
    static float calculatePeak(const juce::AudioBuffer<float>& buffer);
    static bool buffersAreDifferent(const juce::AudioBuffer<float>& buffer1,
                                  const juce::AudioBuffer<float>& buffer2,
                                  float threshold = 0.0001f);
    
    // Generate test signals
    static void generateTestTone(juce::AudioBuffer<float>& buffer, 
                               float frequency = 1000.0f, 
                               float amplitude = 0.5f,
                               double sampleRate = 44100.0);
    
    static void generateWhiteNoise(juce::AudioBuffer<float>& buffer,
                                 float amplitude = 0.2f,
                                 int seed = 12345);
    
    // Print diagnostic results
    static void printResults(const std::vector<DiagnosticResult>& results);
    
    // Run comprehensive test suite
    static std::vector<DiagnosticResult> runComprehensiveTest(double sampleRate = 44100.0,
                                                            int blockSize = 512);
};