#pragma once
#include <map>
#include <string>
#include <juce_core/juce_core.h>

/**
 * ParameterValidator - Ensures parameter consistency across the plugin
 * 
 * The plugin architecture uses a standardized 15-parameter interface:
 * - PluginProcessor always sends parameters 0-14
 * - All values are normalized to 0.0-1.0 range
 * - Engines may use fewer parameters but should handle 0-14 gracefully
 */
class ParameterValidator {
public:
    static constexpr int kStandardParameterCount = 15;
    
    /**
     * Validates and sanitizes parameters before sending to engines
     * Ensures all values are in valid 0-1 range and adds defaults for missing params
     */
    static std::map<int, float> validateParameters(const std::map<int, float>& input) {
        std::map<int, float> validated;
        
        // Ensure all 15 parameters exist with valid values
        for (int i = 0; i < kStandardParameterCount; ++i) {
            auto it = input.find(i);
            if (it != input.end()) {
                // Clamp to valid range
                validated[i] = std::max(0.0f, std::min(1.0f, it->second));
            } else {
                // Add default value (0.5 for most params, 1.0 for mix)
                validated[i] = (i == 7 || i == 13) ? 1.0f : 0.5f;
            }
        }
        
        return validated;
    }
    
    /**
     * Helper to safely get parameter with default value
     */
    static float getParam(const std::map<int, float>& params, int index, float defaultValue = 0.5f) {
        auto it = params.find(index);
        return it != params.end() ? it->second : defaultValue;
    }
    
    /**
     * Logs parameter mismatches for debugging
     */
    static void logParameterIssue(const juce::String& engineName, int expectedCount, int receivedCount) {
        if (expectedCount != receivedCount && receivedCount != kStandardParameterCount) {
            DBG("Parameter count mismatch in " + engineName + 
                ": expected " + juce::String(expectedCount) + 
                " or " + juce::String(kStandardParameterCount) +
                ", received " + juce::String(receivedCount));
        }
    }
    
    /**
     * Standard parameter mapping for common controls
     */
    struct StandardParams {
        static constexpr int kParam1 = 0;   // Primary control (freq/rate/interval)
        static constexpr int kParam2 = 1;   // Secondary control (res/depth/key)
        static constexpr int kParam3 = 2;   // Tertiary control (type/scale)
        static constexpr int kParam4 = 3;   // Additional control
        static constexpr int kParam5 = 4;   // Additional control
        static constexpr int kParam6 = 5;   // Additional control
        static constexpr int kParam7 = 6;   // Additional control
        static constexpr int kParam8 = 7;   // Mix/Wet-Dry (standardized)
        static constexpr int kParam9 = 8;   // Additional control
        static constexpr int kParam10 = 9;  // Additional control
        static constexpr int kParam11 = 10; // Additional control
        static constexpr int kParam12 = 11; // Additional control
        static constexpr int kParam13 = 12; // Output/Gain control
        static constexpr int kParam14 = 13; // Secondary mix control
        static constexpr int kParam15 = 14; // Reserved/Future use
    };
};