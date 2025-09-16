#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NexusLookAndFeelDynamic.h"
#include "EngineTypes.h"
#include <memory>
#include <vector>

/**
 * PluginEditorNexusDynamic - Final UI with live engine parameter querying
 * 
 * This implementation queries live engine instances directly,
 * eliminating all dependency on static parameter databases.
 */
class PluginEditorNexusDynamic : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    PluginEditorNexusDynamic(ChimeraAudioProcessor&);
    ~PluginEditorNexusDynamic() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void parentHierarchyChanged() override;
    void visibilityChanged() override;

private:
    ChimeraAudioProcessor& audioProcessor;
    std::unique_ptr<NexusLookAndFeelDynamic> nexusLnF;
    
    // Engine Slot with dynamic parameter querying
    class DynamicEngineSlot : public juce::Component
    {
    public:
        DynamicEngineSlot(ChimeraAudioProcessor& p, int slotIndex);
        ~DynamicEngineSlot();
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        void updateParametersFromLiveEngine();
        
    private:
        ChimeraAudioProcessor& processor;
        const int slot;
        
        juce::ComboBox engineSelector;
        juce::ToggleButton bypassButton;
        juce::Label slotLabel;
        
        // Dynamic parameter controls
        struct DynamicParam {
            juce::String name;
            bool isToggle;
            std::unique_ptr<juce::Component> control;
            std::unique_ptr<juce::Label> label;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttach;
            std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttach;
        };
        
        std::vector<DynamicParam> dynamicParams;
        
        // Attachments
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        
        // Activity visualization
        float activityLevel = 0.0f;
        juce::Colour glowColor;
        
        // Helper to determine toggle parameters
        bool shouldBeToggle(const juce::String& paramName) const;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicEngineSlot)
    };
    
    // AI Command Center
    class AICommandPanel : public juce::Component
    {
    public:
        AICommandPanel(ChimeraAudioProcessor& p);
        void paint(juce::Graphics& g) override;
        void resized() override;
        
    private:
        ChimeraAudioProcessor& processor;
        juce::Label titleLabel{"AI", "TRINITY AI SYSTEM"};
        juce::TextEditor promptInput;
        juce::TextButton executeBtn{"EXECUTE"};
        juce::TextButton enhanceBtn{"ENHANCE"};
        juce::TextButton randomBtn{"RANDOMIZE"};
        
        // Pipeline status lights
        std::array<juce::Component, 4> statusLights;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AICommandPanel)
    };
    
    // Master controls
    class MasterPanel : public juce::Component
    {
    public:
        MasterPanel(ChimeraAudioProcessor& p);
        void paint(juce::Graphics& g) override;
        void resized() override;
        void updateMeters();
        
    private:
        ChimeraAudioProcessor& processor;
        
        // Level meters
        class LevelMeter : public juce::Component
        {
        public:
            void paint(juce::Graphics& g) override;
            void setLevel(float newLevel);
        private:
            float level = 0.0f;
            float peakLevel = 0.0f;
        };
        
        LevelMeter inputMeter;
        LevelMeter outputMeter;
        
        juce::Slider inputGain;
        juce::Slider outputGain;
        juce::Slider mixControl;
        
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttach;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterPanel)
    };
    
    // UI Components
    std::unique_ptr<AICommandPanel> aiPanel;
    std::array<std::unique_ptr<DynamicEngineSlot>, 6> engineSlots;
    std::unique_ptr<MasterPanel> masterPanel;
    
    // Background animation state
    float scanlineY = 0.0f;
    float glowPulse = 0.0f;
    
    // Background rendering
    void drawCarbonFiberBackground(juce::Graphics& g);
    void drawHolographicOverlay(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorNexusDynamic)
};