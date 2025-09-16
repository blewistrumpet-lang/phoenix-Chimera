#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Testing with just ONE parameter attachment
 * To isolate the crash issue
 */
class PluginEditorWithOneAttachment : public juce::AudioProcessorEditor
{
public:
    PluginEditorWithOneAttachment(ChimeraAudioProcessor&);
    ~PluginEditorWithOneAttachment() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    
    // 6 engine selectors - but only ONE will have attachment
    std::array<juce::ComboBox, 6> engineSelectors;
    std::array<juce::Label, 6> slotLabels;
    
    // Single attachment for testing
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slot0Attachment;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWithOneAttachment)
};