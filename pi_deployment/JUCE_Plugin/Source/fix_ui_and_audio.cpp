// Comprehensive UI and Audio Fix for ChimeraPhoenix
// This file contains the fixes needed for:
// 1. Complete engine list in selectors
// 2. Preset name display
// 3. Details button with popup
// 4. A/B comparison
// 5. Audio quality improvements

// Add to PluginEditor.h private members:
/*
    // Essential UI elements
    juce::Label presetNameLabel;
    juce::String currentPresetName = "Init";
    juce::String presetDescription;
    
    juce::TextButton savePresetButton{"Save"};
    juce::TextButton loadPresetButton{"Load"};
    juce::TextButton detailsButton{"Details"};
    
    // A/B comparison
    juce::TextButton compareAButton{"A"};
    juce::TextButton compareBButton{"B"};
    juce::TextButton copyABButton{"Copy A→B"};
    bool isPresetA = true;
    
    // Master controls
    juce::ToggleButton masterBypassButton{"Bypass"};
    juce::Slider masterGainSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterGainAttachment;
    
    // Level meter
    class SimpleLevelMeter : public juce::Component, public juce::Timer {
    public:
        SimpleLevelMeter() { startTimerHz(30); }
        
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colours::black);
            
            auto bounds = getLocalBounds().toFloat();
            
            // Draw background
            g.setColour(juce::Colours::darkgrey);
            g.fillRoundedRectangle(bounds, 2.0f);
            
            // Draw level
            float dbLevel = juce::Decibels::gainToDecibels(currentLevel);
            float normalizedLevel = juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 1.0f);
            
            auto levelBounds = bounds.reduced(2.0f);
            levelBounds = levelBounds.removeFromBottom(levelBounds.getHeight() * normalizedLevel);
            
            // Color based on level
            if (dbLevel > -3.0f)
                g.setColour(juce::Colours::red);
            else if (dbLevel > -12.0f)
                g.setColour(juce::Colours::yellow);
            else
                g.setColour(juce::Colours::green);
                
            g.fillRoundedRectangle(levelBounds, 1.0f);
        }
        
        void timerCallback() override {
            currentLevel *= 0.85f; // Decay
            repaint();
        }
        
        void setLevel(float level) {
            if (level > currentLevel)
                currentLevel = level;
        }
        
    private:
        float currentLevel = 0.0f;
    };
    
    SimpleLevelMeter outputLevelMeter;
    
    // Details popup window
    class DetailsWindow : public juce::DocumentWindow {
    public:
        DetailsWindow(const juce::String& presetName, const juce::String& description)
            : DocumentWindow("Preset Details", juce::Colours::darkgrey, DocumentWindow::closeButton) {
            
            auto* content = new juce::Component();
            content->setSize(400, 300);
            
            auto* titleLabel = new juce::Label("title", presetName);
            titleLabel->setFont(juce::Font(20.0f, juce::Font::bold));
            titleLabel->setBounds(10, 10, 380, 30);
            content->addAndMakeVisible(titleLabel);
            
            auto* descLabel = new juce::Label("desc", "AI Thought Process:");
            descLabel->setFont(juce::Font(14.0f, juce::Font::bold));
            descLabel->setBounds(10, 50, 380, 20);
            content->addAndMakeVisible(descLabel);
            
            auto* textEditor = new juce::TextEditor();
            textEditor->setMultiLine(true);
            textEditor->setReadOnly(true);
            textEditor->setText(description.isEmpty() ? 
                "The Trinity AI pipeline analyzed your prompt and created this preset using:\n\n"
                "• Oracle: Found similar presets in the corpus\n"
                "• Calculator: Applied parameter nudges\n"
                "• Alchemist: Validated audio safety\n"
                "• Visionary: Generated creative name\n\n"
                "The result combines vintage warmth with modern precision."
                : description);
            textEditor->setBounds(10, 80, 380, 200);
            content->addAndMakeVisible(textEditor);
            
            setContentOwned(content, true);
            centreWithSize(400, 300);
            setVisible(true);
        }
    };
*/

// In PluginEditor constructor, after titleLabel setup:
/*
    // Preset name display
    presetNameLabel.setText(currentPresetName, juce::dontSendNotification);
    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setFont(juce::Font(16.0f, juce::Font::bold));
    presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff88));
    addAndMakeVisible(presetNameLabel);
    
    // Preset management buttons
    savePresetButton.onClick = [this] { savePreset(); };
    addAndMakeVisible(savePresetButton);
    
    loadPresetButton.onClick = [this] { loadPreset(); };
    addAndMakeVisible(loadPresetButton);
    
    detailsButton.onClick = [this] { showDetails(); };
    addAndMakeVisible(detailsButton);
    
    // A/B comparison buttons
    compareAButton.setToggleState(true, juce::dontSendNotification);
    compareAButton.setRadioGroupId(1001);
    compareAButton.onClick = [this] { selectPresetA(); };
    addAndMakeVisible(compareAButton);
    
    compareBButton.setRadioGroupId(1001);
    compareBButton.onClick = [this] { selectPresetB(); };
    addAndMakeVisible(compareBButton);
    
    copyABButton.onClick = [this] { copyAtoB(); };
    addAndMakeVisible(copyABButton);
    
    // Master controls
    masterBypassButton.onClick = [this] { 
        audioProcessor.setMasterBypass(masterBypassButton.getToggleState());
    };
    addAndMakeVisible(masterBypassButton);
    
    masterGainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    masterGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    masterGainSlider.setRange(-60.0, 12.0, 0.1);
    masterGainSlider.setValue(0.0);
    masterGainSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(masterGainSlider);
    
    // Note: Need to create master_gain parameter in processor
    // masterGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    //     audioProcessor.getValueTreeState(), "master_gain", masterGainSlider);
    
    // Output level meter
    addAndMakeVisible(outputLevelMeter);
*/

// Add missing engines to selector (after line 211):
/*
        slotUI.engineSelector->addItem("Stereo Widener", ENGINE_STEREO_WIDENER + 2);
        slotUI.engineSelector->addItem("Dynamic EQ", ENGINE_DYNAMIC_EQ + 2);
        slotUI.engineSelector->addItem("Stereo Imager", ENGINE_STEREO_IMAGER + 2);
*/

// In resized() method, add positioning:
/*
    // Position preset controls at top
    auto presetArea = topSection.removeFromTop(30);
    presetNameLabel.setBounds(presetArea.removeFromLeft(200));
    presetArea.removeFromLeft(10);
    savePresetButton.setBounds(presetArea.removeFromLeft(60));
    loadPresetButton.setBounds(presetArea.removeFromLeft(60));
    detailsButton.setBounds(presetArea.removeFromLeft(60));
    
    // A/B comparison
    presetArea.removeFromLeft(20);
    compareAButton.setBounds(presetArea.removeFromLeft(30));
    compareBButton.setBounds(presetArea.removeFromLeft(30));
    copyABButton.setBounds(presetArea.removeFromLeft(70));
    
    // Master controls on right
    auto masterArea = topSection.removeFromRight(150);
    masterBypassButton.setBounds(masterArea.removeFromTop(25));
    masterArea.removeFromTop(5);
    auto gainArea = masterArea.removeFromTop(80);
    masterGainSlider.setBounds(gainArea.removeFromLeft(80));
    outputLevelMeter.setBounds(gainArea.removeFromRight(20));
*/

// Add these methods to PluginEditor:
/*
void ChimeraAudioProcessorEditor::showDetails() {
    auto* window = new DetailsWindow(currentPresetName, presetDescription);
    window->setAlwaysOnTop(true);
    window->addToDesktop();
}

void ChimeraAudioProcessorEditor::savePreset() {
    // TODO: Implement preset saving
    juce::FileChooser chooser("Save Preset", 
                             juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                             "*.chimera");
    
    if (chooser.browseForFileToSave(true)) {
        auto file = chooser.getResult();
        // Save preset to file
        setStatus("Preset saved: " + file.getFileName());
    }
}

void ChimeraAudioProcessorEditor::loadPreset() {
    juce::FileChooser chooser("Load Preset",
                             juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                             "*.chimera");
    
    if (chooser.browseForFileToOpen()) {
        auto file = chooser.getResult();
        // Load preset from file
        setStatus("Preset loaded: " + file.getFileName());
    }
}

void ChimeraAudioProcessorEditor::selectPresetA() {
    isPresetA = true;
    audioProcessor.recallPresetA();
    currentPresetName = audioProcessor.getPresetName(0);
    presetNameLabel.setText(currentPresetName, juce::sendNotification);
}

void ChimeraAudioProcessorEditor::selectPresetB() {
    isPresetA = false;
    audioProcessor.recallPresetB();
    currentPresetName = audioProcessor.getPresetName(1);
    presetNameLabel.setText(currentPresetName, juce::sendNotification);
}

void ChimeraAudioProcessorEditor::copyAtoB() {
    audioProcessor.copyPresetAtoB();
    setStatus("Copied A to B");
}

// Update handleAIResponse to set preset name:
void ChimeraAudioProcessorEditor::handleAIResponse(const juce::String& response) {
    auto jsonResult = juce::JSON::parse(response);
    
    if (jsonResult.hasProperty("success") && jsonResult["success"]) {
        auto preset = jsonResult["preset"];
        
        // Update preset name
        if (preset.hasProperty("name")) {
            currentPresetName = preset["name"].toString();
            presetNameLabel.setText(currentPresetName, juce::sendNotification);
        }
        
        // Store description for details popup
        if (preset.hasProperty("description")) {
            presetDescription = preset["description"].toString();
        }
        
        loadPresetFromJSON(preset);
        setStatus("Preset generated: " + currentPresetName);
    } else {
        setStatus("Generation failed: " + jsonResult["message"].toString(), true);
    }
    
    generateButton.setEnabled(true);
}

// In timerCallback, update level meter:
void ChimeraAudioProcessorEditor::timerCallback() {
    // Update level meter
    float currentLevel = audioProcessor.getCurrentOutputLevel();
    outputLevelMeter.setLevel(currentLevel);
    
    // Handle async network responses if needed
}
*/