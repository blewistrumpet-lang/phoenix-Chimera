#include <iostream>
#include <cmath>
#include <vector>

// Diagnostic program to understand why PitchShifter isn't working

void analyzeFormantShiftIssue() {
    std::cout << "\n=== Analyzing Formant Shift Logic ===" << std::endl;
    
    // Simulate the formant shift code from PitchShifter
    const int FFT_SIZE = 4096;
    std::vector<float> magnitude(FFT_SIZE/2 + 1);
    std::vector<float> shiftedMag(FFT_SIZE/2 + 1, 0.0f);
    
    // Fill with test magnitudes
    for (int i = 0; i <= FFT_SIZE/2; ++i) {
        magnitude[i] = 1.0f / (1.0f + i * 0.001f);  // Decreasing magnitude
    }
    
    // Test different formant values
    float formantValues[] = {0.5f, 1.0f, 1.5f, 2.0f};
    
    for (float formant : formantValues) {
        std::cout << "\nFormant = " << formant << std::endl;
        
        // Clear shifted magnitude
        std::fill(shiftedMag.begin(), shiftedMag.end(), 0.0f);
        
        // Apply formant shift (from PitchShifter line 398-402)
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            const int targetBin = static_cast<int>(bin * formant + 0.5f);
            if (targetBin >= 0 && targetBin <= FFT_SIZE/2) {
                shiftedMag[targetBin] += magnitude[bin];
            }
        }
        
        // Count how many bins have magnitude
        int nonZeroBins = 0;
        for (int bin = 0; bin <= FFT_SIZE/2; ++bin) {
            if (shiftedMag[bin] > 1e-10f) {
                nonZeroBins++;
            }
        }
        
        std::cout << "  Non-zero bins after formant shift: " << nonZeroBins 
                  << " / " << (FFT_SIZE/2 + 1) << std::endl;
        
        // Check first 10 bins
        std::cout << "  First 10 bins: ";
        for (int i = 0; i < 10; ++i) {
            std::cout << shiftedMag[i] << " ";
        }
        std::cout << std::endl;
    }
}

void analyzeDefaultParameters() {
    std::cout << "\n=== Analyzing Default Parameter Values ===" << std::endl;
    
    // From PitchShifter::Impl constructor (line 186)
    float pitchRatio = 1.0f;       // Default: no pitch change
    float formantShift = 1.0f;     // Default: no formant change
    float mixAmount = 1.0f;        // Default: full wet
    
    std::cout << "Default pitchRatio: " << pitchRatio << " (1.0 = no shift)" << std::endl;
    std::cout << "Default formantShift: " << formantShift << " (1.0 = no shift)" << std::endl;
    std::cout << "Default mixAmount: " << mixAmount << " (1.0 = full wet)" << std::endl;
    
    // What happens in updateParameters when we set pitch to 0.5?
    float paramValue = 0.5f;
    float semitones = (paramValue - 0.5f) * 48.0f;  // = 0
    float ratio = std::pow(2.0f, semitones / 12.0f);  // = 1.0
    std::cout << "\nWhen UI param = 0.5:" << std::endl;
    std::cout << "  Semitones: " << semitones << std::endl;
    std::cout << "  Pitch ratio: " << ratio << std::endl;
    
    // What about param = 0.0?
    paramValue = 0.0f;
    semitones = (paramValue - 0.5f) * 48.0f;  // = -24
    ratio = std::pow(2.0f, semitones / 12.0f);  // = 0.25 (2 octaves down)
    std::cout << "\nWhen UI param = 0.0:" << std::endl;
    std::cout << "  Semitones: " << semitones << std::endl;
    std::cout << "  Pitch ratio: " << ratio << std::endl;
    
    // What about param = 1.0?
    paramValue = 1.0f;
    semitones = (paramValue - 0.5f) * 48.0f;  // = 24
    ratio = std::pow(2.0f, semitones / 12.0f);  // = 4.0 (2 octaves up)
    std::cout << "\nWhen UI param = 1.0:" << std::endl;
    std::cout << "  Semitones: " << semitones << std::endl;
    std::cout << "  Pitch ratio: " << ratio << std::endl;
}

void findTheBug() {
    std::cout << "\n=== POTENTIAL BUG FOUND ===" << std::endl;
    
    // Look at updateParameters for formant (line 496)
    float value = 0.5f;  // Default UI value
    float formantShift = 0.5f + value * 1.5f;  // = 0.5 + 0.75 = 1.25!
    
    std::cout << "When formant UI param = 0.5 (default):" << std::endl;
    std::cout << "  formantShift = 0.5 + 0.5 * 1.5 = " << formantShift << std::endl;
    std::cout << "  This is NOT 1.0! It's shifting formants by 25%!" << std::endl;
    
    // What should it be?
    std::cout << "\nTo get formantShift = 1.0 (no shift):" << std::endl;
    float neededValue = (1.0f - 0.5f) / 1.5f;
    std::cout << "  UI param needs to be: " << neededValue << std::endl;
    
    // Check all formant parameter values
    std::cout << "\nFormant parameter mapping:" << std::endl;
    for (float v = 0.0f; v <= 1.0f; v += 0.25f) {
        float fs = 0.5f + v * 1.5f;
        std::cout << "  UI " << v << " -> formant shift " << fs << std::endl;
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "PitchShifter Bug Analysis" << std::endl;
    std::cout << "========================================" << std::endl;
    
    analyzeFormantShiftIssue();
    analyzeDefaultParameters();
    findTheBug();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "CONCLUSION:" << std::endl;
    std::cout << "The formant parameter mapping is WRONG!" << std::endl;
    std::cout << "At default (0.5), formant = 1.25, not 1.0" << std::endl;
    std::cout << "This causes frequency bins to be shifted," << std::endl;
    std::cout << "potentially leaving gaps in the spectrum!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}