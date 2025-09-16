#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SkunkworksLookAndFeel.h"
#include "ChimeraSlotComponent.h"
#include "CommandTerminal.h"
#include <atomic>

class ChimeraAudioProcessorEditorSkunkworks : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    ChimeraAudioProcessorEditorSkunkworks(ChimeraAudioProcessor&);
    ~ChimeraAudioProcessorEditorSkunkworks() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    ChimeraAudioProcessor& audioProcessor;
    SkunkworksLookAndFeel skunkworksLookAndFeel;
    
    // Main sections
    class HeaderPanel : public juce::Component
    {
    public:
        HeaderPanel();
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        juce::Label titleLabel{"title", "CHIMERA PHOENIX"};
        juce::Label subtitleLabel{"subtitle", "MILITARY GRADE AUDIO PROCESSING"};
        juce::Label versionLabel{"version", "v3.0.0"};
        
        // Status LEDs
        struct StatusLED : public juce::Component
        {
            void paint(juce::Graphics& g) override;
            void setState(bool active, juce::Colour color = juce::Colour(0xff00ff44));
            
        private:
            bool isActive{false};
            juce::Colour ledColor;
        };
        
        StatusLED powerLED;
        StatusLED aiLED;
        StatusLED audioLED;
    };
    
    class ControlPanel : public juce::Component
    {
    public:
        ControlPanel(juce::AudioProcessorValueTreeState& apvts);
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // Master controls
        MilitaryKnob inputGainKnob{"INPUT"};
        MilitaryKnob outputGainKnob{"OUTPUT"};
        MilitaryKnob mixKnob{"MIX"};
        
        juce::ToggleButton bypassButton{"BYPASS"};
        juce::ToggleButton panicButton{"PANIC"};
        
        // Preset management
        juce::ComboBox presetSelector;
        juce::TextButton saveButton{"SAVE"};
        juce::TextButton loadButton{"LOAD"};
        
        // A/B comparison
        juce::TextButton compareAButton{"A"};
        juce::TextButton compareBButton{"B"};
        juce::TextButton copyButton{"COPY"};
        
    private:
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    };
    
    class RackPanel : public juce::Component
    {
    public:
        RackPanel(ChimeraAudioProcessor& processor, juce::AudioProcessorValueTreeState& apvts);
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        void updateSlotActivity(int slot, float level);
        
    private:
        std::vector<std::unique_ptr<ChimeraSlotComponent>> slots;
        
        void drawRackFrame(juce::Graphics& g);
    };
    
    class MetersPanel : public juce::Component, private juce::Timer
    {
    public:
        MetersPanel();
        ~MetersPanel() override;
        
        void paint(juce::Graphics& g) override;
        void setLevels(float inputL, float inputR, float outputL, float outputR);
        
    private:
        void timerCallback() override;
        
        struct StereoMeter
        {
            std::atomic<float> leftLevel{0.0f};
            std::atomic<float> rightLevel{0.0f};
            float leftDisplay{0.0f};
            float rightDisplay{0.0f};
            
            void update();
            void paint(juce::Graphics& g, juce::Rectangle<float> bounds, 
                      const juce::String& label, bool showPeak = true);
        };
        
        StereoMeter inputMeter;
        StereoMeter outputMeter;
    };
    
    // Components
    HeaderPanel headerPanel;
    std::unique_ptr<ControlPanel> controlPanel;
    std::unique_ptr<RackPanel> rackPanel;
    std::unique_ptr<CommandTerminal> commandTerminal;
    std::unique_ptr<MetersPanel> metersPanel;
    
    // Layout state
    bool isCommandTerminalVisible{false};
    juce::TextButton terminalToggleButton{"TERMINAL"};
    
    // Network handling for AI
    void handleAIGenerate(const juce::String& prompt);
    void handleAIResponse(const juce::String& response);
    void checkAIServerConnection();
    bool isAIServerConnected{false};
    
    // Background animation
    float backgroundPulse{0.0f};
    std::vector<juce::Point<float>> starField;
    
    void drawBackground(juce::Graphics& g);
    void drawMetalFrame(juce::Graphics& g, juce::Rectangle<float> bounds, 
                       const juce::String& label = "");
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChimeraAudioProcessorEditorSkunkworks)
};