#include "PluginEditorFull.h"

PluginEditorFull::PluginEditorFull(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(900, 720);  // Adjusted to fit all controls properly
    
    // Title
    titleLabel.setText("CHIMERA PHOENIX", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setFont(juce::Font(26.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);
    
    // Logo button
    logoButton.setButtonText("CP");
    logoButton.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
    logoButton.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    logoButton.addListener(this);
    addAndMakeVisible(logoButton);
    
    // Status
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(11.0f));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgreen);
    addAndMakeVisible(statusLabel);
    
    // Preset section
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::left);
    presetLabel.setFont(juce::Font(10.0f));
    presetLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    addAndMakeVisible(presetLabel);
    
    presetCombo.addItem("-- Factory Presets --", 1);
    presetCombo.addItem("Clean Start", 2);
    presetCombo.addItem("Warm Vintage", 3);
    presetCombo.addItem("Modern Punch", 4);
    presetCombo.addItem("Ambient Space", 5);
    presetCombo.addItem("Aggressive Drive", 6);
    presetCombo.addItem("Subtle Enhancement", 7);
    presetCombo.addItem("-- User Presets --", 8);
    presetCombo.setSelectedId(2);
    presetCombo.addListener(this);
    addAndMakeVisible(presetCombo);
    
    prevPresetButton.setButtonText("<");
    prevPresetButton.addListener(this);
    addAndMakeVisible(prevPresetButton);
    
    nextPresetButton.setButtonText(">");
    nextPresetButton.addListener(this);
    addAndMakeVisible(nextPresetButton);
    
    savePresetButton.setButtonText("Save");
    savePresetButton.addListener(this);
    addAndMakeVisible(savePresetButton);
    
    loadPresetButton.setButtonText("Load");
    loadPresetButton.addListener(this);
    addAndMakeVisible(loadPresetButton);
    
    initButton.setButtonText("Init");
    initButton.addListener(this);
    addAndMakeVisible(initButton);
    
    compareButton.setButtonText("A/B");
    compareButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    compareButton.addListener(this);
    addAndMakeVisible(compareButton);
    
    // Trinity Server button
    aiButton.setButtonText("Trinity AI");
    aiButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff4a4aff));
    aiButton.setTooltip("Launch Trinity AI preset generator");
    aiButton.addListener(this);
    addAndMakeVisible(aiButton);
    
    aiStatusLabel.setText("Trinity: Ready", juce::dontSendNotification);
    aiStatusLabel.setJustificationType(juce::Justification::centred);
    aiStatusLabel.setFont(juce::Font(10.0f));
    aiStatusLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    addAndMakeVisible(aiStatusLabel);
    
    // Meters
    inputLabel.setText("IN", juce::dontSendNotification);
    inputLabel.setJustificationType(juce::Justification::centred);
    inputLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(inputLabel);
    
    outputLabel.setText("OUT", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centred);
    outputLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(outputLabel);
    
    addAndMakeVisible(inputMeterL);
    addAndMakeVisible(inputMeterR);
    addAndMakeVisible(outputMeterL);
    addAndMakeVisible(outputMeterR);
    
    // Create all 6 slots with ALL 56 engines
    juce::StringArray engineNames = {
        "-- None --",                  // 0
        "Vintage Opto Compressor",     // 1
        "Classic Compressor",           // 2
        "Transient Shaper",            // 3
        "Noise Gate",                  // 4
        "Mastering Limiter",           // 5
        "Dynamic EQ",                  // 6
        "Parametric EQ",               // 7
        "Vintage Console EQ",          // 8
        "Ladder Filter",               // 9
        "State Variable Filter",       // 10
        "Formant Filter",              // 11
        "Envelope Filter",             // 12
        "Comb Resonator",              // 13
        "Vocal Formant Filter",        // 14
        "Vintage Tube Preamp",         // 15
        "Wave Folder",                 // 16
        "Harmonic Exciter",            // 17
        "Bit Crusher",                 // 18
        "Multiband Saturator",         // 19
        "Muff Fuzz",                   // 20
        "Rodent Distortion",           // 21
        "K-Style Overdrive",           // 22
        "Stereo Chorus",               // 23
        "Resonant Chorus",             // 24
        "Analog Phaser",               // 25
        "Ring Modulator",              // 26
        "Frequency Shifter",           // 27
        "Harmonic Tremolo",            // 28
        "Classic Tremolo",             // 29
        "Rotary Speaker",              // 30
        "Pitch Shifter",               // 31
        "Detune Doubler",              // 32
        "Intelligent Harmonizer",      // 33
        "Tape Echo",                   // 34
        "Digital Delay",               // 35
        "Magnetic Drum Echo",          // 36
        "Bucket Brigade Delay",        // 37
        "Buffer Repeat",               // 38
        "Plate Reverb",                // 39
        "Spring Reverb",               // 40
        "Convolution Reverb",          // 41
        "Shimmer Reverb",              // 42
        "Gated Reverb",                // 43
        "Stereo Widener",              // 44
        "Stereo Imager",               // 45
        "Dimension Expander",          // 46
        "Spectral Freeze",             // 47
        "Spectral Gate",               // 48
        "Phased Vocoder",              // 49
        "Granular Cloud",              // 50
        "Chaos Generator",             // 51
        "Feedback Network",            // 52
        "Mid-Side Processor",          // 53
        "Gain Utility",                // 54
        "Mono Maker",                  // 55
        "Phase Align"                  // 56
    };
    
    juce::StringArray paramNamesDefault = {
        "Param 1", "Param 2", "Param 3", "Param 4",
        "Param 5", "Param 6", "Param 7", "Param 8"
    };
    
    for (int i = 0; i < 6; ++i)
    {
        // Slot label
        slotLabels[i].setText("SLOT " + juce::String(i + 1), juce::dontSendNotification);
        slotLabels[i].setJustificationType(juce::Justification::centredLeft);
        slotLabels[i].setFont(juce::Font(11.0f, juce::Font::bold));
        slotLabels[i].setColour(juce::Label::textColourId, juce::Colours::orange);
        addAndMakeVisible(slotLabels[i]);
        
        // Engine selector
        for (int j = 0; j < engineNames.size(); ++j)
        {
            engineSelectors[i].addItem(engineNames[j], j + 1);
        }
        
        engineSelectors[i].onChange = [this, i]()
        {
            // Get the selected engine ID
            int selectedId = engineSelectors[i].getSelectedId();
            
            if (selectedId > 0) // 0 means no selection
            {
                // The ComboBoxAttachment will handle loading the engine via parameter change
                // We just need to update the parameter names display
                
                // Get the actual engine instance from the processor
                auto& engine = audioProcessor.getEngine(i);
                
                if (engine)
                {
                    // Get parameter names directly from the engine
                    for (int j = 0; j < PARAMS_PER_SLOT; ++j)
                    {
                        // Each engine implements getParameterName()
                        juce::String paramName = engine->getParameterName(j);
                        
                        // Check if this is a valid parameter (non-empty name)
                        if (paramName.isNotEmpty() && paramName != "Unused")
                        {
                            slotParamLabels[i][j].setText(paramName, juce::dontSendNotification);
                            slotParamLabels[i][j].setVisible(true);
                            slotParamSliders[i][j].setVisible(true);
                        }
                        else
                        {
                            // Hide unused parameters
                            slotParamLabels[i][j].setVisible(false);
                            slotParamSliders[i][j].setVisible(false);
                        }
                    }
                }
                else
                {
                    // No engine instance yet - show generic names
                    for (int j = 0; j < PARAMS_PER_SLOT; ++j)
                    {
                        slotParamLabels[i][j].setText("Param " + juce::String(j + 1), juce::dontSendNotification);
                        slotParamLabels[i][j].setVisible(true);
                        slotParamSliders[i][j].setVisible(true);
                    }
                }
            }
            else
            {
                // No engine selected - hide all parameters
                for (int j = 0; j < PARAMS_PER_SLOT; ++j)
                {
                    slotParamLabels[i][j].setVisible(false);
                    slotParamSliders[i][j].setVisible(false);
                }
            }
            
            // Update status
            int count = 0;
            for (int j = 0; j < 6; ++j)
            {
                if (engineSelectors[j].getSelectedId() > 1) count++;
            }
            statusLabel.setText(juce::String(count) + " engines active", juce::dontSendNotification);
        };
        
        addAndMakeVisible(engineSelectors[i]);
        
        // Control buttons
        bypassButtons[i].setButtonText("B");
        bypassButtons[i].setTooltip("Bypass this slot's engine");
        bypassButtons[i].setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(bypassButtons[i]);
        
        soloButtons[i].setButtonText("S");
        soloButtons[i].setTooltip("Solo - Not yet implemented");
        soloButtons[i].setColour(juce::ToggleButton::textColourId, juce::Colours::yellow);
        soloButtons[i].setEnabled(false);  // Disable until implemented
        addAndMakeVisible(soloButtons[i]);
        
        muteButtons[i].setButtonText("M");
        muteButtons[i].setTooltip("Mute - Not yet implemented");
        muteButtons[i].setColour(juce::ToggleButton::textColourId, juce::Colours::red);
        muteButtons[i].setEnabled(false);  // Disable until implemented
        addAndMakeVisible(muteButtons[i]);
        
        // Mix slider
        slotMixLabels[i].setText("Mix:", juce::dontSendNotification);
        slotMixLabels[i].setTooltip("Dry/Wet mix for this slot");
        slotMixLabels[i].setJustificationType(juce::Justification::left);
        slotMixLabels[i].setFont(juce::Font(10.0f));
        slotMixLabels[i].setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(slotMixLabels[i]);
        
        slotMixSliders[i].setSliderStyle(juce::Slider::LinearHorizontal);
        slotMixSliders[i].setTextBoxStyle(juce::Slider::TextBoxRight, false, 35, 16);
        slotMixSliders[i].setRange(0.0, 100.0);
        slotMixSliders[i].setTextValueSuffix("%");
        slotMixSliders[i].setValue(100.0);
        slotMixSliders[i].setTooltip("Dry/Wet mix: 0% = dry signal only, 100% = wet signal only");
        slotMixSliders[i].setColour(juce::Slider::trackColourId, juce::Colours::orange);
        addAndMakeVisible(slotMixSliders[i]);
        
        // Create attachments - use correct parameter name format
        juce::String paramName = "slot" + juce::String(i + 1) + "_engine";
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
        
        // Parameter sliders
        for (int j = 0; j < PARAMS_PER_SLOT; ++j)
        {
            slotParamLabels[i][j].setText(paramNamesDefault[j], juce::dontSendNotification);
            slotParamLabels[i][j].setJustificationType(juce::Justification::centred);
            slotParamLabels[i][j].setFont(juce::Font(9.0f));
            slotParamLabels[i][j].setColour(juce::Label::textColourId, juce::Colours::grey);
            addAndMakeVisible(slotParamLabels[i][j]);
            
            slotParamSliders[i][j].setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slotParamSliders[i][j].setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            slotParamSliders[i][j].setRange(0.0, 1.0);
            slotParamSliders[i][j].setValue(0.5);
            slotParamSliders[i][j].setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::orange);
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
    masterGainLabel.setText("MASTER GAIN", juce::dontSendNotification);
    masterGainLabel.setJustificationType(juce::Justification::centred);
    masterGainLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    masterGainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(masterGainLabel);
    
    masterGainSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    masterGainSlider.setRange(-60.0, 12.0);
    masterGainSlider.setTextValueSuffix(" dB");
    masterGainSlider.setColour(juce::Slider::trackColourId, juce::Colours::white);
    addAndMakeVisible(masterGainSlider);
    
    masterMixLabel.setText("DRY/WET", juce::dontSendNotification);
    masterMixLabel.setJustificationType(juce::Justification::centred);
    masterMixLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    masterMixLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(masterMixLabel);
    
    masterMixSlider.setSliderStyle(juce::Slider::LinearVertical);
    masterMixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    masterMixSlider.setRange(0.0, 100.0);
    masterMixSlider.setTextValueSuffix("%");
    masterMixSlider.setColour(juce::Slider::trackColourId, juce::Colours::cyan);
    addAndMakeVisible(masterMixSlider);
    
    masterBypassButton.setButtonText("BYPASS");
    masterBypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    addAndMakeVisible(masterBypassButton);
    
    // Additional controls
    panicButton.setButtonText("PANIC");
    panicButton.setTooltip("Reset all engines and clear audio buffers");
    panicButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkred);
    panicButton.addListener(this);
    addAndMakeVisible(panicButton);
    
    settingsButton.setButtonText("Settings");
    settingsButton.setTooltip("Open plugin settings");
    settingsButton.addListener(this);
    addAndMakeVisible(settingsButton);
    
    qualityLabel.setText("Quality", juce::dontSendNotification);
    qualityLabel.setJustificationType(juce::Justification::centred);
    qualityLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(qualityLabel);
    
    qualitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    qualitySlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    qualitySlider.setRange(0.0, 2.0, 1.0);
    qualitySlider.setValue(1.0);
    addAndMakeVisible(qualitySlider);
    
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
    
    // Initialize parameter names for already-selected engines
    for (int i = 0; i < 6; ++i)
    {
        // Get current engine selection from parameters
        juce::String engineParam = "slot" + juce::String(i + 1) + "_engine";
        auto* engineP = audioProcessor.getValueTreeState().getParameter(engineParam);
        if (engineP)
        {
            // Get the current value (0.0 to 1.0)
            float normalizedValue = engineP->getValue();
            // Convert to engine index (1-based for combo box)
            int engineIndex = static_cast<int>(normalizedValue * 56.0f) + 1;
            
            // Set the combo box to match current state
            engineSelectors[i].setSelectedId(engineIndex, juce::dontSendNotification);
            
            // Now trigger the onChange to update parameter names
            if (engineIndex > 0)
            {
                auto& engine = audioProcessor.getEngine(i);
                if (engine)
                {
                    // Update parameter names from the engine
                    for (int j = 0; j < PARAMS_PER_SLOT; ++j)
                    {
                        juce::String paramName = engine->getParameterName(j);
                        if (paramName.isNotEmpty() && paramName != "Unused")
                        {
                            slotParamLabels[i][j].setText(paramName, juce::dontSendNotification);
                            slotParamLabels[i][j].setVisible(true);
                            slotParamSliders[i][j].setVisible(true);
                        }
                        else
                        {
                            slotParamLabels[i][j].setVisible(false);
                            slotParamSliders[i][j].setVisible(false);
                        }
                    }
                }
            }
        }
    }
}

PluginEditorFull::~PluginEditorFull()
{
}

void PluginEditorFull::paint(juce::Graphics& g)
{
    // Background gradient
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff1a1a1a), 0, 0,
        juce::Colour(0xff0a0a0a), 0, (float)getHeight(),
        false
    ));
    g.fillAll();
    
    // Header background
    g.setColour(juce::Colour(0xff0f0f0f));
    g.fillRect(0, 0, getWidth(), 80);
    
    // Header line
    g.setColour(juce::Colours::orange.withAlpha(0.5f));
    g.drawLine(0, 80, (float)getWidth(), 80, 2.0f);
    
    // Draw slot backgrounds - adjusted for smaller window
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 3;
        int row = i / 3;
        
        auto slotBounds = juce::Rectangle<int>(
            8 + col * 240,   // Much tighter spacing
            88 + row * 310,  // Reduced spacing
            232,             // Smaller slot width
            302              // Keep height reasonable
        );
        
        g.setColour(juce::Colour(0xff181818));
        g.fillRoundedRectangle(slotBounds.toFloat(), 6.0f);
        
        // Active border
        if (engineSelectors[i].getSelectedId() > 1)
        {
            g.setColour(juce::Colours::orange.withAlpha(0.4f));
            g.drawRoundedRectangle(slotBounds.reduced(1).toFloat(), 6.0f, 2.0f);
        }
    }
    
    // Master section background - positioned within window bounds
    g.setColour(juce::Colour(0xff141414));
    g.fillRoundedRectangle(730.0f, 85.0f, 160.0f, 630.0f, 6.0f);
    
    // Meter backgrounds - adjusted for new master position
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(820, 200, 8, 180);
    g.fillRect(830, 200, 8, 180);
    g.fillRect(820, 440, 8, 180);
    g.fillRect(830, 440, 8, 180);
}

void PluginEditorFull::resized()
{
    auto bounds = getLocalBounds();
    
    // Header section
    auto header = bounds.removeFromTop(80);
    
    // Top row
    auto topRow = header.removeFromTop(45).reduced(10, 5);
    logoButton.setBounds(topRow.removeFromLeft(40));
    topRow.removeFromLeft(10);
    titleLabel.setBounds(topRow.removeFromLeft(200));
    
    // Preset section
    topRow.removeFromLeft(20);
    auto presetSection = topRow.removeFromLeft(400);
    prevPresetButton.setBounds(presetSection.removeFromLeft(30));
    presetCombo.setBounds(presetSection.removeFromLeft(200));
    nextPresetButton.setBounds(presetSection.removeFromLeft(30));
    presetSection.removeFromLeft(10);
    savePresetButton.setBounds(presetSection.removeFromLeft(40));
    loadPresetButton.setBounds(presetSection.removeFromLeft(40));
    presetSection.removeFromLeft(5);
    initButton.setBounds(presetSection.removeFromLeft(35));
    compareButton.setBounds(presetSection.removeFromLeft(35));
    
    // AI button
    topRow.removeFromLeft(20);
    aiButton.setBounds(topRow.removeFromLeft(100));
    
    // Bottom row
    auto bottomRow = header.reduced(10, 0);
    presetLabel.setBounds(bottomRow.removeFromLeft(50));
    bottomRow.removeFromLeft(200);
    statusLabel.setBounds(bottomRow.removeFromLeft(150));
    bottomRow.removeFromLeft(20);
    aiStatusLabel.setBounds(bottomRow.removeFromLeft(100));
    
    // Settings
    settingsButton.setBounds(topRow.removeFromRight(60));
    
    // Layout slots in 3x2 grid - compact to make room for master
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 3;
        int row = i / 3;
        
        auto slotBounds = juce::Rectangle<int>(
            5 + col * 235,    // Tighter horizontal spacing for 3 columns
            85 + row * 310,   // Tighter vertical spacing
            225,              // Narrower slots to fit 3 columns + master
            300               // Keep height reasonable
        );
        
        // Slot header
        auto slotHeader = slotBounds.removeFromTop(25);
        slotLabels[i].setBounds(slotHeader.removeFromLeft(60));
        
        // Control buttons
        slotHeader.removeFromRight(10);
        muteButtons[i].setBounds(slotHeader.removeFromRight(25));
        soloButtons[i].setBounds(slotHeader.removeFromRight(25));
        bypassButtons[i].setBounds(slotHeader.removeFromRight(25));
        
        // Engine selector
        slotBounds.removeFromTop(5);
        engineSelectors[i].setBounds(slotBounds.removeFromTop(24).reduced(5, 0));
        
        // Mix row
        slotBounds.removeFromTop(5);
        auto mixRow = slotBounds.removeFromTop(22);
        slotMixLabels[i].setBounds(mixRow.removeFromLeft(30).reduced(5, 0));
        slotMixSliders[i].setBounds(mixRow.reduced(5, 2));
        
        // Parameters (2 rows of 4)
        slotBounds.removeFromTop(10);
        auto paramArea = slotBounds.removeFromTop(240);
        
        for (int j = 0; j < PARAMS_PER_SLOT; ++j)
        {
            int px = (j % 4) * 55;  // Tighter parameter spacing
            int py = (j / 4) * 120;
            
            auto paramBounds = juce::Rectangle<int>(
                paramArea.getX() + px + 5,  // Less padding
                paramArea.getY() + py + 10,
                50,  // Smaller knobs
                100
            );
            
            slotParamSliders[i][j].setBounds(paramBounds.removeFromTop(50));  // Smaller knobs
            slotParamLabels[i][j].setBounds(paramBounds);
        }
    }
    
    // Master section - positioned within 900px window
    auto masterBounds = juce::Rectangle<int>(730, 90, 160, 620);
    
    // Input meters - adjusted positions
    inputLabel.setBounds(1050, 180, 30, 20);
    inputMeterL.setBounds(820, 200, 8, 180);
    inputMeterR.setBounds(830, 200, 8, 180);
    
    // Output meters - adjusted positions
    outputLabel.setBounds(1050, 400, 30, 20);
    outputMeterL.setBounds(820, 440, 8, 180);
    outputMeterR.setBounds(830, 440, 8, 180);
    
    masterBounds.removeFromTop(20);
    
    // Master Gain
    masterGainLabel.setBounds(masterBounds.removeFromTop(20));
    masterGainSlider.setBounds(masterBounds.removeFromTop(150).reduced(25, 0));
    
    masterBounds.removeFromTop(20);
    
    // Master Mix
    masterMixLabel.setBounds(masterBounds.removeFromTop(20));
    masterMixSlider.setBounds(masterBounds.removeFromTop(150).reduced(25, 0));
    
    masterBounds.removeFromTop(30);
    
    // Master bypass
    masterBypassButton.setBounds(masterBounds.removeFromTop(35).reduced(20, 5));
    
    masterBounds.removeFromTop(20);
    
    // Panic button
    panicButton.setBounds(masterBounds.removeFromTop(30).reduced(20, 2));
    
    // Quality
    masterBounds.removeFromBottom(50);
    qualityLabel.setBounds(masterBounds.removeFromBottom(15));
    qualitySlider.setBounds(masterBounds.removeFromBottom(20).reduced(15, 0));
}

void PluginEditorFull::comboBoxChanged(juce::ComboBox* comboBox)
{
    // Handle preset selector
    if (comboBox == &presetCombo)
    {
        currentPresetIndex = presetCombo.getSelectedId();
        juce::String presetName = presetCombo.getText();
        statusLabel.setText("Loaded: " + presetName, juce::dontSendNotification);
    }
    // Engine selectors are handled by ComboBoxAttachment and onChange lambda
}

void PluginEditorFull::buttonClicked(juce::Button* button)
{
    if (button == &logoButton)
    {
        showAbout();
    }
    else if (button == &savePresetButton)
    {
        savePreset();
    }
    else if (button == &loadPresetButton)
    {
        loadPreset();
    }
    else if (button == &prevPresetButton)
    {
        if (currentPresetIndex > 1)
        {
            currentPresetIndex--;
            presetCombo.setSelectedId(currentPresetIndex);
        }
    }
    else if (button == &nextPresetButton)
    {
        if (currentPresetIndex < presetCombo.getNumItems())
        {
            currentPresetIndex++;
            presetCombo.setSelectedId(currentPresetIndex);
        }
    }
    else if (button == &initButton)
    {
        initializePreset();
    }
    else if (button == &compareButton)
    {
        compareMode = !compareMode;
        compareButton.setToggleState(compareMode, juce::dontSendNotification);
        statusLabel.setText(compareMode ? "A/B Compare: B" : "A/B Compare: A", juce::dontSendNotification);
    }
    else if (button == &aiButton)
    {
        // Launch Trinity AI preset generation dialog
        launchTrinityDialog();
    }
    else if (button == &panicButton)
    {
        // Reset all audio processing
        for (int i = 0; i < 6; ++i)
        {
            bypassButtons[i].setToggleState(true, juce::sendNotification);
        }
        statusLabel.setText("PANIC - All slots bypassed", juce::dontSendNotification);
    }
    else if (button == &settingsButton)
    {
        // Would open settings window
        statusLabel.setText("Settings coming soon", juce::dontSendNotification);
    }
}


void PluginEditorFull::loadPreset()
{
    statusLabel.setText("Loading preset...", juce::dontSendNotification);
}

void PluginEditorFull::savePreset()
{
    statusLabel.setText("Saving preset...", juce::dontSendNotification);
}

void PluginEditorFull::initializePreset()
{
    // Reset all to default
    for (int i = 0; i < 6; ++i)
    {
        engineSelectors[i].setSelectedId(1);
        slotMixSliders[i].setValue(100.0);
        bypassButtons[i].setToggleState(false, juce::sendNotification);
    }
    statusLabel.setText("Initialized", juce::dontSendNotification);
}

void PluginEditorFull::showAbout()
{
    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::InfoIcon,
        "Chimera Phoenix",
        "Version 3.0\n56 Premium DSP Engines\n6-Slot Serial Processing\nTrinity AI Preset Generation\n\n© 2025 Phoenix Audio Labs",
        "OK"
    );
}

void PluginEditorFull::launchTrinityDialog()
{
    // Create a simple prompt dialog
    auto* dialog = new juce::AlertWindow("Trinity AI Preset Generator",
                                         "Enter a creative prompt for AI preset generation:",
                                         juce::AlertWindow::NoIcon);
    
    dialog->addTextEditor("prompt", "", "e.g., warm vintage guitar with tube saturation");
    dialog->addButton("Generate", 1, juce::KeyPress(juce::KeyPress::returnKey));
    dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    
    dialog->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, dialog](int result)
        {
            if (result == 1)  // Generate button clicked
            {
                auto prompt = dialog->getTextEditorContents("prompt");
                if (prompt.isNotEmpty())
                {
                    sendTrinityRequest(prompt);
                }
            }
            delete dialog;
        }
    ));
}

void PluginEditorFull::sendTrinityRequest(const juce::String& prompt)
{
    aiStatusLabel.setText("Trinity: Generating...", juce::dontSendNotification);
    
    // Prepare JSON request
    juce::DynamicObject::Ptr requestObj = new juce::DynamicObject();
    requestObj->setProperty("prompt", prompt);
    requestObj->setProperty("max_generation_time", 30);
    
    juce::String jsonRequest = juce::JSON::toString(juce::var(requestObj.get()));
    
    // Create URL with POST data
    juce::URL trinityUrl("http://localhost:8000/generate");
    trinityUrl = trinityUrl.withPOSTData(jsonRequest);
    
    // Send async request to Trinity server
    auto options = juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
        .withConnectionTimeoutMs(30000)
        .withExtraHeaders("Content-Type: application/json");
    
    // Create a lambda callback for the async request
    auto callback = [this](juce::InputStream* stream, bool /*willFollowRedirect*/, 
                          const juce::String& /*redirectURL*/) -> bool
    {
        if (stream != nullptr)
        {
            juce::String response = stream->readEntireStreamAsString();
            DBG("Trinity raw response: " + response);
            
            // Parse response
            auto jsonResponse = juce::JSON::parse(response);
            DBG("Trinity JSON parsed successfully: " + juce::String(jsonResponse.isVoid() ? "NO" : "YES"));
            if (jsonResponse.hasProperty("success") && jsonResponse["success"])
            {
                auto preset = jsonResponse["preset"];
                DBG("Trinity response has preset property: " + juce::String(preset.isVoid() ? "NO" : "YES"));
                
                // Apply preset to the plugin
                juce::MessageManager::callAsync([this, preset]()
                {
                    
                    // Apply the preset parameters
                    DBG("Trinity response received, checking for slots property...");
                    if (preset.hasProperty("slots"))
                    {
                        auto slots = preset["slots"];
                        DBG("Trinity response has slots property, array: " + juce::String(slots.isArray() ? "YES" : "NO"));
                        if (slots.isArray())
                        {
                            DBG("Processing " + juce::String(slots.size()) + " slots from Trinity response");
                            for (int i = 0; i < juce::jmin(6, slots.size()); ++i)
                            {
                                auto slot = slots[i];
                                DBG("Processing slot " + juce::String(i) + ", has engine_id: " + juce::String(slot.hasProperty("engine_id") ? "YES" : "NO"));
                                if (slot.hasProperty("engine_id"))
                                {
                                    int engineId = slot["engine_id"];
                                    DBG("TRINITY SLOT " + juce::String(i) + ": Loading engine " + juce::String(engineId));
                                    
                                    // Convert engine ID to choice index
                                    int choiceIndex = ChimeraAudioProcessor::engineIDToChoiceIndex(engineId);
                                    DBG("Engine ID " + juce::String(engineId) + " maps to choice index " + juce::String(choiceIndex));
                                    
                                    // CRITICAL FIX: Directly load the engine first
                                    DBG("CALLING setSlotEngine(" + juce::String(i) + ", " + juce::String(engineId) + ")");
                                    audioProcessor.setSlotEngine(i, engineId);
                                    DBG("setSlotEngine call completed for slot " + juce::String(i));
                                    
                                    // Then update the UI dropdown to reflect the change
                                    // Use dontSendNotification to avoid triggering parameter change again
                                    if (i < 6 && engineSelectors[i].getNumItems() > choiceIndex) {
                                        DBG("Updating UI dropdown for slot " + juce::String(i) + " to choice " + juce::String(choiceIndex + 1));
                                        engineSelectors[i].setSelectedId(choiceIndex + 1, juce::dontSendNotification);
                                    }
                                }
                                
                                // Set parameters if available
                                if (slot.hasProperty("parameters"))
                                {
                                    auto params = slot["parameters"];
                                    if (params.isArray())
                                    {
                                        // Note: PluginEditorFull doesn't have parameter controls
                                        // The parameters will be set by the processor when the engine loads
                                        // with its default values or the preset values
                                    }
                                }
                            }
                        }
                    }
                    
                    aiStatusLabel.setText("Trinity: Preset Applied!", juce::dontSendNotification);
                    
                    // Reset status after 3 seconds
                    juce::Timer::callAfterDelay(3000, [this]()
                    {
                        aiStatusLabel.setText("Trinity: Ready", juce::dontSendNotification);
                    });
                });
            }
            else
            {
                juce::MessageManager::callAsync([this]()
                {
                    aiStatusLabel.setText("Trinity: Generation failed", juce::dontSendNotification);
                    juce::Timer::callAfterDelay(3000, [this]()
                    {
                        aiStatusLabel.setText("Trinity: Ready", juce::dontSendNotification);
                    });
                });
            }
        }
        else
        {
            // Server not responding
            juce::MessageManager::callAsync([this]()
            {
                aiStatusLabel.setText("Trinity: Server offline", juce::dontSendNotification);
                
                // Server is offline - Trinity server should be started automatically by PluginProcessor
                
                juce::Timer::callAfterDelay(5000, [this]()
                {
                    aiStatusLabel.setText("Trinity: Ready", juce::dontSendNotification);
                });
            });
        }
        return true;
    };
    
    // Start the async request
    juce::Thread::launch([trinityUrl, options, callback]()
    {
        std::unique_ptr<juce::InputStream> stream(trinityUrl.createInputStream(options));
        callback(stream.get(), false, juce::String());
    });
}

void PluginEditorFull::timerCallback()
{
    // Update UI meters or any other periodic updates
    // This can be left empty if no periodic updates are needed
    // or can update level meters, preset refresh, etc.
}