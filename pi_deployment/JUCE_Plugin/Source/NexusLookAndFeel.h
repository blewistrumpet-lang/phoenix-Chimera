#pragma once
#include <JuceHeader.h>

/**
 * NexusLookAndFeel - Tactile Futurism Aesthetic
 * 
 * A custom look and feel that transforms the UI into an inspiring,
 * professional-grade instrument with industrial design elements.
 */
class NexusLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NexusLookAndFeel();
    ~NexusLookAndFeel() override;
    
    // Custom drawing methods for Tactile Futurism aesthetic
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;
    
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& box) override;
    
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    // Carbon fiber texture background
    static void drawCarbonFiberBackground(juce::Graphics& g, juce::Rectangle<float> bounds);
    
    // 3D beveled module background
    static void draw3DBeveledModule(juce::Graphics& g, juce::Rectangle<float> bounds);
    
private:
    // Color scheme for Tactile Futurism
    const juce::Colour cyanGlow = juce::Colour(0xff00ffcc);
    const juce::Colour darkGrey = juce::Colour(0xff1a1a1a);
    const juce::Colour midGrey = juce::Colour(0xff2a2a2a);
    const juce::Colour lightGrey = juce::Colour(0xff4a4a4a);
    const juce::Colour redAlert = juce::Colour(0xffff006e);
    const juce::Colour yellowWarning = juce::Colour(0xffffcc00);
    
    // Industrial encoder design elements
    void drawIndustrialEncoder(juce::Graphics& g, juce::Rectangle<float> bounds,
                               float angle, bool isSteppedControl);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusLookAndFeel)
};