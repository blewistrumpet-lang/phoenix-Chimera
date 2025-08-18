/**
 * Quick Engine Test
 * Tests basic functionality of select engines
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <memory>

// Mock classes for testing without full JUCE
namespace juce {
    class String {
        std::string str;
    public:
        String() = default;
        String(const char* s) : str(s) {}
        bool containsIgnoreCase(const char* s) const { 
            return str.find(s) != std::string::npos; 
        }
        const char* toRawUTF8() const { return str.c_str(); }
    };
    
    template<typename T>
    class AudioBuffer {
        std::vector<std::vector<T>> channels;
        int numSamples;
    public:
        AudioBuffer(int chans, int samples) : numSamples(samples) {
            channels.resize(chans);
            for (auto& ch : channels) {
                ch.resize(samples, 0);
            }
        }
        
        int getNumChannels() const { return channels.size(); }
        int getNumSamples() const { return numSamples; }
        
        void clear() {
            for (auto& ch : channels) {
                std::fill(ch.begin(), ch.end(), 0);
            }
        }
        
        T getSample(int channel, int sample) const {
            if (channel < channels.size() && sample < numSamples) {
                return channels[channel][sample];
            }
            return 0;
        }
        
        void setSample(int channel, int sample, T value) {
            if (channel < channels.size() && sample < numSamples) {
                channels[channel][sample] = value;
            }
        }
        
        T** getArrayOfWritePointers() {
            static std::vector<T*> ptrs;
            ptrs.clear();
            for (auto& ch : channels) {
                ptrs.push_back(ch.data());
            }
            return ptrs.data();
        }
        
        const T** getArrayOfReadPointers() const {
            static std::vector<const T*> ptrs;
            ptrs.clear();
            for (const auto& ch : channels) {
                ptrs.push_back(ch.data());
            }
            return const_cast<const T**>(ptrs.data());
        }
    };
}

// Include after mocks
#include "JUCE_Plugin/Source/EngineBase.h"

class BasicEngineTest {
private:
    const int SAMPLE_RATE = 48000;
    const int BLOCK_SIZE = 512;
    
    float calculateRMS(const juce::AudioBuffer<float>& buffer) {
        float sum = 0;
        int count = 0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                sum += sample * sample;
                count++;
            }
        }
        return count > 0 ? std::sqrt(sum / count) : 0;
    }
    
    bool hasNaNOrInf(const juce::AudioBuffer<float>& buffer) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float sample = buffer.getSample(ch, i);
                if (std::isnan(sample) || std::isinf(sample)) {
                    return true;
                }
            }
        }
        return false;
    }
    
public:
    void testEngine(EngineBase* engine, const std::string& name) {
        std::cout << "\nTesting: " << name << "\n";
        std::cout << "=======================\n";
        
        // Prepare
        engine->prepareToPlay(SAMPLE_RATE, BLOCK_SIZE);
        
        // Test 1: Process silence
        {
            juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
            buffer.clear();
            engine->process(buffer);
            
            bool hasInvalid = hasNaNOrInf(buffer);
            std::cout << "  Silence test: " << (hasInvalid ? "FAIL (NaN/Inf)" : "PASS") << "\n";
        }
        
        // Test 2: Process impulse
        {
            juce::AudioBuffer<float> buffer(2, SAMPLE_RATE);
            buffer.clear();
            buffer.setSample(0, 100, 1.0f);
            buffer.setSample(1, 100, 1.0f);
            
            engine->process(buffer);
            
            float rms = calculateRMS(buffer);
            bool hasInvalid = hasNaNOrInf(buffer);
            
            std::cout << "  Impulse test: ";
            if (hasInvalid) {
                std::cout << "FAIL (NaN/Inf)\n";
            } else {
                std::cout << "PASS (RMS: " << rms << ")\n";
            }
        }
        
        // Test 3: Reset
        {
            engine->reset();
            juce::AudioBuffer<float> buffer(2, BLOCK_SIZE);
            buffer.clear();
            engine->process(buffer);
            
            float residual = calculateRMS(buffer);
            std::cout << "  Reset test: " << (residual < 1e-6f ? "PASS" : "FAIL") 
                     << " (Residual: " << residual << ")\n";
        }
        
        // Test 4: Parameter count
        {
            int numParams = engine->getNumParameters();
            std::cout << "  Parameters: " << numParams << " [";
            for (int i = 0; i < std::min(5, numParams); ++i) {
                std::cout << engine->getParameterName(i).toRawUTF8();
                if (i < std::min(5, numParams) - 1) std::cout << ", ";
            }
            if (numParams > 5) std::cout << "...";
            std::cout << "]\n";
        }
        
        std::cout << "  Name: " << engine->getName().toRawUTF8() << "\n";
    }
};

// Test individual engines
void testSpringReverb() {
    // This would include the actual SpringReverb.h and test it
    std::cout << "\n[Mock] SpringReverb test would run here\n";
}

int main() {
    std::cout << "====================================\n";
    std::cout << "   Quick DSP Engine Test Suite\n";
    std::cout << "====================================\n";
    
    BasicEngineTest tester;
    
    // Note: To actually test engines, we'd need to include and link them
    // This is a framework demonstration
    
    std::cout << "\nTest framework ready.\n";
    std::cout << "To run actual tests, compile with:\n";
    std::cout << "  g++ -std=c++17 quick_engine_test.cpp \\\n";
    std::cout << "      [engine source files] \\\n";
    std::cout << "      -I/Users/Branden/JUCE/modules \\\n";
    std::cout << "      -IJUCE_Plugin/Source \\\n";
    std::cout << "      [frameworks] \\\n";
    std::cout << "      -o quick_test\n";
    
    return 0;
}