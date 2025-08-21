// Reverb Engine Validation Test Suite
// Tests all 5 reverb engines for proper operation

#include <iostream>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iomanip>
#include "../JUCE_Plugin/JuceLibraryCode/JuceHeader.h"
#include "../JUCE_Plugin/Source/EngineFactory.h"
#include "../JUCE_Plugin/Source/EngineTypes.h"

class ReverbValidator {
public:
    struct TestResult {
        int engineId;
        std::string engineName;
        bool hasReverb;
        float tailLength;
        float decayRate;
        bool parametersWork;
        bool qualityGood;
        std::vector<std::string> issues;
    };
    
    TestResult testReverb(int engineId, const std::string& name) {
        TestResult result;
        result.engineId = engineId;
        result.engineName = name;
        
        std::cout << "\nTesting " << name << " (ID: " << engineId << ")" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        // Create engine
        auto engine = EngineFactory::createEngine(engineId);
        if (!engine) {
            result.issues.push_back("Failed to create engine");
            return result;
        }
        
        // Initialize
        const double sampleRate = 44100.0;
        const int blockSize = 512;
        engine->prepareToPlay(sampleRate, blockSize);
        
        // Test 1: Impulse Response
        std::cout << "Test 1: Impulse Response..." << std::endl;
        juce::AudioBuffer<float> impulseBuffer(2, blockSize * 100); // ~1 second
        impulseBuffer.clear();
        
        // Create impulse (single sample spike)
        impulseBuffer.setSample(0, 0, 1.0f);
        impulseBuffer.setSample(1, 0, 1.0f);
        
        // Set reverb to 100% wet for clear measurement
        std::map<int, float> params;
        // Assuming mix is usually one of the last parameters
        for (int i = 0; i < 10; i++) {
            params[i] = 0.5f; // Set all to middle
        }
        params[params.size() - 1] = 1.0f; // Set mix to 100% wet
        engine->updateParameters(params);
        
        // Process impulse through reverb
        for (int block = 0; block < 100; block++) {
            juce::AudioBuffer<float> processBuffer(2, blockSize);
            
            // Copy portion of impulse buffer
            for (int ch = 0; ch < 2; ch++) {
                for (int s = 0; s < blockSize; s++) {
                    int sourceIndex = block * blockSize + s;
                    if (sourceIndex < impulseBuffer.getNumSamples()) {
                        processBuffer.setSample(ch, s, impulseBuffer.getSample(ch, sourceIndex));
                    }
                }
            }
            
            // Process
            engine->process(processBuffer);
            
            // Copy back
            for (int ch = 0; ch < 2; ch++) {
                for (int s = 0; s < blockSize; s++) {
                    int destIndex = block * blockSize + s;
                    if (destIndex < impulseBuffer.getNumSamples()) {
                        impulseBuffer.setSample(ch, destIndex, processBuffer.getSample(ch, s));
                    }
                }
            }
        }
        
        // Analyze reverb tail
        float maxLevel = 0.0f;
        int tailStartSample = 0;
        int tailEndSample = 0;
        
        // Find max level after impulse
        for (int i = 1; i < impulseBuffer.getNumSamples(); i++) {
            float level = std::abs(impulseBuffer.getSample(0, i));
            if (level > maxLevel) {
                maxLevel = level;
                tailStartSample = i;
            }
        }
        
        // Find where tail drops to -60dB
        float threshold = maxLevel * 0.001f; // -60dB
        for (int i = impulseBuffer.getNumSamples() - 1; i > tailStartSample; i--) {
            float level = std::abs(impulseBuffer.getSample(0, i));
            if (level > threshold) {
                tailEndSample = i;
                break;
            }
        }
        
        result.tailLength = (tailEndSample - tailStartSample) / (float)sampleRate;
        result.hasReverb = (maxLevel > 0.01f) && (result.tailLength > 0.05f);
        
        std::cout << "  Max level: " << maxLevel << std::endl;
        std::cout << "  Tail length: " << result.tailLength << " seconds" << std::endl;
        std::cout << "  Has reverb: " << (result.hasReverb ? "YES" : "NO") << std::endl;
        
        // Test 2: Parameter Response
        std::cout << "\nTest 2: Parameter Response..." << std::endl;
        
        // Test different decay settings
        std::vector<float> decayParams = {0.0f, 0.5f, 1.0f};
        std::vector<float> tailLengths;
        
        for (float decay : decayParams) {
            // Reset buffer
            impulseBuffer.clear();
            impulseBuffer.setSample(0, 0, 1.0f);
            impulseBuffer.setSample(1, 0, 1.0f);
            
            // Set decay parameter (usually param 1 or 2)
            params[1] = decay; // Assuming decay is parameter 1
            engine->updateParameters(params);
            
            // Process
            for (int block = 0; block < 100; block++) {
                juce::AudioBuffer<float> processBuffer(2, blockSize);
                
                for (int ch = 0; ch < 2; ch++) {
                    for (int s = 0; s < blockSize; s++) {
                        int sourceIndex = block * blockSize + s;
                        if (sourceIndex < impulseBuffer.getNumSamples()) {
                            processBuffer.setSample(ch, s, impulseBuffer.getSample(ch, sourceIndex));
                        }
                    }
                }
                
                engine->process(processBuffer);
                
                for (int ch = 0; ch < 2; ch++) {
                    for (int s = 0; s < blockSize; s++) {
                        int destIndex = block * blockSize + s;
                        if (destIndex < impulseBuffer.getNumSamples()) {
                            impulseBuffer.setSample(ch, destIndex, processBuffer.getSample(ch, s));
                        }
                    }
                }
            }
            
            // Measure tail
            float energy = 0.0f;
            for (int i = 1000; i < 10000; i++) { // Sample a section
                energy += std::abs(impulseBuffer.getSample(0, i));
            }
            tailLengths.push_back(energy);
            
            std::cout << "  Decay " << decay << " -> Energy: " << energy << std::endl;
        }
        
        // Check if decay parameter affects tail
        result.parametersWork = (tailLengths[2] > tailLengths[0] * 1.5f) || 
                               (tailLengths[0] > tailLengths[2] * 1.5f);
        
        std::cout << "  Parameters affect output: " << (result.parametersWork ? "YES" : "NO") << std::endl;
        
        // Test 3: Quality Assessment
        std::cout << "\nTest 3: Quality Assessment..." << std::endl;
        
        // Check for metallic ringing (look for sharp peaks in spectrum)
        // This is simplified - real implementation would use FFT
        float variance = 0.0f;
        float mean = 0.0f;
        int sampleCount = std::min(10000, impulseBuffer.getNumSamples());
        
        for (int i = 1000; i < sampleCount; i++) {
            mean += std::abs(impulseBuffer.getSample(0, i));
        }
        mean /= (sampleCount - 1000);
        
        for (int i = 1000; i < sampleCount; i++) {
            float diff = std::abs(impulseBuffer.getSample(0, i)) - mean;
            variance += diff * diff;
        }
        variance /= (sampleCount - 1000);
        
        // High variance might indicate ringing
        result.qualityGood = variance < (mean * mean * 10.0f);
        
        std::cout << "  Mean level: " << mean << std::endl;
        std::cout << "  Variance: " << variance << std::endl;
        std::cout << "  Quality assessment: " << (result.qualityGood ? "GOOD" : "ISSUES DETECTED") << std::endl;
        
        // Compile issues
        if (!result.hasReverb) {
            result.issues.push_back("No reverb tail detected");
        }
        if (!result.parametersWork) {
            result.issues.push_back("Parameters don't affect output");
        }
        if (!result.qualityGood) {
            result.issues.push_back("Possible metallic artifacts");
        }
        
        return result;
    }
    
    void runAllTests() {
        std::cout << "====================================" << std::endl;
        std::cout << "REVERB ENGINE VALIDATION TEST SUITE" << std::endl;
        std::cout << "====================================" << std::endl;
        
        // Test all 5 reverb engines
        std::vector<std::pair<int, std::string>> reverbs = {
            {39, "PlateReverb"},
            {40, "SpringReverb_Platinum"},
            {41, "ConvolutionReverb"},
            {42, "ShimmerReverb"},
            {43, "GatedReverb"}
        };
        
        std::vector<TestResult> results;
        
        for (const auto& [id, name] : reverbs) {
            results.push_back(testReverb(id, name));
        }
        
        // Summary
        std::cout << "\n====================================" << std::endl;
        std::cout << "TEST SUMMARY" << std::endl;
        std::cout << "====================================" << std::endl;
        
        int passCount = 0;
        int failCount = 0;
        
        for (const auto& result : results) {
            bool passed = result.hasReverb && result.parametersWork && result.qualityGood;
            
            std::cout << "\n" << result.engineName << " (ID: " << result.engineId << "): ";
            std::cout << (passed ? "✅ PASSED" : "❌ FAILED") << std::endl;
            
            if (result.hasReverb) {
                std::cout << "  Tail length: " << std::fixed << std::setprecision(2) 
                         << result.tailLength << "s" << std::endl;
            }
            
            if (!result.issues.empty()) {
                std::cout << "  Issues:" << std::endl;
                for (const auto& issue : result.issues) {
                    std::cout << "    - " << issue << std::endl;
                }
            }
            
            if (passed) passCount++;
            else failCount++;
        }
        
        std::cout << "\n====================================" << std::endl;
        std::cout << "FINAL RESULTS" << std::endl;
        std::cout << "====================================" << std::endl;
        std::cout << "Passed: " << passCount << "/" << reverbs.size() << std::endl;
        std::cout << "Failed: " << failCount << "/" << reverbs.size() << std::endl;
        
        if (passCount == reverbs.size()) {
            std::cout << "\n🎉 ALL REVERB ENGINES PASSED!" << std::endl;
        } else {
            std::cout << "\n⚠️  Some reverb engines need attention" << std::endl;
        }
    }
};

int main() {
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI scopedJuce;
    
    ReverbValidator validator;
    validator.runAllTests();
    
    return 0;
}