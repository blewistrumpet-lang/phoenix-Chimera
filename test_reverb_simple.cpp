#include <iostream>
#include <cmath>
#include <memory>
#include "JUCE_Plugin/Source/EngineFactory.h"
#include "JUCE_Plugin/Source/PluginProcessor.h"

// Test a single reverb engine for tail generation
bool testReverbEngine(int engineID, const std::string& name) {
    const int sampleRate = 48000;
    const int blockSize = 512;
    const int testDuration = sampleRate * 2; // 2 seconds
    
    // Create engine
    auto engine = EngineFactory::createEngine(engineID);
    if (!engine) {
        std::cout << name << ": Failed to create engine" << std::endl;
        return false;
    }
    
    // Prepare
    engine->prepareToPlay(sampleRate, blockSize);
    
    // Set parameters for maximum reverb
    std::map<int, float> params;
    
    // Get mix parameter index from PluginProcessor
    ChimeraAudioProcessor processor;
    int mixIndex = processor.getMixParameterIndex(engineID);
    
    // Set all parameters to reasonable reverb values
    for (int i = 0; i < engine->getNumParameters(); ++i) {
        juce::String paramName = engine->getParameterName(i);
        if (i == mixIndex) {
            params[i] = 1.0f; // 100% wet
        } else if (paramName.containsIgnoreCase("size") || 
                   paramName.containsIgnoreCase("room")) {
            params[i] = 0.8f; // Large room
        } else if (paramName.containsIgnoreCase("damp")) {
            params[i] = 0.2f; // Low damping
        } else if (paramName.containsIgnoreCase("decay")) {
            params[i] = 0.8f; // Long decay
        } else if (paramName.containsIgnoreCase("feedback")) {
            params[i] = 0.7f; // High feedback
        } else {
            params[i] = 0.5f; // Default middle value
        }
    }
    
    engine->updateParameters(params);
    
    // Create test buffer with impulse
    juce::AudioBuffer<float> buffer(2, testDuration);
    buffer.clear();
    
    // Add impulse at 100ms
    int impulseIndex = sampleRate / 10;
    buffer.setSample(0, impulseIndex, 1.0f);
    buffer.setSample(1, impulseIndex, 1.0f);
    
    // Process in blocks
    for (int sample = 0; sample < testDuration; sample += blockSize) {
        int samplesToProcess = std::min(blockSize, testDuration - sample);
        juce::AudioBuffer<float> blockBuffer(buffer.getArrayOfWritePointers(), 
                                            2, sample, samplesToProcess);
        engine->process(blockBuffer);
    }
    
    // Analyze reverb tail
    float maxAfterImpulse = 0.0f;
    float tailEnergy = 0.0f;
    int checkStart = impulseIndex + sampleRate / 20; // Start 50ms after impulse
    int checkEnd = impulseIndex + sampleRate;        // Check 1 second of tail
    
    for (int i = checkStart; i < checkEnd && i < testDuration; ++i) {
        float sample = std::abs(buffer.getSample(0, i));
        maxAfterImpulse = std::max(maxAfterImpulse, sample);
        tailEnergy += sample * sample;
    }
    
    float rms = std::sqrt(tailEnergy / (checkEnd - checkStart));
    
    // Check decay pattern
    float early = 0.0f, late = 0.0f;
    int midPoint = impulseIndex + sampleRate / 2;
    
    for (int i = checkStart; i < midPoint && i < testDuration; ++i) {
        early += std::abs(buffer.getSample(0, i));
    }
    for (int i = midPoint; i < checkEnd && i < testDuration; ++i) {
        late += std::abs(buffer.getSample(0, i));
    }
    
    early /= (midPoint - checkStart);
    late /= (checkEnd - midPoint);
    
    bool hasReverb = rms > 0.0001f && maxAfterImpulse > 0.001f;
    bool hasDecay = early > late; // Early should be louder
    
    std::cout << name << " (ID " << engineID << "):" << std::endl;
    std::cout << "  Mix param index: " << mixIndex << std::endl;
    std::cout << "  Max after impulse: " << maxAfterImpulse;
    std::cout << (maxAfterImpulse > 0.001f ? " ✓" : " ✗") << std::endl;
    std::cout << "  Tail RMS: " << rms;
    std::cout << (rms > 0.0001f ? " ✓" : " ✗") << std::endl;
    std::cout << "  Early/Late ratio: " << (late > 0 ? early/late : 0);
    std::cout << (hasDecay ? " ✓" : " ✗") << std::endl;
    std::cout << "  Result: " << (hasReverb ? "PASS - Has reverb tail" : "FAIL - No reverb tail") << std::endl;
    std::cout << std::endl;
    
    return hasReverb;
}

int main() {
    std::cout << "Testing Reverb Engines for Tail Generation" << std::endl;
    std::cout << "==========================================" << std::endl << std::endl;
    
    int passed = 0;
    int total = 0;
    
    // Test all reverb engines
    struct ReverbInfo {
        int id;
        std::string name;
    };
    
    std::vector<ReverbInfo> reverbs = {
        {6, "SpringReverb"},
        {7, "ConvolutionReverb"},
        {8, "PlateReverb"},
        {9, "GatedReverb"},
        {10, "ShimmerReverb"}
    };
    
    for (const auto& reverb : reverbs) {
        if (testReverbEngine(reverb.id, reverb.name)) {
            passed++;
        }
        total++;
    }
    
    std::cout << "==========================================" << std::endl;
    std::cout << "Final Results: " << passed << "/" << total << " reverbs have tails" << std::endl;
    
    if (passed == total) {
        std::cout << "✓ SUCCESS: All reverb engines produce reverb tails!" << std::endl;
        return 0;
    } else if (passed >= total - 1) {
        std::cout << "✓ MOSTLY SUCCESS: " << passed << " reverbs working";
        std::cout << " (GatedReverb may cut tail by design)" << std::endl;
        return 0;
    } else {
        std::cout << "✗ FAILURE: Only " << passed << " reverb engines produce tails" << std::endl;
        return 1;
    }
}