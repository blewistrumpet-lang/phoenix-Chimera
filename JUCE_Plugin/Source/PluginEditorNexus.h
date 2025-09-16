#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "NexusLookAndFeel.h"
#include "UnifiedDefaultParameters.h"
#include "EngineLibrary.h"

class ChimeraAudioProcessorEditorNexus : public juce::AudioProcessorEditor,
                                         private juce::Timer,
                                         private juce::ComponentListener
{
public:
    ChimeraAudioProcessorEditorNexus(ChimeraAudioProcessor&);
    ~ChimeraAudioProcessorEditorNexus() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void componentMovedOrResized(Component& component, bool wasMoved, bool wasResized) override;
    
private:
    class NexusSlotComponent : public juce::Component
    {
    public:
        NexusSlotComponent(ChimeraAudioProcessor& p, int slot);
        ~NexusSlotComponent();
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        void updateParameters();
        void setActivity(float level) { activityLevel = level; repaint(); }
        
    private:
        struct ParameterControl {
            std::unique_ptr<juce::Slider> slider;
            std::unique_ptr<juce::ToggleButton> toggle;
            std::unique_ptr<juce::Label> label;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment;
            std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;
            bool isToggle = false;
        };
        
        ChimeraAudioProcessor& processor;
        int slotIndex;
        float activityLevel = 0.0f;
        
        juce::Label slotTitle;
        juce::ComboBox engineSelector;
        juce::ToggleButton bypassButton{"BYPASS"};
        juce::ToggleButton soloButton{"SOLO"};
        juce::ToggleButton muteButton{"MUTE"};
        
        std::vector<ParameterControl> parameterControls;
        
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        
        void createParametersForEngine(int engineId);
        juce::String getActualParameterName(int engineId, int paramIndex);
        bool shouldBeToggle(const juce::String& paramName);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusSlotComponent)
    };
    
    class HeaderPanel : public juce::Component
    {
    public:
        HeaderPanel();
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        void setServerStatus(bool connected);
        void setCpuUsage(float cpu);
        
        juce::Label titleLabel{"title", "CHIMERA PHOENIX NEXUS"};
        juce::Label subtitleLabel{"subtitle", "Neural Audio Processor"};
        juce::Label versionLabel{"version", "v3.0 NEXUS 2030"};
        
        struct StatusIndicator : public juce::Component
        {
            void paint(juce::Graphics& g) override;
            void setStatus(const juce::String& text, juce::Colour color);
            
            juce::String statusText = "OFFLINE";
            juce::Colour statusColor = juce::Colour(0xffff006e);
        };
        
        StatusIndicator aiStatus;
        StatusIndicator cpuStatus;
        
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderPanel)
    };
    
    class AIControlPanel : public juce::Component
    {
    public:
        AIControlPanel();
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        juce::TextEditor promptInput;
        juce::TextButton generateButton{"GENERATE"};
        juce::TextButton enhanceButton{"ENHANCE"};
        juce::TextButton randomizeButton{"RANDOMIZE"};
        juce::Label statusLabel;
        
        std::function<void(const juce::String&)> onPromptSubmit;
        
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AIControlPanel)
    };
    
    class MasterControlPanel : public juce::Component
    {
    public:
        MasterControlPanel(juce::AudioProcessorValueTreeState& apvts);
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        void updateMeters(float inputLevel, float outputLevel);
        
    private:
        class VUMeter : public juce::Component
        {
        public:
            void paint(juce::Graphics& g) override;
            void setLevel(float newLevel);
            
        private:
            float level = 0.0f;
            float smoothedLevel = 0.0f;
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
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterControlPanel)
    };
    
    // Main components
    ChimeraAudioProcessor& audioProcessor;
    NexusLookAndFeel nexusLookAndFeel;
    
    std::unique_ptr<HeaderPanel> headerPanel;
    std::unique_ptr<AIControlPanel> aiPanel;
    std::unique_ptr<MasterControlPanel> masterPanel;
    std::vector<std::unique_ptr<NexusSlotComponent>> slotComponents;
    
    // Layout management
    void updateLayout();
    int calculateOptimalSlotColumns() const;
    
    // AI Communication
    void sendAIPrompt(const juce::String& prompt);
    void handleAIResponse(const juce::String& response);
    void checkServerConnection();
    
    bool isServerConnected = false;
    float currentCpuUsage = 0.0f;
    
    // Visual effects
    void drawBackground(juce::Graphics& g);
    void drawGridOverlay(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChimeraAudioProcessorEditorNexus)
};