#include "TestEditorIncremental.h"

// STEP 2: Add static components
TestEditorIncremental::TestEditorIncremental(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    DBG("TestEditorIncremental: Constructor starting - WITH STATIC COMPONENTS");
    
    // Set size
    setSize(1200, 800);
    setResizable(false, false);
    
    // Add a title label (static component)
    DBG("Creating title label...");
    titleLabel = std::make_unique<juce::Label>();
    titleLabel->setText("CHIMERA PHOENIX TEST", juce::dontSendNotification);
    titleLabel->setFont(juce::Font(24.0f));
    titleLabel->setJustificationType(juce::Justification::centred);
    titleLabel->setColour(juce::Label::textColourId, juce::Colour(0xff00ffcc));
    addAndMakeVisible(titleLabel.get());
    DBG("Title label created");
    
    // Add a master panel (static component)
    DBG("Creating master panel...");
    masterPanel = std::make_unique<juce::Component>();
    masterPanel->setName("MasterPanel");
    addAndMakeVisible(masterPanel.get());
    DBG("Master panel created");
    
    // STEP 3: Add SlotComponent creation (FIXED)
    DBG("Creating SlotComponents (Fixed version)...");
    for (int i = 0; i < 6; ++i)
    {
        DBG("  Creating slot " + juce::String(i) + "...");
        slots[i] = std::make_unique<SlotComponentFixed>(i);
        addAndMakeVisible(slots[i].get());
        
        // CRITICAL: Initialize components AFTER adding to parent
        DBG("  Initializing slot " + juce::String(i) + " components...");
        slots[i]->initializeComponents();
        
        DBG("  Slot " + juce::String(i) + " created and initialized");
    }
    DBG("All SlotComponents created and initialized");
    
    DBG("TestEditorIncremental: Constructor completed successfully");
}

TestEditorIncremental::~TestEditorIncremental()
{
    DBG("TestEditorIncremental: Destructor called");
}

void TestEditorIncremental::paint(juce::Graphics& g)
{
    // Simple background
    g.fillAll(juce::Colour(0xff222222));
    
    // Draw text to show it's working
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawText("Test Editor - Minimal", getLocalBounds(), juce::Justification::centred);
}

void TestEditorIncremental::resized()
{
    auto bounds = getLocalBounds();
    
    if (titleLabel)
        titleLabel->setBounds(bounds.removeFromTop(60).reduced(10));
    
    if (masterPanel)
        masterPanel->setBounds(bounds.reduced(10));
}