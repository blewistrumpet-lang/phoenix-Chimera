#pragma once

#include <JuceHeader.h>

/**
 * Visual display component for a rotary encoder
 * Shows parameter name, formatted value, bank indicator, and button state
 */
class EncoderDisplay : public juce::Component
{
public:
    EncoderDisplay(int encoderNumber)
        : number(encoderNumber)
    {
        setSize(100, 80);
    }

    void setPosition(int pos)
    {
        if (position != pos) {
            position = pos;
            repaint();
        }
    }

    void setButtonPressed(bool pressed)
    {
        if (buttonPressed != pressed) {
            buttonPressed = pressed;
            repaint();
        }
    }

    /**
     * Set parameter information for display
     * @param name Parameter name (e.g., "Input", "Mix", "Output")
     * @param value Normalized parameter value (0.0 to 1.0)
     * @param paramID Parameter identifier for formatting
     * @param bank Bank indicator ("A" or "B")
     */
    void setParameterInfo(const juce::String& name, float value, const juce::String& paramID, const juce::String& bank)
    {
        if (paramName != name || paramValue != value || parameterID != paramID || bankIndicator != bank) {
            paramName = name;
            paramValue = value;
            parameterID = paramID;
            bankIndicator = bank;
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

        // Border - change color based on bank
        juce::Colour borderColour = bankIndicator == "B"
            ? juce::Colour(0xffffaa00)  // Orange for Bank B
            : juce::Colour(0xff00b6d4);  // Cyan for Bank A
        g.setColour(borderColour);
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 8.0f, 2.0f);

        // Bank indicator (top-left corner, subtle)
        g.setColour(borderColour.withAlpha(0.6f));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText(bankIndicator, bounds.removeFromTop(16).reduced(4, 2),
                   juce::Justification::topLeft);

        // Parameter name
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(paramName,
                   bounds.removeFromTop(18),
                   juce::Justification::centred);

        // Formatted parameter value
        g.setFont(juce::Font(16.0f, juce::Font::bold));
        g.setColour(borderColour);
        juce::String formattedValue = formatParameterValue(paramValue, parameterID);
        g.drawText(formattedValue,
                   bounds.removeFromTop(26),
                   juce::Justification::centred);

        // Button indicator
        auto buttonArea = bounds.removeFromBottom(16).reduced(25, 4);
        if (buttonPressed) {
            g.setColour(juce::Colour(0xffef4444)); // Red when pressed
            g.fillEllipse(buttonArea.toFloat());
        } else {
            g.setColour(juce::Colour(0xff444444));
            g.fillEllipse(buttonArea.toFloat());
        }
    }

private:
    int number;
    int position = 0;
    bool buttonPressed = false;
    juce::String paramName;
    float paramValue = 0.0f;
    juce::String parameterID;
    juce::String bankIndicator = "A";

    /**
     * Format parameter value based on parameter type
     * @param normalizedValue Normalized value (0.0 to 1.0)
     * @param paramID Parameter identifier
     * @return Formatted string with appropriate units
     */
    juce::String formatParameterValue(float normalizedValue, const juce::String& paramID)
    {
        if (paramID == "input_gain" || paramID == "output_level") {
            // Input/Output gain: 0.0-2.0 range, display as dB
            // 0.0 -> 1.0 normalized maps to 0.0 -> 2.0 actual
            float actualValue = normalizedValue * 2.0f;

            // Convert to dB: 20 * log10(value)
            // Handle special cases
            if (actualValue <= 0.001f) {
                return "-inf dB";
            }

            float dB = 20.0f * std::log10(actualValue);

            // Format with sign
            if (dB >= 0.05f) {
                return "+" + juce::String(dB, 1) + " dB";
            } else if (dB <= -0.05f) {
                return juce::String(dB, 1) + " dB";
            } else {
                return "0.0 dB";
            }
        }
        else if (paramID == "mix_wetdry") {
            // Mix: 0.0-1.0 range, display as percentage
            int percentage = static_cast<int>(normalizedValue * 100.0f);
            return juce::String(percentage) + "%";
        }
        else {
            // Default: show as percentage
            int percentage = static_cast<int>(normalizedValue * 100.0f);
            return juce::String(percentage) + "%";
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EncoderDisplay)
};

/**
 * Visual display component for a three-way switch
 * Shows current position (UP/MIDDLE/DOWN)
 */
class SwitchDisplay : public juce::Component
{
public:
    enum class Position {
        UP,
        MIDDLE,
        DOWN
    };

    SwitchDisplay(int switchNumber)
        : number(switchNumber)
    {
        setSize(80, 60);
    }

    void setPosition(Position pos)
    {
        if (position != pos) {
            position = pos;
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

        // Border
        g.setColour(juce::Colour(0xff00b6d4));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 6.0f, 2.0f);

        // Title
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("SW " + juce::String(number + 1),
                   bounds.removeFromTop(18),
                   juce::Justification::centred);

        // Position text
        juce::String posText;
        juce::Colour posColour;

        switch (position) {
            case Position::UP:
                posText = "UP";
                posColour = juce::Colour(0xff10b981); // Green
                break;
            case Position::MIDDLE:
                posText = "MID";
                posColour = juce::Colour(0xfffbbf24); // Yellow
                break;
            case Position::DOWN:
                posText = "DOWN";
                posColour = juce::Colour(0xffef4444); // Red
                break;
        }

        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.setColour(posColour);
        g.drawText(posText, bounds, juce::Justification::centred);
    }

private:
    int number;
    Position position = Position::MIDDLE;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SwitchDisplay)
};
