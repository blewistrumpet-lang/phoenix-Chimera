#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
    Safe Dynamic Nexus UI - No timers, simplified components
*/
class PluginEditorNexusDynamicSafe : public juce::AudioProcessorEditor
{
public:
    PluginEditorNexusDynamicSafe(ChimeraAudioProcessor&);
    ~PluginEditorNexusDynamicSafe() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ChimeraAudioProcessor& audioProcessor;
    
    // Simple slot controls - no nested classes
    struct SlotControls {
        juce::Label label;
        juce::ComboBox engineSelector;
        juce::ToggleButton bypassButton;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        
        // Dynamic parameter controls
        std::vector<std::unique_ptr<juce::Slider>> sliders;
        std::vector<std::unique_ptr<juce::Label>> labels;
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    };
    
    std::array<SlotControls, 6> slots;
    
    // Title
    juce::Label titleLabel;
    
    // Helper methods
    void initializeSlot(int slotIndex);
    void updateSlotParameters(int slotIndex);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorNexusDynamicSafe)
};