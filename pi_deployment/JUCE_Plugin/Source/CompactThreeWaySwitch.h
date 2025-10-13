#pragma once
#include <JuceHeader.h>
#include "TrinityLookAndFeel.h"

/**
 * CompactThreeWaySwitch - Minimal 3-position switch for Trinity UI
 *
 * Visual Size: 20×16px (tiny vertical switch)
 * Touch Target: 44×44px (expanded hit area)
 *
 * Positions:
 * - UP (0): Top position
 * - CENTER (1): Middle position
 * - DOWN (2): Bottom position
 *
 * Usage:
 * - A/B Comparison Switch: UP=A, CENTER=OFF, DOWN=B
 * - Voice Mode Switch: UP=POLY, CENTER=MONO, DOWN=UNISON
 * - Engine Mode Switch: UP=SERIAL, CENTER=PARALLEL, DOWN=HYBRID
 *
 * Features:
 * - Click to cycle through positions
 * - Visual feedback with cyan accent color
 * - Optional label below switch
 */
class CompactThreeWaySwitch : public juce::Component
{
public:
    enum Position {
        UP = 0,
        CENTER = 1,
        DOWN = 2
    };

    CompactThreeWaySwitch(const juce::String& labelText = "");
    ~CompactThreeWaySwitch() override = default;

    // Position management
    void setPosition(Position newPosition);
    Position getPosition() const { return currentPosition; }

    // For parameter attachment
    void setValue(int value);  // 0, 1, or 2
    int getValue() const { return static_cast<int>(currentPosition); }

    // Callbacks
    std::function<void(Position)> onPositionChanged;

    // Visual
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Touch target expansion
    bool hitTest(int x, int y) override;

private:
    Position currentPosition = CENTER;
    juce::String label;

    // Visual constants
    static constexpr int VISUAL_WIDTH = 20;
    static constexpr int VISUAL_HEIGHT = 16;
    static constexpr int TOUCH_SIZE = 44;
    static constexpr int LABEL_HEIGHT = 10;
    static constexpr int SWITCH_HEIGHT_PER_POSITION = 5;  // 5px per position

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactThreeWaySwitch)
};
