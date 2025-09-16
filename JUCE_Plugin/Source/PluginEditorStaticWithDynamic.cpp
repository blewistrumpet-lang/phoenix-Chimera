#include "PluginEditorStaticWithDynamic.h"

//==============================================================================
// SlotComponentStatic
//==============================================================================
SlotComponentStatic::SlotComponentStatic(ChimeraAudioProcessor& p, int slotIndex)
    : processor(p), slot(slotIndex)
{
    // Slot label
    slotLabel.setText("Slot " + juce::String(slot + 1), juce::dontSendNotification);
    slotLabel.setJustificationType(juce::Justification::centred);
    slotLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(slotLabel);
    
    // Engine selector
    engineSelector.addItem("None", 1);
    engineSelector.addItem("ClassicCompressor", 2);
    engineSelector.addItem("NoiseGate", 3);
    engineSelector.addItem("TransientShaper", 4);
    engineSelector.addItem("BitCrusher", 5);
    engineSelector.addItem("KStyleOverdrive", 6);
    engineSelector.addItem("ClassicChorus", 7);
    engineSelector.addItem("AnalogPhaser", 8);
    engineSelector.addItem("PlateReverb", 9);
    engineSelector.addItem("SpringReverb", 10);
    engineSelector.addItem("GatedReverb", 11);
    engineSelector.addItem("TapeEcho", 12);
    engineSelector.addItem("DigitalDelay", 13);
    engineSelector.addItem("HallReverb", 14);
    engineSelector.addItem("ShimmerReverb", 15);
    
    // Add more engines as needed...
    
    // Removed onChange to prevent dynamic updates for now
    // engineSelector.onChange = [this]()
    // {
    //     updateParameterDisplay();
    // };
    
    addAndMakeVisible(engineSelector);
    
    // Create engine selector attachment
    juce::String engineParamName = "engineType" + juce::String(slot);
    engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processor.getValueTreeState(), engineParamName, engineSelector
    );
    
    // Create all parameter controls (hidden initially)
    for (int i = 0; i < MAX_PARAMS; ++i)
    {
        // Parameter label
        paramLabels[i].setText("Param " + juce::String(i + 1), juce::dontSendNotification);
        paramLabels[i].setJustificationType(juce::Justification::left);
        paramLabels[i].setVisible(false);
        addAndMakeVisible(paramLabels[i]);
        
        // Parameter slider
        paramSliders[i].setSliderStyle(juce::Slider::RotaryVerticalDrag);
        paramSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
        paramSliders[i].setRange(0.0, 1.0);
        paramSliders[i].setValue(0.5);
        paramSliders[i].setVisible(false);
        addAndMakeVisible(paramSliders[i]);
        
        // We'll create attachments dynamically when engine changes
    }
    
    // Don't do initial update - just leave params hidden
    // updateParameterDisplay();
}

SlotComponentStatic::~SlotComponentStatic()
{
}

void SlotComponentStatic::resized()
{
    auto bounds = getLocalBounds();
    
    // Slot header
    slotLabel.setBounds(bounds.removeFromTop(20));
    engineSelector.setBounds(bounds.removeFromTop(25).reduced(5, 0));
    
    bounds.removeFromTop(10);
    
    // Layout parameter controls in a grid
    const int paramWidth = 60;
    const int paramHeight = 80;
    const int cols = 3;
    const int spacing = 5;
    
    for (int i = 0; i < MAX_PARAMS; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        
        auto paramBounds = juce::Rectangle<int>(
            col * (paramWidth + spacing) + spacing,
            bounds.getY() + row * (paramHeight + spacing),
            paramWidth,
            paramHeight
        );
        
        paramLabels[i].setBounds(paramBounds.removeFromTop(15));
        paramSliders[i].setBounds(paramBounds);
    }
}

void SlotComponentStatic::updateParameterDisplay()
{
    // DISABLED - This function was causing crashes
    // Just hide all parameters for now
    for (int i = 0; i < MAX_PARAMS; ++i)
    {
        paramLabels[i].setVisible(false);
        paramSliders[i].setVisible(false);
        paramAttachments[i].reset();
    }
    visibleParamCount = 0;
}

//==============================================================================
// PluginEditorStaticWithDynamic
//==============================================================================
PluginEditorStaticWithDynamic::PluginEditorStaticWithDynamic(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(900, 700);
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX - Static UI with Dynamic Parameters", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    addAndMakeVisible(titleLabel);
    
    // Status
    statusLabel.setText("Select engines to see their parameters", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(14.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(statusLabel);
    
    // Create slot components
    for (int i = 0; i < 6; ++i)
    {
        slots[i] = std::make_unique<SlotComponentStatic>(audioProcessor, i);
        addAndMakeVisible(slots[i].get());
    }
    
    // Master controls
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
    
    // Timer removed to prevent crashes
    // startTimer(100);
}

PluginEditorStaticWithDynamic::~PluginEditorStaticWithDynamic()
{
    // Timer removed
}

void PluginEditorStaticWithDynamic::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    
    // Draw slot backgrounds
    auto bounds = getLocalBounds();
    bounds.removeFromTop(100); // Skip header
    bounds.removeFromBottom(80); // Skip master section
    
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        
        auto slotBounds = juce::Rectangle<int>(
            col * (getWidth() / 2) + 10,
            100 + row * 180 + 10,
            (getWidth() / 2) - 20,
            170
        );
        
        g.setColour(juce::Colour(0xff2a2a2a));
        g.fillRoundedRectangle(slotBounds.toFloat(), 5.0f);
        
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawRoundedRectangle(slotBounds.toFloat(), 5.0f, 1.0f);
    }
    
    // Draw master section background
    auto masterBounds = getLocalBounds().removeFromBottom(70).reduced(10, 5);
    g.setColour(juce::Colour(0xff252525));
    g.fillRoundedRectangle(masterBounds.toFloat(), 5.0f);
}

void PluginEditorStaticWithDynamic::resized()
{
    auto bounds = getLocalBounds();
    
    // Header
    titleLabel.setBounds(bounds.removeFromTop(40));
    statusLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(20);
    
    // Layout slots in 2x3 grid
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        
        auto slotBounds = juce::Rectangle<int>(
            col * (getWidth() / 2) + 15,
            bounds.getY() + row * 180 + 15,
            (getWidth() / 2) - 30,
            160
        );
        
        slots[i]->setBounds(slotBounds);
    }
    
    // Master controls at bottom
    auto masterBounds = getLocalBounds().removeFromBottom(70).reduced(20, 10);
    
    auto gainSection = masterBounds.removeFromLeft(masterBounds.getWidth() / 2);
    masterGainLabel.setBounds(gainSection.removeFromTop(20));
    masterGainSlider.setBounds(gainSection.reduced(10, 5));
    
    auto mixSection = masterBounds;
    masterMixLabel.setBounds(mixSection.removeFromTop(20));
    masterMixSlider.setBounds(mixSection.reduced(10, 5));
}

// Timer callback removed to prevent crashes