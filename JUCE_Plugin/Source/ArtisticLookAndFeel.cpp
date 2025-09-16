#include "ArtisticLookAndFeel.h"

ArtisticLookAndFeel::ArtisticLookAndFeel()
{
    // Set default colors for modern artistic theme
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(ColorScheme::text));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId, juce::Colour(ColorScheme::text));
    setColour(juce::TextEditor::textColourId, juce::Colour(ColorScheme::text));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(ColorScheme::panel));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(ColorScheme::accent).withAlpha(0.3f));
    setColour(juce::ComboBox::textColourId, juce::Colour(ColorScheme::text));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(ColorScheme::panel));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(ColorScheme::accent).withAlpha(0.3f));
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(ColorScheme::background));
}

ArtisticLookAndFeel::~ArtisticLookAndFeel() = default;

void ArtisticLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, 
                                          int width, int height,
                                          float sliderPos, 
                                          float rotaryStartAngle, 
                                          float rotaryEndAngle,
                                          juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height);
    auto knobBounds = bounds.reduced(6.0f);
    
    // Draw soft shadow
    drawSoftShadow(g, knobBounds.expanded(2.0f), 8.0f, 0.2f);
    
    // Draw track
    drawKnobTrack(g, knobBounds, rotaryStartAngle, rotaryEndAngle, sliderPos);
    
    // Draw knob body with gradient
    auto center = knobBounds.getCentre();
    auto radius = knobBounds.getWidth() * 0.4f;
    
    juce::ColourGradient knobGradient(
        juce::Colour(ColorScheme::panel).brighter(0.2f),
        center.x - radius * 0.5f, center.y - radius * 0.5f,
        juce::Colour(ColorScheme::panel).darker(0.3f),
        center.x + radius * 0.5f, center.y + radius * 0.5f,
        true
    );
    
    g.setGradientFill(knobGradient);
    g.fillEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2);
    
    // Draw subtle rim
    g.setColour(juce::Colour(ColorScheme::accent).withAlpha(0.3f));
    g.drawEllipse(center.x - radius, center.y - radius, radius * 2, radius * 2, 1.5f);
    
    // Draw indicator
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    drawKnobIndicator(g, knobBounds, angle, 
                     slider.isEnabled() ? juce::Colour(ColorScheme::accent) : juce::Colour(ColorScheme::textDim));
}

void ArtisticLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                          bool shouldDrawButtonAsHighlighted, 
                                          bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto toggleBounds = bounds.removeFromLeft(44.0f).reduced(2.0f);
    
    // Modern iOS-style toggle switch
    bool isOn = button.getToggleState();
    
    // Track
    auto trackBounds = toggleBounds.withSizeKeepingCentre(36.0f, 20.0f);
    g.setColour(isOn ? juce::Colour(ColorScheme::accent) : juce::Colour(ColorScheme::panel));
    g.fillRoundedRectangle(trackBounds, 10.0f);
    
    // Knob with animation position
    float knobX = isOn ? trackBounds.getRight() - 18.0f : trackBounds.getX() + 2.0f;
    auto knobBounds = juce::Rectangle<float>(knobX, trackBounds.getY() + 2.0f, 16.0f, 16.0f);
    
    drawSoftShadow(g, knobBounds, 4.0f, 0.2f);
    g.setColour(juce::Colours::white);
    g.fillEllipse(knobBounds);
    
    // Label
    g.setFont(getModernFont(14.0f));
    g.setColour(juce::Colour(ColorScheme::text));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centredLeft);
}

void ArtisticLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    
    // Modern gradient button
    auto baseColor = button.findColour(juce::TextButton::buttonColourId);
    if (baseColor == juce::Colour()) baseColor = juce::Colour(ColorScheme::accent);
    
    if (shouldDrawButtonAsDown)
        baseColor = baseColor.darker(0.2f);
    else if (shouldDrawButtonAsHighlighted)
        baseColor = baseColor.brighter(0.1f);
    
    // Soft shadow
    if (!shouldDrawButtonAsDown)
        drawSoftShadow(g, bounds, 6.0f, 0.15f);
    
    // Gradient fill
    juce::ColourGradient gradient(
        baseColor.brighter(0.1f), bounds.getX(), bounds.getY(),
        baseColor.darker(0.1f), bounds.getX(), bounds.getBottom(),
        false
    );
    
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Subtle border
    g.setColour(baseColor.brighter(0.3f).withAlpha(0.5f));
    g.drawRoundedRectangle(bounds, 6.0f, 0.5f);
}

void ArtisticLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, 
                                      bool isButtonDown,
                                      int buttonX, int buttonY, int buttonW, int buttonH,
                                      juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    
    // Modern flat design
    g.setColour(juce::Colour(ColorScheme::panel));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Subtle border
    g.setColour(juce::Colour(ColorScheme::accent).withAlpha(0.3f));
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Arrow
    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float>(buttonX, buttonY, buttonW, buttonH).reduced(buttonW * 0.3f);
    arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                     arrowBounds.getRight(), arrowBounds.getY(),
                     arrowBounds.getCentreX(), arrowBounds.getBottom());
    
    g.setColour(juce::Colour(ColorScheme::textDim));
    g.fillPath(arrow);
}

void ArtisticLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited())
    {
        auto bounds = label.getLocalBounds().toFloat();
        
        g.setFont(getModernFont(label.getFont().getHeight()));
        g.setColour(label.findColour(juce::Label::textColourId));
        g.drawText(label.getText(), bounds, label.getJustificationType());
    }
}

void ArtisticLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                                   const juce::String& text,
                                                   const juce::Justification& position,
                                                   juce::GroupComponent& group)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    
    // Glass panel effect
    drawGlassPanel(g, bounds, 8.0f, 0.05f);
    
    // Title if present
    if (text.isNotEmpty())
    {
        auto titleBounds = bounds.removeFromTop(24.0f).reduced(12.0f, 0);
        g.setFont(getModernFont(14.0f, true));
        g.setColour(juce::Colour(ColorScheme::text));
        g.drawText(text, titleBounds, juce::Justification::centredLeft);
    }
}

void ArtisticLookAndFeel::drawGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds, 
                                        float cornerRadius, float opacity)
{
    // Subtle glass background
    g.setColour(juce::Colour(ColorScheme::glass));
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    // Gradient overlay for depth
    juce::ColourGradient glassGradient(
        juce::Colours::white.withAlpha(opacity),
        bounds.getX(), bounds.getY(),
        juce::Colours::transparentBlack,
        bounds.getX(), bounds.getCentreY(),
        false
    );
    g.setGradientFill(glassGradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
    
    // Subtle border
    g.setColour(juce::Colours::white.withAlpha(0.1f));
    g.drawRoundedRectangle(bounds, cornerRadius, 0.5f);
}

void ArtisticLookAndFeel::drawModernLED(juce::Graphics& g, juce::Rectangle<float> bounds,
                                       bool isOn, juce::Colour color)
{
    // Outer glow when on
    if (isOn)
    {
        g.setColour(color.withAlpha(0.3f));
        g.fillEllipse(bounds.expanded(3.0f));
        
        g.setColour(color.withAlpha(0.5f));
        g.fillEllipse(bounds.expanded(1.0f));
    }
    
    // Main LED
    g.setColour(isOn ? color : color.darker(0.7f).withAlpha(0.3f));
    g.fillEllipse(bounds);
    
    // Highlight
    if (isOn)
    {
        auto highlight = bounds.reduced(bounds.getWidth() * 0.3f)
                               .translated(-bounds.getWidth() * 0.1f, -bounds.getHeight() * 0.1f);
        g.setColour(color.brighter(0.5f).withAlpha(0.7f));
        g.fillEllipse(highlight);
    }
}

void ArtisticLookAndFeel::drawSoftShadow(juce::Graphics& g, juce::Rectangle<float> bounds, 
                                        float radius, float opacity)
{
    for (int i = 0; i < 3; ++i)
    {
        float offset = radius * (1.0f - i * 0.3f);
        float alpha = opacity * (0.3f - i * 0.1f);
        
        g.setColour(juce::Colours::black.withAlpha(alpha));
        g.fillRoundedRectangle(bounds.translated(0, offset * 0.5f).expanded(offset * 0.5f), 
                              bounds.getHeight() * 0.1f);
    }
}

juce::Font ArtisticLookAndFeel::getModernFont(float height, bool isBold)
{
    auto font = juce::Font(juce::FontOptions()
        .withName("Inter, SF Pro Display, Helvetica Neue, Arial")
        .withHeight(height));
    return isBold ? font.boldened() : font;
}

juce::Font ArtisticLookAndFeel::getLabelFont(juce::Label&)
{
    return getModernFont(13.0f);
}

juce::Font ArtisticLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return getModernFont(14.0f);
}

void ArtisticLookAndFeel::drawKnobIndicator(juce::Graphics& g, juce::Rectangle<float> bounds, 
                                           float angle, juce::Colour color)
{
    auto center = bounds.getCentre();
    auto radius = bounds.getWidth() * 0.4f;
    
    // Draw indicator line
    auto lineStart = center.getPointOnCircumference(radius * 0.6f, angle);
    auto lineEnd = center.getPointOnCircumference(radius * 0.9f, angle);
    
    g.setColour(color);
    g.drawLine(juce::Line<float>(lineStart, lineEnd), 2.5f);
    
    // Small dot at end
    g.fillEllipse(lineEnd.x - 2, lineEnd.y - 2, 4, 4);
}

void ArtisticLookAndFeel::drawKnobTrack(juce::Graphics& g, juce::Rectangle<float> bounds,
                                       float startAngle, float endAngle, float value)
{
    auto center = bounds.getCentre();
    auto radius = bounds.getWidth() * 0.45f;
    
    // Background track
    juce::Path backgroundArc;
    backgroundArc.addCentredArc(center.x, center.y, radius, radius, 0,
                               startAngle, endAngle, true);
    
    g.setColour(juce::Colour(ColorScheme::panel).darker(0.3f));
    g.strokePath(backgroundArc, juce::PathStrokeType(3.0f));
    
    // Value track
    if (value > 0.01f)
    {
        juce::Path valueArc;
        auto currentAngle = startAngle + value * (endAngle - startAngle);
        valueArc.addCentredArc(center.x, center.y, radius, radius, 0,
                              startAngle, currentAngle, true);
        
        g.setColour(juce::Colour(ColorScheme::accent));
        g.strokePath(valueArc, juce::PathStrokeType(3.0f));
    }
}