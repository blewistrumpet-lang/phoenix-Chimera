#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
    Minimal Dynamic Nexus UI for testing - queries live engines safely
*/
class PluginEditorNexusDynamicMinimal : public juce::AudioProcessorEditor
{
public:
    PluginEditorNexusDynamicMinimal(ChimeraAudioProcessor&);
    ~PluginEditorNexusDynamicMinimal() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    ChimeraAudioProcessor& audioProcessor;
    
    // Simple test controls
    juce::Label titleLabel;
    juce::ComboBox slot1EngineSelector;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> slot1Attachment;
    
    // Parameter displays
    juce::Label paramInfoLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorNexusDynamicMinimal)
};