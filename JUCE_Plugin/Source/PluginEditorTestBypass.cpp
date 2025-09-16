#include "PluginEditorTestBypass.h"

PluginEditorTestBypass::PluginEditorTestBypass(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(800, 600);
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX - Testing Bypass", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(20.0f));
    addAndMakeVisible(titleLabel);
    
    // Status
    statusLabel.setText("SimpleFinal + Bypass + Mix sliders", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(14.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(statusLabel);
    
    // Create all 6 selectors and bypass buttons
    for (int i = 0; i < 6; ++i)
    {
        // Slot label
        slotLabels[i].setText("Slot " + juce::String(i + 1), juce::dontSendNotification);
        slotLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(slotLabels[i]);
        
        // Engine selector
        engineSelectors[i].addItem("None", 1);
        engineSelectors[i].addItem("ClassicCompressor", 2);
        engineSelectors[i].addItem("NoiseGate", 3);
        engineSelectors[i].addItem("TransientShaper", 4);
        engineSelectors[i].addItem("BitCrusher", 5);
        engineSelectors[i].addItem("KStyleOverdrive", 6);
        engineSelectors[i].addItem("ClassicChorus", 7);
        engineSelectors[i].addItem("AnalogPhaser", 8);
        engineSelectors[i].addItem("PlateReverb", 9);
        engineSelectors[i].addItem("SpringReverb", 10);
        engineSelectors[i].addItem("GatedReverb", 11);
        
        addAndMakeVisible(engineSelectors[i]);
        
        // Bypass button
        bypassButtons[i].setButtonText("Bypass");
        addAndMakeVisible(bypassButtons[i]);
        
        // Slot mix slider
        slotMixLabels[i].setText("Mix", juce::dontSendNotification);
        slotMixLabels[i].setJustificationType(juce::Justification::left);
        addAndMakeVisible(slotMixLabels[i]);
        
        slotMixSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        slotMixSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
        slotMixSliders[i].setRange(0.0, 100.0);
        slotMixSliders[i].setTextValueSuffix("%");
        addAndMakeVisible(slotMixSliders[i]);
        
        // Create attachments
        juce::String paramName = "engineType" + juce::String(i);
        engineAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getValueTreeState(), paramName, engineSelectors[i]
        );
        
        // Try to attach bypass - may not exist for all slots
        juce::String bypassParam = "slot" + juce::String(i + 1) + "_bypass";
        auto* param = audioProcessor.getValueTreeState().getParameter(bypassParam);
        if (param)
        {
            bypassAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                audioProcessor.getValueTreeState(), bypassParam, bypassButtons[i]
            );
        }
        
        // Try to attach mix slider
        juce::String mixParam = "slot" + juce::String(i + 1) + "_mix";
        auto* mixP = audioProcessor.getValueTreeState().getParameter(mixParam);
        if (mixP)
        {
            slotMixAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.getValueTreeState(), mixParam, slotMixSliders[i]
            );
        }
    }
    
    // Master Gain
    masterGainLabel.setText("Master Gain", juce::dontSendNotification);
    masterGainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterGainLabel);
    
    masterGainSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterGainSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    masterGainSlider.setRange(-60.0, 12.0);
    addAndMakeVisible(masterGainSlider);
    
    masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "masterGain", masterGainSlider
    );
    
    // Master Mix
    masterMixLabel.setText("Dry/Wet Mix", juce::dontSendNotification);
    masterMixLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterMixLabel);
    
    masterMixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    masterMixSlider.setRange(0.0, 100.0);
    masterMixSlider.setTextValueSuffix(" %");
    addAndMakeVisible(masterMixSlider);
    
    masterMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "masterMix", masterMixSlider
    );
    
    // Add 4 static parameter sliders for slot 1
    juce::StringArray paramNames = {"Param 1", "Param 2", "Param 3", "Param 4"};
    for (int i = 0; i < NUM_STATIC_PARAMS; ++i)
    {
        slot1Labels[i].setText(paramNames[i], juce::dontSendNotification);
        slot1Labels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(slot1Labels[i]);
        
        slot1Sliders[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slot1Sliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        slot1Sliders[i].setRange(0.0, 1.0);
        addAndMakeVisible(slot1Sliders[i]);
        
        juce::String paramId = "slot1_param" + juce::String(i + 1);
        auto* param = audioProcessor.getValueTreeState().getParameter(paramId);
        if (param)
        {
            slot1Attachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.getValueTreeState(), paramId, slot1Sliders[i]
            );
        }
    }
}

PluginEditorTestBypass::~PluginEditorTestBypass()
{
}

void PluginEditorTestBypass::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
}

void PluginEditorTestBypass::resized()
{
    auto bounds = getLocalBounds();
    
    // Header
    titleLabel.setBounds(bounds.removeFromTop(40));
    statusLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(20);
    
    // Engine selectors with bypass and mix in 2x3 grid
    auto selectorArea = bounds.removeFromTop(280);
    int slotWidth = selectorArea.getWidth() / 2;
    int slotHeight = selectorArea.getHeight() / 3;
    
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        
        auto slotBounds = juce::Rectangle<int>(
            col * slotWidth + 20,
            selectorArea.getY() + row * slotHeight + 10,
            slotWidth - 40,
            slotHeight - 20
        );
        
        slotLabels[i].setBounds(slotBounds.removeFromTop(18));
        engineSelectors[i].setBounds(slotBounds.removeFromTop(24).reduced(10, 0));
        bypassButtons[i].setBounds(slotBounds.removeFromTop(22).reduced(40, 0));
        
        auto mixRow = slotBounds.removeFromTop(20);
        slotMixLabels[i].setBounds(mixRow.removeFromLeft(30));
        slotMixSliders[i].setBounds(mixRow.reduced(5, 0));
    }
    
    // Slot 1 parameters
    auto paramArea = bounds.removeFromTop(100);
    int paramWidth = 70;
    int paramX = (getWidth() - (NUM_STATIC_PARAMS * paramWidth)) / 2;
    
    for (int i = 0; i < NUM_STATIC_PARAMS; ++i)
    {
        auto paramBounds = juce::Rectangle<int>(
            paramX + i * paramWidth,
            paramArea.getY(),
            paramWidth,
            90
        );
        
        slot1Labels[i].setBounds(paramBounds.removeFromTop(15));
        slot1Sliders[i].setBounds(paramBounds);
    }
    
    // Master controls at bottom
    bounds.removeFromTop(10);
    auto masterArea = bounds.removeFromTop(100);
    
    auto gainSection = masterArea.removeFromTop(50);
    masterGainLabel.setBounds(gainSection.removeFromTop(20).reduced(20, 0));
    masterGainSlider.setBounds(gainSection.reduced(40, 5));
    
    auto mixSection = masterArea;
    masterMixLabel.setBounds(mixSection.removeFromTop(20).reduced(20, 0));
    masterMixSlider.setBounds(mixSection.reduced(40, 5));
}