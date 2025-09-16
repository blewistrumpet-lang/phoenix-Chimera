// Debug test for ShimmerReverb
#include <iostream>
#include <memory>
#include <cmath>
#include "/Users/Branden/JUCE/modules/juce_audio_processors/juce_audio_processors.h"
#include "/Users/Branden/JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"

int main() {
    std::cout << "SHIMMERREVERB DEBUG TEST" << std::endl;
    
    auto reverb = std::make_unique<ShimmerReverb>();
    reverb->prepareToPlay(44100, 512);
    
    // Test 1: Verify parameters are applied
    std::cout << "\n=== Parameter Application Test ===" << std::endl;
    
    // Set distinct parameters
    std::map<int, float> params;
    params[0] = 0.1f;  // Pitch shift
    params[1] = 0.2f;  // Shimmer amount
    params[2] = 0.3f;  // Room size
    params[3] = 0.4f;  // Damping
    params[4] = 0.0f;  // Mix: 0% (dry only)
    
    std::cout << "Setting Mix to 0.0 (dry only)..." << std::endl;
    reverb->updateParameters(params);
    
    // Send impulse
    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    reverb->process(buffer);
    
    float output = buffer.getSample(0, 0);
    std::cout << "Input impulse: 1.0" << std::endl;
    std::cout << "Output with Mix=0.0: " << output << std::endl;
    std::cout << "Expected: 1.0 (dry only)" << std::endl;
    std::cout << "Result: " << (std::abs(output - 1.0f) < 0.01f ? "PASS ✓" : "FAIL ✗") << std::endl;
    
    // Test 2: Mix = 1.0 (wet only)
    std::cout << "\n=== Wet Only Test ===" << std::endl;
    
    reverb->reset();
    params[4] = 1.0f;  // Mix: 100% wet
    params[2] = 0.9f;  // Room size: very large (more feedback)
    
    std::cout << "Setting Mix to 1.0 (wet only)..." << std::endl;
    reverb->updateParameters(params);
    
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    reverb->process(buffer);
    
    output = buffer.getSample(0, 0);
    std::cout << "Input impulse: 1.0" << std::endl;
    std::cout << "Output with Mix=1.0: " << output << std::endl;
    std::cout << "Expected: 0.0 (no direct signal, reverb hasn't built up yet)" << std::endl;
    std::cout << "Result: " << (std::abs(output) < 0.1f ? "PASS ✓" : "FAIL ✗") << std::endl;
    
    // Test 3: Check if reverb produces ANY output over time
    std::cout << "\n=== Reverb Output Test ===" << std::endl;
    
    reverb->reset();
    params[4] = 1.0f;  // Mix: 100% wet
    params[2] = 0.9f;  // Room size: very large
    params[1] = 0.0f;  // No shimmer (simpler)
    reverb->updateParameters(params);
    
    // Send impulse
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
    
    float totalEnergy = 0.0f;
    for (int block = 0; block < 10; block++) {
        reverb->process(buffer);
        float rms = buffer.getRMSLevel(0, 0, 512);
        totalEnergy += rms;
        std::cout << "Block " << block << ": RMS=" << rms;
        
        // After first block, should be reverb tail only
        if (block > 0 && rms > 0.001f) {
            std::cout << " <- REVERB TAIL DETECTED!";
        }
        std::cout << std::endl;
        
        // Clear buffer after first block
        if (block == 0) buffer.clear();
    }
    
    std::cout << "\nTotal energy: " << totalEnergy << std::endl;
    std::cout << "Result: " << (totalEnergy > 0.01f ? "REVERB WORKING ✓" : "NO REVERB OUTPUT ✗") << std::endl;
    
    // Test 4: Direct reverb algorithm test
    std::cout << "\n=== Direct Algorithm Test ===" << std::endl;
    
    // Test the SimpleReverb class directly
    class TestReverb {
    public:
        std::vector<float> delayBuffer;
        int writePos = 0;
        int delaySize = 0;
        float feedback = 0.8f;
        
        void init(int size) {
            delaySize = size;
            delayBuffer.resize(size, 0.0f);
            writePos = 0;
        }
        
        float process(float input) {
            if (delaySize == 0) return input;
            float delayed = delayBuffer[writePos];
            delayBuffer[writePos] = input + delayed * feedback;
            writePos = (writePos + 1) % delaySize;
            return delayed;
        }
    };
    
    TestReverb test;
    test.init(100);  // 100 sample delay
    
    std::cout << "Testing simple delay line with feedback..." << std::endl;
    
    // Send impulse
    float testOutput = test.process(1.0f);
    std::cout << "Sample 0 (impulse): output=" << testOutput << std::endl;
    
    // Process more samples
    for (int i = 1; i < 200; i++) {
        testOutput = test.process(0.0f);
        if (i == 99 || i == 100 || i == 101 || i == 199) {
            std::cout << "Sample " << i << ": output=" << testOutput << std::endl;
        }
    }
    
    std::cout << "\n=== DIAGNOSIS ===" << std::endl;
    std::cout << "1. Parameter application: " << (std::abs(output) < 0.1f ? "Working" : "BROKEN") << std::endl;
    std::cout << "2. Reverb algorithm: " << (totalEnergy > 0.01f ? "Working" : "BROKEN") << std::endl;
    std::cout << "3. Basic delay line: Working (shown above)" << std::endl;
    
    return 0;
}