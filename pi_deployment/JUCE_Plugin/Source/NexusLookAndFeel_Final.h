#pragma once
#include <JuceHeader.h>

/**
 * NEXUS LOOK AND FEEL - TACTILE FUTURISM / INDUSTRIAL CYBERPUNK
 * 
 * Design Philosophy: Star Wars module meets Blade Runner interface
 * This is the authoritative aesthetic implementation for Project Chimera Phoenix
 * 
 * Color Palette:
 * - Base: Deep space blacks (#111827, #1F2937)
 * - Primary: Holographic neon cyan (#00ffcc)
 * - Secondary: Warning magenta (#ff006e)
 * - Text: Clean off-white (#E5E7EB)
 */
class NexusLookAndFeel_Final : public juce::LookAndFeel_V4
{
public:
    NexusLookAndFeel_Final();
    ~NexusLookAndFeel_Final() override;
    
    // Core Color Definitions - Exact specs from mandate
    struct Colors {
        static constexpr uint32_t baseBlack = 0xff111827;        // Deep space black
        static constexpr uint32_t baseDark = 0xff1F2937;         // Dark charcoal
        static constexpr uint32_t primaryCyan = 0xff00ffcc;      // Holographic neon cyan
        static constexpr uint32_t secondaryMagenta = 0xffff006e; // Hot warning magenta
        static constexpr uint32_t textPrimary = 0xffE5E7EB;      // Clean off-white
        static constexpr uint32_t textSecondary = 0xff9CA3AF;    // Muted text
        static constexpr uint32_t panelGlow = 0x2000ffcc;        // Cyan glow overlay
        static constexpr uint32_t shadowDeep = 0x80000000;       // Deep shadow
    };
    
    // Industrial Rotary Encoder - Heavy machined appearance
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    // Tactical Toggle Switch
    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;
    
    // Clean rectangular button with glow
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted,
                             bool shouldDrawButtonAsDown) override;
    
    // Dropdown with futuristic styling
    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                     int buttonX, int buttonY, int buttonW, int buttonH,
                     juce::ComboBox& box) override;
    
    // Text editor for AI input
    void fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                 juce::TextEditor& textEditor) override;
    
    // Label rendering
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
    // Custom visual effects
    void drawCarbonFiberTexture(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawHolographicPanel(juce::Graphics& g, juce::Rectangle<float> bounds, bool isActive);
    void drawScanlineEffect(juce::Graphics& g, juce::Rectangle<float> bounds, float phase);
    void drawNeonGlow(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color, float intensity);
    void drawCornerBrackets(juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour color);
    
    // Font management
    juce::Font getTacticalFont(float height, bool bold = false);
    juce::Font getMonospacedFont(float height);
    
    // Animation support
    float getScanlinePhase() const { return scanlinePhase; }
    void updateAnimations();
    
private:
    float scanlinePhase = 0.0f;
    
    // Helper to create industrial knob grip texture
    void drawKnobGrip(juce::Graphics& g, juce::Point<float> center, float radius);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusLookAndFeel_Final)
};