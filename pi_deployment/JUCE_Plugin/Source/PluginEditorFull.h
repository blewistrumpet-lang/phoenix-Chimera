#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Full-featured UI with preset management, meters, and all controls
 */

// Custom level meter component
class SimpleLevelMeter : public juce::Component, public juce::Timer
{
public:
    SimpleLevelMeter()
    {
        // DO NOT start timer in constructor - wait for parentHierarchyChanged
    }
    
    ~SimpleLevelMeter() override
    {
        stopTimer();
    }
    
    void parentHierarchyChanged() override
    {
        // Only start timer when we have a valid parent window
        if (getParentComponent() != nullptr && !isTimerRunning())
        {
            startTimerHz(30);
        }
        else if (getParentComponent() == nullptr && isTimerRunning())
        {
            stopTimer();
        }
    }
    
    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();
        
        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(bounds, 2.0f);
        
        // Meter fill
        float fillHeight = bounds.getHeight() * displayLevel;
        auto fillBounds = bounds.removeFromBottom(fillHeight);
        
        // Gradient based on level
        if (displayLevel > 0.9f)
            g.setColour(juce::Colours::red);
        else if (displayLevel > 0.7f)
            g.setColour(juce::Colours::orange);
        else
            g.setColour(juce::Colours::green);
            
        g.fillRoundedRectangle(fillBounds, 2.0f);
    }
    
    void timerCallback() override
    {
        // Smooth decay
        displayLevel = displayLevel * 0.85f;
        if (displayLevel < 0.01f) displayLevel = 0.0f;
        repaint();
    }
    
    void setLevel(float newLevel)
    {
        if (newLevel > displayLevel)
            displayLevel = newLevel;
    }
    
private:
    float displayLevel = 0.0f;
};

class PluginEditorFull : public juce::AudioProcessorEditor,
                         public juce::Button::Listener,
                         public juce::ComboBox::Listener,
                         public juce::Timer
{
public:
    PluginEditorFull(ChimeraAudioProcessor&);
    ~PluginEditorFull() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void timerCallback() override;  // For updating parameter names
    
private:
    void loadPreset();
    void savePreset();
    void initializePreset();
    void showAbout();
    void launchTrinityDialog();
    void sendTrinityRequest(const juce::String& prompt);
    void updateParameterNamesForSlot(int slot);
    
    ChimeraAudioProcessor& audioProcessor;
    
    // Header section
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::TextButton logoButton;
    
    // Preset management
    juce::ComboBox presetCombo;
    juce::TextButton savePresetButton;
    juce::TextButton loadPresetButton;
    juce::TextButton prevPresetButton;
    juce::TextButton nextPresetButton;
    juce::TextButton initButton;
    juce::TextButton compareButton;
    juce::Label presetLabel;
    
    // Trinity AI Server
    juce::TextButton aiButton;
    juce::Label aiStatusLabel;
    std::unique_ptr<juce::DialogWindow> trinityDialog;
    
    // Meters
    SimpleLevelMeter inputMeterL;
    SimpleLevelMeter inputMeterR;
    SimpleLevelMeter outputMeterL;
    SimpleLevelMeter outputMeterR;
    juce::Label inputLabel;
    juce::Label outputLabel;
    
    // 6 slots with all controls
    std::array<juce::ComboBox, 6> engineSelectors;
    std::array<juce::Label, 6> slotLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>, 6> engineAttachments;
    
    std::array<juce::ToggleButton, 6> bypassButtons;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>, 6> bypassAttachments;
    
    std::array<juce::ToggleButton, 6> soloButtons;
    std::array<juce::ToggleButton, 6> muteButtons;
    
    std::array<juce::Slider, 6> slotMixSliders;
    std::array<juce::Label, 6> slotMixLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 6> slotMixAttachments;
    
    // Parameters for all 6 slots
    static constexpr int PARAMS_PER_SLOT = 8;  // Increased to 8
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
    
    // Additional controls
    juce::TextButton panicButton;
    juce::TextButton settingsButton;
    juce::Slider qualitySlider;
    juce::Label qualityLabel;
    
    // State
    bool compareMode = false;
    int currentPresetIndex = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorFull)
};