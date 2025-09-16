#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SlotComponent.h"

// MINIMAL WORKING EDITOR - GRADUALLY ADDING COMPLEXITY
class PluginEditorMinimal : public juce::AudioProcessorEditor
{
public:
    PluginEditorMinimal(ChimeraAudioProcessor& p)
        : AudioProcessorEditor(&p), audioProcessor(p),
          engineAttachment(nullptr)
    {
        setSize(800, 600);
        
        titleLabel.setText("CHIMERA PHOENIX - ADDING SLOTCOMPONENT", juce::dontSendNotification);
        titleLabel.setBounds(10, 10, 780, 30);
        addAndMakeVisible(titleLabel);
        
        statusLabel.setText("Testing: ONE SlotComponent", juce::dontSendNotification);
        statusLabel.setBounds(10, 50, 780, 30);
        addAndMakeVisible(statusLabel);
        
        // STEP 1: Add a single slider (PASSED)
        testSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        testSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        testSlider.setRange(0.0, 1.0, 0.01);
        testSlider.setValue(0.5);
        testSlider.setBounds(100, 100, 100, 120);
        addAndMakeVisible(testSlider);
        
        sliderLabel.setText("Test Param", juce::dontSendNotification);
        sliderLabel.setBounds(100, 230, 100, 20);
        sliderLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(sliderLabel);
        
        // STEP 2: Add a ComboBox WITHOUT attachment (PASSED)
        testCombo.addItem("Option 1", 1);
        testCombo.addItem("Option 2", 2);
        testCombo.addItem("Option 3", 3);
        testCombo.setSelectedId(1);
        testCombo.setBounds(250, 100, 150, 30);
        addAndMakeVisible(testCombo);
        
        comboLabel.setText("Test Combo (no attach)", juce::dontSendNotification);
        comboLabel.setBounds(250, 140, 150, 20);
        comboLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(comboLabel);
        
        // STEP 3: Add a ComboBox WITH attachment to slot1_engine
        engineCombo.setBounds(450, 100, 200, 30);
        addAndMakeVisible(engineCombo);
        
        // Populate with engine choices
        auto* param = audioProcessor.getValueTreeState().getParameter("slot1_engine");
        if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
        {
            for (int i = 0; i < choiceParam->choices.size(); ++i)
            {
                engineCombo.addItem(choiceParam->choices[i], i + 1);
            }
        }
        
        // Create attachment AFTER adding items
        engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getValueTreeState(), "slot1_engine", engineCombo
        );
        
        engineLabel.setText("Slot 1 Engine (attached)", juce::dontSendNotification);
        engineLabel.setBounds(450, 140, 200, 20);
        engineLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(engineLabel);
        
        // STEP 4: Add ONE SlotComponent
        testSlot = std::make_unique<SlotComponent>(0);
        testSlot->setBounds(50, 260, 300, 250);
        addAndMakeVisible(testSlot.get());
        // NOTE: Not creating attachments yet, just the component
    }
    
    ~PluginEditorMinimal() override = default;
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::darkgrey);
    }
    
    void resized() override {}
    
private:
    ChimeraAudioProcessor& audioProcessor;
    juce::Label titleLabel;
    juce::Label statusLabel;
    juce::Slider testSlider;
    juce::Label sliderLabel;
    juce::ComboBox testCombo;
    juce::Label comboLabel;
    juce::ComboBox engineCombo;
    juce::Label engineLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> engineAttachment;
    std::unique_ptr<SlotComponent> testSlot;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorMinimal)
};