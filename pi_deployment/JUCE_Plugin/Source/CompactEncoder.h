#pragma once
#include <JuceHeader.h>
#include "TrinityLookAndFeel.h"

/**
 * CompactEncoder - Ultra-compact rotary encoder for Trinity UI
 *
 * Visual Size: 16×16px (tiny encoder knob)
 * Touch Target: 44×44px (expanded hit area for touch accuracy)
 * Style: Dark gray ring with cyan position marker
 *
 * Features:
 * - Vertical drag interaction (0-127 MIDI range)
 * - Optional label display (6px font)
 * - Visual feedback via LED-style position marker
 * - Reuses TrinityLookAndFeel's cached encoder ring
 */
class CompactEncoder : public juce::Component
{
public:
    CompactEncoder(const juce::String& labelText = "");
    ~CompactEncoder() override = default;

    // Value management
    void setValue(float newValue);  // 0.0-1.0 normalized
    float getValue() const { return value; }

    // Attachment support for APVTS
    void setSliderStyle(juce::Slider::SliderStyle style) { slider.setSliderStyle(style); }
    juce::Slider& getSlider() { return slider; }

    // Layout
    void resized() override;
    void paint(juce::Graphics& g) override;

    // Touch target expansion (44×44px hit area)
    bool hitTest(int x, int y) override;

private:
    juce::Slider slider;
    juce::String label;
    float value = 0.0f;

    // Visual constants
    static constexpr int VISUAL_SIZE = 16;      // 16×16px visual encoder
    static constexpr int TOUCH_SIZE = 44;       // 44×44px touch target
    static constexpr int LABEL_HEIGHT = 10;     // Space for 6px label

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactEncoder)
};
