#include "CompactVoiceButton.h"

CompactVoiceButton::CompactVoiceButton()
{
    // Set initial button text
    setButtonText("TAP TO SPEAK");

    // Configure appearance - TrinityLookAndFeel will handle gradient rendering
    setSize(200, 30);

    // Setup hold timer
    holdTimer.onHold = [this]() {
        handleHold();
        holdTimerActive = false;
    };
}

void CompactVoiceButton::setState(ButtonState newState)
{
    if (currentState == newState)
        return;

    currentState = newState;

    // Update button text based on state
    switch (currentState)
    {
        case ButtonState::Idle:
            setButtonText("TAP TO SPEAK");
            break;
        case ButtonState::Recording:
            setButtonText("RECORDING...");
            break;
        case ButtonState::Processing:
            setButtonText("PROCESSING...");
            break;
    }

    repaint();
}

void CompactVoiceButton::paint(juce::Graphics& g)
{
    // Let TrinityLookAndFeel draw the base gradient button
    juce::TextButton::paint(g);

    auto bounds = getLocalBounds().toFloat();

    // Add state-specific overlays
    switch (currentState)
    {
        case ButtonState::Recording:
        {
            // Pulsing red overlay (simple version - could add animation timer)
            g.setColour(juce::Colours::red.withAlpha(0.3f));
            g.fillRoundedRectangle(bounds, 15.0f);
            break;
        }
        case ButtonState::Processing:
        {
            // Cyan overlay for processing state
            g.setColour(TrinityColors::accentCyan.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds, 15.0f);
            break;
        }
        case ButtonState::Idle:
        default:
            // No additional overlay
            break;
    }
}

void CompactVoiceButton::mouseDown(const juce::MouseEvent& e)
{
    juce::TextButton::mouseDown(e);

    // Start hold timer
    holdTimerActive = true;
    holdTimer.startTimer(HOLD_THRESHOLD_MS);
}

void CompactVoiceButton::mouseUp(const juce::MouseEvent& e)
{
    juce::TextButton::mouseUp(e);

    // Cancel hold timer if still active
    if (holdTimerActive)
    {
        holdTimer.stopTimer();
        holdTimerActive = false;

        // Check for double-tap
        auto currentTime = juce::Time::getCurrentTime();
        auto timeSinceLastTap = (currentTime - lastTapTime).inMilliseconds();

        if (timeSinceLastTap < DOUBLE_TAP_WINDOW_MS && timeSinceLastTap > 0)
        {
            handleDoubleTap();
            lastTapTime = juce::Time();  // Reset to prevent triple-tap
        }
        else
        {
            handleSingleTap();
            lastTapTime = currentTime;
        }
    }
}

void CompactVoiceButton::handleSingleTap()
{
    if (onGesture)
        onGesture(GestureType::SingleTap);
}

void CompactVoiceButton::handleDoubleTap()
{
    if (onGesture)
        onGesture(GestureType::DoubleTap);
}

void CompactVoiceButton::handleHold()
{
    if (onGesture)
        onGesture(GestureType::Hold);
}
