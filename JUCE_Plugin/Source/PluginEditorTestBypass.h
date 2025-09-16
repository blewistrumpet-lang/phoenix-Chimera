#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Testing SimpleFinal + Bypass buttons
 */
class PluginEditorTestBypass : public juce::AudioProcessorEditor
{
public:
    PluginEditorTestBypass(ChimeraAudioProcessor&);
    ~PluginEditorTestBypass() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    
    // 6 engine selectors
    std::array<juce::ComboBox, 6> engineSelectors;
    std::array<juce::Label, 6> slotLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 6> engineAttachments;
    
    // ADD: Bypass buttons
    std::array<juce::ToggleButton, 6> bypassButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 6> bypassAttachments;
    
    // ADD: Slot mix sliders
    std::array<juce::Slider, 6> slotMixSliders;
    std::array<juce::Label, 6> slotMixLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 6> slotMixAttachments;
    
    // Master controls
    juce::Slider masterGainSlider;
    juce::Label masterGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    
    juce::Slider masterMixSlider;
    juce::Label masterMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;
    
    // Static parameter sliders for slot 1
    static constexpr int NUM_STATIC_PARAMS = 4;
    std::array<juce::Slider, NUM_STATIC_PARAMS> slot1Sliders;
    std::array<juce::Label, NUM_STATIC_PARAMS> slot1Labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, NUM_STATIC_PARAMS> slot1Attachments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorTestBypass)
};