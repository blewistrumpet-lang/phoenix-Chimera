#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Simple working UI - just selectors and master controls
 * No dynamic updates, no parameter sliders, no complex components
 */
class PluginEditorSimpleFinal : public juce::AudioProcessorEditor
{
public:
    PluginEditorSimpleFinal(ChimeraAudioProcessor&);
    ~PluginEditorSimpleFinal() override;
    
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
    
    // Master controls
    juce::Slider masterGainSlider;
    juce::Label masterGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    
    juce::Slider masterMixSlider;
    juce::Label masterMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;
    
    // Static parameter sliders for slot 1 (always visible, fixed labels)
    static constexpr int NUM_STATIC_PARAMS = 4;
    std::array<juce::Slider, NUM_STATIC_PARAMS> slot1Sliders;
    std::array<juce::Label, NUM_STATIC_PARAMS> slot1Labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, NUM_STATIC_PARAMS> slot1Attachments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorSimpleFinal)
};