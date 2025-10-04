#include "SkunkworksLookAndFeel.h"

SkunkworksLookAndFeel::SkunkworksLookAndFeel()
{
    // Set default colors for military/industrial theme
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(ColorScheme::amberLED));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(ColorScheme::panelMetal));
    setColour(juce::Label::textColourId, juce::Colour(ColorScheme::textStencil));
    setColour(juce::TextEditor::textColourId, juce::Colour(ColorScheme::amberLED));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(ColorScheme::panelBackground));
    setColour(juce::ComboBox::textColourId, juce::Colour(ColorScheme::textStencil));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(ColorScheme::panelMetal));
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(ColorScheme::panelBackground));
    
    // Initialize textures
    createMetalTexture();
    createWearPattern();
}

SkunkworksLookAndFeel::~SkunkworksLookAndFeel() = default;

void SkunkworksLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, 
                                            int width, int height,
                                            float sliderPos, 
                                            float rotaryStartAngle, 
                                            float rotaryEndAngle,
                                            juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height);
    auto knobBounds = bounds.reduced(8.0f);
    
    // Draw shadow beneath knob
    drawKnobShadow(g, knobBounds);
    
    // Calculate angle for current value
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Draw tick marks around knob
    drawKnobTicks(g, knobBounds, rotaryStartAngle, rotaryEndAngle);
    
    // Draw main knob body with industrial styling
    drawKnobBody(g, knobBounds, angle);
    
    // Draw LED indicator for current value
    auto ledBounds = knobBounds.reduced(knobBounds.getWidth() * 0.35f);
    ledBounds = ledBounds.withHeight(4.0f).translated(0, -knobBounds.getHeight() * 0.15f);
    
    bool isActive = slider.isEnabled() && !slider.isMouseButtonDown();
    auto ledColor = isActive ? juce::Colour(ColorScheme::greenLED) : juce::Colour(ColorScheme::warningRed);
    drawLEDIndicator(g, ledBounds, isActive, ledColor);
    
    // Draw value readout below knob
    if (slider.isMouseOverOrDragging() || slider.isMouseButtonDown())
    {
        auto displayBounds = bounds.removeFromBottom(20.0f).reduced(5.0f, 0);
        auto value = juce::String(slider.getValue(), 2);
        drawSegmentedDisplay(g, displayBounds, value);
    }
}

void SkunkworksLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted, 
                                            bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto switchBounds = bounds.removeFromLeft(50.0f).reduced(5.0f);
    
    // Draw switch housing
    drawMetalPanel(g, switchBounds, true);
    
    // Draw switch lever
    bool isOn = button.getToggleState();
    auto switchPath = createSwitchPath(switchBounds.reduced(5.0f), isOn);
    
    g.setColour(juce::Colour(ColorScheme::panelMetal).brighter(0.2f));
    g.fillPath(switchPath);
    
    g.setColour(juce::Colour(ColorScheme::screwMetal));
    g.strokePath(switchPath, juce::PathStrokeType(1.0f));
    
    // Draw LED indicator
    auto ledBounds = switchBounds.removeFromBottom(8.0f).reduced(switchBounds.getWidth() * 0.3f, 0);
    drawLEDIndicator(g, ledBounds, isOn, 
                    isOn ? juce::Colour(ColorScheme::greenLED) : juce::Colour(ColorScheme::warningRed));
    
    // Draw label with stencil font
    g.setFont(getStencilFont(14.0f));
    g.setColour(juce::Colour(ColorScheme::textStencil));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centredLeft);
}

void SkunkworksLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted,
                                                bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    
    // Draw recessed metal panel
    drawMetalPanel(g, bounds, shouldDrawButtonAsDown);
    
    // Draw corner screws
    float screwSize = 6.0f;
    drawScrew(g, bounds.getX() + 5, bounds.getY() + 5, screwSize);
    drawScrew(g, bounds.getRight() - 5 - screwSize, bounds.getY() + 5, screwSize);
    drawScrew(g, bounds.getX() + 5, bounds.getBottom() - 5 - screwSize, screwSize);
    drawScrew(g, bounds.getRight() - 5 - screwSize, bounds.getBottom() - 5 - screwSize, screwSize);
    
    // Highlight on hover
    if (shouldDrawButtonAsHighlighted && !shouldDrawButtonAsDown)
    {
        g.setColour(juce::Colour(ColorScheme::amberLED).withAlpha(0.1f));
        g.fillRoundedRectangle(bounds.reduced(2.0f), 2.0f);
    }
    
    // Warning stripe for important buttons
    if (button.getButtonText().containsIgnoreCase("generate") || 
        button.getButtonText().containsIgnoreCase("delete"))
    {
        juce::Path warnStripe;
        warnStripe.addRectangle(bounds.removeFromBottom(3.0f));
        g.setColour(juce::Colour(ColorScheme::warningRed));
        g.fillPath(warnStripe);
    }
}

void SkunkworksLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, 
                                        bool isButtonDown,
                                        int buttonX, int buttonY, int buttonW, int buttonH,
                                        juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    
    // Draw military-style selector panel
    drawMetalPanel(g, bounds, isButtonDown);
    
    // Draw arrow indicator
    juce::Path arrow;
    auto arrowBounds = juce::Rectangle<float>(buttonX, buttonY, buttonW, buttonH).reduced(buttonW * 0.3f);
    arrow.addTriangle(arrowBounds.getX(), arrowBounds.getY(),
                     arrowBounds.getRight(), arrowBounds.getY(),
                     arrowBounds.getCentreX(), arrowBounds.getBottom());
    
    g.setColour(juce::Colour(ColorScheme::amberLED));
    g.fillPath(arrow);
}

void SkunkworksLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited())
    {
        auto bounds = label.getLocalBounds().toFloat();
        
        // Use stencil font for labels
        g.setFont(getStencilFont(label.getFont().getHeight()));
        g.setColour(label.findColour(juce::Label::textColourId));
        
        // Add subtle embossed effect
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawText(label.getText(), bounds.translated(1, 1), label.getJustificationType());
        
        g.setColour(label.findColour(juce::Label::textColourId));
        g.drawText(label.getText(), bounds, label.getJustificationType());
    }
}

void SkunkworksLookAndFeel::drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                                     const juce::String& text,
                                                     const juce::Justification& position,
                                                     juce::GroupComponent& group)
{
    auto bounds = juce::Rectangle<float>(0, 0, width, height);
    
    // Draw main panel
    drawMetalPanel(g, bounds);
    
    // Draw title plate if text exists
    if (text.isNotEmpty())
    {
        auto titleBounds = bounds.removeFromTop(25.0f).reduced(10.0f, 2.0f);
        
        // Metal nameplate
        g.setColour(juce::Colour(ColorScheme::panelMetal).darker(0.3f));
        g.fillRoundedRectangle(titleBounds, 2.0f);
        
        // Embossed text
        g.setFont(getStencilFont(14.0f));
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawText(text, titleBounds.translated(1, 1), juce::Justification::centred);
        g.setColour(juce::Colour(ColorScheme::textStencil));
        g.drawText(text, titleBounds, juce::Justification::centred);
    }
}

void SkunkworksLookAndFeel::drawMetalPanel(juce::Graphics& g, juce::Rectangle<float> bounds, bool isRecessed)
{
    // Base panel
    g.setColour(juce::Colour(ColorScheme::panelMetal));
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Metal texture overlay
    if (metalTexture.isValid())
    {
        g.setOpacity(0.3f);
        g.drawImage(metalTexture, bounds, juce::RectanglePlacement::fillDestination);
        g.setOpacity(1.0f);
    }
    
    // Beveled edges
    juce::Path edge;
    edge.addRoundedRectangle(bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), 3.0f);
    
    if (isRecessed)
    {
        // Dark top/left for recessed look
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.strokePath(edge, juce::PathStrokeType(1.0f), 
                    juce::AffineTransform::translation(-1, -1));
        
        // Light bottom/right
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.strokePath(edge, juce::PathStrokeType(1.0f), 
                    juce::AffineTransform::translation(1, 1));
    }
    else
    {
        // Light top/left for raised look
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.strokePath(edge, juce::PathStrokeType(1.0f), 
                    juce::AffineTransform::translation(-1, -1));
        
        // Dark bottom/right
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.strokePath(edge, juce::PathStrokeType(1.0f), 
                    juce::AffineTransform::translation(1, 1));
    }
    
    // Wear marks
    if (wearPattern.isValid())
    {
        g.setOpacity(0.1f);
        g.drawImage(wearPattern, bounds, juce::RectanglePlacement::fillDestination);
        g.setOpacity(1.0f);
    }
}

void SkunkworksLookAndFeel::drawScrew(juce::Graphics& g, float x, float y, float size)
{
    auto bounds = juce::Rectangle<float>(x, y, size, size);
    
    // Screw head
    g.setColour(juce::Colour(ColorScheme::screwMetal));
    g.fillEllipse(bounds);
    
    // Inner circle
    g.setColour(juce::Colour(ColorScheme::screwMetal).darker(0.3f));
    g.drawEllipse(bounds.reduced(1.0f), 0.5f);
    
    // Phillips head slot
    auto slotLength = size * 0.6f;
    auto center = bounds.getCentre();
    
    g.setColour(juce::Colours::black.withAlpha(0.7f));
    g.drawLine(center.x - slotLength/2, center.y, center.x + slotLength/2, center.y, 1.0f);
    g.drawLine(center.x, center.y - slotLength/2, center.x, center.y + slotLength/2, 1.0f);
}

void SkunkworksLookAndFeel::drawLEDIndicator(juce::Graphics& g, juce::Rectangle<float> bounds,
                                            bool isOn, juce::Colour ledColor)
{
    // LED housing
    g.setColour(juce::Colours::black);
    g.fillEllipse(bounds);
    
    if (isOn)
    {
        // Glowing LED
        auto glowBounds = bounds.expanded(2.0f);
        g.setColour(ledColor.withAlpha(0.3f));
        g.fillEllipse(glowBounds);
        
        g.setColour(ledColor);
        g.fillEllipse(bounds.reduced(1.0f));
        
        // Bright center
        g.setColour(ledColor.brighter(0.5f));
        g.fillEllipse(bounds.reduced(bounds.getWidth() * 0.3f));
    }
    else
    {
        // Dim LED
        g.setColour(ledColor.darker(0.7f).withAlpha(0.5f));
        g.fillEllipse(bounds.reduced(1.0f));
    }
}

void SkunkworksLookAndFeel::drawSegmentedDisplay(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                const juce::String& text, juce::Colour displayColor)
{
    // Display background
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // Display border
    g.setColour(juce::Colour(ColorScheme::panelMetal).darker(0.5f));
    g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
    
    // Segmented display text
    g.setFont(getTerminalFont(bounds.getHeight() * 0.8f));
    g.setColour(displayColor.withAlpha(0.2f)); // Dim segments
    g.drawText("888.88", bounds, juce::Justification::centred);
    
    g.setColour(displayColor); // Bright active segments
    g.drawText(text, bounds, juce::Justification::centred);
}

juce::Font SkunkworksLookAndFeel::getStencilFont(float height)
{
    // Use bold sans-serif to simulate stencil
    auto font = juce::Font(juce::FontOptions().withName("Arial Black").withHeight(height));
    return font.boldened();
}

juce::Font SkunkworksLookAndFeel::getTerminalFont(float height)
{
    // Use monospaced font for displays
    return juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withHeight(height));
}

juce::Font SkunkworksLookAndFeel::getLabelFont(juce::Label&)
{
    return getStencilFont(14.0f);
}

juce::Font SkunkworksLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return getStencilFont(14.0f);
}

void SkunkworksLookAndFeel::createMetalTexture()
{
    // Create procedural brushed metal texture
    metalTexture = juce::Image(juce::Image::RGB, 256, 256, true);
    juce::Graphics g(metalTexture);
    
    // Base metal color
    g.fillAll(juce::Colour(ColorScheme::panelMetal));
    
    // Add horizontal brush strokes
    juce::Random random;
    for (int y = 0; y < 256; ++y)
    {
        auto brightness = random.nextFloat() * 0.1f - 0.05f;
        g.setColour(juce::Colours::white.withAlpha(brightness));
        g.drawHorizontalLine(y, 0, 256);
    }
}

void SkunkworksLookAndFeel::createWearPattern()
{
    // Create procedural wear/scratch pattern
    wearPattern = juce::Image(juce::Image::RGB, 256, 256, true);
    juce::Graphics g(wearPattern);
    
    g.fillAll(juce::Colours::transparentBlack);
    
    // Add random scratches and wear marks
    juce::Random random;
    g.setColour(juce::Colour(ColorScheme::wearMark));
    
    for (int i = 0; i < 20; ++i)
    {
        auto x1 = random.nextFloat() * 256;
        auto y1 = random.nextFloat() * 256;
        auto x2 = x1 + random.nextFloat() * 50 - 25;
        auto y2 = y1 + random.nextFloat() * 50 - 25;
        
        g.drawLine(x1, y1, x2, y2, random.nextFloat() * 2.0f);
    }
}

void SkunkworksLookAndFeel::drawKnobShadow(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    auto shadow = bounds.expanded(3.0f).translated(2, 2);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillEllipse(shadow);
}

void SkunkworksLookAndFeel::drawKnobBody(juce::Graphics& g, juce::Rectangle<float> bounds, float angle)
{
    // Outer ring
    g.setColour(juce::Colour(ColorScheme::screwMetal).darker(0.2f));
    g.fillEllipse(bounds);
    
    // Inner knob
    auto innerBounds = bounds.reduced(4.0f);
    g.setColour(juce::Colour(ColorScheme::panelMetal));
    g.fillEllipse(innerBounds);
    
    // Metallic highlight
    auto highlight = innerBounds.reduced(innerBounds.getWidth() * 0.3f)
                                .translated(-innerBounds.getWidth() * 0.1f, -innerBounds.getHeight() * 0.1f);
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    g.fillEllipse(highlight);
    
    // Position indicator line
    auto center = bounds.getCentre();
    auto lineLength = bounds.getWidth() * 0.35f;
    auto lineEnd = center.getPointOnCircumference(lineLength, angle);
    
    g.setColour(juce::Colour(ColorScheme::warningRed));
    g.drawLine(juce::Line<float>(center, lineEnd), 3.0f);
    
    // Center screw
    drawScrew(g, center.x - 3, center.y - 3, 6);
}

void SkunkworksLookAndFeel::drawKnobTicks(juce::Graphics& g, juce::Rectangle<float> bounds,
                                         float startAngle, float endAngle, int numTicks)
{
    auto center = bounds.getCentre();
    auto radius = bounds.getWidth() * 0.5f;
    
    for (int i = 0; i < numTicks; ++i)
    {
        auto angle = startAngle + (i * (endAngle - startAngle) / (numTicks - 1));
        auto tickStart = center.getPointOnCircumference(radius + 5, angle);
        auto tickEnd = center.getPointOnCircumference(radius + 10, angle);
        
        // Major ticks at 0, 50%, 100%
        bool isMajor = (i == 0 || i == numTicks/2 || i == numTicks-1);
        
        g.setColour(juce::Colour(ColorScheme::textDimmed));
        g.drawLine(juce::Line<float>(tickStart, tickEnd), isMajor ? 2.0f : 1.0f);
    }
}

juce::Path SkunkworksLookAndFeel::createSwitchPath(juce::Rectangle<float> bounds, bool isOn)
{
    juce::Path path;
    auto center = bounds.getCentre();
    
    if (isOn)
    {
        // Up position
        path.addRectangle(bounds.withHeight(bounds.getHeight() * 0.6f));
    }
    else
    {
        // Down position
        path.addRectangle(bounds.withHeight(bounds.getHeight() * 0.6f)
                                .translated(0, bounds.getHeight() * 0.4f));
    }
    
    return path;
}