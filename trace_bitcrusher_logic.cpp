#include <iostream>
#include <iomanip>

int main() {
    std::cout << "=== TRACING BITCRUSHER SAMPLE COUNTER LOGIC ===" << std::endl;
    
    // When downsample = 0, sampleRateReduction = 1.0
    float sampleRateReduction = 1.0f;
    float thermalFactor = 1.0f;
    float sampleCounter = 0.0f;
    
    std::cout << "\nWith sampleRateReduction = 1.0 (no reduction):" << std::endl;
    float reductionRate = std::max(0.001f, sampleRateReduction * thermalFactor);
    std::cout << "reductionRate = " << reductionRate << std::endl;
    
    std::cout << "\nSimulating sample processing:" << std::endl;
    for (int i = 0; i < 10; ++i) {
        sampleCounter += reductionRate;
        std::cout << "Sample " << i << ": sampleCounter = " << std::fixed << std::setprecision(3) << sampleCounter;
        
        if (sampleCounter >= 1.0f) {
            // Assuming jitter = 0 for simplicity
            float jitterAmount = 0.0f;
            sampleCounter = jitterAmount;
            std::cout << " -> TRIGGERED! Reset to " << sampleCounter;
        }
        std::cout << std::endl;
    }
    
    std::cout << "\n=== INSIGHT ===" << std::endl;
    std::cout << "With reductionRate = 1.0, sampleCounter reaches 1.0 EVERY sample!" << std::endl;
    std::cout << "This means we update the held sample EVERY time" << std::endl;
    std::cout << "This is correct behavior - no downsampling occurs" << std::endl;
    
    std::cout << "\n=== BUT WAIT - Let's check the oversampling path ===" << std::endl;
    std::cout << "The comprehensive test uses 8192 samples" << std::endl;
    std::cout << "With 2x oversampling, that's 16384 iterations" << std::endl;
    std::cout << "Each iteration does:" << std::endl;
    std::cout << "1. Add DC offset" << std::endl;
    std::cout << "2. Check gate threshold" << std::endl;
    std::cout << "3. Increment sampleCounter" << std::endl;
    std::cout << "4. If counter >= 1.0:" << std::endl;
    std::cout << "   - Calculate anti-aliasing filter (with division!)" << std::endl;
    std::cout << "   - Apply dither" << std::endl;
    std::cout << "   - Quantize" << std::endl;
    std::cout << "   - Apply soft clipping" << std::endl;
    std::cout << "5. Apply aliasing interpolation" << std::endl;
    std::cout << "6. Mix with dry signal" << std::endl;
    
    std::cout << "\n16384 iterations × all these operations = potential for timeout!" << std::endl;
    
    std::cout << "\n=== THE SMOKING GUN ===" << std::endl;
    std::cout << "Look at the anti-aliasing filter calculation when sampleCounter >= 1.0:" << std::endl;
    std::cout << "  float cutoff = 0.5f / std::max(0.001f, m_sampleRateReduction.current * thermalFactor);" << std::endl;
    std::cout << "  float alpha = 1.0f - std::exp(-2.0f * M_PI * cutoff);" << std::endl;
    std::cout << "\nIf sampleRateReduction = 1.0:" << std::endl;
    float cutoff = 0.5f / std::max(0.001f, 1.0f);
    std::cout << "  cutoff = 0.5f / 1.0f = " << cutoff << std::endl;
    float alpha = 1.0f - std::exp(-2.0f * M_PI * cutoff);
    std::cout << "  alpha = 1.0f - exp(-2π × " << cutoff << ") = " << alpha << std::endl;
    std::cout << "\nThis alpha value of " << alpha << " means very aggressive filtering!" << std::endl;
    std::cout << "The filter states could accumulate numerical errors over 16384 iterations" << std::endl;
    
    return 0;
}