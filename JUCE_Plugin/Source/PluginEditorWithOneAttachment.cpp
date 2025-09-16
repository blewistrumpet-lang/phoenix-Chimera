#include "PluginEditorWithOneAttachment.h"

PluginEditorWithOneAttachment::PluginEditorWithOneAttachment(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(800, 600);
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX - Testing ONE Attachment", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(18.0f));
    addAndMakeVisible(titleLabel);
    
    // Status
    statusLabel.setText("Only Slot 1 has parameter attachment", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(14.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    addAndMakeVisible(statusLabel);
    
    // Create all 6 selectors
    for (int i = 0; i < 6; ++i)
    {
        // Slot label
        slotLabels[i].setText("Slot " + juce::String(i + 1), juce::dontSendNotification);
        slotLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(slotLabels[i]);
        
        // Engine selector - populate with real engine names
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
        
        engineSelectors[i].setSelectedId(1);
        
        // Add onChange handler for all
        engineSelectors[i].onChange = [this, i]()
        {
            juce::String selectedName = engineSelectors[i].getText();
            juce::String attachmentStatus = (i == 0) ? " (ATTACHED)" : " (not attached)";
            statusLabel.setText("Slot " + juce::String(i + 1) + " selected: " + selectedName + attachmentStatus, 
                              juce::dontSendNotification);
        };
        
        addAndMakeVisible(engineSelectors[i]);
    }
    
    // Create ONE attachment for slot 0 only
    auto* param = audioProcessor.getValueTreeState().getParameter("engineType0");
    if (param)
    {
        slot0Attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getValueTreeState(), "engineType0", engineSelectors[0]
        );
        statusLabel.setText("Slot 1 attachment created successfully", juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("WARNING: Could not find engineType0 parameter", juce::dontSendNotification);
        statusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
}

PluginEditorWithOneAttachment::~PluginEditorWithOneAttachment()
{
    // Attachment will be destroyed automatically
}

void PluginEditorWithOneAttachment::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));
}

void PluginEditorWithOneAttachment::resized()
{
    auto bounds = getLocalBounds();
    
    titleLabel.setBounds(bounds.removeFromTop(50));
    statusLabel.setBounds(bounds.removeFromTop(30));
    
    bounds.removeFromTop(20);
    
    // Layout selectors in 2x3 grid
    int slotWidth = bounds.getWidth() / 2;
    int slotHeight = bounds.getHeight() / 3;
    
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 2;
        int row = i / 2;
        
        auto slotBounds = juce::Rectangle<int>(
            col * slotWidth + 20,
            bounds.getY() + row * slotHeight + 10,
            slotWidth - 40,
            slotHeight - 20
        );
        
        slotLabels[i].setBounds(slotBounds.removeFromTop(25));
        engineSelectors[i].setBounds(slotBounds.removeFromTop(30).reduced(10, 0));
    }
}