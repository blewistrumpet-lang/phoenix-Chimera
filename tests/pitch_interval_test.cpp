// Pitch Engine Musical Interval Test
// Tests current pitch mapping and proposes improvements

#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <iomanip>

// Musical interval definitions
struct MusicalInterval {
    std::string name;
    int semitones;
    float ratio;  // Frequency ratio
};

std::vector<MusicalInterval> musicalIntervals = {
    {"Octave Down", -12, 0.5f},
    {"Major 7th Down", -11, 0.5297f},
    {"Minor 7th Down", -10, 0.5612f},
    {"Major 6th Down", -9, 0.5946f},
    {"Minor 6th Down", -8, 0.6300f},
    {"Perfect 5th Down", -7, 0.6674f},
    {"Tritone Down", -6, 0.7071f},
    {"Perfect 4th Down", -5, 0.7492f},
    {"Major 3rd Down", -4, 0.7937f},
    {"Minor 3rd Down", -3, 0.8409f},
    {"Major 2nd Down", -2, 0.8909f},
    {"Minor 2nd Down", -1, 0.9439f},
    {"Unison", 0, 1.0f},
    {"Minor 2nd Up", 1, 1.0595f},
    {"Major 2nd Up", 2, 1.1225f},
    {"Minor 3rd Up", 3, 1.1892f},
    {"Major 3rd Up", 4, 1.2599f},
    {"Perfect 4th Up", 5, 1.3348f},
    {"Tritone Up", 6, 1.4142f},
    {"Perfect 5th Up", 7, 1.4983f},
    {"Minor 6th Up", 8, 1.5874f},
    {"Major 6th Up", 9, 1.6818f},
    {"Minor 7th Up", 10, 1.7818f},
    {"Major 7th Up", 11, 1.8877f},
    {"Octave Up", 12, 2.0f}
};

// Current PitchShifter mapping (from code)
float currentPitchMapping(float param) {
    // From PitchShifter.cpp line 486: 0.25f + value * 3.75f
    return 0.25f + param * 3.75f;  // Range: 0.25 to 4.0 (ratio)
}

// Current IntelligentHarmonizer mapping
float currentHarmonizerMapping(float param) {
    // Parameter 0-1 maps to interval selection
    // But it's unclear what intervals these map to
    return param;  
}

// Proposed improved mapping for PitchShifter
float improvedPitchMapping(float param) {
    // Map 0-1 to -24 to +24 semitones
    float semitones = (param - 0.5f) * 48.0f;
    // Convert to ratio
    return std::pow(2.0f, semitones / 12.0f);
}

// Proposed discrete interval mapping for IntelligentHarmonizer
int improvedHarmonizerMapping(float param) {
    // Quantize to common intervals
    const int intervals[] = {-12, -7, -5, -4, -3, 0, 3, 4, 5, 7, 12, 12};
    int index = static_cast<int>(param * 11.99f);
    return intervals[std::min(index, 11)];
}

void testCurrentMapping() {
    std::cout << "==================================" << std::endl;
    std::cout << "CURRENT PITCH MAPPING ANALYSIS" << std::endl;
    std::cout << "==================================" << std::endl;
    
    std::cout << "\nPitchShifter Current Mapping (Ratio-based):" << std::endl;
    std::cout << "Parameter | Ratio  | Semitones | Musical Interval" << std::endl;
    std::cout << "----------|--------|-----------|------------------" << std::endl;
    
    float testParams[] = {0.0f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 1.0f};
    
    for (float param : testParams) {
        float ratio = currentPitchMapping(param);
        float semitones = 12.0f * std::log2(ratio);
        
        // Find closest musical interval
        std::string interval = "Custom";
        float minDiff = 100.0f;
        for (const auto& mi : musicalIntervals) {
            float diff = std::abs(semitones - mi.semitones);
            if (diff < minDiff && diff < 0.5f) {
                minDiff = diff;
                interval = mi.name;
            }
        }
        
        std::cout << std::fixed << std::setprecision(3) 
                  << std::setw(9) << param << " | "
                  << std::setw(6) << ratio << " | "
                  << std::setw(9) << semitones << " | "
                  << interval << std::endl;
    }
    
    std::cout << "\nPROBLEMS:" << std::endl;
    std::cout << "1. Non-musical intervals (e.g., param=0.25 gives +5.49 semitones)" << std::endl;
    std::cout << "2. Asymmetric range (down: -24 semitones, up: +24 semitones)" << std::endl;
    std::cout << "3. No clear mapping to common intervals" << std::endl;
    std::cout << "4. Unison (ratio=1.0) is at param=0.2, not intuitive" << std::endl;
}

void testImprovedMapping() {
    std::cout << "\n==================================" << std::endl;
    std::cout << "PROPOSED IMPROVED MAPPING" << std::endl;
    std::cout << "==================================" << std::endl;
    
    std::cout << "\nImproved PitchShifter Mapping (Semitone-based):" << std::endl;
    std::cout << "Parameter | Semitones | Ratio  | Musical Interval" << std::endl;
    std::cout << "----------|-----------|--------|------------------" << std::endl;
    
    float testParams[] = {0.0f, 0.25f, 0.375f, 0.5f, 0.625f, 0.75f, 1.0f};
    
    for (float param : testParams) {
        float semitones = (param - 0.5f) * 48.0f;
        float ratio = std::pow(2.0f, semitones / 12.0f);
        
        // Find closest musical interval
        std::string interval = "Custom";
        float minDiff = 100.0f;
        for (const auto& mi : musicalIntervals) {
            float diff = std::abs(semitones - mi.semitones);
            if (diff < minDiff && diff < 0.5f) {
                minDiff = diff;
                interval = mi.name;
            }
        }
        
        std::cout << std::fixed << std::setprecision(3) 
                  << std::setw(9) << param << " | "
                  << std::setw(9) << semitones << " | "
                  << std::setw(6) << ratio << " | "
                  << interval << std::endl;
    }
    
    std::cout << "\nIMPROVEMENTS:" << std::endl;
    std::cout << "1. param=0.5 gives unison (0 semitones) - intuitive center" << std::endl;
    std::cout << "2. Symmetric range: ±24 semitones" << std::endl;
    std::cout << "3. Linear mapping to semitones" << std::endl;
    std::cout << "4. Easy to create presets for common intervals" << std::endl;
}

void testHarmonizerIntervals() {
    std::cout << "\n==================================" << std::endl;
    std::cout << "INTELLIGENT HARMONIZER INTERVALS" << std::endl;
    std::cout << "==================================" << std::endl;
    
    std::cout << "\nProposed Discrete Interval Mapping:" << std::endl;
    std::cout << "Parameter Range | Interval | Description" << std::endl;
    std::cout << "----------------|----------|-------------" << std::endl;
    
    struct IntervalMapping {
        float minParam;
        float maxParam;
        int semitones;
        std::string name;
    };
    
    std::vector<IntervalMapping> mappings = {
        {0.00f, 0.08f, -12, "Octave Down"},
        {0.08f, 0.17f, -7, "Perfect 5th Down"},
        {0.17f, 0.25f, -5, "Perfect 4th Down"},
        {0.25f, 0.33f, -4, "Major 3rd Down"},
        {0.33f, 0.42f, -3, "Minor 3rd Down"},
        {0.42f, 0.50f, 0, "Unison"},
        {0.50f, 0.58f, 3, "Minor 3rd Up"},
        {0.58f, 0.67f, 4, "Major 3rd Up"},
        {0.67f, 0.75f, 5, "Perfect 4th Up"},
        {0.75f, 0.83f, 7, "Perfect 5th Up"},
        {0.83f, 0.92f, 12, "Octave Up"},
        {0.92f, 1.00f, 19, "Octave + 5th Up"}
    };
    
    for (const auto& m : mappings) {
        std::cout << std::fixed << std::setprecision(2)
                  << m.minParam << " - " << m.maxParam << " | "
                  << std::setw(8) << m.semitones << " | "
                  << m.name << std::endl;
    }
    
    std::cout << "\nKEY FEATURES:" << std::endl;
    std::cout << "1. Discrete musical intervals only" << std::endl;
    std::cout << "2. Common harmonies emphasized" << std::endl;
    std::cout << "3. Unison at center (0.42-0.50)" << std::endl;
    std::cout << "4. Octaves at extremes" << std::endl;
}

void generatePresetCode() {
    std::cout << "\n==================================" << std::endl;
    std::cout << "C++ CODE FOR IMPROVED MAPPING" << std::endl;
    std::cout << "==================================" << std::endl;
    
    std::cout << "\n// For PitchShifter.cpp updateParameters():" << std::endl;
    std::cout << "case kPitch: {" << std::endl;
    std::cout << "    // Convert 0-1 param to -24 to +24 semitones" << std::endl;
    std::cout << "    float semitones = (value - 0.5f) * 48.0f;" << std::endl;
    std::cout << "    float ratio = std::pow(2.0f, semitones / 12.0f);" << std::endl;
    std::cout << "    pimpl->pitchRatio.setTarget(ratio);" << std::endl;
    std::cout << "    break;" << std::endl;
    std::cout << "}" << std::endl;
    
    std::cout << "\n// For IntelligentHarmonizer.cpp:" << std::endl;
    std::cout << "const int kHarmonyIntervals[] = {" << std::endl;
    std::cout << "    -12, -7, -5, -4, -3, 0, 3, 4, 5, 7, 12, 19" << std::endl;
    std::cout << "};" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "case kInterval: {" << std::endl;
    std::cout << "    int index = static_cast<int>(value * 11.99f);" << std::endl;
    std::cout << "    int semitones = kHarmonyIntervals[std::min(index, 11)];" << std::endl;
    std::cout << "    pimpl->interval.set(semitones);" << std::endl;
    std::cout << "    break;" << std::endl;
    std::cout << "}" << std::endl;
}

int main() {
    std::cout << "PITCH ENGINE MUSICAL INTERVAL ANALYSIS" << std::endl;
    std::cout << "======================================" << std::endl;
    
    testCurrentMapping();
    testImprovedMapping();
    testHarmonizerIntervals();
    generatePresetCode();
    
    std::cout << "\n==================================" << std::endl;
    std::cout << "RECOMMENDATIONS" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "1. Update PitchShifter to use semitone-based mapping" << std::endl;
    std::cout << "2. Add discrete interval selection to IntelligentHarmonizer" << std::endl;
    std::cout << "3. Create musical presets for common use cases" << std::endl;
    std::cout << "4. Add visual feedback showing current interval" << std::endl;
    std::cout << "5. Consider adding a 'snap to interval' mode" << std::endl;
    
    return 0;
}