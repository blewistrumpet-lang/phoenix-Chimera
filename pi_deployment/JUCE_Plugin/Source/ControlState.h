#pragma once

#include "JuceHeader.h"
#include "EventBus.h"

/**
 * Control State Management for Trinity GPIO
 * Manages modes, variants, and encoder mappings
 */
class ControlState
{
public:
    // Control modes (from MODE switch)
    enum class Mode {
        PRESET,     // Browse/load/save presets
        MIX,        // Macro controls (warmth/size/punch)
        AI          // AI generation/refinement
    };

    // Variant states (from VARIANT switch - SW2)
    enum class Variant {
        A,          // Bank A active
        MORPH,      // Morphing between A and B (future)
        B           // Bank B active
    };

    // Bypass states (from BYPASS switch - SW3)
    enum class Bypass {
        TRUE_BYPASS,  // Dry signal only
        PROCESS,      // Normal processing
        KILL_DRY      // 100% wet signal
    };

    // Current system state
    struct State {
        Mode mode = Mode::PRESET;
        Variant variant = Variant::A;
        Bypass bypass = Bypass::PROCESS;  // SW3 bypass state

        // Encoder assignments for current mode
        juce::String encoder1Label = "Browse";
        juce::String encoder2Label = "Mix";
        juce::String encoder3Label = "Output";

        // Get mode as string
        juce::String getModeString() const {
            switch (mode) {
                case Mode::PRESET: return "PRESET";
                case Mode::MIX:    return "MIX";
                case Mode::AI:     return "AI";
                default:           return "UNKNOWN";
            }
        }

        // Get variant as string
        juce::String getVariantString() const {
            switch (variant) {
                case Variant::A:     return "A";
                case Variant::MORPH: return "MORPH";
                case Variant::B:     return "B";
                default:             return "UNKNOWN";
            }
        }

        // Get bypass as string
        juce::String getBypassString() const {
            switch (bypass) {
                case Bypass::TRUE_BYPASS: return "BYPASS";
                case Bypass::PROCESS:     return "PROCESS";
                case Bypass::KILL_DRY:    return "WET ONLY";
                default:                  return "UNKNOWN";
            }
        }
    };

    ControlState() = default;
    ~ControlState() = default;

    // Get current state
    const State& getState() const { return state; }

    // Mode control
    void setMode(Mode newMode) {
        if (state.mode != newMode) {
            state.mode = newMode;
            updateEncoderLabels();
            DBG("Mode changed to: " << state.getModeString());
        }
    }

    // Variant control
    void setVariant(Variant newVariant) {
        if (state.variant != newVariant) {
            state.variant = newVariant;
            DBG("Variant changed to: " << state.getVariantString());
        }
    }

    // Bypass control (SW3)
    void setBypass(Bypass newBypass) {
        if (state.bypass != newBypass) {
            state.bypass = newBypass;
            DBG("Bypass changed to: " << state.getBypassString());
        }
    }

    // Get encoder behavior for current mode
    struct EncoderBehavior {
        juce::String parameterID;  // Which parameter to control
        float sensitivity;         // How fast to change
        bool needsPickup;          // Whether to wait for pickup after mode change
    };

    EncoderBehavior getEncoderBehavior(int encoderIndex) const {
        EncoderBehavior behavior;
        behavior.sensitivity = 0.005f;  // Default sensitivity (reduced for smoother control)
        behavior.needsPickup = false;   // Disabled for testing - enable later with proper pickup logic

        switch (state.mode) {
            case Mode::PRESET:
                switch (encoderIndex) {
                    case 0:  // Preset browsing (0-9)
                        behavior.parameterID = "preset_index";
                        behavior.sensitivity = 1.0f;  // Integer steps
                        behavior.needsPickup = false;  // Direct preset selection
                        break;
                    case 1:  // Mix control
                        behavior.parameterID = "mix_wetdry";
                        behavior.sensitivity = 0.005f;
                        break;
                    case 2:  // Output level
                        behavior.parameterID = "output_level";
                        behavior.sensitivity = 0.01f;
                        break;
                }
                break;

            case Mode::MIX:
                switch (encoderIndex) {
                    case 0:  // WARMTH macro (dark ← → bright)
                        behavior.parameterID = "macro_warmth";
                        behavior.sensitivity = 0.01f;
                        behavior.needsPickup = true;  // Enable pickup for mode switch
                        break;
                    case 1:  // SIZE macro (tight ← → spacious)
                        behavior.parameterID = "macro_size";
                        behavior.sensitivity = 0.01f;
                        behavior.needsPickup = true;
                        break;
                    case 2:  // PUNCH macro (soft ← → aggressive)
                        behavior.parameterID = "macro_punch";
                        behavior.sensitivity = 0.01f;
                        behavior.needsPickup = true;
                        break;
                }
                break;

            case Mode::AI:
                switch (encoderIndex) {
                    case 0:  // Complexity
                        behavior.parameterID = "ai_complexity";
                        behavior.sensitivity = 0.01f;
                        break;
                    case 1:  // Refine
                        behavior.parameterID = "ai_refine";
                        behavior.sensitivity = 0.01f;
                        break;
                    case 2:  // Evolve
                        behavior.parameterID = "ai_evolve";
                        behavior.sensitivity = 0.01f;
                        break;
                }
                break;
        }

        return behavior;
    }

private:
    State state;

    void updateEncoderLabels() {
        switch (state.mode) {
            case Mode::PRESET:
                state.encoder1Label = "Browse";
                state.encoder2Label = "Mix";
                state.encoder3Label = "Output";
                break;

            case Mode::MIX:
                state.encoder1Label = "Warmth";
                state.encoder2Label = "Size";
                state.encoder3Label = "Punch";
                break;

            case Mode::AI:
                state.encoder1Label = "Complexity";
                state.encoder2Label = "Refine";
                state.encoder3Label = "Evolve";
                break;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlState)
};