#include "TrinityLookAndFeel.h"

TrinityLookAndFeel::TrinityLookAndFeel()
{
    // Set default colors
    setColour(juce::ResizableWindow::backgroundColourId, TrinityColors::background);
    setColour(juce::TextButton::buttonColourId, TrinityColors::accentCyan);
    setColour(juce::TextButton::textColourOffId, TrinityColors::textPrimary);

    // Cache expensive gradients once at startup
    cacheGradients();
}

void TrinityLookAndFeel::cacheGradients()
{
    // ========================================================================
    // Voice Button Gradient (200×30px, purple→cyan)
    // ========================================================================
    cachedVoiceButtonGradient = juce::Image(juce::Image::ARGB, 200, 30, true);
    {
        juce::Graphics g(cachedVoiceButtonGradient);

        juce::ColourGradient gradient(
            TrinityColors::accentPurple, 0.0f, 0.0f,     // Top-left purple
            TrinityColors::accentCyan,   200.0f, 30.0f,  // Bottom-right cyan
            false
        );

        g.setGradientFill(gradient);
        g.fillRoundedRectangle(0.0f, 0.0f, 200.0f, 30.0f, 15.0f);
    }

    // ========================================================================
    // Encoder Ring (16×16px, dark gray circle)
    // ========================================================================
    cachedEncoderRing = juce::Image(juce::Image::ARGB, 16, 16, true);
    {
        juce::Graphics g(cachedEncoderRing);
        g.setColour(TrinityColors::encoderRing);
        g.fillEllipse(0.0f, 0.0f, 16.0f, 16.0f);
    }
}

void TrinityLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                          int x, int y, int width, int height,
                                          float sliderPosProportional,
                                          float rotaryStartAngle,
                                          float rotaryEndAngle,
                                          juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height);
    auto centre = bounds.getCentre();

    // Fixed 16×16px encoder ring size (matching HTML design)
    const float encoderSize = 16.0f;
    const float radius = encoderSize / 2.0f;

    // Center the encoder ring in the component
    float encoderX = centre.x - radius;
    float encoderY = centre.y - radius;

    // Draw outer ring (dark gray #222)
    g.setColour(TrinityColors::encoderRing);
    g.fillEllipse(encoderX, encoderY, encoderSize, encoderSize);

    // Calculate position marker angle
    auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    // Draw position marker (2×5px cyan bar from top)
    juce::Path marker;
    marker.addRectangle(-1.0f, -radius + 1.0f, 2.0f, 5.0f);
    marker.applyTransform(juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));

    g.setColour(TrinityColors::accentCyan);
    g.fillPath(marker);

    // Draw center dot (4×4px)
    g.setColour(TrinityColors::encoderCenter);
    g.fillEllipse(centre.x - 2.0f, centre.y - 2.0f, 4.0f, 4.0f);
}

void TrinityLookAndFeel::drawButtonBackground(juce::Graphics& g,
                                              juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();

    // Check if this is the voice button (200×30px)
    if (button.getWidth() == 200 && button.getHeight() == 30)
    {
        // Draw cached gradient
        g.drawImageAt(cachedVoiceButtonGradient, 0, 0);

        // Press state overlay
        if (shouldDrawButtonAsDown)
        {
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.fillRoundedRectangle(bounds, 15.0f);
        }
        // Hover state overlay
        else if (shouldDrawButtonAsHighlighted)
        {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRoundedRectangle(bounds, 15.0f);
        }
    }
    else
    {
        // Default button rendering
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(bounds, 5.0f);
    }
}

juce::Font TrinityLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    // Voice button uses 10px bold font
    if (buttonHeight == 30)
        return juce::Font(juce::FontOptions().withHeight(10.0f)).boldened();

    // Default
    return juce::Font(juce::FontOptions().withHeight(12.0f));
}
