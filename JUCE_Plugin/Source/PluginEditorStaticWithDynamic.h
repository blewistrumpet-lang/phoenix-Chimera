#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Static UI with Dynamic Content
 * Fixed component hierarchy with visibility/content changes only
 * This is the working implementation following the proven pattern
 */

class SlotComponentStatic : public juce::Component
{
public:
    SlotComponentStatic(ChimeraAudioProcessor& p, int slotIndex);
    ~SlotComponentStatic();
    
    void resized() override;
    void updateParameterDisplay();
    
private:
    ChimeraAudioProcessor& processor;
    int slot;
    
    juce::Label slotLabel;
    juce::ComboBox engineSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
    
    // Fixed 15 parameter controls - created once, visibility toggled
    static constexpr int MAX_PARAMS = 15;
    std::array<juce::Slider, MAX_PARAMS> paramSliders;
    std::array<juce::Label, MAX_PARAMS> paramLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, MAX_PARAMS> paramAttachments;
    
    int visibleParamCount = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotComponentStatic)
};

class PluginEditorStaticWithDynamic : public juce::AudioProcessorEditor
{
public:
    PluginEditorStaticWithDynamic(ChimeraAudioProcessor&);
    ~PluginEditorStaticWithDynamic() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    
    // Fixed 6 slot components
    std::array<std::unique_ptr<SlotComponentStatic>, 6> slots;
    
    // Master controls
    juce::Slider masterGainSlider;
    juce::Label masterGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    
    juce::Slider masterMixSlider;
    juce::Label masterMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorStaticWithDynamic)
};