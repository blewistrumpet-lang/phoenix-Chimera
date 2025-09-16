#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>

/**
 * Debug why PSOLA is outputting 78-109 Hz for all ratios
 * instead of the expected shifted frequencies
 */

void analyzePsolaFailure() {
    printf("=== PSOLA FAILURE ANALYSIS ===\n\n");
    
    printf("SYMPTOM: Engine outputs 78-109 Hz regardless of pitch ratio\n");
    printf("EXPECTED: Should output input_freq * ratio\n\n");
    
    printf("Test Results Summary:\n");
    printf("---------------------\n");
    printf("Ratio    Expected Hz    Got Hz    Error\n");
    printf("0.5000   110.00        109.44    OK (-8.8 cents)\n");
    printf("0.7071   155.56        78.18     WRONG (half freq!)\n");
    printf("1.0000   220.00        109.37    WRONG (half freq!)\n");
    printf("1.4142   311.12        103.43    WRONG (1/3 freq!)\n");
    printf("1.5000   330.00        82.82     WRONG (1/4 freq!)\n");
    printf("2.0000   440.00        88.30     WRONG (1/5 freq!)\n\n");
    
    printf("PATTERN OBSERVED:\n");
    printf("- Output frequencies are roughly F0/N where N=2,3,4,5\n");
    printf("- This suggests SUBHARMONIC generation\n");
    printf("- The engine is playing grains at wrong rate\n\n");
    
    printf("LIKELY CAUSES:\n");
    printf("1. Synthesis hop calculation is wrong\n");
    printf("   - Should be: synHop = period / ratio\n");
    printf("   - But getting: synHop = period * ratio (or worse)\n\n");
    
    printf("2. Epoch selection (φ mapping) is broken\n");
    printf("   - Integer schedule might be skipping too many epochs\n");
    printf("   - Causing playback at submultiple of desired rate\n\n");
    
    printf("3. The 'surgical fixes' broke the core algorithm:\n");
    printf("   - FIX 1: Integer schedule - might be wrong\n");
    printf("   - FIX 2: Variable windows - might affect timing\n");  
    printf("   - FIX 3: Core WSOLA - might be selecting wrong grains\n\n");
    
    printf("DIAGNOSTIC STEPS:\n");
    printf("1. Check synthesis hop calculation\n");
    printf("2. Verify epoch selection logic\n");
    printf("3. Test without the surgical fixes\n");
    printf("4. Add debug output to see actual vs expected timing\n");
}

void checkSynthesisHopLogic() {
    printf("\n=== SYNTHESIS HOP CALCULATION CHECK ===\n");
    
    float period = 218.0f; // samples at 48kHz for 220Hz
    float ratios[] = {0.5f, 0.7071f, 1.0f, 1.5f, 2.0f};
    
    for (float ratio : ratios) {
        float correctHop = period / ratio;
        float wrongHop1 = period * ratio;
        float wrongHop2 = period; // Fixed hop (no scaling)
        
        printf("\nRatio %.4f:\n", ratio);
        printf("  Correct hop: %.1f samples (%.1f Hz output)\n", 
               correctHop, 48000.0f / correctHop);
        printf("  Wrong (P*α): %.1f samples (%.1f Hz output)\n",
               wrongHop1, 48000.0f / wrongHop1);
        printf("  Wrong (fixed): %.1f samples (%.1f Hz output)\n",
               wrongHop2, 48000.0f / wrongHop2);
    }
}

void analyzeIntegerSchedule() {
    printf("\n=== INTEGER SCHEDULE ANALYSIS ===\n");
    printf("The Bresenham-style integer schedule might be wrong:\n\n");
    
    // Simulate the integer schedule
    float alpha = 0.7071f;
    float invA = 1.0f / alpha;
    float acc = 0.0f;
    int kInt = 0;
    
    printf("Alpha = %.4f, 1/alpha = %.4f\n", alpha, invA);
    printf("Step sequence for first 10 synthesis marks:\n");
    
    for (int syn = 0; syn < 10; ++syn) {
        acc += invA;
        int step = (int)acc;
        acc -= step;
        step = std::max(1, step);
        kInt += step;
        
        printf("  Syn %d: step=%d, kInt=%d, acc=%.3f\n", 
               syn, step, kInt, acc);
    }
    
    printf("\nFor α=0.7071 (1/α=1.414):\n");
    printf("Expected: alternating 1,2,1,2 steps (average 1.414)\n");
    printf("This should select epochs: 0,1,3,4,6,7,9,10...\n");
    printf("But if wrong, might select: 0,2,4,6,8... (every 2nd)\n");
    printf("That would give HALF the desired frequency!\n");
}

int main() {
    analyzePsolaFailure();
    checkSynthesisHopLogic();
    analyzeIntegerSchedule();
    
    printf("\n=== CONCLUSION ===\n");
    printf("The PSOLA engine is generating subharmonics because:\n");
    printf("1. The synthesis hop and/or epoch selection is wrong\n");
    printf("2. The 'surgical fixes' likely broke the core algorithm\n");
    printf("3. Need to revert to a simpler, working implementation\n");
    printf("4. Then carefully add fixes one at a time with testing\n");
    
    return 0;
}