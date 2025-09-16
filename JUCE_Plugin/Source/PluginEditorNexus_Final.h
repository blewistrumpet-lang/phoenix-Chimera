#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NexusLookAndFeel_Final.h"
#include "GeneratedParameterDatabase.h"

/**
 * NEXUS PLUGIN EDITOR - FINAL IMPLEMENTATION
 * 
 * Two-column layout:
 * - Left: AI Command Center
 * - Right: 6-Slot Rack with dynamic parameters
 * 
 * This implementation perfectly reflects the GeneratedParameterDatabase
 */
class PluginEditorNexus_Final : public juce::AudioProcessorEditor,
                                private juce::Timer
{
public:
    PluginEditorNexus_Final(ChimeraAudioProcessor&);
    ~PluginEditorNexus_Final() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    
private:
    //==============================================================================
    // AI COMMAND CENTER (Left Column)
    //==============================================================================
    class AICommandCenter : public juce::Component
    {
    public:
        AICommandCenter();
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // Components
        juce::Label titleLabel{"title", "AI COMMAND CENTER"};
        juce::Label statusLabel{"status", "SYSTEM READY"};
        
        juce::TextEditor promptInput;
        juce::TextButton executeButton{"EXECUTE"};
        juce::TextButton enhanceButton{"ENHANCE"};
        juce::TextButton randomizeButton{"RANDOMIZE"};
        
        // Trinity Pipeline status
        class PipelineStatus : public juce::Component
        {
        public:
            void paint(juce::Graphics& g) override;
            void setStage(int stage, bool active);
            
        private:
            bool stages[4] = {false, false, false, false};
            const char* stageNames[4] = {"VISIONARY", "ORACLE", "CALCULATOR", "ALCHEMIST"};
        };
        
        PipelineStatus pipelineStatus;
        
        // Callback
        std::function<void(const juce::String&)> onPromptExecute;
        
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AICommandCenter)
    };
    
    //==============================================================================
    // ENGINE SLOT (Dynamic Parameter System)
    //==============================================================================
    class EngineSlot : public juce::Component
    {
    public:
        EngineSlot(ChimeraAudioProcessor& proc, int slotIndex);
        ~EngineSlot();
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // Core functionality
        void updateParametersFromDatabase();
        void setActivity(float level) { activityLevel = level; repaint(); }
        
    private:
        ChimeraAudioProcessor& processor;
        int slot;
        float activityLevel = 0.0f;
        
        // UI Components
        juce::Label slotLabel;
        juce::ComboBox engineSelector;
        juce::ToggleButton bypassButton{"BYPASS"};
        
        // Dynamic parameter storage
        struct DynamicParameter
        {
            std::unique_ptr<juce::Component> control;  // Either Slider or ToggleButton
            std::unique_ptr<juce::Label> label;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
            std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;
            bool isToggle = false;
            juce::String name;
        };
        
        std::vector<DynamicParameter> parameters;
        
        // Value tree attachments
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        
        // Database query
        void createParametersForEngine(int engineId);
        const ChimeraParameters::ParameterInfo* getParameterInfo(int engineId, int paramIndex);
        bool isParameterToggle(const juce::String& name);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineSlot)
    };
    
    //==============================================================================
    // MASTER SECTION (Bottom bar)
    //==============================================================================
    class MasterSection : public juce::Component
    {
    public:
        MasterSection(juce::AudioProcessorValueTreeState& apvts);
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        void updateMeters(float input, float output);
        
    private:
        // VU Meter
        class VUMeter : public juce::Component
        {
        public:
            void paint(juce::Graphics& g) override;
            void setLevel(float newLevel);
            
        private:
            float level = 0.0f;
            float peakLevel = 0.0f;
        };
        
        juce::Slider inputGain;
        juce::Slider outputGain;
        juce::Slider mixControl;
        
        VUMeter inputMeter;
        VUMeter outputMeter;
        
        juce::Label inputLabel{"input", "INPUT"};
        juce::Label outputLabel{"output", "OUTPUT"};
        juce::Label mixLabel{"mix", "MIX"};
        
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterSection)
    };
    
    //==============================================================================
    // Main Components
    //==============================================================================
    ChimeraAudioProcessor& audioProcessor;
    NexusLookAndFeel_Final nexusLookAndFeel;
    
    std::unique_ptr<AICommandCenter> aiCenter;
    std::vector<std::unique_ptr<EngineSlot>> engineSlots;
    std::unique_ptr<MasterSection> masterSection;
    
    // Background rendering
    void drawBackground(juce::Graphics& g);
    void drawTitleBar(juce::Graphics& g);
    
    // AI Communication
    void executeAIPrompt(const juce::String& prompt);
    void handleAIResponse(const juce::String& response);
    
    // Animation
    float animationPhase = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorNexus_Final)
};