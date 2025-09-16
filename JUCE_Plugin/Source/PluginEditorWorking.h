#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Working UI with all proven features
 * Engine selectors + Bypass + Mix + Parameters for all slots
 */
class PluginEditorWorking : public juce::AudioProcessorEditor
{
public:
    PluginEditorWorking(ChimeraAudioProcessor&);
    ~PluginEditorWorking() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    
    // 6 slots with all controls
    std::array<juce::ComboBox, 6> engineSelectors;
    std::array<juce::Label, 6> slotLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 6> engineAttachments;
    
    std::array<juce::ToggleButton, 6> bypassButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 6> bypassAttachments;
    
    std::array<juce::Slider, 6> slotMixSliders;
    std::array<juce::Label, 6> slotMixLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 6> slotMixAttachments;
    
    // Parameters for all 6 slots (4 params each)
    static constexpr int PARAMS_PER_SLOT = 4;
    std::array<std::array<juce::Slider, PARAMS_PER_SLOT>, 6> slotParamSliders;
    std::array<std::array<juce::Label, PARAMS_PER_SLOT>, 6> slotParamLabels;
    std::array<std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, PARAMS_PER_SLOT>, 6> slotParamAttachments;
    
    // Master controls
    juce::Slider masterGainSlider;
    juce::Label masterGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    
    juce::Slider masterMixSlider;
    juce::Label masterMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;
    
    juce::ToggleButton masterBypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> masterBypassAttachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWorking)
};