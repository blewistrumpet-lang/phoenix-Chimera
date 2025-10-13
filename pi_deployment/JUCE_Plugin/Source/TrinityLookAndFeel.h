#pragma once
#include <JuceHeader.h>

/**
 * Trinity Color Palette - Modern dark theme optimized for OLED
 */
namespace TrinityColors {
    const juce::Colour background    = juce::Colour(0xff0a0a0a);  // Near-black
    const juce::Colour cardDark      = juce::Colour(0xff000000);  // Pure black cards
    const juce::Colour encoderRing   = juce::Colour(0xff222222);  // Dark gray encoder ring
    const juce::Colour encoderCenter = juce::Colour(0xff1a1a1a);  // Center dot
    const juce::Colour accentCyan    = juce::Colour(0xff00ffcc);  // Electric cyan
    const juce::Colour accentPurple  = juce::Colour(0xff7b68ee);  // Medium purple
    const juce::Colour accentGold    = juce::Colour(0xffffd700);  // Gold for hybrid engines
    const juce::Colour textPrimary   = juce::Colour(0xffffffff);  // White
    const juce::Colour textSecondary = juce::Colour(0xff888888);  // Gray
    const juce::Colour textTertiary  = juce::Colour(0xff666666);  // Darker gray
}

/**
 * Trinity Compact LookAndFeel - Custom styling for 480×320 Pi display
 * Features:
 * - Pre-cached gradients for performance
 * - Compact component rendering
 * - Touch-optimized hit areas
 */
class TrinityLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TrinityLookAndFeel();
    ~TrinityLookAndFeel() override = default;

    // Encoder rendering
    void drawRotarySlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPosProportional,
                         float rotaryStartAngle,
                         float rotaryEndAngle,
                         juce::Slider& slider) override;

    // Button rendering
    void drawButtonBackground(juce::Graphics& g,
                            juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

    // Cached gradient accessors
    const juce::Image& getVoiceButtonGradient() const { return cachedVoiceButtonGradient; }
    const juce::Image& getEncoderRing() const { return cachedEncoderRing; }

private:
    void cacheGradients();

    // Pre-rendered gradients for performance
    juce::Image cachedVoiceButtonGradient;  // 200×30px purple→cyan
    juce::Image cachedEncoderRing;          // 16×16px dark gray circle

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrinityLookAndFeel)
};
