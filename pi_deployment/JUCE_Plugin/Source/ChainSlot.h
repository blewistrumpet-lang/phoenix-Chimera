#pragma once
#include <JuceHeader.h>
#include "TrinityLookAndFeel.h"

/**
 * ChainSlot - Visual signal chain slot for Trinity UI
 *
 * Size: 70×28px (compact engine card)
 *
 * States:
 * - INACTIVE: Empty slot (dark gray, "EMPTY" text)
 * - PREMIUM: Premium engine (cyan border, white text)
 * - HYBRID: Hybrid engine (gold border, white text)
 * - EXPERIMENTAL: Experimental engine (purple border, gray text)
 *
 * Visual Elements:
 * - Border: 1px, color-coded by state
 * - Background: Pure black (TrinityColors::cardDark)
 * - Text: Engine name (8px font, truncated if needed)
 * - Activity Indicator: Small dot showing processing activity
 *
 * Usage:
 * - Display 6 slots horizontally to show signal chain
 * - Click to select/configure engine
 * - Visual feedback for active processing
 */
class ChainSlot : public juce::Component
{
public:
    enum class SlotState {
        Inactive,
        Premium,
        Hybrid,
        Experimental
    };

    ChainSlot(int slotNumber = 0);
    ~ChainSlot() override = default;

    // State management
    void setState(SlotState newState, const juce::String& engineName = "");
    SlotState getState() const { return currentState; }

    // Activity visualization
    void setActivity(float level);  // 0.0 = silent, 1.0 = full activity
    float getActivity() const { return activityLevel; }

    // Click callback
    std::function<void(int)> onSlotClicked;

    // Visual
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    SlotState currentState = SlotState::Inactive;
    juce::String engineName = "EMPTY";
    int slotIndex;
    float activityLevel = 0.0f;

    // Visual constants
    static constexpr int WIDTH = 70;
    static constexpr int HEIGHT = 28;
    static constexpr int BORDER_THICKNESS = 1;
    static constexpr int ACTIVITY_DOT_SIZE = 4;

    // Get border color based on state
    juce::Colour getBorderColor() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChainSlot)
};
