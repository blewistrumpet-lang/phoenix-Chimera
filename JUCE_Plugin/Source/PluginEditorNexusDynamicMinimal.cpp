#include "PluginEditorNexusDynamicMinimal.h"

//==============================================================================
PluginEditorNexusDynamicMinimal::PluginEditorNexusDynamicMinimal(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set basic size
    setSize(800, 400);
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX - Dynamic UI (Minimal Test)", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Engine selector for slot 1
    slot1EngineSelector.setTextWhenNothingSelected("Select Engine");
    
    // Safely get parameter and populate choices
    auto* param = audioProcessor.getValueTreeState().getParameter("slot1_engine");
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        for (const auto& choice : choiceParam->choices)
        {
            slot1EngineSelector.addItem(choice, slot1EngineSelector.getNumItems() + 1);
        }
    }
    
    addAndMakeVisible(slot1EngineSelector);
    
    // Create attachment
    slot1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "slot1_engine", slot1EngineSelector
    );
    
    // Parameter info label
    paramInfoLabel.setText("Select an engine to see live parameter info", juce::dontSendNotification);
    paramInfoLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(paramInfoLabel);
    
    // Update parameter info when selection changes
    slot1EngineSelector.onChange = [this]()
    {
        // Get selected engine
        auto& engine = audioProcessor.getEngine(0);
        if (engine)
        {
            // Query live engine for parameters
            int numParams = engine->getNumParameters();
            juce::String info = "Live Engine Info:\n";
            info += "Parameters: " + juce::String(numParams) + "\n";
            
            for (int i = 0; i < numParams && i < 5; ++i)
            {
                info += "  " + juce::String(i+1) + ": " + engine->getParameterName(i) + "\n";
            }
            
            if (numParams > 5)
            {
                info += "  ... and " + juce::String(numParams - 5) + " more\n";
            }
            
            paramInfoLabel.setText(info, juce::dontSendNotification);
        }
        else
        {
            paramInfoLabel.setText("No engine selected", juce::dontSendNotification);
        }
    };
}

PluginEditorNexusDynamicMinimal::~PluginEditorNexusDynamicMinimal()
{
}

void PluginEditorNexusDynamicMinimal::paint(juce::Graphics& g)
{
    // Simple gradient background
    g.fillAll(juce::Colour(0xff1F2937));
    
    // Draw border
    g.setColour(juce::Colour(0xff00ffcc));
    g.drawRect(getLocalBounds(), 2);
}

void PluginEditorNexusDynamicMinimal::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    titleLabel.setBounds(bounds.removeFromTop(40));
    bounds.removeFromTop(20);
    
    slot1EngineSelector.setBounds(bounds.removeFromTop(30).withWidth(300));
    bounds.removeFromTop(20);
    
    paramInfoLabel.setBounds(bounds);
}