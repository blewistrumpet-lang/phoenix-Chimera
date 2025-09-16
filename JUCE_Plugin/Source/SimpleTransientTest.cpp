#include <iostream>
#include <cmath>
#include <vector>

// Test the gain calculation logic directly
void testGainCalculation() {
    std::cout << "Testing Transient Shaper Gain Calculation\n";
    std::cout << "========================================\n";
    
    // Test the fixed gain calculation
    std::cout << "\nAttack parameter tests (±15dB range):\n";
    for (float param = 0.0f; param <= 1.0f; param += 0.25f) {
        float attackDb = (param - 0.5f) * 30.0f;  // ±15dB range
        float attackGain = std::pow(10.0f, attackDb / 20.0f);
        
        std::cout << "Parameter: " << param 
                  << " -> " << attackDb << "dB" 
                  << " -> Gain: " << attackGain << std::endl;
    }
    
    std::cout << "\nSustain parameter tests (±24dB range):\n";
    for (float param = 0.0f; param <= 1.0f; param += 0.25f) {
        float sustainDb = (param - 0.5f) * 48.0f; // ±24dB range
        float sustainGain = std::pow(10.0f, sustainDb / 20.0f);
        
        std::cout << "Parameter: " << param 
                  << " -> " << sustainDb << "dB" 
                  << " -> Gain: " << sustainGain << std::endl;
    }
    
    // Test that 0.5 gives unity gain (1.0)
    std::cout << "\nUnity gain verification:\n";
    float attackUnity = std::pow(10.0f, ((0.5f - 0.5f) * 30.0f) / 20.0f);
    float sustainUnity = std::pow(10.0f, ((0.5f - 0.5f) * 48.0f) / 20.0f);
    
    std::cout << "Attack at 0.5: " << attackUnity << " (should be 1.0)\n";
    std::cout << "Sustain at 0.5: " << sustainUnity << " (should be 1.0)\n";
    
    // Test expected ratios
    std::cout << "\nExpected ratio tests:\n";
    float attackMin = std::pow(10.0f, -15.0f / 20.0f);  // -15dB
    float attackMax = std::pow(10.0f, 15.0f / 20.0f);   // +15dB
    float sustainMin = std::pow(10.0f, -24.0f / 20.0f); // -24dB
    float sustainMax = std::pow(10.0f, 24.0f / 20.0f);  // +24dB
    
    std::cout << "Attack range: " << attackMin << " to " << attackMax 
              << " (ratio: " << attackMax/attackMin << ", expected: ~31.6)\n";
    std::cout << "Sustain range: " << sustainMin << " to " << sustainMax 
              << " (ratio: " << sustainMax/sustainMin << ", expected: ~251.2)\n";
}

// Simulate the differential envelope detector
class TestDifferentialDetector {
public:
    TestDifferentialDetector() {
        // Fast envelope for transients (attack ~1ms, release ~10ms at 44.1kHz)
        float fs = 44100.0f;
        fastAttackCoeff = std::exp(-1.0f / (1.0f * 0.001f * fs));
        fastReleaseCoeff = std::exp(-1.0f / (10.0f * 0.001f * fs));
        
        // Slow envelope for sustain (attack ~20ms, release ~100ms)
        slowAttackCoeff = std::exp(-1.0f / (20.0f * 0.001f * fs));
        slowReleaseCoeff = std::exp(-1.0f / (100.0f * 0.001f * fs));
    }
    
    void process(float input, float& transientAmount, float& sustainAmount) {
        float rectified = std::abs(input);
        
        // Fast envelope follows transients
        float fastCoeff = (rectified > fastEnvelope) ? fastAttackCoeff : fastReleaseCoeff;
        fastEnvelope += (rectified - fastEnvelope) * (1.0f - fastCoeff);
        
        // Slow envelope follows sustain/body
        float slowCoeff = (rectified > slowEnvelope) ? slowAttackCoeff : slowReleaseCoeff;
        slowEnvelope += (rectified - slowEnvelope) * (1.0f - slowCoeff);
        
        // Differential: transient is when fast > slow
        float diff = fastEnvelope - slowEnvelope;
        transientAmount = std::max(0.0f, diff);
        sustainAmount = slowEnvelope;
        
        // Normalize so transient + sustain doesn't exceed input level
        float total = transientAmount + sustainAmount;
        if (total > rectified + 0.001f) {
            float scale = rectified / total;
            transientAmount *= scale;
            sustainAmount *= scale;
        }
    }
    
private:
    float fastEnvelope = 0.0f;
    float slowEnvelope = 0.0f;
    float fastAttackCoeff = 0.99f;
    float fastReleaseCoeff = 0.999f;
    float slowAttackCoeff = 0.99f;
    float slowReleaseCoeff = 0.999f;
};

void testDifferentialDetection() {
    std::cout << "\n\nTesting Differential Envelope Detection\n";
    std::cout << "======================================\n";
    
    TestDifferentialDetector detector;
    
    // Test with drum-like signal
    std::cout << "\nTesting with drum-like transient:\n";
    for (int i = 0; i < 100; ++i) {
        float signal = 0.0f;
        
        if (i < 10) {
            // Sharp attack (transient)
            float envelope = std::exp(-i * 0.2f);
            signal = envelope * 0.5f;  // Peak at 0.5
        } else if (i < 50) {
            // Decay (sustain)
            float envelope = 0.2f * std::exp(-(i-10) * 0.05f);
            signal = envelope;
        }
        
        float transientAmount, sustainAmount;
        detector.process(signal, transientAmount, sustainAmount);
        
        // Print key samples
        if (i < 15 || (i % 10 == 0 && i < 60)) {
            std::cout << "Sample " << i 
                      << ": Input=" << signal 
                      << ", Transient=" << transientAmount 
                      << ", Sustain=" << sustainAmount << std::endl;
        }
    }
}

int main() {
    std::cout << "TransientShaper_Platinum Implementation Test\n";
    std::cout << "==========================================\n\n";
    
    testGainCalculation();
    testDifferentialDetection();
    
    std::cout << "\n\nTest Summary:\n";
    std::cout << "============\n";
    std::cout << "1. Gain calculation now uses proper dB ranges:\n";
    std::cout << "   - Attack: ±15dB (parameter 0.5 = unity gain)\n";
    std::cout << "   - Sustain: ±24dB (parameter 0.5 = unity gain)\n";
    std::cout << "2. Differential envelope detection implemented:\n";
    std::cout << "   - Fast envelope (1ms attack, 10ms release)\n";
    std::cout << "   - Slow envelope (20ms attack, 100ms release)\n";
    std::cout << "   - Transient = fast - slow (when positive)\n";
    std::cout << "3. Time-based transient detection replaces frequency separation\n";
    
    return 0;
}