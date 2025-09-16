#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Testing with ALL 6 parameter attachments
 * To see if multiple attachments cause crashes
 */
class PluginEditorWithAllAttachments : public juce::AudioProcessorEditor
{
public:
    PluginEditorWithAllAttachments(ChimeraAudioProcessor&);
    ~PluginEditorWithAllAttachments() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    
    // 6 engine selectors
    std::array<juce::ComboBox, 6> engineSelectors;
    std::array<juce::Label, 6> slotLabels;
    
    // Attachments for ALL 6 slots
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 6> engineAttachments;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWithAllAttachments)
};