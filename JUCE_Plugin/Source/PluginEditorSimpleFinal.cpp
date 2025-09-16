#include "PluginEditorSimpleFinal.h"

PluginEditorSimpleFinal::PluginEditorSimpleFinal(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(800, 500);
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(20.0f));
    addAndMakeVisible(titleLabel);
    
    // Status
    statusLabel.setText("Simple Working UI", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(14.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(statusLabel);
    
    // Create all 6 selectors and their attachments
    for (int i = 0; i < 6; ++i)
    {
        // Slot label
        slotLabels[i].setText("Slot " + juce::String(i + 1), juce::dontSendNotification);
        slotLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(slotLabels[i]);
        
        // Engine selector - populate with engine names
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
        
        // Add onChange to update status
        engineSelectors[i].onChange = [this, i]()
        {
            juce::String status = "Active: ";
            int count = 0;
            for (int j = 0; j < 6; ++j)
            {
                if (engineSelectors[j].getSelectedId() > 1)
                {
                    if (count > 0) status += ", ";
                    status += engineSelectors[j].getText();
                    count++;
                }
            }
            if (count == 0) status = "No engines selected";
            statusLabel.setText(status, juce::dontSendNotification);
        };
        
        addAndMakeVisible(engineSelectors[i]);
        
        // Create attachment
        juce::String paramName = "engineType" + juce::String(i);
        engineAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getValueTreeState(), paramName, engineSelectors[i]
        );
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
        
        // Attach to slot 1 parameters
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

PluginEditorSimpleFinal::~PluginEditorSimpleFinal()
{
}

void PluginEditorSimpleFinal::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
}

void PluginEditorSimpleFinal::resized()
{
    auto bounds = getLocalBounds();
    
    // Header
    titleLabel.setBounds(bounds.removeFromTop(40));
    statusLabel.setBounds(bounds.removeFromTop(25));
    
    bounds.removeFromTop(20);
    
    // Engine selectors in 2x3 grid
    auto selectorArea = bounds.removeFromTop(200);
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
        
        slotLabels[i].setBounds(slotBounds.removeFromTop(20));
        engineSelectors[i].setBounds(slotBounds.removeFromTop(25).reduced(10, 0));
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