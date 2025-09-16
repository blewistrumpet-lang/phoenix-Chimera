#include "PluginEditorWorking.h"

PluginEditorWorking::PluginEditorWorking(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(1000, 750);
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(22.0f));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);
    
    // Status
    statusLabel.setText("Multi-Engine Processor", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(statusLabel);
    
    // Create all 6 slots
    for (int i = 0; i < 6; ++i)
    {
        // Slot label
        slotLabels[i].setText("SLOT " + juce::String(i + 1), juce::dontSendNotification);
        slotLabels[i].setJustificationType(juce::Justification::centredLeft);
        slotLabels[i].setFont(juce::Font(11.0f, juce::Font::bold));
        slotLabels[i].setColour(juce::Label::textColourId, juce::Colours::orange);
        addAndMakeVisible(slotLabels[i]);
        
        // Engine selector
        engineSelectors[i].addItem("-- None --", 1);
        engineSelectors[i].addItem("Classic Compressor", 2);
        engineSelectors[i].addItem("Noise Gate", 3);
        engineSelectors[i].addItem("Transient Shaper", 4);
        engineSelectors[i].addItem("BitCrusher", 5);
        engineSelectors[i].addItem("K-Style Overdrive", 6);
        engineSelectors[i].addItem("Classic Chorus", 7);
        engineSelectors[i].addItem("Analog Phaser", 8);
        engineSelectors[i].addItem("Plate Reverb", 9);
        engineSelectors[i].addItem("Spring Reverb", 10);
        engineSelectors[i].addItem("Gated Reverb", 11);
        engineSelectors[i].addItem("Tape Echo", 12);
        engineSelectors[i].addItem("Digital Delay", 13);
        engineSelectors[i].addItem("Parametric EQ", 14);
        engineSelectors[i].addItem("Ring Modulator", 15);
        
        engineSelectors[i].onChange = [this, i]()
        {
            // Update status to show active engines
            int count = 0;
            for (int j = 0; j < 6; ++j)
            {
                if (engineSelectors[j].getSelectedId() > 1) count++;
            }
            statusLabel.setText(juce::String(count) + " engines active", juce::dontSendNotification);
        };
        
        addAndMakeVisible(engineSelectors[i]);
        
        // Bypass button
        bypassButtons[i].setButtonText("Bypass");
        bypassButtons[i].setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(bypassButtons[i]);
        
        // Mix slider
        slotMixLabels[i].setText("Mix", juce::dontSendNotification);
        slotMixLabels[i].setJustificationType(juce::Justification::left);
        slotMixLabels[i].setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(slotMixLabels[i]);
        
        slotMixSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        slotMixSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 35, 16);
        slotMixSliders[i].setRange(0.0, 100.0);
        slotMixSliders[i].setTextValueSuffix("%");
        slotMixSliders[i].setValue(100.0);
        addAndMakeVisible(slotMixSliders[i]);
        
        // Create attachments
        juce::String paramName = "engineType" + juce::String(i);
        engineAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getValueTreeState(), paramName, engineSelectors[i]
        );
        
        juce::String bypassParam = "slot" + juce::String(i + 1) + "_bypass";
        auto* bypassP = audioProcessor.getValueTreeState().getParameter(bypassParam);
        if (bypassP)
        {
            bypassAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                audioProcessor.getValueTreeState(), bypassParam, bypassButtons[i]
            );
        }
        
        juce::String mixParam = "slot" + juce::String(i + 1) + "_mix";
        auto* mixP = audioProcessor.getValueTreeState().getParameter(mixParam);
        if (mixP)
        {
            slotMixAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.getValueTreeState(), mixParam, slotMixSliders[i]
            );
        }
        
        // Parameter sliders for each slot
        for (int j = 0; j < PARAMS_PER_SLOT; ++j)
        {
            slotParamLabels[i][j].setText("P" + juce::String(j + 1), juce::dontSendNotification);
            slotParamLabels[i][j].setJustificationType(juce::Justification::centred);
            slotParamLabels[i][j].setFont(juce::Font(9.0f));
            slotParamLabels[i][j].setColour(juce::Label::textColourId, juce::Colours::grey);
            addAndMakeVisible(slotParamLabels[i][j]);
            
            slotParamSliders[i][j].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slotParamSliders[i][j].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slotParamSliders[i][j].setRange(0.0, 1.0);
            slotParamSliders[i][j].setValue(0.5);
            addAndMakeVisible(slotParamSliders[i][j]);
            
            // Try to attach
            juce::String paramId = "slot" + juce::String(i + 1) + "_param" + juce::String(j + 1);
            auto* param = audioProcessor.getValueTreeState().getParameter(paramId);
            if (param)
            {
                slotParamAttachments[i][j] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.getValueTreeState(), paramId, slotParamSliders[i][j]
                );
            }
        }
    }
    
    // Master controls
    masterGainLabel.setText("Master Gain", juce::dontSendNotification);
    masterGainLabel.setJustificationType(juce::Justification::centred);
    masterGainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(masterGainLabel);
    
    masterGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    masterGainSlider.setRange(-60.0, 12.0);
    masterGainSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(masterGainSlider);
    
    masterMixLabel.setText("Dry/Wet", juce::dontSendNotification);
    masterMixLabel.setJustificationType(juce::Justification::centred);
    masterMixLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(masterMixLabel);
    
    masterMixSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    masterMixSlider.setRange(0.0, 100.0);
    masterMixSlider.setTextValueSuffix("%");
    addAndMakeVisible(masterMixSlider);
    
    masterBypassButton.setButtonText("Master\nBypass");
    masterBypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(masterBypassButton);
    
    // Master attachments
    masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "masterGain", masterGainSlider
    );
    
    masterMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "masterMix", masterMixSlider
    );
    
    auto* masterBypassParam = audioProcessor.getValueTreeState().getParameter("masterBypass");
    if (masterBypassParam)
    {
        masterBypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getValueTreeState(), "masterBypass", masterBypassButton
        );
    }
}

PluginEditorWorking::~PluginEditorWorking()
{
}

void PluginEditorWorking::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Header background
    g.setColour(juce::Colour(0xff0f0f0f));
    g.fillRect(0, 0, getWidth(), 60);
    
    // Draw slot backgrounds
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 3;
        int row = i / 3;
        
        auto slotBounds = juce::Rectangle<int>(
            10 + col * 320,
            70 + row * 320,
            310,
            310
        );
        
        g.setColour(juce::Colour(0xff202020));
        g.fillRoundedRectangle(slotBounds.toFloat(), 4.0f);
        
        // Active border
        if (engineSelectors[i].getSelectedId() > 1)
        {
            g.setColour(juce::Colours::orange.withAlpha(0.5f));
            g.drawRoundedRectangle(slotBounds.toFloat(), 4.0f, 2.0f);
        }
    }
    
    // Master section background
    g.setColour(juce::Colour(0xff181818));
    g.fillRoundedRectangle(970.0f, 70.0f, 120.0f, 620.0f, 4.0f);
}

void PluginEditorWorking::resized()
{
    auto bounds = getLocalBounds();
    
    // Header
    auto header = bounds.removeFromTop(60);
    titleLabel.setBounds(header.removeFromTop(35).reduced(10, 5));
    statusLabel.setBounds(header.reduced(10, 0));
    
    // Layout slots in 3x2 grid
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 3;
        int row = i / 3;
        
        auto slotBounds = juce::Rectangle<int>(
            15 + col * 320,
            75 + row * 320,
            300,
            300
        );
        
        // Slot header
        auto slotHeader = slotBounds.removeFromTop(20);
        slotLabels[i].setBounds(slotHeader);
        
        // Engine selector
        slotBounds.removeFromTop(5);
        engineSelectors[i].setBounds(slotBounds.removeFromTop(22).reduced(5, 0));
        
        // Bypass and mix row
        slotBounds.removeFromTop(5);
        auto controlRow = slotBounds.removeFromTop(25);
        bypassButtons[i].setBounds(controlRow.removeFromLeft(60));
        controlRow.removeFromLeft(10);
        slotMixLabels[i].setBounds(controlRow.removeFromLeft(25));
        slotMixSliders[i].setBounds(controlRow.reduced(5, 2));
        
        // Parameters
        slotBounds.removeFromTop(10);
        auto paramArea = slotBounds.removeFromTop(200);
        
        for (int j = 0; j < PARAMS_PER_SLOT; ++j)
        {
            int px = (j % 2) * 150;
            int py = (j / 2) * 100;
            
            auto paramBounds = juce::Rectangle<int>(
                paramArea.getX() + px + 35,
                paramArea.getY() + py + 10,
                80,
                80
            );
            
            slotParamSliders[i][j].setBounds(paramBounds.removeFromTop(60));
            slotParamLabels[i][j].setBounds(paramBounds);
        }
    }
    
    // Master section
    auto masterBounds = juce::Rectangle<int>(975, 75, 110, 610);
    
    masterBounds.removeFromTop(20);
    
    // Gain
    masterGainLabel.setBounds(masterBounds.removeFromTop(20));
    masterGainSlider.setBounds(masterBounds.removeFromTop(180).reduced(20, 0));
    
    masterBounds.removeFromTop(20);
    
    // Mix
    masterMixLabel.setBounds(masterBounds.removeFromTop(20));
    masterMixSlider.setBounds(masterBounds.removeFromTop(180).reduced(20, 0));
    
    masterBounds.removeFromTop(40);
    
    // Master bypass
    masterBypassButton.setBounds(masterBounds.removeFromTop(50).reduced(15, 5));
}