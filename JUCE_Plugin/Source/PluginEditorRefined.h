#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ArtisticLookAndFeel.h"

class ChimeraAudioProcessorEditorRefined : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    ChimeraAudioProcessorEditorRefined(ChimeraAudioProcessor&);
    ~ChimeraAudioProcessorEditorRefined() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    ChimeraAudioProcessor& audioProcessor;
    ArtisticLookAndFeel artisticLookAndFeel;
    
    // Compact Header Section
    class HeaderSection : public juce::Component
    {
    public:
        HeaderSection();
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        // Branding
        juce::Label logoLabel{"logo", "CHIMERA"};
        juce::Label versionLabel{"version", "PHOENIX 3.0"};
        
        // Status indicators (compact)
        class StatusLED : public juce::Component
        {
        public:
            void paint(juce::Graphics& g) override;
            void setState(bool active, juce::Colour color = juce::Colour(0xff10b981))
            { 
                isActive = active; 
                ledColor = color;
                repaint();
            }
            
        private:
            bool isActive{false};
            juce::Colour ledColor;
        };
        
        StatusLED aiStatusLED;
        juce::Label aiStatusLabel{"ai", "AI"};
    };
    
    // AI Prompt Section (compact and elegant)
    class AIPromptSection : public juce::Component
    {
    public:
        AIPromptSection();
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        juce::TextEditor promptInput;
        juce::TextButton generateButton{"Generate"};
        juce::Label statusLabel;
        
        std::function<void(const juce::String&)> onGenerate;
    };
    
    // Refined Slot Component
    class RefinedSlotComponent : public juce::Component
    {
    public:
        RefinedSlotComponent(int slotNumber, juce::AudioProcessorValueTreeState& apvts);
        ~RefinedSlotComponent();
        
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        void updateParameters();
        void setProcessingLevel(float level) { processingLevel = level; repaint(); }
        
    private:
        int slotNum;
        juce::AudioProcessorValueTreeState& valueTreeState;
        
        // Compact controls
        juce::Label slotLabel;
        juce::ComboBox engineSelector;
        juce::ToggleButton bypassButton{"Bypass"};
        
        // Parameters with labels (dynamic layout)
        struct ParamControl {
            std::unique_ptr<juce::Slider> slider;
            std::unique_ptr<juce::ToggleButton> toggleButton;
            std::unique_ptr<juce::Label> label;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
            std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachment;
        };
        std::vector<ParamControl> paramControls;
        
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        
        float processingLevel{0.0f};
        
        void layoutParameters();
    };
    
    // Master Section
    class MasterSection : public juce::Component
    {
    public:
        MasterSection(juce::AudioProcessorValueTreeState& apvts);
        void paint(juce::Graphics& g) override;
        void resized() override;
        
        void setInputLevel(float level) { inputMeter = level; repaint(); }
        void setOutputLevel(float level) { outputMeter = level; repaint(); }
        
    private:
        juce::Slider inputGain;
        juce::Slider outputGain;
        juce::Slider mixKnob;
        
        juce::Label inputLabel{"input", "Input"};
        juce::Label outputLabel{"output", "Output"};
        juce::Label mixLabel{"mix", "Mix"};
        
        float inputMeter{0.0f};
        float outputMeter{0.0f};
        
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
        
        void drawMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float level, bool isInput);
    };
    
    // Components
    std::unique_ptr<HeaderSection> headerSection;
    std::unique_ptr<AIPromptSection> aiPromptSection;
    std::vector<std::unique_ptr<RefinedSlotComponent>> slotComponents;
    std::unique_ptr<MasterSection> masterSection;
    
    // AI handling
    void handleAIPrompt(const juce::String& prompt);
    void handleAIResponse(const juce::String& response);
    void checkServerConnection();
    bool isServerConnected{false};
    
    // Background
    void drawBackground(juce::Graphics& g);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChimeraAudioProcessorEditorRefined)
};