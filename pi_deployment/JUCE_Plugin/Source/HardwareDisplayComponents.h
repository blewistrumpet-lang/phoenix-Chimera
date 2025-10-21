#pragma once

#include <JuceHeader.h>

/**
 * Visual display component for a rotary encoder
 * Shows position value and button state
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

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds();

        // Background
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

        // Border
        g.setColour(juce::Colour(0xff00b6d4));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1), 8.0f, 2.0f);

        // Title
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText("ENC " + juce::String(number + 1),
                   bounds.removeFromTop(20),
                   juce::Justification::centred);

        // Position value
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.setColour(juce::Colour(0xff00b6d4));
        g.drawText(juce::String(position),
                   bounds.removeFromTop(30),
                   juce::Justification::centred);

        // Button indicator
        auto buttonArea = bounds.removeFromBottom(20).reduced(25, 5);
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
