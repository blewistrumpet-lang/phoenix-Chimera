#pragma once

#include <JuceHeader.h>
#include "SkunkworksLookAndFeel.h"

class MilitaryKnob : public juce::Slider
{
public:
    MilitaryKnob(const juce::String& paramName = "");
    
    void paint(juce::Graphics& g) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    
    void setLEDColor(juce::Colour color) { ledColor = color; repaint(); }
    void setDisplayValue(const juce::String& value) { displayValue = value; repaint(); }
    
private:
    juce::Colour ledColor{SkunkworksLookAndFeel::ColorScheme::greenLED};
    juce::String displayValue;
    juce::String parameterName;
    bool isHovering{false};
    
    // Animation
    float glowIntensity{0.0f};
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MilitaryKnob)
};

//==============================================================================

class ChimeraSlotComponent : public juce::Component,
                            private juce::AudioProcessorValueTreeState::Listener,
                            private juce::Timer
{
public:
    enum SlotStyle {
        StyleDynamics,      // Blue metal with VU meters
        StyleDistortion,    // Red/orange with heat warnings
        StyleModulation,    // Green with oscilloscope
        StyleTimeBased,     // Purple with delay readout
        StyleSpectral,      // Cyan with spectrum analyzer
        StyleUtility,       // Grey standard panel
        StyleEmpty          // Dark inactive panel
    };
    
    ChimeraSlotComponent(int slotNumber, 
                        juce::AudioProcessorValueTreeState& apvts,
                        std::function<void(int)> onEngineChanged = nullptr);
    ~ChimeraSlotComponent() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Slot management
    void setEngine(int engineIndex);
    void setSlotStyle(SlotStyle style);
    void updateParameterVisibility();
    
    // Visual feedback
    void setProcessingLevel(float level); // 0.0 to 1.0 for activity meter
    void setWarningState(bool hasWarning, const juce::String& message = "");
    void pulseActivity(); // Brief visual pulse when processing
    
    // Get current state
    int getCurrentEngine() const { return currentEngineIndex; }
    bool isBypassed() const { return bypassButton.getToggleState(); }
    
private:
    // Core properties
    int slotNumber;
    int currentEngineIndex{0};
    SlotStyle currentStyle{StyleEmpty};
    juce::AudioProcessorValueTreeState& valueTreeState;
    std::function<void(int)> engineChangedCallback;
    
    // UI Components
    juce::Label slotLabel;
    juce::Label engineNameLabel;
    juce::ComboBox engineSelector;
    juce::ToggleButton bypassButton{"BYPASS"};
    juce::TextButton menuButton{"≡"};
    
    // Parameter controls
    std::vector<std::unique_ptr<MilitaryKnob>> paramKnobs;
    std::vector<std::unique_ptr<juce::Label>> paramLabels;
    
    // Visual elements
    struct ActivityMeter : public juce::Component {
        void paint(juce::Graphics& g) override;
        void setLevel(float newLevel) { level = newLevel; repaint(); }
        float getLevel() const { return level; }
        float level{0.0f};  // Made public for access
    };
    
    ActivityMeter activityMeter;
    juce::Label warningLabel;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> paramAttachments;
    
    // Animation properties
    float pulseAnimation{0.0f};
    float warningFlash{0.0f};
    bool isWarning{false};
    
    // Timer callback for animations
    void timerCallback() override;
    
    // AudioProcessorValueTreeState::Listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    // Helper methods
    void drawSlotBackground(juce::Graphics& g);
    void drawStatusLEDs(juce::Graphics& g);
    void layoutControls();
    juce::Colour getStyleColor() const;
    juce::String getEngineCategory(int engineIndex) const;
    
    // Style-specific drawing
    void drawDynamicsStyle(juce::Graphics& g);
    void drawDistortionStyle(juce::Graphics& g);
    void drawModulationStyle(juce::Graphics& g);
    void drawTimeBasedStyle(juce::Graphics& g);
    void drawSpectralStyle(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChimeraSlotComponent)
};