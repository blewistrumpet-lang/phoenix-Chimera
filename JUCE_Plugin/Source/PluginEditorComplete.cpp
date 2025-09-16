#include "PluginEditorComplete.h"

//==============================================================================
// SlotPanel Implementation
//==============================================================================
SlotPanel::SlotPanel(ChimeraAudioProcessor& p, int slotIndex)
    : processor(p), slot(slotIndex)
{
    // Slot title
    slotLabel.setText("SLOT " + juce::String(slot + 1), juce::dontSendNotification);
    slotLabel.setJustificationType(juce::Justification::centredLeft);
    slotLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    slotLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(slotLabel);
    
    // Engine selector
    engineSelector.addItem("-- None --", 1);
    engineSelector.addItem("Classic Compressor", 2);
    engineSelector.addItem("Noise Gate", 3);
    engineSelector.addItem("Transient Shaper", 4);
    engineSelector.addItem("BitCrusher", 5);
    engineSelector.addItem("K-Style Overdrive", 6);
    engineSelector.addItem("Classic Chorus", 7);
    engineSelector.addItem("Analog Phaser", 8);
    engineSelector.addItem("Plate Reverb", 9);
    engineSelector.addItem("Spring Reverb", 10);
    engineSelector.addItem("Gated Reverb", 11);
    engineSelector.addItem("Tape Echo", 12);
    engineSelector.addItem("Digital Delay", 13);
    engineSelector.addItem("Hall Reverb", 14);
    engineSelector.addItem("Shimmer Reverb", 15);
    engineSelector.addItem("Parametric EQ", 16);
    engineSelector.addItem("Vintage Filter", 17);
    engineSelector.addItem("Ring Modulator", 18);
    engineSelector.addItem("Pitch Shifter", 19);
    engineSelector.addItem("Harmonizer", 20);
    
    addAndMakeVisible(engineSelector);
    
    // Bypass button
    bypassButton.setButtonText("Bypass");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(bypassButton);
    
    // Mix slider
    mixSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
    mixSlider.setRange(0.0, 100.0);
    mixSlider.setTextValueSuffix("%");
    mixSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgrey);
    addAndMakeVisible(mixSlider);
    
    // Create attachments for main controls
    juce::String slotStr = juce::String(slot);
    
    engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getValueTreeState(), "engineType" + slotStr, engineSelector
    );
    
    auto* bypassParam = processor.getValueTreeState().getParameter("slot" + juce::String(slot + 1) + "_bypass");
    if (bypassParam)
    {
        bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor.getValueTreeState(), "slot" + juce::String(slot + 1) + "_bypass", bypassButton
        );
    }
    
    auto* mixParam = processor.getValueTreeState().getParameter("slot" + juce::String(slot + 1) + "_mix");
    if (mixParam)
    {
        mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor.getValueTreeState(), "slot" + juce::String(slot + 1) + "_mix", mixSlider
        );
    }
    
    // Parameter sliders - create 8 static ones
    juce::StringArray defaultParamNames = {
        "Param 1", "Param 2", "Param 3", "Param 4",
        "Param 5", "Param 6", "Param 7", "Param 8"
    };
    
    for (int i = 0; i < PARAMS_PER_SLOT; ++i)
    {
        // Label
        paramLabels[i].setText(defaultParamNames[i], juce::dontSendNotification);
        paramLabels[i].setJustificationType(juce::Justification::centred);
        paramLabels[i].setFont(juce::Font(10.0f));
        paramLabels[i].setColour(juce::Label::textColourId, juce::Colours::grey);
        addAndMakeVisible(paramLabels[i]);
        
        // Slider
        paramSliders[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
        paramSliders[i].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        paramSliders[i].setRange(0.0, 1.0);
        paramSliders[i].setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
        paramSliders[i].setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
        addAndMakeVisible(paramSliders[i]);
        
        // Try to attach to parameter
        juce::String paramId = "slot" + juce::String(slot + 1) + "_param" + juce::String(i + 1);
        auto* param = processor.getValueTreeState().getParameter(paramId);
        if (param)
        {
            paramAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                processor.getValueTreeState(), paramId, paramSliders[i]
            );
        }
    }
}

SlotPanel::~SlotPanel()
{
}

void SlotPanel::paint(juce::Graphics& g)
{
    // Background
    g.setColour(juce::Colour(0xff1e1e1e));
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);
    
    // Border
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);
    
    // Active indicator
    if (engineSelector.getSelectedId() > 1)
    {
        g.setColour(juce::Colours::orange.withAlpha(0.3f));
        g.drawRoundedRectangle(getLocalBounds().reduced(1).toFloat(), 4.0f, 2.0f);
    }
}

void SlotPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    
    // Top row - slot label and bypass
    auto topRow = bounds.removeFromTop(20);
    slotLabel.setBounds(topRow.removeFromLeft(60));
    bypassButton.setBounds(topRow.removeFromRight(60));
    
    // Engine selector
    bounds.removeFromTop(4);
    engineSelector.setBounds(bounds.removeFromTop(22));
    
    // Mix slider
    bounds.removeFromTop(4);
    mixSlider.setBounds(bounds.removeFromTop(20));
    
    // Parameters in 2 rows of 4
    bounds.removeFromTop(8);
    auto paramArea = bounds.removeFromTop(100);
    
    const int paramSize = 45;
    const int paramSpacing = (getWidth() - 16 - (4 * paramSize)) / 3;
    
    for (int row = 0; row < 2; ++row)
    {
        auto rowBounds = row == 0 ? paramArea.removeFromTop(50) : paramArea;
        int x = 0;
        
        for (int col = 0; col < 4; ++col)
        {
            int paramIndex = row * 4 + col;
            if (paramIndex < PARAMS_PER_SLOT)
            {
                auto paramBounds = juce::Rectangle<int>(x, rowBounds.getY(), paramSize, 50);
                paramSliders[paramIndex].setBounds(paramBounds.removeFromTop(35));
                paramLabels[paramIndex].setBounds(paramBounds);
                x += paramSize + paramSpacing;
            }
        }
    }
}

//==============================================================================
// PluginEditorComplete Implementation
//==============================================================================
PluginEditorComplete::PluginEditorComplete(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set look and feel
    lookAndFeel.setColour(juce::Slider::textBoxTextColourId, juce::Colours::lightgrey);
    lookAndFeel.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    lookAndFeel.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a2a));
    lookAndFeel.setColour(juce::ComboBox::textColourId, juce::Colours::lightgrey);
    lookAndFeel.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3a3a));
    setLookAndFeel(&lookAndFeel);
    
    setSize(900, 720);
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);
    
    // Status
    statusLabel.setText("Multi-Engine Processor", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(12.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(statusLabel);
    
    // Preset buttons
    presetButton.setButtonText("Load Preset");
    presetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    addAndMakeVisible(presetButton);
    
    saveButton.setButtonText("Save Preset");
    saveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a3a3a));
    addAndMakeVisible(saveButton);
    
    // Create slot panels
    for (int i = 0; i < 6; ++i)
    {
        slotPanels[i] = std::make_unique<SlotPanel>(audioProcessor, i);
        addAndMakeVisible(slotPanels[i].get());
    }
    
    // Master section
    masterGroup.setText("MASTER");
    masterGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff4a4a4a));
    masterGroup.setColour(juce::GroupComponent::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(masterGroup);
    
    // Master Gain
    masterGainLabel.setText("Gain", juce::dontSendNotification);
    masterGainLabel.setJustificationType(juce::Justification::centred);
    masterGainLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(masterGainLabel);
    
    masterGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    masterGainSlider.setRange(-60.0, 12.0);
    masterGainSlider.setTextValueSuffix(" dB");
    masterGainSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    masterGainSlider.setColour(juce::Slider::trackColourId, juce::Colours::orange);
    addAndMakeVisible(masterGainSlider);
    
    // Master Mix
    masterMixLabel.setText("Mix", juce::dontSendNotification);
    masterMixLabel.setJustificationType(juce::Justification::centred);
    masterMixLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(masterMixLabel);
    
    masterMixSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    masterMixSlider.setRange(0.0, 100.0);
    masterMixSlider.setTextValueSuffix("%");
    masterMixSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    masterMixSlider.setColour(juce::Slider::trackColourId, juce::Colours::cyan);
    addAndMakeVisible(masterMixSlider);
    
    // Master Bypass
    masterBypassButton.setButtonText("Master Bypass");
    masterBypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(masterBypassButton);
    
    // Create master attachments
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

PluginEditorComplete::~PluginEditorComplete()
{
    setLookAndFeel(nullptr);
}

void PluginEditorComplete::paint(juce::Graphics& g)
{
    // Background gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1a1a1a), 0, 0,
        juce::Colour(0xff0a0a0a), 0, (float)getHeight(),
        false
    ));
    g.fillAll();
    
    // Header background
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(0, 0, getWidth(), 70);
    
    // Header line
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawLine(0, 70, (float)getWidth(), 70, 1.0f);
}

void PluginEditorComplete::resized()
{
    auto bounds = getLocalBounds();
    
    // Header section
    auto header = bounds.removeFromTop(70);
    header.reduce(20, 10);
    
    auto titleArea = header.removeFromTop(35);
    titleLabel.setBounds(titleArea);
    
    auto subHeader = header;
    statusLabel.setBounds(subHeader.removeFromLeft(200));
    saveButton.setBounds(subHeader.removeFromRight(100));
    subHeader.removeFromRight(10);
    presetButton.setBounds(subHeader.removeFromRight(100));
    
    bounds.removeFromTop(10);
    
    // Main area - slots on left, master on right
    auto mainArea = bounds.reduced(10);
    auto masterArea = mainArea.removeFromRight(140);
    
    // Layout slots in 3x2 grid
    const int slotWidth = mainArea.getWidth() / 3;
    const int slotHeight = mainArea.getHeight() / 2;
    const int padding = 5;
    
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 3;
        int row = i / 3;
        
        slotPanels[i]->setBounds(
            mainArea.getX() + col * slotWidth + padding,
            mainArea.getY() + row * slotHeight + padding,
            slotWidth - padding * 2,
            slotHeight - padding * 2
        );
    }
    
    // Master section
    masterGroup.setBounds(masterArea);
    masterArea.reduce(10, 20);
    masterArea.removeFromTop(10);
    
    // Master controls
    auto masterControls = masterArea.removeFromTop(masterArea.getHeight() - 30);
    
    auto gainSection = masterControls.removeFromLeft(60);
    masterGainLabel.setBounds(gainSection.removeFromTop(20));
    masterGainSlider.setBounds(gainSection);
    
    masterControls.removeFromLeft(10);
    
    auto mixSection = masterControls;
    masterMixLabel.setBounds(mixSection.removeFromTop(20));
    masterMixSlider.setBounds(mixSection);
    
    // Master bypass at bottom
    masterBypassButton.setBounds(masterArea.reduced(5));
}