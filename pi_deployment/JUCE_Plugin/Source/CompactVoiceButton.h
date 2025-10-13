#pragma once
#include <JuceHeader.h>
#include "TrinityLookAndFeel.h"

/**
 * CompactVoiceButton - Gradient voice control button for Trinity UI
 *
 * Size: 200×30px
 * Gradient: Purple→Cyan (pre-cached in TrinityLookAndFeel)
 *
 * Gesture Detection:
 * - Single Tap: Start/stop recording
 * - Hold (500ms): Hold for recording mode
 * - Double Tap (300ms window): Quick cancel/reset
 *
 * Visual States:
 * - IDLE: Gradient with white text "TAP TO SPEAK"
 * - RECORDING: Pulsing red overlay
 * - PROCESSING: Cyan overlay with spinner
 * - PRESSED: Dark overlay (20% black)
 * - HOVER: Light overlay (10% white)
 */
class CompactVoiceButton : public juce::TextButton
{
public:
    enum class GestureType {
        None,
        SingleTap,
        DoubleTap,
        Hold
    };

    enum class ButtonState {
        Idle,
        Recording,
        Processing
    };

    CompactVoiceButton();
    ~CompactVoiceButton() override = default;

    // State management
    void setState(ButtonState newState);
    ButtonState getState() const { return currentState; }

    // Gesture callbacks
    std::function<void(GestureType)> onGesture;

    // Visual updates
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    ButtonState currentState = ButtonState::Idle;

    // Gesture detection
    juce::Time lastTapTime;
    bool holdTimerActive = false;

    void handleSingleTap();
    void handleDoubleTap();
    void handleHold();

    // Visual constants
    static constexpr int HOLD_THRESHOLD_MS = 500;      // 500ms hold time
    static constexpr int DOUBLE_TAP_WINDOW_MS = 300;   // 300ms double-tap window

    // Timer for hold detection
    class HoldTimer : public juce::Timer
    {
    public:
        std::function<void()> onHold;
        void timerCallback() override { if (onHold) onHold(); }
    };
    HoldTimer holdTimer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactVoiceButton)
};
