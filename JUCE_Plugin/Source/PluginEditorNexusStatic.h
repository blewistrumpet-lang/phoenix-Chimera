#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SlotComponent.h"
#include "TrinityNetworkClient.h"
#include "TrinityTextBox.h"

// Forward declaration
class NexusLookAndFeel;

/**
 * PluginEditorNexusStatic - STABLE UI with NO dynamic component creation
 * 
 * All components are created ONCE in constructor.
 * Only content and visibility change during runtime.
 * NOW INCLUDES TRINITY AI INTEGRATION with glowing text input.
 */
class PluginEditorNexusStatic : public juce::AudioProcessorEditor,
                                private juce::AudioProcessorValueTreeState::Listener,
                                private juce::Timer,
                                public TrinityNetworkClient::Listener
{
public:
    PluginEditorNexusStatic(ChimeraAudioProcessor&);
    ~PluginEditorNexusStatic() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
    // Parameter listener for engine changes
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    
    // Timer for deferred initialization
    void timerCallback() override;
    
    // Trinity network client listener
    void trinityConnectionStateChanged(TrinityNetworkClient::ConnectionState newState) override;
    void trinityMessageReceived(const TrinityNetworkClient::TrinityResponse& response) override;
    void trinitySessionStarted(const juce::String& sessionId) override;
    void trinitySessionEnded(const juce::String& sessionId) override;
    void trinityError(const juce::String& error) override;
    
    // Trinity AI actions
    void initializeTrinityAI();
    void sendPluginStateToTrinity();
    void applyTrinityParameterSuggestions(const juce::Array<juce::var>& suggestions);
    void applyTrinityPreset(const juce::var& presetData);
    void applyTrinityPresetFromParameters(const juce::var& presetData);
    
private:
    ChimeraAudioProcessor& audioProcessor;
    
    // Fixed UI components - created ONCE
    juce::Label titleLabel;
    juce::Label presetNameLabel;
    juce::String currentPresetName = "Init";
    
    // Trinity AI components
    std::unique_ptr<TrinityNetworkClient> trinityClient;
    std::unique_ptr<TrinityTextBox> trinityTextBox;
    
    // Fixed array of 6 slot components
    std::array<std::unique_ptr<SlotComponent>, 6> slots;
    
    // Attachments for each slot
    struct SlotAttachments {
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment;
        std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 15> paramAttachments;
    };
    std::array<SlotAttachments, 6> slotAttachments;
    
    // Helper methods
    void initializeSlot(int slotIndex);
    void initializeSlotSafe(int slotIndex);  // Version without ComboBox attachments
    void createComboBoxAttachments();  // Deferred ComboBox attachment creation
    void updateSlotEngine(int slotIndex);
    void populateEngineSelector(int slotIndex);
    
    bool comboBoxAttachmentsCreated = false;
    
    // Tactile Futurism look and feel
    std::unique_ptr<NexusLookAndFeel> nexusLookAndFeel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorNexusStatic)
};