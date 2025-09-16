#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Basic UI with engine selectors but NO attachments
 * Testing if ComboBoxes alone cause issues
 */
class PluginEditorBasicWithSelectors : public juce::AudioProcessorEditor
{
public:
    PluginEditorBasicWithSelectors(ChimeraAudioProcessor&);
    ~PluginEditorBasicWithSelectors() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    juce::Label titleLabel;
    juce::Label statusLabel;
    
    // 6 engine selectors - NO attachments yet
    std::array<juce::ComboBox, 6> engineSelectors;
    std::array<juce::Label, 6> slotLabels;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorBasicWithSelectors)
};