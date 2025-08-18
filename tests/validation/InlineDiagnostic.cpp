/*
 * INLINE DIAGNOSTIC CODE
 * 
 * Copy and paste this directly into PluginProcessor.cpp to debug engines.
 * No additional files needed - completely self-contained.
 */

// Add this method to your ChimeraAudioProcessor class:

void ChimeraAudioProcessor::debugEngines() {
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "ENGINE DEBUG - Testing Core Engines\n";
    std::cout << std::string(50, '=') << "\n";
    
    const double sampleRate = getSampleRate();
    const int blockSize = 512;
    
    // Helper functions
    auto calculateRMS = [](const juce::AudioBuffer<float>& buffer) -> float {
        float sum = 0.0f;
        int count = 0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            const float* data = buffer.getReadPointer(ch);
            for (int s = 0; s < buffer.getNumSamples(); ++s) {
                sum += data[s] * data[s];
                count++;
            }
        }
        return count > 0 ? std::sqrt(sum / count) : 0.0f;
    };
    
    auto buffersMatch = [](const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b) -> bool {
        if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples()) 
            return false;
        for (int ch = 0; ch < a.getNumChannels(); ++ch) {
            const float* dataA = a.getReadPointer(ch);
            const float* dataB = b.getReadPointer(ch);
            for (int s = 0; s < a.getNumSamples(); ++s) {
                if (std::abs(dataA[s] - dataB[s]) > 0.0001f) return false;
            }
        }
        return true;
    };
    
    // Create test signal - 1kHz tone at -6dB
    juce::AudioBuffer<float> testSignal(2, blockSize);
    const double phase_inc = 2.0 * juce::MathConstants<double>::pi * 1000.0 / sampleRate;
    for (int ch = 0; ch < 2; ++ch) {
        float* data = testSignal.getWritePointer(ch);
        double phase = 0.0;
        for (int s = 0; s < blockSize; ++s) {
            data[s] = 0.5f * std::sin(phase);  // -6dB amplitude
            phase += phase_inc;
        }
    }
    
    float inputRMS = calculateRMS(testSignal);
    std::cout << "Test Signal: 1kHz tone, RMS = " << std::fixed << std::setprecision(3) << inputRMS << "\n\n";
    
    // TEST 1: PlateReverb
    std::cout << "1. PLATE REVERB TEST\n";
    try {
        auto reverb = std::make_unique<PlateReverb>();
        reverb->prepareToPlay(sampleRate, blockSize);
        
        juce::AudioBuffer<float> inputCopy, outputBuffer;
        inputCopy.makeCopyOf(testSignal);
        outputBuffer.makeCopyOf(testSignal);
        
        // Set reverb parameters
        std::map<int, float> params = {{0, 0.7f}, {1, 0.4f}, {2, 0.1f}, {3, 0.5f}}; // Size, Damping, Predelay, Mix
        reverb->updateParameters(params);
        reverb->process(outputBuffer);
        
        float outputRMS = calculateRMS(outputBuffer);
        bool audioChanged = !buffersMatch(inputCopy, outputBuffer);
        float gainChange = (inputRMS > 0.001f) ? 20.0f * std::log10(outputRMS / inputRMS) : 0.0f;
        
        std::cout << "   Status: ✓ SUCCESS\n";
        std::cout << "   Audio Modified: " << (audioChanged ? "YES ✓" : "NO ✗") << "\n";
        std::cout << "   Input RMS: " << inputRMS << "\n";
        std::cout << "   Output RMS: " << outputRMS << "\n";
        std::cout << "   Gain Change: " << gainChange << " dB\n";
        std::cout << "   Mix Setting: " << params[3] << " (50% wet)\n";
        std::cout << "   Expected: Should add reverb tail\n\n";
        
    } catch (...) {
        std::cout << "   Status: ✗ FAILED - Exception thrown\n\n";
    }
    
    // TEST 2: ClassicCompressor
    std::cout << "2. CLASSIC COMPRESSOR TEST\n";
    try {
        auto compressor = std::make_unique<ClassicCompressor>();
        compressor->prepareToPlay(sampleRate, blockSize);
        
        juce::AudioBuffer<float> inputCopy, outputBuffer;
        inputCopy.makeCopyOf(testSignal);
        outputBuffer.makeCopyOf(testSignal);
        
        // Set compressor parameters - should compress the -6dB signal
        std::map<int, float> params = {
            {0, 0.3f},  // Threshold (-18dB)
            {1, 0.6f},  // Ratio (high)
            {2, 0.1f},  // Fast attack
            {3, 0.3f},  // Medium release
            {4, 0.2f},  // Soft knee
            {5, 0.5f},  // Makeup gain
            {6, 1.0f},  // 100% wet
            {7, 0.0f},  // No lookahead
            {8, 0.5f},  // Auto release
            {9, 0.0f}   // No sidechain
        };
        compressor->updateParameters(params);
        compressor->process(outputBuffer);
        
        float outputRMS = calculateRMS(outputBuffer);
        bool audioChanged = !buffersMatch(inputCopy, outputBuffer);
        float gainChange = (inputRMS > 0.001f) ? 20.0f * std::log10(outputRMS / inputRMS) : 0.0f;
        
        std::cout << "   Status: ✓ SUCCESS\n";
        std::cout << "   Audio Modified: " << (audioChanged ? "YES ✓" : "NO ✗") << "\n";
        std::cout << "   Input RMS: " << inputRMS << "\n";
        std::cout << "   Output RMS: " << outputRMS << "\n";
        std::cout << "   Gain Change: " << gainChange << " dB\n";
        std::cout << "   Mix Setting: " << params[6] << " (100% wet)\n";
        std::cout << "   Expected: Should compress and reduce level\n\n";
        
    } catch (...) {
        std::cout << "   Status: ✗ FAILED - Exception thrown\n\n";
    }
    
    // TEST 3: RodentDistortion
    std::cout << "3. RODENT DISTORTION TEST\n";
    try {
        auto distortion = std::make_unique<RodentDistortion>();
        distortion->prepareToPlay(sampleRate, blockSize);
        
        juce::AudioBuffer<float> inputCopy, outputBuffer;
        inputCopy.makeCopyOf(testSignal);
        outputBuffer.makeCopyOf(testSignal);
        
        // Set distortion parameters - should add significant distortion
        std::map<int, float> params = {
            {0, 0.7f},  // High gain
            {1, 0.5f},  // Moderate filter
            {2, 0.6f},  // Significant clipping
            {3, 0.5f},  // Neutral tone
            {4, 0.8f},  // Boosted output
            {5, 1.0f},  // 100% wet
            {6, 0.0f},  // RAT mode
            {7, 0.4f}   // Moderate presence
        };
        distortion->updateParameters(params);
        distortion->process(outputBuffer);
        
        float outputRMS = calculateRMS(outputBuffer);
        bool audioChanged = !buffersMatch(inputCopy, outputBuffer);
        float gainChange = (inputRMS > 0.001f) ? 20.0f * std::log10(outputRMS / inputRMS) : 0.0f;
        
        std::cout << "   Status: ✓ SUCCESS\n";
        std::cout << "   Audio Modified: " << (audioChanged ? "YES ✓" : "NO ✗") << "\n";
        std::cout << "   Input RMS: " << inputRMS << "\n";
        std::cout << "   Output RMS: " << outputRMS << "\n";
        std::cout << "   Gain Change: " << gainChange << " dB\n";
        std::cout << "   Mix Setting: " << params[5] << " (100% wet)\n";
        std::cout << "   Expected: Should add distortion and harmonics\n\n";
        
    } catch (...) {
        std::cout << "   Status: ✗ FAILED - Exception thrown\n\n";
    }
    
    std::cout << std::string(50, '=') << "\n";
    std::cout << "ENGINE DEBUG COMPLETE\n";
    std::cout << "Check console output above for results.\n";
    std::cout << "All engines should show 'Audio Modified: YES' if working.\n";
    std::cout << std::string(50, '=') << "\n\n";
}

/*
 * TO USE THIS DIAGNOSTIC:
 * 
 * 1. Add the method declaration to PluginProcessor.h:
 *    void debugEngines();
 * 
 * 2. Add these includes to PluginProcessor.cpp:
 *    #include "PlateReverb.h"
 *    #include "ClassicCompressor.h" 
 *    #include "RodentDistortion.h"
 * 
 * 3. Call debugEngines() from wherever you want to test, for example:
 *    
 *    In prepareToPlay():
 *    debugEngines();  // Run once at startup
 *    
 *    In processBlock() with a guard:
 *    static bool debugRun = false;
 *    if (!debugRun) {
 *        debugEngines();
 *        debugRun = true;
 *    }
 * 
 * The diagnostic will print detailed results to the console showing whether
 * each engine is working correctly and processing audio as expected.
 */