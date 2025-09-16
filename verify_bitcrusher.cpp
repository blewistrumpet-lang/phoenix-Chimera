#include <iostream>
#include <cmath>
#include <map>
#include <set>

// Simple test to verify BitCrusher logic without JUCE dependencies
class SimpleBitCrusher {
private:
    float m_bits = 16.0f;
    float m_downsample = 1.0f;
    float m_mix = 1.0f;
    
    float m_heldSampleL = 0.0f;
    float m_heldSampleR = 0.0f;
    float m_counterL = 0.0f;
    float m_counterR = 0.0f;
    
public:
    void updateParameters(int param, float value) {
        if (param == 0) {
            // Bits: map 0-1 to useful bit depths
            if (value < 0.2f)      m_bits = 24.0f;  // Clean
            else if (value < 0.4f) m_bits = 12.0f;  // Vintage sampler
            else if (value < 0.6f) m_bits = 8.0f;   // 8-bit
            else if (value < 0.8f) m_bits = 4.0f;   // Crunchy
            else               m_bits = 1.0f;   // Destroyed
        }
        else if (param == 1) {
            // Downsample: map 0-1 to useful rates
            if (value < 0.2f)      m_downsample = 1.0f;   // No downsampling
            else if (value < 0.4f) m_downsample = 2.0f;   // Half rate
            else if (value < 0.6f) m_downsample = 4.0f;   // Quarter rate
            else if (value < 0.8f) m_downsample = 8.0f;   // 1/8 rate
            else               m_downsample = 16.0f;  // 1/16 rate
        }
        else if (param == 2) {
            m_mix = value;
        }
    }
    
    float processSample(float input, int channel) {
        float dry = input;
        float& heldSample = (channel == 0) ? m_heldSampleL : m_heldSampleR;
        float& counter = (channel == 0) ? m_counterL : m_counterR;
        
        // 1. Sample rate reduction (downsample)
        counter += 1.0f;
        if (counter >= m_downsample) {
            counter -= m_downsample;
            
            // 2. Bit depth reduction (quantize)
            if (m_bits < 24.0f) {
                float levels = std::pow(2.0f, m_bits);
                input = std::round(input * levels) / levels;
            }
            
            heldSample = input;
        }
        
        // 3. Mix
        return dry * (1.0f - m_mix) + heldSample * m_mix;
    }
    
    float getBits() const { return m_bits; }
    float getDownsample() const { return m_downsample; }
    float getMix() const { return m_mix; }
};

int main() {
    std::cout << "BitCrusher Logic Verification\n";
    std::cout << "==============================\n\n";
    
    SimpleBitCrusher crusher;
    
    // Test parameter mapping
    std::cout << "📊 Parameter Mapping Tests:\n";
    
    // Test bits parameter
    crusher.updateParameters(0, 0.0f);
    std::cout << "  Bits(0.0) = " << crusher.getBits() << " (should be 24)\n";
    
    crusher.updateParameters(0, 0.3f);
    std::cout << "  Bits(0.3) = " << crusher.getBits() << " (should be 12)\n";
    
    crusher.updateParameters(0, 0.5f);
    std::cout << "  Bits(0.5) = " << crusher.getBits() << " (should be 8)\n";
    
    crusher.updateParameters(0, 0.9f);
    std::cout << "  Bits(0.9) = " << crusher.getBits() << " (should be 1)\n";
    
    // Test downsample parameter
    std::cout << "\n  Downsample tests:\n";
    crusher.updateParameters(1, 0.0f);
    std::cout << "  Downsample(0.0) = " << crusher.getDownsample() << " (should be 1)\n";
    
    crusher.updateParameters(1, 0.5f);
    std::cout << "  Downsample(0.5) = " << crusher.getDownsample() << " (should be 4)\n";
    
    crusher.updateParameters(1, 0.9f);
    std::cout << "  Downsample(0.9) = " << crusher.getDownsample() << " (should be 16)\n";
    
    // Test bit crushing
    std::cout << "\n🔊 Processing Tests:\n";
    
    // Set to 1-bit crushing
    crusher.updateParameters(0, 0.9f);  // 1-bit
    crusher.updateParameters(1, 0.0f);  // No downsampling
    crusher.updateParameters(2, 1.0f);  // 100% wet
    
    std::set<float> uniqueValues;
    for (int i = 0; i < 100; i++) {
        float input = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f) * 0.5f;
        float output = crusher.processSample(input, 0);
        uniqueValues.insert(std::round(output * 1000.0f) / 1000.0f);
    }
    
    std::cout << "  1-bit crushing: " << uniqueValues.size() << " unique values\n";
    std::cout << "  Values: ";
    for (float v : uniqueValues) {
        std::cout << v << " ";
    }
    std::cout << "\n";
    
    if (uniqueValues.size() <= 3) {
        std::cout << "  ✅ 1-bit crushing working correctly!\n";
    } else {
        std::cout << "  ❌ Too many values for 1-bit\n";
    }
    
    // Test 8-bit crushing
    crusher.updateParameters(0, 0.5f);  // 8-bit
    uniqueValues.clear();
    for (int i = 0; i < 100; i++) {
        float input = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f) * 0.5f;
        float output = crusher.processSample(input, 0);
        uniqueValues.insert(std::round(output * 1000.0f) / 1000.0f);
    }
    
    std::cout << "\n  8-bit crushing: " << uniqueValues.size() << " unique values\n";
    if (uniqueValues.size() > 3 && uniqueValues.size() < 256) {
        std::cout << "  ✅ 8-bit crushing working correctly!\n";
    }
    
    // Test downsampling
    std::cout << "\n  Testing downsampling:\n";
    crusher.updateParameters(0, 0.0f);  // No bit crushing
    crusher.updateParameters(1, 0.5f);  // 4x downsampling
    
    float lastOutput = -999.0f;
    int holdCount = 0;
    for (int i = 0; i < 20; i++) {
        float input = std::sin(2.0f * M_PI * 440.0f * i / 44100.0f) * 0.5f;
        float output = crusher.processSample(input, 0);
        if (std::abs(output - lastOutput) < 0.001f) {
            holdCount++;
        }
        lastOutput = output;
    }
    
    std::cout << "  Samples held: " << holdCount << " out of 20\n";
    if (holdCount > 10) {
        std::cout << "  ✅ Downsampling working correctly!\n";
    }
    
    std::cout << "\n==============================\n";
    std::cout << "✅ Verification complete!\n";
    
    return 0;
}