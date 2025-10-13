#include "CompactEncoder.h"

CompactEncoder::CompactEncoder(const juce::String& labelText)
    : label(labelText)
{
    // Configure slider for rotary encoder behavior
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRange(0.0, 127.0, 1.0);  // MIDI range
    slider.setValue(0.0);
    slider.setPopupDisplayEnabled(false, false, this);

    // Track value changes
    slider.onValueChange = [this]() {
        value = static_cast<float>(slider.getValue()) / 127.0f;
    };

    addAndMakeVisible(slider);
}

void CompactEncoder::setValue(float newValue)
{
    value = juce::jlimit(0.0f, 1.0f, newValue);
    slider.setValue(value * 127.0, juce::dontSendNotification);
}

void CompactEncoder::resized()
{
    auto bounds = getLocalBounds();

    // Center the 16×16px visual encoder in the component
    auto encoderBounds = bounds.withSizeKeepingCentre(VISUAL_SIZE, VISUAL_SIZE);

    // But expand slider hit area to 44×44px for touch
    auto touchBounds = bounds.withSizeKeepingCentre(TOUCH_SIZE, TOUCH_SIZE);
    slider.setBounds(touchBounds);
}

void CompactEncoder::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Draw label above encoder (matching HTML design)
    if (label.isNotEmpty())
    {
        g.setColour(juce::Colour(0xff666666));
        g.setFont(juce::Font(juce::FontOptions().withHeight(7.0f)));

        auto labelBounds = bounds.removeFromTop(10);
        g.drawText(label, labelBounds, juce::Justification::centred);
    }

    // Skip encoder ring visual (drawn by slider/LookAndFeel)
    bounds.removeFromTop(16);
    bounds.removeFromTop(2);

    // Draw value below encoder
    g.setColour(TrinityColors::accentCyan);
    g.setFont(juce::Font(juce::FontOptions().withHeight(8.0f)));

    // Format value based on label
    juce::String valueText;
    float normalizedValue = value;

    if (label == "FILT") {
        // Filter frequency: 200Hz - 20kHz
        int freq = static_cast<int>(200 + normalizedValue * 19800);
        if (freq >= 1000) {
            valueText = juce::String(freq / 1000.0f, 1) + "k";
        } else {
            valueText = juce::String(freq) + "Hz";
        }
    } else if (label == "MIX") {
        // Mix percentage: 0-100%
        int percent = static_cast<int>(normalizedValue * 100);
        valueText = juce::String(percent) + "%";
    } else if (label == "PRST") {
        // Preset number: 001-500
        int preset = static_cast<int>(normalizedValue * 499) + 1;
        valueText = juce::String(preset).paddedLeft('0', 3);
    } else {
        // Generic 0-127
        valueText = juce::String(static_cast<int>(normalizedValue * 127));
    }

    g.drawText(valueText, bounds, juce::Justification::centred);
}

bool CompactEncoder::hitTest(int x, int y)
{
    // Expand hit area to 44×44px touch target
    auto bounds = getLocalBounds();
    auto touchArea = bounds.withSizeKeepingCentre(TOUCH_SIZE, TOUCH_SIZE);

    return touchArea.contains(x, y);
}
