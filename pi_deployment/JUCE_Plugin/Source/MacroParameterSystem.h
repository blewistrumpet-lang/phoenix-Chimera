#pragma once

#include "JuceHeader.h"
#include <array>

/**
 * Macro Parameter System for Trinity GPIO Control
 *
 * MIX Mode provides 3 musical macro controls that affect ALL slot engines:
 * - WARMTH: Dark ← → Bright (frequency content)
 * - SIZE:   Tight ← → Spacious (reverb/delay/space)
 * - PUNCH:  Soft ← → Aggressive (dynamics/drive)
 *
 * Each engine interprets these based on its type.
 * Simple implementation: linear offsets applied to relevant parameters.
 */
class MacroParameterSystem {
public:
    enum class MacroType {
        WARMTH = 0,  // E1 in MIX mode
        SIZE = 1,    // E2 in MIX mode
        PUNCH = 2    // E3 in MIX mode
    };

    // Macro values stored centrally (0.5 = neutral)
    struct MacroState {
        float warmth = 0.5f;  // 0=dark, 0.5=neutral, 1=bright
        float size = 0.5f;    // 0=tight, 0.5=neutral, 1=spacious
        float punch = 0.5f;   // 0=soft, 0.5=neutral, 1=aggressive
    };

    MacroParameterSystem() = default;
    ~MacroParameterSystem() = default;

    /**
     * Set a macro value (called by encoder events in MIX mode)
     * @param macro Which macro to update
     * @param value New value (0.0 to 1.0, 0.5 is neutral)
     */
    void setMacroValue(MacroType macro, float value) {
        value = juce::jlimit(0.0f, 1.0f, value);

        switch (macro) {
            case MacroType::WARMTH:
                state.warmth = value;
                DBG("[MACRO] WARMTH set to " << value << " (" << getWarmthDescription(value) << ")");
                break;
            case MacroType::SIZE:
                state.size = value;
                DBG("[MACRO] SIZE set to " << value << " (" << getSizeDescription(value) << ")");
                break;
            case MacroType::PUNCH:
                state.punch = value;
                DBG("[MACRO] PUNCH set to " << value << " (" << getPunchDescription(value) << ")");
                break;
        }
    }

    /**
     * Get current value for a macro
     */
    float getMacroValue(MacroType macro) const {
        switch (macro) {
            case MacroType::WARMTH: return state.warmth;
            case MacroType::SIZE:   return state.size;
            case MacroType::PUNCH:  return state.punch;
            default: return 0.5f;
        }
    }

    /**
     * Get the macro state for engines to query
     */
    const MacroState& getState() const { return state; }

    /**
     * Calculate parameter offset based on macro and parameter type
     *
     * Engines call this to adjust their parameters:
     * - Filters: warmth affects cutoff
     * - EQs: warmth affects frequency/gain
     * - Reverbs: size affects decay/room
     * - Delays: size affects time/feedback
     * - Compressors: punch affects threshold/ratio
     * - Distortions: punch affects drive/gain
     *
     * @return Offset to add to normalized parameter (-0.5 to +0.5)
     */
    float calculateParameterOffset(MacroType macro, const juce::String& parameterHint) const {
        float macroValue = getMacroValue(macro);
        float offset = (macroValue - 0.5f);  // Convert to -0.5 to +0.5 range

        // Scale based on parameter type for musical results
        if (macro == MacroType::WARMTH) {
            if (parameterHint.containsIgnoreCase("freq") ||
                parameterHint.containsIgnoreCase("cutoff")) {
                return offset * 0.3f;  // Moderate frequency adjustment
            }
            if (parameterHint.containsIgnoreCase("tone") ||
                parameterHint.containsIgnoreCase("bright")) {
                return offset * 0.4f;  // Slightly more for tone controls
            }
        }
        else if (macro == MacroType::SIZE) {
            if (parameterHint.containsIgnoreCase("decay") ||
                parameterHint.containsIgnoreCase("size") ||
                parameterHint.containsIgnoreCase("room")) {
                return offset * 0.4f;  // Good range for reverb
            }
            if (parameterHint.containsIgnoreCase("delay") ||
                parameterHint.containsIgnoreCase("feedback")) {
                return offset * 0.3f;  // More subtle for delays
            }
            if (parameterHint.containsIgnoreCase("depth")) {
                return offset * 0.5f;  // Full range for modulation depth
            }
        }
        else if (macro == MacroType::PUNCH) {
            if (parameterHint.containsIgnoreCase("thresh")) {
                return -offset * 0.4f;  // Inverted: more punch = lower threshold
            }
            if (parameterHint.containsIgnoreCase("ratio")) {
                return offset * 0.3f;  // Higher ratio for more punch
            }
            if (parameterHint.containsIgnoreCase("drive") ||
                parameterHint.containsIgnoreCase("gain")) {
                return offset * 0.4f;  // Good range for drive
            }
        }

        return offset * 0.25f;  // Default: conservative adjustment
    }

    /**
     * Reset all macros to neutral (0.5)
     */
    void reset() {
        state.warmth = 0.5f;
        state.size = 0.5f;
        state.punch = 0.5f;
        DBG("[MACRO] All macros reset to neutral");
    }

    /**
     * Get display label for current macro value
     */
    juce::String getMacroLabel(MacroType macro) const {
        switch (macro) {
            case MacroType::WARMTH: return "Warmth";
            case MacroType::SIZE:   return "Size";
            case MacroType::PUNCH:  return "Punch";
            default: return "";
        }
    }

    /**
     * Store/recall for A/B comparison
     */
    void captureToBank(bool isBankB) {
        if (isBankB) {
            bankB = state;
        } else {
            bankA = state;
        }
    }

    void recallFromBank(bool isBankB) {
        state = isBankB ? bankB : bankA;
    }

private:
    MacroState state;
    MacroState bankA;  // For A/B switching
    MacroState bankB;

    // Helper functions for debug descriptions
    juce::String getWarmthDescription(float value) const {
        if (value < 0.3f) return "Dark";
        if (value < 0.45f) return "Warm";
        if (value < 0.55f) return "Neutral";
        if (value < 0.7f) return "Bright";
        return "Brilliant";
    }

    juce::String getSizeDescription(float value) const {
        if (value < 0.3f) return "Tight";
        if (value < 0.45f) return "Small";
        if (value < 0.55f) return "Medium";
        if (value < 0.7f) return "Large";
        return "Huge";
    }

    juce::String getPunchDescription(float value) const {
        if (value < 0.3f) return "Soft";
        if (value < 0.45f) return "Gentle";
        if (value < 0.55f) return "Neutral";
        if (value < 0.7f) return "Punchy";
        return "Aggressive";
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroParameterSystem)
};