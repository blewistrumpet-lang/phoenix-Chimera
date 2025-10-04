#pragma once

#include <JuceHeader.h>

class ArtisticLookAndFeel : public juce::LookAndFeel_V4 
{
public:
    ArtisticLookAndFeel();
    ~ArtisticLookAndFeel() override;
    
    // Refined color palette - more artistic and modern
    struct ColorScheme {
        static constexpr uint32_t background = 0xff1a1a1f;       // Deep charcoal
        static constexpr uint32_t panel = 0xff252530;            // Soft dark grey
        static constexpr uint32_t accent = 0xff6366f1;           // Modern purple-blue
        static constexpr uint32_t secondary = 0xffa78bfa;        // Soft purple
        static constexpr uint32_t success = 0xff10b981;          // Emerald green
        static constexpr uint32_t warning = 0xfff59e0b;          // Amber
        static constexpr uint32_t error = 0xffef4444;            // Soft red
        static constexpr uint32_t text = 0xfff3f4f6;             // Off-white
        static constexpr uint32_t textDim = 0xff9ca3af;          // Muted grey
        static constexpr uint32_t glass = 0x20ffffff;            // Glass effect
    };
    
    // Elegant rotary slider
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    // Modern toggle button
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    // Sleek button
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
    
    // Modern combo box
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& box) override;
    
    // Clean label
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    // Panel with subtle glass effect
    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                  const juce::String& text,
                                  const juce::Justification& position,
                                  juce::GroupComponent& group) override;
    
    // Custom artistic elements
    void drawGlassPanel(juce::Graphics& g, juce::Rectangle<float> bounds, 
                       float cornerRadius = 8.0f, float opacity = 0.1f);
    
    void drawModernLED(juce::Graphics& g, juce::Rectangle<float> bounds,
                      bool isOn, juce::Colour color = juce::Colour(ColorScheme::success));
    
    void drawSoftShadow(juce::Graphics& g, juce::Rectangle<float> bounds, 
                       float radius = 10.0f, float opacity = 0.3f);
    
    // Font management
    juce::Font getModernFont(float height, bool isBold = false);
    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    
private:
    void drawKnobIndicator(juce::Graphics& g, juce::Rectangle<float> bounds, 
                          float angle, juce::Colour color);
    void drawKnobTrack(juce::Graphics& g, juce::Rectangle<float> bounds,
                      float startAngle, float endAngle, float value);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArtisticLookAndFeel)
};