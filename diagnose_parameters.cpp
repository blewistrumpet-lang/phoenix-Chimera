#include <cstdio>
#include <map>
#include <vector>
#include <cmath>

// Simulate what the plugin is sending vs what IntelligentHarmonizer expects

void analyzeParameterFlow() {
    printf("=== PARAMETER MAPPING DIAGNOSIS ===\n\n");
    
    // What PluginProcessor sends (from updateEngineParameters)
    printf("1. PLUGIN PROCESSOR SENDS:\n");
    printf("   Parameters as map<int, float> with indices 0-14\n");
    printf("   Values are NORMALIZED (0.0 - 1.0)\n\n");
    
    // Simulate what gets sent for each parameter
    std::map<int, float> sentParams;
    for (int i = 0; i < 15; ++i) {
        sentParams[i] = 0.5f; // Center position for all params
    }
    
    printf("   Sent parameters:\n");
    for (const auto& [id, val] : sentParams) {
        printf("   param[%d] = %.3f\n", id, val);
    }
    
    printf("\n2. INTELLIGENT HARMONIZER EXPECTS:\n");
    printf("   Parameter 0: Interval (0-1 normalized, 0.5 = unison)\n");
    printf("   Parameter 1: Key (0-1 normalized to 0-11)\n");
    printf("   Parameter 2: Scale (0-1 normalized to 0-9)\n");
    printf("   Parameter 3: Voices (not used)\n");
    printf("   Parameter 4: Spread (not used)\n");
    printf("   Parameter 5: Humanize (not used)\n");
    printf("   Parameter 6: Formant (0-1 range)\n");
    printf("   Parameter 7: Mix (0-1 range)\n");
    
    printf("\n3. PARAMETER CONVERSION ANALYSIS:\n");
    
    // Test interval parameter conversion
    float testIntervals[] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};
    printf("\n   Interval Parameter (param 0):\n");
    for (float val : testIntervals) {
        // Current conversion in IntelligentHarmonizer
        int intervalSemitones = static_cast<int>((val - 0.5f) * 48.0f);
        intervalSemitones = std::max(-24, std::min(24, intervalSemitones));
        float ratio = std::pow(2.0f, intervalSemitones / 12.0f);
        
        printf("   %.2f -> %+d semitones -> ratio %.4f\n", val, intervalSemitones, ratio);
    }
    
    // Test scale parameter
    printf("\n   Scale Parameter (param 2):\n");
    float testScales[] = {0.0f, 0.11f, 0.22f, 0.5f, 1.0f};
    for (float val : testScales) {
        int scaleIndex = static_cast<int>(val * 9.0f + 0.5f);
        printf("   %.2f -> scale index %d\n", val, scaleIndex);
    }
    
    // Test mix parameter
    printf("\n   Mix Parameter (param 7):\n");
    printf("   0.0 -> 0%% wet (dry signal only)\n");
    printf("   0.5 -> 50%% wet\n");
    printf("   1.0 -> 100%% wet (processed only)\n");
    
    printf("\n4. POTENTIAL ISSUES FOUND:\n");
    
    // Check if parameters are actually being called
    printf("\n   A. Parameter Update Chain:\n");
    printf("   - PluginProcessor::parameterChanged() -> called when UI changes\n");
    printf("   - PluginProcessor::updateEngineParameters() -> sends to engine\n");
    printf("   - IntelligentHarmonizer::updateParameters() -> receives map\n");
    printf("   - BUT: Are the parameter IDs matching what the UI sends?\n");
    
    // Check parameter ranges
    printf("\n   B. Parameter Range Issues:\n");
    printf("   - Interval at 0.5 should = unison (ratio 1.0)\n");
    printf("   - But integer casting might cause issues:\n");
    float centerVal = 0.5f;
    float computed = (centerVal - 0.5f) * 48.0f;
    int semitones = static_cast<int>(computed);
    printf("     0.5 -> (0.5 - 0.5) * 48 = %.6f -> int = %d\n", computed, semitones);
    printf("     This SHOULD give 0 semitones (unison)\n");
    
    // Check if the parameters are actually connected
    printf("\n   C. APVTS Connection Check:\n");
    printf("   - Need to verify slot[N]_param[M] naming matches\n");
    printf("   - Engine ID 41 is IntelligentHarmonizer\n");
    printf("   - Parameters should be: slot[N]_param1 through slot[N]_param8\n");
    
    printf("\n5. ACTUAL PARAMETER NAMES IN APVTS:\n");
    printf("   Based on IntelligentHarmonizer header:\n");
    printf("   - kInterval = 0\n");
    printf("   - kKey = 1\n");
    printf("   - kScale = 2\n");
    printf("   - kVoices = 3\n");
    printf("   - kSpread = 4\n");
    printf("   - kHumanize = 5\n");
    printf("   - kFormant = 6\n");
    printf("   - kMix = 7\n");
    printf("   Total: 8 parameters (not 15!)\n");
}

void testParameterConversion() {
    printf("\n\n=== PARAMETER CONVERSION TEST ===\n");
    
    // Test the actual conversion logic
    auto convertInterval = [](float normalized) {
        // Center position (0.5) = no shift
        if (std::fabs(normalized - 0.5f) < 0.01f) {
            return 1.0f; // Unity ratio
        }
        
        int intervalSemitones = static_cast<int>((normalized - 0.5f) * 48.0f);
        intervalSemitones = std::max(-24, std::min(24, intervalSemitones));
        float ratio = std::pow(2.0f, intervalSemitones / 12.0f);
        return ratio;
    };
    
    // Test critical values
    struct TestCase {
        float input;
        const char* name;
        float expectedRatio;
    } cases[] = {
        {0.0f, "Min (-24 st)", 0.25f},
        {0.25f, "Down octave (-12 st)", 0.5f},
        {0.4375f, "Down tritone (-6 st)", 0.7071f},
        {0.5f, "Center (unison)", 1.0f},
        {0.5625f, "Up tritone (+6 st)", 1.4142f},
        {0.625f, "Up fifth (+7 st)", 1.5f},
        {0.75f, "Up octave (+12 st)", 2.0f},
        {1.0f, "Max (+24 st)", 4.0f},
    };
    
    printf("\nInterval parameter conversion test:\n");
    printf("Input  -> Semitones -> Ratio  -> Expected -> Status\n");
    
    for (const auto& test : cases) {
        float ratio = convertInterval(test.input);
        int semitones = static_cast<int>((test.input - 0.5f) * 48.0f);
        bool pass = std::fabs(ratio - test.expectedRatio) < 0.01f;
        
        printf("%.4f -> %+3d st    -> %.4f -> %.4f   -> %s\n",
               test.input, semitones, ratio, test.expectedRatio, 
               pass ? "PASS" : "FAIL");
        
        if (!pass) {
            printf("  ERROR: %s not working correctly!\n", test.name);
        }
    }
    
    // Check if the UI is sending the right values
    printf("\n\nUI VALUE MAPPING:\n");
    printf("The UI encoder/slider should map:\n");
    printf("- Full left  = 0.0 (normalized) = -24 semitones\n");
    printf("- Center     = 0.5 (normalized) = 0 semitones (unison)\n");
    printf("- Full right = 1.0 (normalized) = +24 semitones\n");
    printf("\nIf the encoder doesn't change anything, check:\n");
    printf("1. Is parameterChanged() being called?\n");
    printf("2. Is the parameter ID correct? (slot[N]_param1 for interval)\n");
    printf("3. Is updateEngineParameters() being called after engine load?\n");
    printf("4. Are the values actually changing in the APVTS?\n");
}

int main() {
    analyzeParameterFlow();
    testParameterConversion();
    
    printf("\n\n=== DIAGNOSIS COMPLETE ===\n");
    printf("\nKEY FINDINGS:\n");
    printf("1. IntelligentHarmonizer expects 8 parameters, but PluginProcessor sends 15\n");
    printf("2. Only params 0-7 are used, params 8-14 are ignored\n");
    printf("3. Parameter 0 (Interval) should work if normalized correctly\n");
    printf("4. Parameter 7 (Mix) should control dry/wet blend\n");
    printf("\nNEXT STEPS:\n");
    printf("1. Add debug output to updateParameters() to see what's received\n");
    printf("2. Verify APVTS parameter names match slot[N]_paramM format\n");
    printf("3. Check if parameterChanged() is triggered when moving encoders\n");
    
    return 0;
}