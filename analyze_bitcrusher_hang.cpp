#include <iostream>
#include <iomanip>

int main() {
    std::cout << "=== BITCRUSHER HANG ANALYSIS ===" << std::endl;
    std::cout << "\nWhen downsample parameter = 0.0:" << std::endl;
    
    // From updateParameters:
    float downsample = 0.0f;
    float sampleRateReduction_target = downsample < 0.01f ? 1.0f : 1.0f + downsample * 99.0f;
    std::cout << "1. sampleRateReduction.target = " << sampleRateReduction_target << std::endl;
    
    // After smoothing (assuming it reaches target):
    float sampleRateReduction_current = 1.0f;
    float thermalFactor = 1.0f;
    
    // In process loop:
    float reductionRate = std::max(0.001f, sampleRateReduction_current * thermalFactor);
    std::cout << "2. reductionRate = max(0.001f, " << sampleRateReduction_current 
              << " * " << thermalFactor << ") = " << reductionRate << std::endl;
    
    // Wait, this is wrong! When sampleRateReduction = 1.0, reductionRate should be 1.0, not 0.001!
    // Let me recalculate:
    reductionRate = std::max(0.001f, 1.0f * 1.0f);
    std::cout << "   CORRECTED: reductionRate = " << reductionRate << std::endl;
    
    std::cout << "\n=== WAIT, THE LOGIC IS INVERTED! ===" << std::endl;
    std::cout << "When downsample = 0, sampleRateReduction = 1.0" << std::endl;
    std::cout << "This means we SHOULD be sampling at full rate (no reduction)" << std::endl;
    std::cout << "But the code treats this as the reduction FACTOR, not RATE!" << std::endl;
    
    std::cout << "\n=== THE REAL PROBLEM ===" << std::endl;
    std::cout << "The comprehensive test FFT buffer size is 8192 samples" << std::endl;
    std::cout << "With oversampling, that's 16384 samples to process" << std::endl;
    std::cout << "The BitCrusher processes this in a tight loop" << std::endl;
    
    std::cout << "\nLet me check what happens with parameter smoothing:" << std::endl;
    
    // Smoothing simulation
    float current = 32.0f; // Initial bit depth
    float target = 1.0f;   // New target
    float smoothing = 0.99f;
    
    std::cout << "\nSmoothing from " << current << " to " << target << ":" << std::endl;
    for (int i = 0; i < 10; ++i) {
        current = target + (current - target) * smoothing;
        std::cout << "  Iteration " << i << ": current = " << std::fixed << std::setprecision(6) << current << std::endl;
    }
    
    std::cout << "\nAfter 100 iterations:" << std::endl;
    for (int i = 0; i < 100; ++i) {
        current = target + (current - target) * smoothing;
    }
    std::cout << "  current = " << current << std::endl;
    
    std::cout << "\nAfter 1000 iterations:" << std::endl;
    for (int i = 0; i < 1000; ++i) {
        current = target + (current - target) * smoothing;
    }
    std::cout << "  current = " << current << std::endl;
    
    std::cout << "\n=== INSIGHT ===" << std::endl;
    std::cout << "The smoothing takes a LONG time to converge!" << std::endl;
    std::cout << "During this time, we might have values very close to 0" << std::endl;
    std::cout << "This could cause the anti-aliasing filter calculation to explode!" << std::endl;
    
    return 0;
}