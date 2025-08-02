#include <JuceHeader.h>
#include "../Source/EngineFactory.h"
#include "../Source/ParameterDefinitions.h"
#include <memory>
#include <cmath>

class RotarySpeakerTest : public juce::UnitTest {
public:
    RotarySpeakerTest() : UnitTest("Rotary Speaker Engine Test", "Engines") {}
    
    void runTest() override {
        beginTest("Engine Creation and Basic Properties");
        testEngineCreation();
        
        beginTest("Doppler Effect Processing");
        testDopplerEffect();
        
        beginTest("Speed Control and Acceleration");
        testSpeedControl();
        
        beginTest("Stereo Field Generation");
        testStereoProcessing();
        
        beginTest("Cabinet Resonance");
        testCabinetResonance();
    }
    
private:
    void testEngineCreation() {
        auto engine = EngineFactory::createEngine(ENGINE_ROTARY_SPEAKER);
        
        expect(engine != nullptr, "Rotary Speaker engine should be created");
        expect(engine->getName() == "Rotary Speaker", "Name should match");
        expect(engine->getNumParameters() == 4, "Should have 4 parameters");
        
        // Check parameter names
        expect(engine->getParameterName(0) == "Speed", "Parameter 0 should be Speed");
        expect(engine->getParameterName(1) == "Acceleration", "Parameter 1 should be Acceleration");
        expect(engine->getParameterName(2) == "Mic Distance", "Parameter 2 should be Mic Distance");
        expect(engine->getParameterName(3) == "Stereo Width", "Parameter 3 should be Stereo Width");
    }
    
    void testDopplerEffect() {
        auto engine = EngineFactory::createEngine(ENGINE_ROTARY_SPEAKER);
        engine->prepareToPlay(44100.0, 512);
        
        // Set medium speed for clear Doppler effect
        std::map<int, float> params;
        params[0] = 0.5f;  // Medium speed
        params[1] = 0.5f;  // Normal acceleration
        params[2] = 0.5f;  // Medium mic distance
        params[3] = 1.0f;  // Full stereo
        engine->updateParameters(params);
        
        // Create test signal - constant sine wave
        juce::AudioBuffer<float> buffer(2, 4410); // 100ms at 44.1kHz
        fillWithSineWave(buffer, 440.0f, 44100.0);
        
        // Process
        engine->process(buffer);
        
        // Analyze pitch variation (Doppler effect)
        float minPitch, maxPitch;
        analyzePitchVariation(buffer, 440.0f, 44100.0, minPitch, maxPitch);
        
        // With Doppler effect, we should see pitch variation
        float pitchVariation = maxPitch - minPitch;
        expect(pitchVariation > 5.0f, "Should have noticeable pitch variation from Doppler effect");
        expect(pitchVariation < 50.0f, "Pitch variation should be reasonable");
    }
    
    void testSpeedControl() {
        auto engine = EngineFactory::createEngine(ENGINE_ROTARY_SPEAKER);
        engine->prepareToPlay(44100.0, 512);
        
        // Test slow speed
        std::map<int, float> params;
        params[0] = 0.0f;  // Slow speed
        params[1] = 0.0f;  // Instant acceleration (for testing)
        engine->updateParameters(params);
        
        juce::AudioBuffer<float> bufferSlow(2, 44100); // 1 second
        fillWithImpulse(bufferSlow, 1000, 44100.0);
        engine->process(bufferSlow);
        
        float modRateSlow = calculateModulationRate(bufferSlow, 44100.0);
        
        // Test fast speed
        params[0] = 1.0f;  // Fast speed
        engine->updateParameters(params);
        
        juce::AudioBuffer<float> bufferFast(2, 44100);
        fillWithImpulse(bufferFast, 1000, 44100.0);
        engine->process(bufferFast);
        
        float modRateFast = calculateModulationRate(bufferFast, 44100.0);
        
        // Fast should be significantly faster than slow
        expect(modRateFast > modRateSlow * 5.0f, "Fast speed should be much faster than slow");
        expect(modRateSlow < 2.0f, "Slow speed should be below 2 Hz");
        expect(modRateFast > 5.0f && modRateFast < 10.0f, "Fast speed should be 5-10 Hz");
    }
    
    void testStereoProcessing() {
        auto engine = EngineFactory::createEngine(ENGINE_ROTARY_SPEAKER);
        engine->prepareToPlay(44100.0, 512);
        
        // Set parameters for maximum stereo effect
        std::map<int, float> params;
        params[0] = 0.5f;  // Medium speed
        params[3] = 1.0f;  // Full stereo width
        engine->updateParameters(params);
        
        // Process mono input
        juce::AudioBuffer<float> buffer(2, 4410);
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float value = std::sin(2.0f * M_PI * 1000.0f * i / 44100.0f);
            buffer.setSample(0, i, value);
            buffer.setSample(1, i, value); // Same in both channels
        }
        
        engine->process(buffer);
        
        // Calculate stereo correlation
        float correlation = calculateStereoCorrelation(buffer);
        
        // Should have decorrelated the channels
        expect(correlation < 0.9f, "Channels should be decorrelated");
        expect(correlation > -0.5f, "Channels shouldn't be completely out of phase");
    }
    
    void testCabinetResonance() {
        auto engine = EngineFactory::createEngine(ENGINE_ROTARY_SPEAKER);
        engine->prepareToPlay(44100.0, 512);
        
        // Create impulse response test
        juce::AudioBuffer<float> buffer(2, 1024);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        engine->process(buffer);
        
        // Check for resonant tail (cabinet response)
        float energy = 0.0f;
        for (int i = 100; i < 500; ++i) { // Check tail energy
            energy += std::abs(buffer.getSample(0, i));
        }
        
        expect(energy > 0.01f, "Should have some resonant tail from cabinet");
    }
    
    // Helper functions
    void fillWithSineWave(juce::AudioBuffer<float>& buffer, float freq, float sampleRate) {
        const float phaseInc = 2.0f * M_PI * freq / sampleRate;
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float phase = 0.0f;
            auto* data = buffer.getWritePointer(ch);
            
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                data[i] = std::sin(phase);
                phase += phaseInc;
            }
        }
    }
    
    void fillWithImpulse(juce::AudioBuffer<float>& buffer, int interval, float sampleRate) {
        buffer.clear();
        
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto* data = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); i += interval) {
                data[i] = 1.0f;
            }
        }
    }
    
    void analyzePitchVariation(const juce::AudioBuffer<float>& buffer, float basePitch, 
                              float sampleRate, float& minPitch, float& maxPitch) {
        // Simple zero-crossing based pitch detection for testing
        const int windowSize = static_cast<int>(sampleRate / basePitch * 4);
        minPitch = basePitch * 2.0f;
        maxPitch = 0.0f;
        
        auto* data = buffer.getReadPointer(0);
        
        for (int start = 0; start < buffer.getNumSamples() - windowSize; start += windowSize / 2) {
            int zeroCrossings = 0;
            float lastSample = data[start];
            
            for (int i = start + 1; i < start + windowSize && i < buffer.getNumSamples(); ++i) {
                if ((lastSample < 0 && data[i] >= 0) || (lastSample >= 0 && data[i] < 0)) {
                    zeroCrossings++;
                }
                lastSample = data[i];
            }
            
            float detectedFreq = (zeroCrossings / 2.0f) * (sampleRate / windowSize);
            minPitch = std::min(minPitch, detectedFreq);
            maxPitch = std::max(maxPitch, detectedFreq);
        }
    }
    
    float calculateModulationRate(const juce::AudioBuffer<float>& buffer, float sampleRate) {
        // Detect modulation rate by analyzing amplitude envelope
        auto* data = buffer.getReadPointer(0);
        
        // Simple envelope follower
        float envelope = 0.0f;
        const float attack = 0.999f;
        const float release = 0.99f;
        
        std::vector<float> envelopeData;
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float input = std::abs(data[i]);
            if (input > envelope) {
                envelope = envelope * attack + input * (1.0f - attack);
            } else {
                envelope = envelope * release;
            }
            
            if (i % 100 == 0) { // Downsample for analysis
                envelopeData.push_back(envelope);
            }
        }
        
        // Count peaks in envelope
        int peaks = 0;
        for (size_t i = 1; i < envelopeData.size() - 1; ++i) {
            if (envelopeData[i] > envelopeData[i-1] && envelopeData[i] > envelopeData[i+1]) {
                peaks++;
            }
        }
        
        float duration = buffer.getNumSamples() / sampleRate;
        return peaks / duration;
    }
    
    float calculateStereoCorrelation(const juce::AudioBuffer<float>& buffer) {
        if (buffer.getNumChannels() < 2) return 1.0f;
        
        auto* left = buffer.getReadPointer(0);
        auto* right = buffer.getReadPointer(1);
        
        float sumL = 0.0f, sumR = 0.0f, sumLR = 0.0f;
        float sumL2 = 0.0f, sumR2 = 0.0f;
        
        const int n = buffer.getNumSamples();
        
        for (int i = 0; i < n; ++i) {
            sumL += left[i];
            sumR += right[i];
            sumLR += left[i] * right[i];
            sumL2 += left[i] * left[i];
            sumR2 += right[i] * right[i];
        }
        
        float meanL = sumL / n;
        float meanR = sumR / n;
        
        float covariance = (sumLR / n) - (meanL * meanR);
        float stdL = std::sqrt((sumL2 / n) - (meanL * meanL));
        float stdR = std::sqrt((sumR2 / n) - (meanR * meanR));
        
        if (stdL * stdR == 0.0f) return 0.0f;
        
        return covariance / (stdL * stdR);
    }
};

// Register the test
static RotarySpeakerTest rotarySpeakerTest;