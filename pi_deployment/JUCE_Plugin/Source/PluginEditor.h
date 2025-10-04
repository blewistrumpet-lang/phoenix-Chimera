#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <atomic>

class CommandCenterLookAndFeel : public juce::LookAndFeel_V4 {
public:
    CommandCenterLookAndFeel();
    
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;
    
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, 
                            const juce::Colour& backgroundColour,
                            bool shouldDrawButtonAsHighlighted, 
                            bool shouldDrawButtonAsDown) override;
    
    void drawLabel(juce::Graphics& g, juce::Label& label) override;
    
private:
    juce::Colour primaryColor = juce::Colour(0xff00d4ff);    // Cyan
    juce::Colour secondaryColor = juce::Colour(0xffff6b00);  // Orange
    juce::Colour backgroundColor = juce::Colour(0xff0a0a0a); // Near black
    juce::Colour panelColor = juce::Colour(0xff1a1a1a);      // Dark grey
};

class ChimeraAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::AudioProcessorValueTreeState::Listener,
                                    private juce::Timer {
public:
    ChimeraAudioProcessorEditor(ChimeraAudioProcessor&);
    ~ChimeraAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    ChimeraAudioProcessor& audioProcessor;
    CommandCenterLookAndFeel lookAndFeel;
    
    // AI Command Section (Left Column)
    juce::Label titleLabel;
    juce::TextEditor promptBox;
    juce::TextButton generateButton;
    juce::Label statusLabel;
    
    // Preset Management
    juce::Label presetNameLabel;
    juce::String currentPresetName = "Init";
    juce::String presetDescription;
    juce::TextButton savePresetButton{"Save"};
    juce::TextButton loadPresetButton{"Load"};
    juce::TextButton detailsButton{"Details"};
    
    // A/B Comparison
    juce::TextButton compareAButton{"A"};
    juce::TextButton compareBButton{"B"};
    juce::TextButton copyABButton{"Copy →"};
    bool isPresetA = true;
    
    // Master Controls
    juce::ToggleButton masterBypassButton{"Master Bypass"};
    
    // Level Meter
    class SimpleLevelMeter : public juce::Component, public juce::Timer {
    public:
        SimpleLevelMeter() { startTimerHz(30); }
        ~SimpleLevelMeter() override { stopTimer(); }
        
        void paint(juce::Graphics& g) override {
            auto bounds = getLocalBounds().toFloat();
            
            // Background
            g.setColour(juce::Colour(0xff1a1a1a));
            g.fillRoundedRectangle(bounds, 2.0f);
            
            // Draw level
            float dbLevel = juce::Decibels::gainToDecibels(currentLevel.load());
            float normalizedLevel = juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 1.0f);
            normalizedLevel = juce::jlimit(0.0f, 1.0f, normalizedLevel);
            
            if (normalizedLevel > 0.01f) {
                auto levelBounds = bounds.reduced(2.0f);
                levelBounds = levelBounds.removeFromBottom(levelBounds.getHeight() * normalizedLevel);
                
                // Color based on level
                if (dbLevel > -3.0f)
                    g.setColour(juce::Colours::red);
                else if (dbLevel > -12.0f)
                    g.setColour(juce::Colours::yellow);
                else
                    g.setColour(juce::Colour(0xff00d4ff));
                    
                g.fillRoundedRectangle(levelBounds, 1.0f);
            }
        }
        
        void timerCallback() override {
            float targetLevel = currentLevel.load() * 0.85f; // Smooth decay
            currentLevel.store(targetLevel);
            repaint();
        }
        
        void setLevel(float level) {
            float current = currentLevel.load();
            if (level > current) {
                currentLevel.store(level);
            }
        }
        
    private:
        std::atomic<float> currentLevel{0.0f};
    };
    
    SimpleLevelMeter outputLevelMeter;
    
    // Macro Controls
    struct MacroControl {
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::array<MacroControl, 3> macroControls;
    
    // Rack Display (Right Column)
    struct SlotUI {
        juce::Label slotLabel;
        std::unique_ptr<juce::ComboBox> engineSelector;
        std::unique_ptr<juce::ToggleButton> bypassButton;
        std::vector<std::unique_ptr<juce::Slider>> paramSliders;
        std::vector<std::unique_ptr<juce::Label>> paramLabels;
        std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliderAttachments;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
        juce::Component slotPanel;
    };
    
    static constexpr int NUM_SLOTS = 6;
    std::array<SlotUI, NUM_SLOTS> slotUIs;
    
    // Network handling
    std::unique_ptr<juce::URL> currentRequest;
    void generateButtonClicked();
    void handleAIResponse(const juce::String& response);
    void loadPresetFromJSON(const juce::var& preset);
    
    // UI Updates
    void parameterChanged(const juce::String& parameterID, float newValue) override;
    void updateSlotParameters(int slot);
    void updateMacroControls(const juce::var& macroData);
    void setStatus(const juce::String& message, bool isError = false);
    
    // Preset Management
    void showDetails();
    void savePreset();
    void loadPreset();
    void selectPresetA();
    void selectPresetB();
    void copyAtoB();
    
    // Styling
    void applyRetrofuturistStyling();
    
    // Details Window
    class DetailsWindow : public juce::DocumentWindow {
    public:
        DetailsWindow(const juce::String& presetName, const juce::String& description);
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChimeraAudioProcessorEditor)
};