#include "NexusLookAndFeel.h"

NexusLookAndFeel::NexusLookAndFeel()
{
    // Set default colors for the Tactile Futurism aesthetic
    setColour(juce::Slider::textBoxTextColourId, cyanGlow);
    setColour(juce::Slider::textBoxBackgroundColourId, darkGrey.withAlpha(0.8f));
    setColour(juce::Slider::textBoxOutlineColourId, midGrey);
    
    setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    setColour(juce::ComboBox::backgroundColourId, darkGrey);
    setColour(juce::ComboBox::textColourId, cyanGlow);
    setColour(juce::ComboBox::outlineColourId, midGrey);
    setColour(juce::ComboBox::arrowColourId, cyanGlow);
}

NexusLookAndFeel::~NexusLookAndFeel() {}

void NexusLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                        juce::Slider& slider)
{
    const float radius = juce::jmin(width / 2, height / 2) - 2.0f;
    const float centreX = x + width * 0.5f;
    const float centreY = y + height * 0.5f;
    const float rx = centreX - radius;
    const float ry = centreY - radius;
    const float rw = radius * 2.0f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Modern flat background - no gradients
    g.setColour(juce::Colour(0xff0a0b0d));  // Near black
    g.fillEllipse(rx, ry, rw, rw);
    
    // Single subtle ring
    g.setColour(juce::Colour(0xff1e2028));  // Subtle border
    g.drawEllipse(rx, ry, rw, rw, 1.0f);
    
    // Position indicator - thin modern line
    juce::Path pointer;
    const float pointerLength = radius * 0.75f;
    const float pointerThickness = 1.5f;
    pointer.addRectangle(-pointerThickness * 0.5f, -pointerLength, pointerThickness, pointerLength * 0.6f);
    
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    
    // Single clean line - no glow
    g.setColour(juce::Colour(0xff00d4ff));  // Electric blue
    g.fillPath(pointer);
    
    g.restoreState();
    
    // Minimal center dot
    g.setColour(juce::Colour(0xff00d4ff));
    g.fillEllipse(centreX - 2, centreY - 2, 4, 4);
}

void NexusLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1);
    
    // Determine button colors based on text
    juce::Colour activeColour = juce::Colour(0xff00d4ff);  // Default blue
    
    if (button.getButtonText() == "B") {  // Bypass
        activeColour = juce::Colour(0xffff006e);  // Magenta
    } else if (button.getButtonText() == "S") {  // Solo
        activeColour = juce::Colour(0xffffcc00);  // Yellow
    }
    
    // Minimal flat design
    if (button.getToggleState()) {
        // Active - filled with color
        g.setColour(activeColour);
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(juce::Colours::black);
    } else {
        // Inactive - just border
        g.setColour(juce::Colour(0xff1e2028));
        g.fillRoundedRectangle(bounds, 2.0f);
        g.setColour(juce::Colour(0xff2a2d38));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
        g.setColour(activeColour.withAlpha(0.5f));
    }
    
    // Draw text - tiny
    g.setFont(10.0f);
    g.drawText(button.getButtonText(), bounds, juce::Justification::centred);
}

void NexusLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2);
    
    // Industrial button style
    juce::Colour buttonColour = shouldDrawButtonAsDown ? cyanGlow : midGrey;
    
    if (shouldDrawButtonAsHighlighted) {
        buttonColour = buttonColour.brighter(0.2f);
    }
    
    // Metallic gradient
    juce::ColourGradient gradient(buttonColour.brighter(0.3f), bounds.getTopLeft(),
                                  buttonColour.darker(0.3f), bounds.getBottomRight(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(shouldDrawButtonAsDown ? cyanGlow : lightGrey);
    g.drawRoundedRectangle(bounds, 4.0f, 1.5f);
}

void NexusLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                    int buttonX, int buttonY, int buttonW, int buttonH,
                                    juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    
    // Industrial dropdown style
    juce::ColourGradient gradient(midGrey, bounds.getTopLeft(),
                                  darkGrey, bounds.getBottomRight(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Border with glow when active
    g.setColour(isButtonDown ? cyanGlow : lightGrey);
    g.drawRoundedRectangle(bounds, 3.0f, 1.5f);
    
    // Arrow indicator
    juce::Path arrow;
    arrow.addTriangle(buttonX + buttonW * 0.3f, buttonY + buttonH * 0.4f,
                     buttonX + buttonW * 0.7f, buttonY + buttonH * 0.4f,
                     buttonX + buttonW * 0.5f, buttonY + buttonH * 0.6f);
    
    g.setColour(cyanGlow.withAlpha(0.8f));
    g.fillPath(arrow);
}

void NexusLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited()) {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        
        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
        g.setFont(label.getFont());
        
        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
        g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                        juce::jmax(1, (int)(textArea.getHeight() / label.getFont().getHeight())),
                        label.getMinimumHorizontalScale());
    }
}

void NexusLookAndFeel::drawCarbonFiberBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Carbon fiber texture simulation
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(bounds);
    
    // Create weave pattern
    const float weaveSize = 4.0f;
    g.setColour(juce::Colour(0xff141414));
    
    for (float y = bounds.getY(); y < bounds.getBottom(); y += weaveSize * 2) {
        for (float x = bounds.getX(); x < bounds.getRight(); x += weaveSize * 2) {
            g.fillRect(x, y, weaveSize, weaveSize);
            g.fillRect(x + weaveSize, y + weaveSize, weaveSize, weaveSize);
        }
    }
    
    // Subtle gradient overlay
    juce::ColourGradient overlay(juce::Colour(0x10ffffff), bounds.getTopLeft(),
                                 juce::Colour(0x00ffffff), bounds.getBottomRight(), false);
    g.setGradientFill(overlay);
    g.fillRect(bounds);
}

void NexusLookAndFeel::draw3DBeveledModule(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Main module background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Inner bevel - darker inset
    auto insetBounds = bounds.reduced(2);
    juce::ColourGradient bevelGradient(juce::Colour(0xff0a0a0a), insetBounds.getTopLeft(),
                                       juce::Colour(0xff2a2a2a), insetBounds.getBottomRight(), false);
    g.setGradientFill(bevelGradient);
    g.fillRoundedRectangle(insetBounds, 5.0f);
    
    // Highlight on top edge
    g.setColour(juce::Colour(0xff3a3a3a).withAlpha(0.5f));
    g.drawLine(bounds.getX() + 6, bounds.getY() + 2,
              bounds.getRight() - 6, bounds.getY() + 2, 1.0f);
    
    // Shadow on bottom edge
    g.setColour(juce::Colour(0xff000000).withAlpha(0.5f));
    g.drawLine(bounds.getX() + 6, bounds.getBottom() - 2,
              bounds.getRight() - 6, bounds.getBottom() - 2, 1.0f);
    
    // Outer frame
    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);
}

void NexusLookAndFeel::drawIndustrialEncoder(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             float angle, bool isSteppedControl)
{
    // Additional industrial details for stepped controls
    if (isSteppedControl) {
        // Draw notches around the edge
        const float centreX = bounds.getCentreX();
        const float centreY = bounds.getCentreY();
        const float radius = bounds.getWidth() / 2.0f;
        
        g.setColour(yellowWarning.withAlpha(0.5f));
        
        for (int i = 0; i < 12; ++i) {
            float notchAngle = i * (juce::MathConstants<float>::twoPi / 12.0f);
            float x1 = centreX + (radius - 4) * std::cos(notchAngle);
            float y1 = centreY + (radius - 4) * std::sin(notchAngle);
            float x2 = centreX + radius * std::cos(notchAngle);
            float y2 = centreY + radius * std::sin(notchAngle);
            
            g.drawLine(x1, y1, x2, y2, 1.0f);
        }
    }
}