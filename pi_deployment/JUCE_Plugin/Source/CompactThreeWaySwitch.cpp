#include "CompactThreeWaySwitch.h"

CompactThreeWaySwitch::CompactThreeWaySwitch(const juce::String& labelText)
    : label(labelText)
{
    setSize(VISUAL_WIDTH, VISUAL_HEIGHT + (label.isNotEmpty() ? LABEL_HEIGHT : 0));
}

void CompactThreeWaySwitch::setPosition(Position newPosition)
{
    if (currentPosition == newPosition)
        return;

    currentPosition = newPosition;

    if (onPositionChanged)
        onPositionChanged(currentPosition);

    repaint();
}

void CompactThreeWaySwitch::setValue(int value)
{
    value = juce::jlimit(0, 2, value);
    setPosition(static_cast<Position>(value));
}

void CompactThreeWaySwitch::resized()
{
    // Component resizing handled in paint
}

void CompactThreeWaySwitch::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Reserve space for label if present
    auto labelBounds = juce::Rectangle<int>();
    if (label.isNotEmpty())
    {
        labelBounds = bounds.removeFromBottom(LABEL_HEIGHT);
    }

    // Center the 20×16px visual switch
    auto switchBounds = bounds.withSizeKeepingCentre(VISUAL_WIDTH, VISUAL_HEIGHT).toFloat();

    // Draw background track (dark gray vertical bar)
    g.setColour(TrinityColors::encoderRing);
    g.fillRoundedRectangle(switchBounds, 3.0f);

    // Draw switch positions (three dots)
    float dotRadius = 2.0f;
    float yOffset = SWITCH_HEIGHT_PER_POSITION;

    for (int i = 0; i < 3; ++i)
    {
        float y = switchBounds.getY() + yOffset + (i * SWITCH_HEIGHT_PER_POSITION);
        float x = switchBounds.getCentreX();

        // Highlight current position with cyan
        if (i == static_cast<int>(currentPosition))
            g.setColour(TrinityColors::accentCyan);
        else
            g.setColour(TrinityColors::textTertiary);

        g.fillEllipse(x - dotRadius, y - dotRadius, dotRadius * 2, dotRadius * 2);
    }

    // Draw active position indicator (larger dot)
    float activeY = switchBounds.getY() + yOffset + (static_cast<int>(currentPosition) * SWITCH_HEIGHT_PER_POSITION);
    float activeX = switchBounds.getCentreX();

    g.setColour(TrinityColors::accentCyan);
    g.fillEllipse(activeX - 3.0f, activeY - 3.0f, 6.0f, 6.0f);

    // Draw label
    if (label.isNotEmpty())
    {
        g.setColour(TrinityColors::textSecondary);
        g.setFont(juce::Font(juce::FontOptions().withHeight(10.0f)).boldened());  // Increased from 6px to 10px for readability
        g.drawText(label, labelBounds, juce::Justification::centred);
    }
}

void CompactThreeWaySwitch::mouseDown(const juce::MouseEvent& e)
{
    // Cycle through positions: UP → CENTER → DOWN → UP
    int nextPosition = (static_cast<int>(currentPosition) + 1) % 3;
    setPosition(static_cast<Position>(nextPosition));
}

bool CompactThreeWaySwitch::hitTest(int x, int y)
{
    // Expand hit area to 44×44px touch target
    auto bounds = getLocalBounds();
    auto touchArea = bounds.withSizeKeepingCentre(TOUCH_SIZE, TOUCH_SIZE);

    return touchArea.contains(x, y);
}
