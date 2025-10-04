#include "NexusLookAndFeelDynamic.h"
#include <cmath>

NexusLookAndFeelDynamic::NexusLookAndFeelDynamic()
{
    // Set default colors
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(Colors::baseDark));
    setColour(juce::Label::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(Colors::baseBlack));
    setColour(juce::TextEditor::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(Colors::primaryCyan));
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(Colors::baseBlack));
    setColour(juce::ComboBox::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(Colors::primaryCyan));
    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(Colors::baseBlack));
    setColour(juce::PopupMenu::textColourId, juce::Colour(Colors::textPrimary));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(Colors::primaryCyan).withAlpha(0.3f));
}

NexusLookAndFeelDynamic::~NexusLookAndFeelDynamic() = default;

void NexusLookAndFeelDynamic::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPosProportional, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height);
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.4f;
    auto center = bounds.getCentre();
    
    // Draw industrial encoder body
    drawIndustrialKnob(g, bounds.withSizeKeepingCentre(radius * 2, radius * 2), 
                      rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle),
                      slider.isEnabled());
}

void NexusLookAndFeelDynamic::drawIndustrialKnob(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                 float angle, bool isActive)
{
    auto center = bounds.getCentre();
    auto radius = bounds.getWidth() * 0.5f;
    
    // Outer ring - dark metal
    g.setColour(juce::Colour(Colors::baseBlack));
    g.fillEllipse(bounds);
    
    // Inner ring with machined grip texture
    auto innerBounds = bounds.reduced(bounds.getWidth() * 0.1f);
    g.setColour(juce::Colour(Colors::baseDark));
    g.fillEllipse(innerBounds);
    
    // Draw grip ridges
    for (int i = 0; i < 24; ++i)
    {
        float ridgeAngle = (float)i * (juce::MathConstants<float>::twoPi / 24.0f);
        float x1 = center.x + std::cos(ridgeAngle) * radius * 0.7f;
        float y1 = center.y + std::sin(ridgeAngle) * radius * 0.7f;
        float x2 = center.x + std::cos(ridgeAngle) * radius * 0.9f;
        float y2 = center.y + std::sin(ridgeAngle) * radius * 0.9f;
        
        g.setColour(juce::Colour(Colors::baseBlack).withAlpha(0.5f));
        g.drawLine(x1, y1, x2, y2, 1.0f);
    }
    
    // Center cap
    auto capBounds = bounds.reduced(bounds.getWidth() * 0.35f);
    g.setColour(juce::Colour(Colors::baseBlack));
    g.fillEllipse(capBounds);
    
    // Position indicator - single cyan line
    float indicatorLength = radius * 0.8f;
    float endX = center.x + std::cos(angle - juce::MathConstants<float>::halfPi) * indicatorLength;
    float endY = center.y + std::sin(angle - juce::MathConstants<float>::halfPi) * indicatorLength;
    
    g.setColour(juce::Colour(Colors::primaryCyan));
    g.drawLine(center.x, center.y, endX, endY, 3.0f);
    
    // Add glow if active
    if (isActive)
    {
        drawHolographicGlow(g, bounds, juce::Colour(Colors::primaryCyan), 0.3f);
    }
}

void NexusLookAndFeelDynamic::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(2);
    
    // Draw tactical switch
    drawTacticalSwitch(g, bounds.removeFromLeft(bounds.getHeight()),
                      button.getToggleState(), shouldDrawButtonAsHighlighted);
    
    // Draw text
    g.setColour(button.getToggleState() ? juce::Colour(Colors::primaryCyan) : juce::Colour(Colors::textSecondary));
    g.setFont(getTacticalFont(14.0f));
    g.drawText(button.getButtonText(), bounds, juce::Justification::centredLeft);
}

void NexusLookAndFeelDynamic::drawTacticalSwitch(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                 bool isOn, bool isHighlighted)
{
    // Switch body
    g.setColour(juce::Colour(Colors::baseBlack));
    g.fillRoundedRectangle(bounds, 2.0f);
    
    // Switch position
    auto switchBounds = bounds.reduced(2);
    if (isOn)
    {
        switchBounds.removeFromRight(switchBounds.getWidth() * 0.5f);
        g.setColour(juce::Colour(Colors::primaryCyan));
    }
    else
    {
        switchBounds.removeFromLeft(switchBounds.getWidth() * 0.5f);
        g.setColour(juce::Colour(Colors::baseDark));
    }
    
    g.fillRoundedRectangle(switchBounds, 1.0f);
    
    // Highlight on hover
    if (isHighlighted)
    {
        g.setColour(juce::Colour(Colors::primaryCyan).withAlpha(0.2f));
        g.drawRoundedRectangle(bounds, 2.0f, 1.0f);
    }
}

void NexusLookAndFeelDynamic::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                           int buttonX, int buttonY, int buttonW, int buttonH,
                                           juce::ComboBox& comboBox)
{
    auto bounds = comboBox.getLocalBounds().toFloat();
    
    // Background
    g.setColour(juce::Colour(Colors::baseBlack));
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Border with glow
    g.setColour(comboBox.isPopupActive() ? juce::Colour(Colors::primaryCyan) : juce::Colour(Colors::baseDark));
    g.drawRoundedRectangle(bounds.reduced(1), 3.0f, 2.0f);
    
    // Arrow
    juce::Path arrow;
    float arrowX = bounds.getRight() - 15;
    float arrowY = bounds.getCentreY();
    arrow.addTriangle(arrowX - 5, arrowY - 3, arrowX + 5, arrowY - 3, arrowX, arrowY + 3);
    g.setColour(juce::Colour(Colors::primaryCyan));
    g.fillPath(arrow);
}

void NexusLookAndFeelDynamic::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                   const juce::Colour& backgroundColour,
                                                   bool shouldDrawButtonAsHighlighted,
                                                   bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    
    // Main button body
    auto baseColor = shouldDrawButtonAsDown ? backgroundColour.darker(0.2f) : backgroundColour;
    g.setColour(baseColor);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Industrial bevel effect
    if (!shouldDrawButtonAsDown)
    {
        g.setColour(baseColor.brighter(0.2f));
        g.drawLine(bounds.getX() + 1, bounds.getY() + 1, 
                  bounds.getRight() - 1, bounds.getY() + 1, 1.0f);
        g.drawLine(bounds.getX() + 1, bounds.getY() + 1,
                  bounds.getX() + 1, bounds.getBottom() - 1, 1.0f);
    }
    
    // Glow on hover
    if (shouldDrawButtonAsHighlighted)
    {
        drawHolographicGlow(g, bounds, backgroundColour, 0.5f);
    }
}

void NexusLookAndFeelDynamic::fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                                       juce::TextEditor& textEditor)
{
    g.setColour(juce::Colour(Colors::baseBlack));
    g.fillRoundedRectangle(0, 0, (float)width, (float)height, 3.0f);
}

void NexusLookAndFeelDynamic::drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                                    juce::TextEditor& textEditor)
{
    g.setColour(textEditor.hasKeyboardFocus(false) ? 
               juce::Colour(Colors::primaryCyan) : 
               juce::Colour(Colors::baseDark));
    g.drawRoundedRectangle(1, 1, width - 2.0f, height - 2.0f, 3.0f, 2.0f);
}

void NexusLookAndFeelDynamic::drawLabel(juce::Graphics& g, juce::Label& label)
{
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited())
    {
        g.setColour(label.findColour(juce::Label::textColourId));
        g.setFont(getTacticalFont(label.getFont().getHeight()));
        g.drawText(label.getText(), label.getLocalBounds(),
                  label.getJustificationType(), true);
    }
}

void NexusLookAndFeelDynamic::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    // Holographic panel background
    g.setColour(juce::Colour(Colors::baseBlack).withAlpha(0.95f));
    g.fillRoundedRectangle(0, 0, (float)width, (float)height, 5.0f);
    
    // Cyan border
    g.setColour(juce::Colour(Colors::primaryCyan));
    g.drawRoundedRectangle(1, 1, width - 2.0f, height - 2.0f, 5.0f, 2.0f);
}

void NexusLookAndFeelDynamic::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                                bool isSeparator, bool isActive, bool isHighlighted,
                                                bool isTicked, bool hasSubMenu, const juce::String& text,
                                                const juce::String& shortcutKeyText,
                                                const juce::Drawable* icon, const juce::Colour* textColour)
{
    if (isSeparator)
    {
        auto r = area.reduced(5, 0);
        g.setColour(juce::Colour(Colors::baseDark));
        g.fillRect(r.removeFromTop(1));
        return;
    }
    
    if (isHighlighted)
    {
        g.setColour(juce::Colour(Colors::primaryCyan).withAlpha(0.2f));
        g.fillRect(area);
    }
    
    g.setColour(isHighlighted ? juce::Colour(Colors::primaryCyan) : juce::Colour(Colors::textPrimary));
    g.setFont(getTacticalFont(14.0f));
    
    auto textArea = area.reduced(10, 0);
    g.drawText(text, textArea, juce::Justification::centredLeft);
    
    if (hasSubMenu)
    {
        juce::Path arrow;
        auto areaCopy = area;
        auto arrowArea = areaCopy.removeFromRight(20).toFloat();
        arrow.addTriangle(arrowArea.getCentreX() - 3, arrowArea.getCentreY() - 4,
                         arrowArea.getCentreX() - 3, arrowArea.getCentreY() + 4,
                         arrowArea.getCentreX() + 3, arrowArea.getCentreY());
        g.fillPath(arrow);
    }
}

void NexusLookAndFeelDynamic::drawHolographicGlow(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                  juce::Colour glowColor, float intensity)
{
    for (int i = 3; i > 0; --i)
    {
        g.setColour(glowColor.withAlpha(intensity / (float)(i * 2)));
        g.drawRoundedRectangle(bounds.expanded((float)i * 2), 5.0f, 1.0f);
    }
}

void NexusLookAndFeelDynamic::drawCarbonFiberTexture(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Simple carbon fiber pattern
    for (int y = bounds.getY(); y < bounds.getBottom(); y += 4)
    {
        for (int x = bounds.getX(); x < bounds.getRight(); x += 4)
        {
            if ((x / 4 + y / 4) % 2 == 0)
            {
                g.setColour(juce::Colour(Colors::baseBlack).withAlpha(0.3f));
                g.fillRect(x, y, 2, 2);
            }
        }
    }
}

juce::Font NexusLookAndFeelDynamic::getIndustrialFont(float height)
{
    return juce::Font(juce::FontOptions().withHeight(height)).withStyle(juce::Font::bold);
}

juce::Font NexusLookAndFeelDynamic::getTacticalFont(float height)
{
    return juce::Font(juce::FontOptions().withHeight(height));
}