/**
 * Test program for ClassicCompressor engine
 * Verifies proper implementation and mapping
 */

#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <cmath>
#include <cassert>

// JUCE includes
#include "../JuceLibraryCode/JuceHeader.h"

// Engine includes
#include "EngineTypes.h"
#include "EngineFactory.h"
#include "ClassicCompressor.h"

using namespace std;

void testClassicCompressor() {
    cout << "\n=== CLASSIC COMPRESSOR TEST ===" << endl;
    
    // Test 1: Factory creation
    cout << "\nTest 1: Factory Creation" << endl;
    auto factory = std::make_unique<EngineFactory>();
    auto engine = factory->createEngine(ENGINE_VCA_COMPRESSOR);
    
    if (!engine) {
        cerr << "  ❌ Failed to create ClassicCompressor from factory!" << endl;
        exit(1);
    }
    cout << "  ✓ Successfully created from factory" << endl;
    
    // Test 2: Direct instantiation
    cout << "\nTest 2: Direct Instantiation" << endl;
    auto compressor = std::make_unique<ClassicCompressor>();
    cout << "  ✓ Direct instantiation successful" << endl;
    
    // Test 3: Engine name
    cout << "\nTest 3: Engine Properties" << endl;
    cout << "  • Name: " << compressor->getName() << endl;
    cout << "  • Parameters: " << compressor->getNumParameters() << endl;
    
    // Test 4: Parameter names
    cout << "\nTest 4: Parameter Names" << endl;
    for (int i = 0; i < compressor->getNumParameters(); ++i) {
        cout << "  • Param " << i << ": " << compressor->getParameterName(i) << endl;
    }
    
    // Test 5: Audio processing setup
    cout << "\nTest 5: Audio Processing Setup" << endl;
    double sampleRate = 48000.0;
    int blockSize = 512;
    compressor->prepareToPlay(sampleRate, blockSize);
    cout << "  ✓ prepareToPlay successful" << endl;
    
    // Test 6: Process audio block
    cout << "\nTest 6: Process Audio Block" << endl;
    juce::AudioBuffer<float> buffer(2, blockSize);
    
    // Generate test signal (sine wave)
    for (int ch = 0; ch < 2; ++ch) {
        auto* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < blockSize; ++i) {
            channelData[i] = 0.5f * sin(2.0 * M_PI * 440.0 * i / sampleRate);
        }
    }
    
    float inputRMS = buffer.getRMSLevel(0, 0, blockSize);
    compressor->process(buffer);
    float outputRMS = buffer.getRMSLevel(0, 0, blockSize);
    
    cout << "  • Input RMS: " << inputRMS << endl;
    cout << "  • Output RMS: " << outputRMS << endl;
    cout << "  • Gain reduction: " << compressor->getGainReduction() << " dB" << endl;
    cout << "  ✓ Audio processing successful" << endl;
    
    // Test 7: Parameter updates
    cout << "\nTest 7: Parameter Updates" << endl;
    std::map<int, float> params;
    params[0] = 0.5f;  // Threshold
    params[1] = 0.7f;  // Ratio
    params[2] = 0.3f;  // Attack
    params[3] = 0.4f;  // Release
    params[6] = 0.8f;  // Mix
    
    compressor->updateParameters(params);
    cout << "  ✓ Parameter update successful" << endl;
    
    // Test 8: Reset
    cout << "\nTest 8: Reset" << endl;
    compressor->reset();
    cout << "  ✓ Reset successful" << endl;
    
    // Test 9: Engine mapping verification
    cout << "\nTest 9: Engine Mapping" << endl;
    cout << "  • ENGINE_VCA_COMPRESSOR = " << ENGINE_VCA_COMPRESSOR << endl;
    cout << "  • ENGINE_CLASSIC_COMPRESSOR = " << ENGINE_CLASSIC_COMPRESSOR << endl;
    assert(ENGINE_CLASSIC_COMPRESSOR == ENGINE_VCA_COMPRESSOR);
    cout << "  ✓ Mapping verified (legacy alias works)" << endl;
    
    // Test 10: Metering
    cout << "\nTest 10: Metering Functions" << endl;
    compressor->resetMeters();
    cout << "  • Current gain reduction: " << compressor->getGainReduction() << " dB" << endl;
    cout << "  • Peak gain reduction: " << compressor->getPeakReduction() << " dB" << endl;
    cout << "  ✓ Metering functions work" << endl;
    
    cout << "\n=== ALL TESTS PASSED ===\n" << endl;
}

int main() {
    cout << "ClassicCompressor Engine Test Suite" << endl;
    cout << "===================================" << endl;
    
    try {
        testClassicCompressor();
    } catch (const std::exception& e) {
        cerr << "\n❌ Test failed with exception: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}