#pragma once

#include <JuceHeader.h>

class SkunkworksLookAndFeel : public juce::LookAndFeel_V4 
{
public:
    SkunkworksLookAndFeel();
    ~SkunkworksLookAndFeel() override;
    
    // Color scheme - military/industrial palette
    struct ColorScheme {
        static constexpr uint32_t panelBackground = 0xff1a1a1a;      // Dark gunmetal
        static constexpr uint32_t panelMetal = 0xff2d2d2d;           // Brushed aluminum
        static constexpr uint32_t warningRed = 0xffcc2222;           // Military warning red
        static constexpr uint32_t amberLED = 0xffffaa00;             // Classic amber LED
        static constexpr uint32_t greenLED = 0xff00ff44;             // Active green LED
        static constexpr uint32_t textStencil = 0xffcccccc;          // Stenciled text
        static constexpr uint32_t textDimmed = 0xff666666;           // Inactive text
        static constexpr uint32_t screwMetal = 0xff888888;           // Screw/rivet color
        static constexpr uint32_t wearMark = 0xff3a3a3a;             // Wear/scratch marks
    };
    
    // Rotary slider with industrial design
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    // Toggle button as mil-spec switch
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    // Text button with military styling
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
    
    // ComboBox as military selector switch
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& box) override;
    
    // Label with stenciled text
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    // Panel backgrounds with metal texture
    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                  const juce::String& text,
                                  const juce::Justification& position,
                                  juce::GroupComponent& group) override;
    
    // Custom methods for Skunkworks elements
    void drawMetalPanel(juce::Graphics& g, juce::Rectangle<float> bounds, 
                       bool isRecessed = false);
    
    void drawScrew(juce::Graphics& g, float x, float y, float size);
    
    void drawLEDIndicator(juce::Graphics& g, juce::Rectangle<float> bounds,
                         bool isOn, juce::Colour ledColor = juce::Colour(ColorScheme::greenLED));
    
    void drawSegmentedDisplay(juce::Graphics& g, juce::Rectangle<float> bounds,
                             const juce::String& text, juce::Colour displayColor = juce::Colour(ColorScheme::amberLED));
    
    // Font management
    juce::Font getStencilFont(float height);
    juce::Font getTerminalFont(float height);
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    
private:
    // Texture generation
    void createMetalTexture();
    void createWearPattern();
    
    // Cached textures and patterns
    juce::Image metalTexture;
    juce::Image wearPattern;
    std::unique_ptr<juce::Drawable> screwDrawable;
    
    // Fonts
    juce::Font stencilFont;
    juce::Font terminalFont;
    
    // Helper methods
    void drawKnobShadow(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawKnobBody(juce::Graphics& g, juce::Rectangle<float> bounds, float angle);
    void drawKnobTicks(juce::Graphics& g, juce::Rectangle<float> bounds, 
                      float startAngle, float endAngle, int numTicks = 11);
    juce::Path createSwitchPath(juce::Rectangle<float> bounds, bool isOn);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SkunkworksLookAndFeel)
};