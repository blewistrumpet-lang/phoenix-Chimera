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
        MIX,        // Macro controls (tone/space/energy)
        AI          // AI generation/refinement
    };

    // Variant states (from VARIANT switch)
    enum class Variant {
        A,          // Bank A active
        MORPH,      // Morphing between A and B (future)
        B           // Bank B active
    };

    // Current system state
    struct State {
        Mode mode = Mode::PRESET;
        Variant variant = Variant::A;
        bool liveActive = false;  // LIVE switch (future)

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

    // LIVE control (future)
    void setLiveActive(bool active) {
        state.liveActive = active;
        DBG("LIVE mode: " << (active ? "ON" : "OFF"));
    }

    // Get encoder behavior for current mode
    struct EncoderBehavior {
        juce::String parameterID;  // Which parameter to control
        float sensitivity;         // How fast to change
        bool needsPickup;          // Whether to wait for pickup after mode change
    };

    EncoderBehavior getEncoderBehavior(int encoderIndex) const {
        EncoderBehavior behavior;
        behavior.sensitivity = 0.01f;  // Default sensitivity
        behavior.needsPickup = true;   // Always require pickup for now

        switch (state.mode) {
            case Mode::PRESET:
                switch (encoderIndex) {
                    case 0:  // Input gain
                        behavior.parameterID = "input_gain";
                        behavior.sensitivity = 0.02f;
                        break;
                    case 1:  // Mix control
                        behavior.parameterID = "mix_wetdry";
                        behavior.sensitivity = 0.01f;
                        break;
                    case 2:  // Output level
                        behavior.parameterID = "output_level";
                        behavior.sensitivity = 0.02f;
                        break;
                }
                break;

            case Mode::MIX:
                switch (encoderIndex) {
                    case 0:  // Input gain (for now, until macros implemented)
                        behavior.parameterID = "input_gain";
                        behavior.sensitivity = 0.02f;
                        break;
                    case 1:  // Mix wetdry (for now, until macros implemented)
                        behavior.parameterID = "mix_wetdry";
                        behavior.sensitivity = 0.01f;
                        break;
                    case 2:  // Output level (for now, until macros implemented)
                        behavior.parameterID = "output_level";
                        behavior.sensitivity = 0.02f;
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
                state.encoder1Label = "Input";
                state.encoder2Label = "Mix";
                state.encoder3Label = "Output";
                break;

            case Mode::MIX:
                state.encoder1Label = "Input";
                state.encoder2Label = "Mix";
                state.encoder3Label = "Output";
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