#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SlotComponentFixed.h"

// Minimal test editor for incremental debugging
class TestEditorIncremental : public juce::AudioProcessorEditor
{
public:
    TestEditorIncremental(ChimeraAudioProcessor& p);
    ~TestEditorIncremental() override;
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    // Static components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Component> masterPanel;
    
    // STEP 3: Add SlotComponent array (FIXED version)
    std::array<std::unique_ptr<SlotComponentFixed>, 6> slots;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestEditorIncremental)
};