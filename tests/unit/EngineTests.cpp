#include <JuceHeader.h>
#include "../Source/EngineFactory.h"
#include "../Source/ParameterDefinitions.h"
#include <memory>
#include <iostream>

class EngineTests : public juce::UnitTest {
public:
    EngineTests() : UnitTest("DSP Engine Tests", "Engines") {}
    
    void runTest() override {
        beginTest("Engine Factory Creation");
        testEngineCreation();
        
        beginTest("Rodent Distortion Processing");
        testRodentDistortion();
        
        beginTest("Muff Fuzz Processing");
        testMuffFuzz();
        
        beginTest("Classic Tremolo Processing");
        testClassicTremolo();
        
        beginTest("Parameter Updates");
        testParameterUpdates();
    }
    
private:
    void testEngineCreation() {
        // Test that all engines can be created
        auto rodent = EngineFactory::createEngine(ENGINE_RODENT_DISTORTION);
        expect(rodent != nullptr, "Rodent Distortion should be created");
        expect(rodent->getName() == "Rodent Distortion", "Name should match");
        expect(rodent->getNumParameters() == 6, "Should have 6 parameters");
        
        auto muff = EngineFactory::createEngine(ENGINE_MUFF_FUZZ);
        expect(muff != nullptr, "Muff Fuzz should be created");
        expect(muff->getName() == "Muff Fuzz", "Name should match");
        expect(muff->getNumParameters() == 4, "Should have 4 parameters");
        
        auto tremolo = EngineFactory::createEngine(ENGINE_CLASSIC_TREMOLO);
        expect(tremolo != nullptr, "Classic Tremolo should be created");
        expect(tremolo->getName() == "Classic Tremolo", "Name should match");
        expect(tremolo->getNumParameters() == 5, "Should have 5 parameters");
    }
    
    void testRodentDistortion() {
        auto engine = EngineFactory::createEngine(ENGINE_RODENT_DISTORTION);
        engine->prepareToPlay(44100.0, 512);
        
        // Create test buffer with sine wave
        juce::AudioBuffer<float> buffer(2, 512);
        fillWithSineWave(buffer, 440.0f, 44100.0);
        
        // Process
        engine->process(buffer);
        
        // Check that output is non-zero and contains distortion
        float rms = calculateRMS(buffer);
        expect(rms > 0.0f, "Output should be non-zero");
        
        // Check for harmonic distortion
        float thd = calculateTHD(buffer, 440.0f, 44100.0);
        expect(thd > 0.01f, "Should introduce harmonic distortion");
    }
    
    void testMuffFuzz() {
        auto engine = EngineFactory::createEngine(ENGINE_MUFF_FUZZ);
        engine->prepareToPlay(44100.0, 512);
        
        // Create test buffer
        juce::AudioBuffer<float> buffer(2, 512);
        fillWithSineWave(buffer, 440.0f, 44100.0);
        
        // Set high sustain
        std::map<int, float> params;
        params[0] = 0.9f;  // High sustain
        engine->updateParameters(params);
        
        // Process
        engine->process(buffer);
        
        // Check for fuzz characteristics
        float peakLevel = findPeakLevel(buffer);
        expect(peakLevel < 1.0f, "Should be clipped");
        
        float thd = calculateTHD(buffer, 440.0f, 44100.0);
        expect(thd > 0.1f, "Should have significant harmonic distortion");
    }
    
    void testClassicTremolo() {
        auto engine = EngineFactory::createEngine(ENGINE_CLASSIC_TREMOLO);
        engine->prepareToPlay(44100.0, 512);
        
        // Set tremolo parameters
        std::map<int, float> params;
        params[0] = 0.5f;  // 10 Hz rate
        params[1] = 0.8f;  // 80% depth
        engine->updateParameters(params);
        
        // Create longer buffer to see modulation
        juce::AudioBuffer<float> buffer(2, 4410);  // 100ms at 44.1kHz
        fillWithConstant(buffer, 1.0f);
        
        // Process
        engine->process(buffer);
        
        // Check for amplitude modulation
        float min, max;
        findMinMax(buffer, min, max);
        
        expect(min < 0.9f, "Should have amplitude dips");
        expect(max > 0.9f, "Should have amplitude peaks");
        
        float modulationDepth = (max - min) / max;
        expect(modulationDepth > 0.3f, "Should have significant modulation");
    }
    
    void testParameterUpdates() {
        auto engine = EngineFactory::createEngine(ENGINE_RODENT_DISTORTION);
        
        // Test parameter names
        expect(engine->getParameterName(0) == "Gain", "Parameter 0 should be Gain");
        expect(engine->getParameterName(1) == "Filter", "Parameter 1 should be Filter");
        expect(engine->getParameterName(2) == "Clipping", "Parameter 2 should be Clipping");
        
        // Test parameter updates
        std::map<int, float> params;
        params[0] = 0.0f;  // Min gain
        params[5] = 0.0f;  // Dry mix
        engine->updateParameters(params);
        
        // Process with dry mix
        juce::AudioBuffer<float> buffer(2, 512);
        fillWithSineWave(buffer, 440.0f, 44100.0);
        auto originalRMS = calculateRMS(buffer);
        
        engine->process(buffer);
        auto processedRMS = calculateRMS(buffer);
        
        expectWithinAbsoluteError(processedRMS, originalRMS, 0.01f, 
                                 "Dry mix should pass through unchanged");
    }
    
    // Helper functions
    void fillWithSineWave(juce::AudioBuffer<float>& buffer, float freq, float sampleRate) {
        const int numSamples = buffer.getNumSamples();
        const float phaseInc = 2.0f * M_PI * freq / sampleRate;
        float phase = 0.0f;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            phase = 0.0f;
            for (int i = 0; i < numSamples; ++i) {
                data[i] = 0.5f * std::sin(phase);
                phase += phaseInc;
            }
        }
    }
    
    void fillWithConstant(juce::AudioBuffer<float>& buffer, float value) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = value;
            }
        }
    }
    
    float calculateRMS(const juce::AudioBuffer<float>& buffer) {
        float sum = 0.0f;
        int totalSamples = 0;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                sum += data[i] * data[i];
                totalSamples++;
            }
        }
        
        return std::sqrt(sum / totalSamples);
    }
    
    float findPeakLevel(const juce::AudioBuffer<float>& buffer) {
        float peak = 0.0f;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                peak = std::max(peak, std::abs(data[i]));
            }
        }
        
        return peak;
    }
    
    void findMinMax(const juce::AudioBuffer<float>& buffer, float& min, float& max) {
        min = std::numeric_limits<float>::max();
        max = std::numeric_limits<float>::lowest();
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                min = std::min(min, data[i]);
                max = std::max(max, data[i]);
            }
        }
    }
    
    float calculateTHD(const juce::AudioBuffer<float>& buffer, float fundamental, float sampleRate) {
        // Simple THD estimation by comparing energy at harmonics
        // This is a simplified version for testing
        const int numSamples = buffer.getNumSamples();
        if (numSamples < 1024) return 0.0f;
        
        // Use first channel
        auto* data = buffer.getReadPointer(0);
        
        // Simple peak detection to estimate distortion
        float sumSquared = 0.0f;
        float sumAbs = 0.0f;
        
        for (int i = 0; i < numSamples; ++i) {
            sumSquared += data[i] * data[i];
            sumAbs += std::abs(data[i]);
        }
        
        float rms = std::sqrt(sumSquared / numSamples);
        float avgAbs = sumAbs / numSamples;
        
        // Crest factor as proxy for distortion
        float crestFactor = (avgAbs > 0) ? rms / avgAbs : 1.0f;
        
        // Pure sine wave has crest factor ~0.707, distorted signal will be different
        return std::abs(crestFactor - 0.707f);
    }
};

// Register the test
static EngineTests engineTests;