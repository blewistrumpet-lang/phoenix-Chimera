#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Complete UI Implementation
 * All slots with parameters, bypass, and full controls
 */

class SlotPanel : public juce::Component
{
public:
    SlotPanel(ChimeraAudioProcessor& p, int slotIndex);
    ~SlotPanel();
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& processor;
    int slot;
    
    // Slot controls
    juce::Label slotLabel;
    juce::ComboBox engineSelector;
    juce::ToggleButton bypassButton;
    juce::Slider mixSlider;
    
    // Parameter controls (static, always 8 visible)
    static constexpr int PARAMS_PER_SLOT = 8;
    std::array<juce::Slider, PARAMS_PER_SLOT> paramSliders;
    std::array<juce::Label, PARAMS_PER_SLOT> paramLabels;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, PARAMS_PER_SLOT> paramAttachments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SlotPanel)
};

class PluginEditorComplete : public juce::AudioProcessorEditor
{
public:
    PluginEditorComplete(ChimeraAudioProcessor&);
    ~PluginEditorComplete() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    // Header
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::TextButton presetButton;
    juce::TextButton saveButton;
    
    // Slot panels
    std::array<std::unique_ptr<SlotPanel>, 6> slotPanels;
    
    // Master section
    juce::GroupComponent masterGroup;
    juce::Slider masterGainSlider;
    juce::Label masterGainLabel;
    juce::Slider masterMixSlider;
    juce::Label masterMixLabel;
    juce::ToggleButton masterBypassButton;
    
    // Master attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> masterBypassAttachment;
    
    // Visual style
    juce::LookAndFeel_V4 lookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorComplete)
};