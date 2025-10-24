#pragma once

#include "JuceHeader.h"

/**
 * A/B State Engine - Dual Parameter Banks
 * Week 2 of Trinity GPIO Plan
 *
 * Stores two independent parameter banks (A and B) that can be instantly switched.
 * Used for A/B comparison and preset tweaking.
 */
class ABStateEngine
{
public:
    // Parameter bank - stores all controllable parameters
    struct ParamBank {
        float input_gain = 1.0f;     // 0.0 - 2.0
        float mix_wetdry = 0.5f;     // 0.0 - 1.0
        float output_level = 1.0f;   // 0.0 - 2.0

        // Future: Add engine parameters, macro values, etc.
    };

    ABStateEngine() = default;
    ~ABStateEngine() = default;

    // Get current active bank (read-only)
    const ParamBank& getActiveBank() const {
        return activeIsB ? bankB : bankA;
    }

    // Get specific bank (read-only)
    const ParamBank& getBankA() const { return bankA; }
    const ParamBank& getBankB() const { return bankB; }

    // Check which bank is active
    bool isBankB() const { return activeIsB; }

    // Toggle between banks
    void toggleBank() {
        activeIsB = !activeIsB;
        DBG("A/B Bank switched to: " << (activeIsB ? "B" : "A"));
    }

    // Switch to specific bank
    void switchToBank(bool useB) {
        if (activeIsB != useB) {
            activeIsB = useB;
            DBG("A/B Bank switched to: " << (activeIsB ? "B" : "A"));
        }
    }

    // Set parameter in active bank
    void setParameter(const juce::String& paramID, float value) {
        ParamBank& bank = activeIsB ? bankB : bankA;

        if (paramID == "input_gain") {
            bank.input_gain = juce::jlimit(0.0f, 2.0f, value);
        }
        else if (paramID == "mix_wetdry") {
            bank.mix_wetdry = juce::jlimit(0.0f, 1.0f, value);
        }
        else if (paramID == "output_level") {
            bank.output_level = juce::jlimit(0.0f, 2.0f, value);
        }
    }

    // Get parameter from active bank
    float getParameter(const juce::String& paramID) const {
        const ParamBank& bank = activeIsB ? bankB : bankA;

        if (paramID == "input_gain") return bank.input_gain;
        if (paramID == "mix_wetdry") return bank.mix_wetdry;
        if (paramID == "output_level") return bank.output_level;

        return 0.0f;  // Unknown parameter
    }

    // Copy active bank to the other bank
    void copyActiveToOther() {
        if (activeIsB) {
            bankA = bankB;
            DBG("Copied Bank B to Bank A");
        } else {
            bankB = bankA;
            DBG("Copied Bank A to Bank B");
        }
    }

    // Load both banks from external data (for preset loading)
    void loadBanks(const ParamBank& newBankA, const ParamBank& newBankB) {
        bankA = newBankA;
        bankB = newBankB;
        DBG("Loaded new banks A and B");
    }

private:
    ParamBank bankA;
    ParamBank bankB;
    bool activeIsB = false;  // false = Bank A active, true = Bank B active

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ABStateEngine)
};
