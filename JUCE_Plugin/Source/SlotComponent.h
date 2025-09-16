#pragma once

#include <JuceHeader.h>
#include "EngineBase.h"
#include "EngineTypes.h"
#include "ParameterControlMap.h"
#include "ParameterFormatter.h"

/**
 * SlotComponent - Advanced UI with semantic control types
 * 
 * Supports up to 15 parameters with dynamic control types:
 * - Rotary encoders for continuous parameters
 * - Toggle buttons for on/off parameters
 * - Stepped encoders for discrete choices
 * - All continuous parameters use rotary encoders
 */
class SlotComponent : public juce::Component,
                      public juce::Slider::Listener
{
public:
    SlotComponent(int slotIndex);
    ~SlotComponent() override;
    
    /**
     * Updates the visibility and control types based on engine
     */
    void update(EngineBase* currentEngine, int engineId);
    
    // Public access for attachments
    juce::ComboBox& getEngineSelector() { return engineSelector; }
    juce::ToggleButton& getBypassButton() { return bypassButton; }
    juce::ToggleButton& getSoloButton() { return soloButton; }
    
    // Generic parameter control access
    juce::Component* getParameterControl(int index);
    juce::Slider* getSlider(int index) { 
        if (index >= 0 && index < 15) 
            return &sliders[index]; 
        return nullptr;
    }
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
    // Slider listener
    void sliderValueChanged(juce::Slider* slider) override;
    
    // Get required height for current parameter count
    int getRequiredHeight() const;
    
private:
    const int slotNumber;
    
    // Fixed UI components - created ONCE, never destroyed
    juce::Label slotLabel;
    juce::ComboBox engineSelector;
    juce::ToggleButton bypassButton;
    juce::ToggleButton soloButton;
    
    // Control types enum - simplified to encoders and buttons only
    enum ControlType {
        CONTROL_ROTARY,
        CONTROL_TOGGLE,
        CONTROL_STEPPED
    };
    
    // Fixed arrays of 15 parameters with multiple control types
    std::array<juce::Slider, 15> sliders;        // For rotary/slider/stepped
    std::array<juce::ToggleButton, 15> toggles;  // For toggle parameters
    std::array<juce::Label, 15> labels;
    std::array<juce::Label, 15> valueLabels;     // Parameter value displays
    std::array<ControlType, 15> controlTypes;
    
    // Track current visibility state
    int visibleParamCount = 0;
    int currentEngineId = -1;
    juce::String currentEngineName;
    EngineBase* currentEngine = nullptr;  // Keep reference for semantic rules
    
    // Design system constants - Unified sizing for visual consistency
    static constexpr int KNOB_SIZE = 38;            // Unified size for all encoders
    static constexpr int TOGGLE_HEIGHT = 20;        // Height for toggle buttons
    static constexpr int LABEL_WIDTH = 70;          // Wider labels to prevent truncation
    
    // Layout constants
    static constexpr int LABEL_HEIGHT = 14;         // Readable label text
    static constexpr int VALUE_HEIGHT = 10;         // Smaller value display
    
    // Parameter control mapping
    ControlType getControlTypeForParameter(int engineId, int paramIndex);
    void configureControlForType(int paramIndex, ControlType type);
    
    // Visual hierarchy - REMOVED for consistent sizing
    // All encoders now use the same KNOB_SIZE for professional appearance
    
    // Dynamic layout
    void layoutParametersGrid(juce::Rectangle<int> bounds);
    int calculateOptimalColumns(int paramCount);
    
    // Slot state
    bool isCollapsed = false;
    int collapsedHeight = 60;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotComponent)
};