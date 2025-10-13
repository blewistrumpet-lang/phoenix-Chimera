#include "ChainSlot.h"

ChainSlot::ChainSlot(int slotNumber)
    : slotIndex(slotNumber)
{
    setSize(WIDTH, HEIGHT);
}

void ChainSlot::setState(SlotState newState, const juce::String& newEngineName)
{
    if (currentState == newState && engineName == newEngineName)
        return;

    currentState = newState;
    engineName = newEngineName.isEmpty() ? "EMPTY" : newEngineName;

    repaint();
}

void ChainSlot::setActivity(float level)
{
    activityLevel = juce::jlimit(0.0f, 1.0f, level);
    repaint();
}

void ChainSlot::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Determine if active (matching HTML design)
    bool isActive = currentState != SlotState::Inactive;

    // Draw background with transparency based on state
    if (isActive) {
        // Active modules: colored background with transparency
        juce::Colour bgColor;
        switch (currentState) {
            case SlotState::Premium:
                bgColor = juce::Colour(0xffffffff).withAlpha(0.1f);
                break;
            case SlotState::Hybrid:
                bgColor = TrinityColors::accentGold.withAlpha(0.1f);
                break;
            case SlotState::Experimental:
                bgColor = juce::Colour(0xff8a2be2).withAlpha(0.1f); // BlueViolet
                break;
            default:
                bgColor = TrinityColors::cardDark;
        }
        g.setColour(bgColor);
        g.fillRoundedRectangle(bounds, 3.0f);
    } else {
        // Inactive modules: dark gray background
        g.setColour(juce::Colour(0x801e1e1e)); // rgba(30,30,30,0.5)
        g.fillRoundedRectangle(bounds, 3.0f);
    }

    // Draw border (color-coded by state)
    juce::Colour borderColor;
    if (isActive) {
        switch (currentState) {
            case SlotState::Premium:
                borderColor = juce::Colour(0xffffffff).withAlpha(0.3f);
                break;
            case SlotState::Hybrid:
                borderColor = TrinityColors::accentGold.withAlpha(0.3f);
                break;
            case SlotState::Experimental:
                borderColor = juce::Colour(0xff8a2be2).withAlpha(0.3f);
                break;
            default:
                borderColor = TrinityColors::encoderRing;
        }
    } else {
        borderColor = juce::Colour(0xff505050).withAlpha(0.3f);
    }
    g.setColour(borderColor);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

    // Draw type badge at top
    auto contentBounds = bounds.reduced(2.0f, 4.0f);
    auto typeBounds = contentBounds.removeFromTop(8);

    juce::String typeText;
    switch (currentState) {
        case SlotState::Premium:
            typeText = "PREM";
            break;
        case SlotState::Hybrid:
            typeText = "HYBR";
            break;
        case SlotState::Experimental:
            typeText = "EXPR";
            break;
        case SlotState::Inactive:
            typeText = "OFF";
            break;
    }

    g.setFont(juce::Font(juce::FontOptions().withHeight(5.0f)).boldened());
    g.setColour(isActive ? TrinityColors::textPrimary : juce::Colour(0xff444444));
    g.drawText(typeText, typeBounds, juce::Justification::centred);

    contentBounds.removeFromTop(1);

    // Draw engine name
    g.setFont(juce::Font(juce::FontOptions().withHeight(6.0f)));
    g.setColour(isActive ? TrinityColors::textPrimary : juce::Colour(0xff444444));
    g.drawText(engineName, contentBounds, juce::Justification::centred, true);
}

void ChainSlot::mouseDown(const juce::MouseEvent& e)
{
    if (onSlotClicked)
        onSlotClicked(slotIndex);
}

juce::Colour ChainSlot::getBorderColor() const
{
    switch (currentState)
    {
        case SlotState::Premium:
            return TrinityColors::accentCyan;
        case SlotState::Hybrid:
            return TrinityColors::accentGold;
        case SlotState::Experimental:
            return TrinityColors::accentPurple;
        case SlotState::Inactive:
        default:
            return TrinityColors::encoderRing;
    }
}
