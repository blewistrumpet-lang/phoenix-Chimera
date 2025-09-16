#include <iostream>
#include <vector>
#include <cmath>
#include <iomanip>
#include <fstream>
#include "JUCE_Plugin/Source/PlateReverb.h"
#include "JUCE_Plugin/Source/ShimmerReverb.h"
#include "JUCE_Plugin/Source/SpringReverb.h"
#include "JUCE_Plugin/Source/GatedReverb.h"
#include "JUCE_Plugin/Source/ConvolutionReverb.h"

class ReverbDiagnostics {
public:
    struct TestResult {
        std::string name;
        float dryGain;
        float wetGain;
        float feedbackAmount;
        float decayTime;
        bool isStable;
        float maxOutput;
        std::string issues;
    };
    
    static TestResult analyzeReverb(EngineBase* reverb, const std::string& name) {
        TestResult result;
        result.name = name;
        
        // Initialize
        double sampleRate = 44100.0;
        int blockSize = 512;
        reverb->prepareToPlay(sampleRate, blockSize);
        
        // Test with minimal settings first
        std::map<int, float> params;
        params[0] = 0.3f;  // Size/Room Size - low
        params[1] = 0.3f;  // Decay/Damping - low
        params[2] = 1.0f;  // Mix - full wet to analyze reverb
        if (reverb->getNumParameters() > 3) {
            params[3] = 0.3f;
        }
        reverb->updateParameters(params);
        
        // Test impulse response
        juce::AudioBuffer<float> buffer(2, blockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        
        // Process and measure decay
        std::vector<float> envelope;
        float maxSample = 0.0f;
        
        for (int block = 0; block < 200; ++block) { // 200 blocks = ~2.3 seconds
            reverb->process(buffer);
            
            float blockMax = 0.0f;
            float blockEnergy = 0.0f;
            
            for (int ch = 0; ch < 2; ++ch) {
                for (int s = 0; s < blockSize; ++s) {
                    float sample = buffer.getSample(ch, s);
                    
                    if (!std::isfinite(sample)) {
                        result.isStable = false;
                        result.issues = "NaN/Inf detected";
                        return result;
                    }
                    
                    float absSample = std::abs(sample);
                    blockMax = std::max(blockMax, absSample);
                    blockEnergy += sample * sample;
                }
            }
            
            maxSample = std::max(maxSample, blockMax);
            envelope.push_back(blockEnergy / (2.0f * blockSize));
            
            // Clear for next block
            buffer.clear();
        }
        
        result.maxOutput = maxSample;
        
        // Analyze envelope for decay time and stability
        float initialEnergy = envelope[1]; // Skip first block (impulse)
        float decayThreshold = initialEnergy * 0.001f; // -60dB
        
        int decayBlocks = 0;
        for (size_t i = 1; i < envelope.size(); ++i) {
            if (envelope[i] < decayThreshold) {
                decayBlocks = i;
                break;
            }
        }
        
        result.decayTime = (decayBlocks * blockSize) / sampleRate;
        
        // Check for growth (instability)
        bool growing = false;
        for (size_t i = 10; i < envelope.size() - 10; ++i) {
            float avgBefore = 0.0f;
            float avgAfter = 0.0f;
            for (int j = 0; j < 5; ++j) {
                avgBefore += envelope[i - j];
                avgAfter += envelope[i + j];
            }
            if (avgAfter > avgBefore * 1.5f) {
                growing = true;
                break;
            }
        }
        
        result.isStable = !growing && maxSample < 2.0f;
        
        // Test dry/wet mix
        params[2] = 0.0f; // Dry
        reverb->updateParameters(params);
        buffer.clear();
        buffer.setSample(0, 0, 0.5f);
        buffer.setSample(1, 0, 0.5f);
        reverb->process(buffer);
        result.dryGain = buffer.getSample(0, 0) / 0.5f;
        
        params[2] = 1.0f; // Wet
        reverb->updateParameters(params);
        buffer.clear();
        buffer.setSample(0, 0, 0.5f);
        buffer.setSample(1, 0, 0.5f);
        reverb->process(buffer);
        result.wetGain = buffer.getSample(0, 0) / 0.5f;
        
        // Identify issues
        if (result.dryGain < 0.9f) {
            result.issues += "Dry signal attenuated; ";
        }
        if (result.wetGain < 0.1f) {
            result.issues += "Wet signal too quiet; ";
        }
        if (result.wetGain > 5.0f) {
            result.issues += "Wet signal too loud; ";
        }
        if (result.decayTime < 0.1f) {
            result.issues += "Decay too short; ";
        }
        if (!result.isStable) {
            result.issues += "Unstable feedback; ";
        }
        
        return result;
    }
};

int main() {
    std::cout << "\n=== REVERB DIAGNOSTICS ===" << std::endl;
    std::cout << std::setw(20) << "Engine" 
              << std::setw(12) << "Dry Gain"
              << std::setw(12) << "Wet Gain"
              << std::setw(12) << "Max Out"
              << std::setw(12) << "Decay(s)"
              << std::setw(10) << "Stable?"
              << "  Issues" << std::endl;
    std::cout << std::string(100, '-') << std::endl;
    
    PlateReverb plate;
    auto plateResult = ReverbDiagnostics::analyzeReverb(&plate, "PlateReverb");
    std::cout << std::setw(20) << plateResult.name
              << std::setw(12) << std::fixed << std::setprecision(3) << plateResult.dryGain
              << std::setw(12) << plateResult.wetGain
              << std::setw(12) << plateResult.maxOutput
              << std::setw(12) << plateResult.decayTime
              << std::setw(10) << (plateResult.isStable ? "Yes" : "NO")
              << "  " << plateResult.issues << std::endl;
    
    ShimmerReverb shimmer;
    auto shimmerResult = ReverbDiagnostics::analyzeReverb(&shimmer, "ShimmerReverb");
    std::cout << std::setw(20) << shimmerResult.name
              << std::setw(12) << std::fixed << std::setprecision(3) << shimmerResult.dryGain
              << std::setw(12) << shimmerResult.wetGain
              << std::setw(12) << shimmerResult.maxOutput
              << std::setw(12) << shimmerResult.decayTime
              << std::setw(10) << (shimmerResult.isStable ? "Yes" : "NO")
              << "  " << shimmerResult.issues << std::endl;
    
    SpringReverb spring;
    auto springResult = ReverbDiagnostics::analyzeReverb(&spring, "SpringReverb");
    std::cout << std::setw(20) << springResult.name
              << std::setw(12) << std::fixed << std::setprecision(3) << springResult.dryGain
              << std::setw(12) << springResult.wetGain
              << std::setw(12) << springResult.maxOutput
              << std::setw(12) << springResult.decayTime
              << std::setw(10) << (springResult.isStable ? "Yes" : "NO")
              << "  " << springResult.issues << std::endl;
    
    GatedReverb gated;
    auto gatedResult = ReverbDiagnostics::analyzeReverb(&gated, "GatedReverb");
    std::cout << std::setw(20) << gatedResult.name
              << std::setw(12) << std::fixed << std::setprecision(3) << gatedResult.dryGain
              << std::setw(12) << gatedResult.wetGain
              << std::setw(12) << gatedResult.maxOutput
              << std::setw(12) << gatedResult.decayTime
              << std::setw(10) << (gatedResult.isStable ? "Yes" : "NO")
              << "  " << gatedResult.issues << std::endl;
    
    ConvolutionReverb conv;
    auto convResult = ReverbDiagnostics::analyzeReverb(&conv, "ConvolutionReverb");
    std::cout << std::setw(20) << convResult.name
              << std::setw(12) << std::fixed << std::setprecision(3) << convResult.dryGain
              << std::setw(12) << convResult.wetGain
              << std::setw(12) << convResult.maxOutput
              << std::setw(12) << convResult.decayTime
              << std::setw(10) << (convResult.isStable ? "Yes" : "NO")
              << "  " << convResult.issues << std::endl;
    
    std::cout << "\n=== ANALYSIS SUMMARY ===" << std::endl;
    std::cout << "PlateReverb: " << (plateResult.isStable ? "✓" : "✗") << " " << plateResult.issues << std::endl;
    std::cout << "ShimmerReverb: " << (shimmerResult.isStable ? "✓" : "✗") << " " << shimmerResult.issues << std::endl;
    std::cout << "SpringReverb: " << (springResult.isStable ? "✓" : "✗") << " " << springResult.issues << std::endl;
    std::cout << "GatedReverb: " << (gatedResult.isStable ? "✓" : "✗") << " " << gatedResult.issues << std::endl;
    std::cout << "ConvolutionReverb: " << (convResult.isStable ? "✓" : "✗") << " " << convResult.issues << std::endl;
    
    return 0;
}
