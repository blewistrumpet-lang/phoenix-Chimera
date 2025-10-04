#pragma once

#include <JuceHeader.h>

/**
 * NexusLookAndFeelDynamic - Tactile Futurism / Industrial Cyberpunk aesthetic
 * 
 * Implements the visual design for Chimera Phoenix with:
 * - Industrial rotary encoders with cyan glow
 * - Tactical toggle switches with magenta accents
 * - Carbon fiber textures
 * - Holographic panel effects
 */
class NexusLookAndFeelDynamic : public juce::LookAndFeel_V4
{
public:
    NexusLookAndFeelDynamic();
    ~NexusLookAndFeelDynamic() override;
    
    // Color scheme constants
    struct Colors
    {
        static constexpr uint32_t baseBlack = 0xff111827;      // Deep space black
        static constexpr uint32_t baseDark = 0xff1F2937;       // Dark charcoal
        static constexpr uint32_t primaryCyan = 0xff00ffcc;    // Holographic cyan
        static constexpr uint32_t secondaryMagenta = 0xffff006e; // Hot magenta
        static constexpr uint32_t textPrimary = 0xffE5E7EB;    // Off-white
        static constexpr uint32_t textSecondary = 0xff9CA3AF;  // Gray
        static constexpr uint32_t warning = 0xffffcc00;        // Warning yellow
    };
    
    // Rotary slider (industrial encoder style)
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPosProportional, float rotaryStartAngle,
                         float rotaryEndAngle, juce::Slider& slider) override;
    
    // Toggle button (tactical switch style)
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    
    // ComboBox (dropdown with holographic style)
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& comboBox) override;
    
    // Text button (glowing tactical button)
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;
    
    // Text editor (holographic input field)
    void fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                 juce::TextEditor& textEditor) override;
    
    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                              juce::TextEditor& textEditor) override;
    
    // Label
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    // Popup menu (holographic dropdown)
    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;
    
    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                          bool isSeparator, bool isActive, bool isHighlighted,
                          bool isTicked, bool hasSubMenu, const juce::String& text,
                          const juce::String& shortcutKeyText,
                          const juce::Drawable* icon, const juce::Colour* textColour) override;
    
private:
    // Helper methods for complex drawing
    void drawIndustrialKnob(juce::Graphics& g, juce::Rectangle<float> bounds,
                           float angle, bool isActive);
    
    void drawCarbonFiberTexture(juce::Graphics& g, juce::Rectangle<float> bounds);
    
    void drawHolographicGlow(juce::Graphics& g, juce::Rectangle<float> bounds,
                            juce::Colour glowColor, float intensity);
    
    void drawTacticalSwitch(juce::Graphics& g, juce::Rectangle<float> bounds,
                           bool isOn, bool isHighlighted);
    
    // Fonts
    juce::Font getIndustrialFont(float height);
    juce::Font getTacticalFont(float height);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusLookAndFeelDynamic)
};