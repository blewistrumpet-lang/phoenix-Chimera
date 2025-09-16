#include "PluginEditorBasic.h"

PluginEditorBasic::PluginEditorBasic(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p)
{
    // Absolute minimal - just set size and add a label
    setSize(600, 400);
    
    infoLabel.setText("CHIMERA PHOENIX - BASIC UI\n\nIf you can see this, the plugin loaded successfully.\n\nThis is a minimal UI for testing.", 
                     juce::dontSendNotification);
    infoLabel.setJustificationType(juce::Justification::centred);
    infoLabel.setFont(juce::Font(16.0f));
    addAndMakeVisible(infoLabel);
}

PluginEditorBasic::~PluginEditorBasic()
{
    // Nothing to clean up
}

void PluginEditorBasic::paint(juce::Graphics& g)
{
    // Simple background
    g.fillAll(juce::Colours::darkgrey);
}

void PluginEditorBasic::resized()
{
    infoLabel.setBounds(getLocalBounds());
}