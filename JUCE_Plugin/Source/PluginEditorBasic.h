#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * ABSOLUTE MINIMAL UI - Just a label, nothing else
 * NO attachments, NO listeners, NO dynamic behavior
 */
class PluginEditorBasic : public juce::AudioProcessorEditor
{
public:
    PluginEditorBasic(ChimeraAudioProcessor&);
    ~PluginEditorBasic() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
private:
    juce::Label infoLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorBasic)
};