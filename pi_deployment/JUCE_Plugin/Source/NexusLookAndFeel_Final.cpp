#include "NexusLookAndFeel_Final.h"
#include <cmath>

NexusLookAndFeel_Final::NexusLookAndFeel_Final()
{
    // Set all default colors to match our palette
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(Colors::baseBlack));
    setColour(juce::Label::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(Colors::baseDark));
    setColour(juce::TextEditor::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextEditor::highlightColourId, juce::Colour(Colors::primaryCyan).withAlpha(0.3f));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(Colors::baseDark));
    setColour(juce::ComboBox::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::ComboBox::arrowColourId, juce::Colour(Colors::primaryCyan));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(Colors::baseDark));
    setColour(juce::PopupMenu::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(Colors::primaryCyan).withAlpha(0.2f));
    setColour(juce::TextButton::buttonColourId, juce::Colour(Colors::baseDark));
    setColour(juce::TextButton::textColourOffId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextButton::textColourOnId, juce::Colour(Colors::primaryCyan));
    setColour(juce::ToggleButton::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::ToggleButton::tickColourId, juce::Colour(Colors::primaryCyan));
    
    // Use a modern tactical font
    // Default font will be used
}

NexusLookAndFeel_Final::~NexusLookAndFeel_Final() = default;

void NexusLookAndFeel_Final::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                              juce::Slider& slider)
{
    // Industrial rotary encoder with machined grip texture
    auto bounds = juce::Rectangle<float>(x, y, width, height);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 6.0f;
    auto center = bounds.getCentre();
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Draw subtle carbon fiber background
    drawCarbonFiberTexture(g, bounds.reduced(radius * 0.4f));
    
    // Outer ring - machined metal appearance
    g.setColour(juce::Colour(Colors::baseDark));
    g.fillEllipse(center.x - radius - 2, center.y - radius - 2, 
                  (radius + 2) * 2, (radius + 2) * 2);
    
    // Track groove
    juce::Path trackPath;
    trackPath.addCentredArc(center.x, center.y, radius, radius, 0,
                           rotaryStartAngle, rotaryEndAngle, true);
    
    g.setColour(juce::Colour(0x20ffffff));
    g.strokePath(trackPath, juce::PathStrokeType(4.0f));
    
    // Active track with neon glow
    if (sliderPos > 0.01f)
    {
        juce::Path activePath;
        activePath.addCentredArc(center.x, center.y, radius, radius, 0,
                                rotaryStartAngle, angle, true);
        
        // Glow effect
        g.setColour(juce::Colour(Colors::primaryCyan).withAlpha(0.2f));
        g.strokePath(activePath, juce::PathStrokeType(6.0f));
        
        g.setColour(juce::Colour(Colors::primaryCyan));
        g.strokePath(activePath, juce::PathStrokeType(2.0f));
    }
    
    // Center knob body
    auto knobRadius = radius * 0.65f;
    
    // Knob shadow
    g.setColour(juce::Colour(Colors::shadowDeep));
    g.fillEllipse(center.x - knobRadius + 1, center.y - knobRadius + 2,
                  knobRadius * 2, knobRadius * 2);
    
    // Knob body with gradient
    juce::ColourGradient knobGrad(
        juce::Colour(0xff2a2a35), center.x - knobRadius, center.y - knobRadius,
        juce::Colour(Colors::baseBlack), center.x + knobRadius, center.y + knobRadius,
        true
    );
    g.setGradientFill(knobGrad);
    g.fillEllipse(center.x - knobRadius, center.y - knobRadius,
                  knobRadius * 2, knobRadius * 2);
    
    // Machined grip texture
    drawKnobGrip(g, center, knobRadius);
    
    // Position indicator - single neon cyan line
    juce::Line<float> indicator(
        center.getPointOnCircumference(knobRadius * 0.3f, angle),
        center.getPointOnCircumference(knobRadius * 0.9f, angle)
    );
    
    g.setColour(juce::Colour(Colors::primaryCyan));
    g.drawLine(indicator, 3.0f);
    
    // Center dot
    g.fillEllipse(center.x - 2, center.y - 2, 4, 4);
}

void NexusLookAndFeel_Final::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto isOn = button.getToggleState();
    
    // Industrial toggle switch
    auto switchWidth = 44.0f;
    auto switchHeight = 24.0f;
    auto switchBounds = juce::Rectangle<float>(5, bounds.getCentreY() - switchHeight/2, 
                                              switchWidth, switchHeight);
    
    // Track background
    g.setColour(juce::Colour(Colors::baseDark));
    g.fillRoundedRectangle(switchBounds, 12.0f);
    
    // Track inner groove
    g.setColour(juce::Colour(0x10000000));
    g.fillRoundedRectangle(switchBounds.reduced(2), 10.0f);
    
    // Switch handle
    auto handleSize = 18.0f;
    auto handleX = isOn ? switchBounds.getRight() - handleSize - 3 : switchBounds.getX() + 3;
    auto handleY = switchBounds.getCentreY() - handleSize/2;
    
    // Handle shadow
    g.setColour(juce::Colour(Colors::shadowDeep));
    g.fillEllipse(handleX + 1, handleY + 1, handleSize, handleSize);
    
    // Handle body
    g.setColour(isOn ? juce::Colour(Colors::primaryCyan) : juce::Colour(Colors::textSecondary));
    g.fillEllipse(handleX, handleY, handleSize, handleSize);
    
    // Glow when on
    if (isOn)
    {
        drawNeonGlow(g, juce::Rectangle<float>(handleX, handleY, handleSize, handleSize),
                    juce::Colour(Colors::primaryCyan), 0.5f);
    }
    
    // Label text
    g.setColour(juce::Colour(Colors::textPrimary));
    g.setFont(getTacticalFont(13.0f));
    
    auto textBounds = bounds.toFloat();
    textBounds.removeFromLeft(switchWidth + 10);
    g.drawText(button.getButtonText(), textBounds, juce::Justification::centredLeft);
}

void NexusLookAndFeel_Final::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto isHighlighted = shouldDrawButtonAsHighlighted;
    auto isDown = shouldDrawButtonAsDown;
    
    // Base panel
    g.setColour(juce::Colour(Colors::baseDark));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Inner gradient
    if (isDown)
    {
        g.setColour(juce::Colour(Colors::primaryCyan).withAlpha(0.2f));
        g.fillRoundedRectangle(bounds.reduced(1), 3.0f);
    }
    
    // Border glow
    if (isHighlighted || isDown)
    {
        g.setColour(juce::Colour(Colors::primaryCyan).withAlpha(isDown ? 0.8f : 0.4f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.5f);
    }
    else
    {
        g.setColour(juce::Colour(0x30ffffff));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    }
}

void NexusLookAndFeel_Final::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                          int buttonX, int buttonY, int buttonW, int buttonH,
                                          juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    
    // Background
    g.setColour(juce::Colour(Colors::baseDark));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(box.hasKeyboardFocus(true) ? 
                juce::Colour(Colors::primaryCyan).withAlpha(0.6f) : 
                juce::Colour(0x30ffffff));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    
    // Arrow
    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float>(buttonX + buttonW * 0.3f, buttonY + buttonH * 0.4f,
                                             buttonW * 0.4f, buttonH * 0.3f);
    arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                     arrowBounds.getRight(), arrowBounds.getY(),
                     arrowBounds.getCentreX(), arrowBounds.getBottom());
    
    g.setColour(juce::Colour(Colors::primaryCyan));
    g.fillPath(arrow);
}

void NexusLookAndFeel_Final::fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                                      juce::TextEditor& textEditor)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    
    // Dark terminal-style background
    g.setColour(juce::Colour(Colors::baseBlack));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border glow when focused
    if (textEditor.hasKeyboardFocus(true))
    {
        g.setColour(juce::Colour(Colors::primaryCyan).withAlpha(0.4f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 2.0f);
    }
    else
    {
        g.setColour(juce::Colour(0x20ffffff));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
    }
}

void NexusLookAndFeel_Final::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.setColour(label.findColour(juce::Label::textColourId));
    g.setFont(getTacticalFont(label.getFont().getHeight()));
    
    auto bounds = label.getLocalBounds();
    g.drawText(label.getText(), bounds, label.getJustificationType(), true);
}

void NexusLookAndFeel_Final::drawCarbonFiberTexture(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Carbon fiber weave pattern
    g.saveState();
    g.reduceClipRegion(bounds.toNearestInt());
    
    g.setColour(juce::Colour(0x08ffffff));
    
    float weaveSize = 3.0f;
    for (float x = bounds.getX(); x < bounds.getRight(); x += weaveSize * 2)
    {
        for (float y = bounds.getY(); y < bounds.getBottom(); y += weaveSize)
        {
            float offset = (int(y / weaveSize) % 2) * weaveSize;
            g.fillRect(juce::Rectangle<float>(x + offset, y, weaveSize, weaveSize));
        }
    }
    
    g.restoreState();
}

void NexusLookAndFeel_Final::drawHolographicPanel(juce::Graphics& g, juce::Rectangle<float> bounds, bool isActive)
{
    // Base panel with gradient
    juce::ColourGradient panelGrad(
        juce::Colour(Colors::baseDark), bounds.getTopLeft(),
        juce::Colour(Colors::baseBlack), bounds.getBottomRight(),
        false
    );
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Carbon texture overlay
    drawCarbonFiberTexture(g, bounds);
    
    // Holographic corner brackets when active
    if (isActive)
    {
        drawCornerBrackets(g, bounds, juce::Colour(Colors::primaryCyan));
    }
    
    // Edge highlight
    g.setColour(juce::Colour(0x10ffffff));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
}

void NexusLookAndFeel_Final::drawScanlineEffect(juce::Graphics& g, juce::Rectangle<float> bounds, float phase)
{
    g.saveState();
    g.reduceClipRegion(bounds.toNearestInt());
    
    // Horizontal scanlines
    g.setColour(juce::Colour(Colors::primaryCyan).withAlpha(0.03f));
    for (float y = bounds.getY(); y < bounds.getBottom(); y += 2.0f)
    {
        g.drawHorizontalLine(int(y), bounds.getX(), bounds.getRight());
    }
    
    // Animated scanning line
    float scanY = bounds.getY() + (bounds.getHeight() * phase);
    g.setColour(juce::Colour(Colors::primaryCyan).withAlpha(0.1f));
    g.fillRect(juce::Rectangle<float>(bounds.getX(), scanY - 10, bounds.getWidth(), 20));
    
    g.restoreState();
}

void NexusLookAndFeel_Final::drawNeonGlow(juce::Graphics& g, juce::Rectangle<float> bounds, 
                                          juce::Colour color, float intensity)
{
    // Multi-layer glow effect
    for (int i = 3; i > 0; --i)
    {
        float expansion = i * 3.0f;
        float alpha = intensity * (0.15f / i);
        g.setColour(color.withAlpha(alpha));
        g.drawRoundedRectangle(bounds.expanded(expansion), 4.0f + expansion, expansion);
    }
}

void NexusLookAndFeel_Final::drawCornerBrackets(juce::Graphics& g, juce::Rectangle<float> bounds, 
                                                juce::Colour color)
{
    float cornerSize = 20.0f;
    float thickness = 2.0f;
    
    g.setColour(color);
    
    // Top-left
    g.fillRect(bounds.getX(), bounds.getY(), cornerSize, thickness);
    g.fillRect(bounds.getX(), bounds.getY(), thickness, cornerSize);
    
    // Top-right
    g.fillRect(bounds.getRight() - cornerSize, bounds.getY(), cornerSize, thickness);
    g.fillRect(bounds.getRight() - thickness, bounds.getY(), thickness, cornerSize);
    
    // Bottom-left
    g.fillRect(bounds.getX(), bounds.getBottom() - thickness, cornerSize, thickness);
    g.fillRect(bounds.getX(), bounds.getBottom() - cornerSize, thickness, cornerSize);
    
    // Bottom-right
    g.fillRect(bounds.getRight() - cornerSize, bounds.getBottom() - thickness, cornerSize, thickness);
    g.fillRect(bounds.getRight() - thickness, bounds.getBottom() - cornerSize, thickness, cornerSize);
}

void NexusLookAndFeel_Final::drawKnobGrip(juce::Graphics& g, juce::Point<float> center, float radius)
{
    // Machined grip ridges
    g.setColour(juce::Colour(0x15ffffff));
    int numRidges = 12;
    
    for (int i = 0; i < numRidges; ++i)
    {
        float angle = (i * juce::MathConstants<float>::twoPi) / numRidges;
        auto start = center.getPointOnCircumference(radius * 0.6f, angle);
        auto end = center.getPointOnCircumference(radius * 0.95f, angle);
        g.drawLine(start.x, start.y, end.x, end.y, 1.5f);
    }
}

juce::Font NexusLookAndFeel_Final::getTacticalFont(float height, bool bold)
{
    auto font = juce::Font(juce::FontOptions().withHeight(height));
    if (bold) font = font.boldened();
    return font;
}

juce::Font NexusLookAndFeel_Final::getMonospacedFont(float height)
{
    return juce::Font(juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain);
}

void NexusLookAndFeel_Final::updateAnimations()
{
    scanlinePhase += 0.01f;
    if (scanlinePhase > 1.0f)
        scanlinePhase = 0.0f;
}