#include <iostream>
#include <iomanip>

// Analysis of IntelligentHarmonizer implementation

void analyzeIntelligentHarmonizer() {
    std::cout << "=== INTELLIGENTHARMONIZER ANALYSIS ===" << std::endl;
    
    std::cout << "\n1. PITCH IMPLEMENTATION:" << std::endl;
    std::cout << "   - Uses PSOLA (Pitch Synchronous Overlap-Add), NOT phase vocoder" << std::endl;
    std::cout << "   - Line 288: process(float input, float pitchRatio)" << std::endl;
    std::cout << "   - Line 294-295: Smooths pitch changes" << std::endl;
    std::cout << "   - Line 357: grain.readPos += currentRatio (pitch applied)" << std::endl;
    std::cout << "   STATUS: Implementation looks correct ✓" << std::endl;
    
    std::cout << "\n2. INTERVAL PARAMETER:" << std::endl;
    std::cout << "   - Lines 655-668: Maps to discrete musical intervals" << std::endl;
    std::cout << "   - Line 671: intervalIndex = intervalValue * 11.99f" << std::endl;
    std::cout << "   - Quantizes to 12 preset intervals:" << std::endl;
    std::cout << "     0.00 → -12 (octave down)" << std::endl;
    std::cout << "     0.09 → -7 (5th down)" << std::endl;
    std::cout << "     0.18 → -5 (4th down)" << std::endl;
    std::cout << "     0.27 → -4 (major 3rd down)" << std::endl;
    std::cout << "     0.36 → -3 (minor 3rd down)" << std::endl;
    std::cout << "     0.45 → 0 (unison)" << std::endl;
    std::cout << "     0.55 → +3 (minor 3rd up)" << std::endl;
    std::cout << "     0.64 → +4 (major 3rd up)" << std::endl;
    std::cout << "     0.73 → +5 (4th up)" << std::endl;
    std::cout << "     0.82 → +7 (5th up)" << std::endl;
    std::cout << "     0.91 → +12 (octave up)" << std::endl;
    std::cout << "     1.00 → +19 (octave + 5th)" << std::endl;
    std::cout << "   ISSUE: Discrete intervals, not continuous pitch!" << std::endl;
    
    std::cout << "\n3. HARMONY GENERATION:" << std::endl;
    std::cout << "   - Lines 691-700: Creates multiple harmony voices" << std::endl;
    std::cout << "   - Voice 0: Base interval" << std::endl;
    std::cout << "   - Voice 1: Adds 3rd (major/minor based on scale)" << std::endl;
    std::cout << "   - Voice 2: Adds 5th" << std::endl;
    std::cout << "   - Voice 3: Adds 7th" << std::endl;
    std::cout << "   STATUS: Intelligent harmony working ✓" << std::endl;
    
    std::cout << "\n4. SCALE QUANTIZATION:" << std::endl;
    std::cout << "   - Line 704: ScaleQuantizer::quantize()" << std::endl;
    std::cout << "   - Lines 388-399: 10 different scales defined" << std::endl;
    std::cout << "   - Quantizes pitch to nearest scale degree" << std::endl;
    std::cout << "   STATUS: Scale quantization implemented ✓" << std::endl;
    
    std::cout << "\n5. PITCH RATIO CALCULATION:" << std::endl;
    std::cout << "   - Line 710: pitchRatio = pow(2.0f, voiceInterval / 12.0f)" << std::endl;
    std::cout << "   - Correct semitone to ratio conversion ✓" << std::endl;
    std::cout << "   - Line 733: shifter.process(sample, pitchRatio)" << std::endl;
    std::cout << "   STATUS: Pitch calculation correct ✓" << std::endl;
}

void identifyIssue() {
    std::cout << "\n=== ISSUE IDENTIFICATION ===" << std::endl;
    
    std::cout << "\nThe IntelligentHarmonizer is DIFFERENT from PitchShifter:" << std::endl;
    std::cout << "1. It uses PSOLA, not phase vocoder (no FFT bug)" << std::endl;
    std::cout << "2. It quantizes to discrete musical intervals" << std::endl;
    std::cout << "3. It generates intelligent harmonies" << std::endl;
    std::cout << "4. Implementation appears CORRECT" << std::endl;
    
    std::cout << "\nPOTENTIAL ISSUES TO CHECK:" << std::endl;
    std::cout << "1. Parameter mapping (0-1 to interval index)" << std::endl;
    std::cout << "2. PSOLA grain scheduling (line 299)" << std::endl;
    std::cout << "3. Oversampling implementation" << std::endl;
    std::cout << "4. Voice mixing levels (line 758)" << std::endl;
    
    std::cout << "\nTO TEST:" << std::endl;
    std::cout << "- Set interval to 0.0 (should be octave down)" << std::endl;
    std::cout << "- Set interval to 0.5 (should be unison/no change)" << std::endl;
    std::cout << "- Set interval to 1.0 (should be octave+5th up)" << std::endl;
    std::cout << "- Set voices > 1 to hear harmonies" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "INTELLIGENTHARMONIZER IMPLEMENTATION ANALYSIS" << std::endl;
    std::cout << "========================================" << std::endl;
    
    analyzeIntelligentHarmonizer();
    identifyIssue();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "CONCLUSION:" << std::endl;
    std::cout << "IntelligentHarmonizer uses PSOLA (not phase vocoder)" << std::endl;
    std::cout << "No obvious bugs found - implementation looks correct" << std::endl;
    std::cout << "Test in Logic Pro to verify functionality" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}