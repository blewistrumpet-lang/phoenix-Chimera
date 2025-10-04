#pragma once
#include <JuceHeader.h>
#include "EngineBase.h"
#include "ParameterControlMap.h"

class SlotComponentFixed : public juce::Component
{
public:
    enum ControlType {
        CONTROL_ROTARY = 0,
        CONTROL_TOGGLE,
        CONTROL_ROTARY,
        CONTROL_STEPPED
    };
    
    SlotComponentFixed(int slotIndex);
    ~SlotComponentFixed() override;
    
    // CRITICAL: Defer initialization until after construction
    void initializeComponents();
    
    void update(EngineBase* currentEngine, int engineId);
    
    // Accessors for attachments
    juce::ComboBox& getEngineSelector() { return engineSelector; }
    juce::ToggleButton& getBypassButton() { return bypassButton; }
    juce::Slider* getSlider(int index) { 
        return (index >= 0 && index < 15) ? &sliders[index] : nullptr; 
    }
    juce::Component* getParameterControl(int index);
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    int slotNumber;
    
    // Core components - stack allocated is fine
    juce::Label slotLabel;
    juce::ComboBox engineSelector;
    juce::ToggleButton bypassButton;
    
    // Parameter controls - stack allocated
    std::array<juce::Slider, 15> sliders;
    std::array<juce::ToggleButton, 15> toggles;
    std::array<juce::Label, 15> labels;
    std::array<ControlType, 15> controlTypes;
    
    // Track current visibility state
    int visibleParamCount = 0;
    int currentEngineId = -1;
    bool componentsInitialized = false;
    
    // Parameter control mapping
    ControlType getControlTypeForParameter(int engineId, int paramIndex);
    void configureControlForType(int paramIndex, ControlType type);
    
    // Dynamic layout
    void layoutParametersGrid(juce::Rectangle<int> bounds);
    int calculateOptimalColumns(int paramCount);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotComponentFixed)
};