#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "IntelligentHarmonizer.h"
#include "PitchShifter.h"

//==============================================================================
// CommandCenterLookAndFeel Implementation
//==============================================================================
CommandCenterLookAndFeel::CommandCenterLookAndFeel() {
    // Set default colors for the retrofuturist theme
    setColour(juce::Slider::textBoxTextColourId, primaryColor);
    setColour(juce::Slider::textBoxOutlineColourId, primaryColor.withAlpha(0.3f));
    setColour(juce::Label::textColourId, primaryColor);
    setColour(juce::TextEditor::textColourId, primaryColor);
    setColour(juce::TextEditor::backgroundColourId, panelColor);
    setColour(juce::TextEditor::outlineColourId, primaryColor.withAlpha(0.5f));
    setColour(juce::ComboBox::textColourId, primaryColor);
    setColour(juce::ComboBox::backgroundColourId, panelColor);
    setColour(juce::ComboBox::outlineColourId, primaryColor.withAlpha(0.5f));
}

void CommandCenterLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, 
                                               int width, int height,
                                               float sliderPos, 
                                               float rotaryStartAngle, 
                                               float rotaryEndAngle,
                                               juce::Slider& slider) {
    auto radius = juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = x + width * 0.5f;
    auto centreY = y + height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    
    // Background circle
    g.setColour(panelColor);
    g.fillEllipse(rx, ry, rw, rw);
    
    // Outer ring
    g.setColour(primaryColor.withAlpha(0.3f));
    g.drawEllipse(rx, ry, rw, rw, 2.0f);
    
    // Value arc
    juce::Path arc;
    arc.addCentredArc(centreX, centreY, radius - 5, radius - 5, 0.0f,
                      rotaryStartAngle, angle, true);
    g.setColour(slider.isEnabled() ? primaryColor : primaryColor.withAlpha(0.3f));
    g.strokePath(arc, juce::PathStrokeType(3.0f));
    
    // Center indicator
    juce::Path pointer;
    auto pointerLength = radius * 0.6f;
    auto pointerThickness = 3.0f;
    pointer.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    g.setColour(secondaryColor);
    g.fillPath(pointer);
}

void CommandCenterLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown) {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    
    auto baseColour = button.findColour(juce::TextButton::buttonColourId)
                            .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);
    
    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
    
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    g.setColour(primaryColor);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
}

void CommandCenterLookAndFeel::drawLabel(juce::Graphics& g, juce::Label& label) {
    g.fillAll(label.findColour(juce::Label::backgroundColourId));
    
    if (!label.isBeingEdited()) {
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const juce::Font font(label.getFont().withHeight(14.0f));
        
        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
        g.setFont(font);
        
        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
        g.drawFittedText(label.getText(), textArea, label.getJustificationType(),
                        juce::jmax(1, (int)(textArea.getHeight() / font.getHeight())),
                        label.getMinimumHorizontalScale());
    }
}

//==============================================================================
// ChimeraAudioProcessorEditor Implementation
//==============================================================================
ChimeraAudioProcessorEditor::ChimeraAudioProcessorEditor(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&lookAndFeel);
    
    // Title
    titleLabel.setText("CHIMERA COMMAND CENTER", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setFont(juce::Font(24.0f));
    addAndMakeVisible(titleLabel);
    
    // Prompt Box
    promptBox.setMultiLine(true);
    promptBox.setReturnKeyStartsNewLine(true);
    promptBox.setTextToShowWhenEmpty("Enter your sonic vision here...", 
                                    lookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.5f));
    promptBox.setScrollbarsShown(true);
    addAndMakeVisible(promptBox);
    
    // Generate Button
    generateButton.setButtonText("GENERATE");
    generateButton.onClick = [this] { generateButtonClicked(); };
    generateButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.2f));
    addAndMakeVisible(generateButton);
    
    // Status Label
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(statusLabel);
    
    // Preset Name Label
    presetNameLabel.setText(currentPresetName, juce::dontSendNotification);
    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    presetNameLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00ff88));
    addAndMakeVisible(presetNameLabel);
    
    // Preset Management Buttons
    savePresetButton.onClick = [this] { savePreset(); };
    savePresetButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.15f));
    addAndMakeVisible(savePresetButton);
    
    loadPresetButton.onClick = [this] { loadPreset(); };
    loadPresetButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.15f));
    addAndMakeVisible(loadPresetButton);
    
    detailsButton.onClick = [this] { showDetails(); };
    detailsButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.15f));
    addAndMakeVisible(detailsButton);
    
    // A/B Comparison Buttons
    compareAButton.setToggleState(true, juce::dontSendNotification);
    compareAButton.setRadioGroupId(1001);
    compareAButton.onClick = [this] { selectPresetA(); };
    compareAButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00d4ff).withAlpha(0.3f));
    compareAButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00d4ff).withAlpha(0.6f));
    addAndMakeVisible(compareAButton);
    
    compareBButton.setRadioGroupId(1001);
    compareBButton.onClick = [this] { selectPresetB(); };
    compareBButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffff6b00).withAlpha(0.3f));
    compareBButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff6b00).withAlpha(0.6f));
    addAndMakeVisible(compareBButton);
    
    copyABButton.onClick = [this] { copyAtoB(); };
    copyABButton.setColour(juce::TextButton::buttonColourId, lookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.15f));
    addAndMakeVisible(copyABButton);
    
    // Master Bypass
    masterBypassButton.onClick = [this] { 
        // TODO: Implement master bypass in processor
        setStatus(masterBypassButton.getToggleState() ? "Master Bypassed" : "Master Active");
    };
    masterBypassButton.setColour(juce::ToggleButton::textColourId, lookAndFeel.findColour(juce::Label::textColourId));
    addAndMakeVisible(masterBypassButton);
    
    // Output Level Meter
    addAndMakeVisible(outputLevelMeter);
    
    // Create Macro Controls
    for (int i = 0; i < 3; ++i) {
        auto& macro = macroControls[i];
        
        macro.slider = std::make_unique<juce::Slider>();
        macro.slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
        macro.slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        macro.slider->setRange(0.0, 1.0);
        macro.slider->setValue(0.5);
        addAndMakeVisible(macro.slider.get());
        
        macro.label = std::make_unique<juce::Label>();
        macro.label->setText("Macro " + juce::String(i + 1), juce::dontSendNotification);
        macro.label->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(macro.label.get());
    }
    
    // Create Slot UIs
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        auto& slotUI = slotUIs[slot];
        juce::String slotStr = juce::String(slot + 1);
        
        // Slot panel container
        addAndMakeVisible(slotUI.slotPanel);
        
        // Slot label
        slotUI.slotLabel.setText("SLOT " + slotStr, juce::dontSendNotification);
        slotUI.slotLabel.setJustificationType(juce::Justification::centred);
        slotUI.slotLabel.setFont(juce::Font(18.0f));
        slotUI.slotPanel.addAndMakeVisible(slotUI.slotLabel);
        
        // Engine selector
        slotUI.engineSelector = std::make_unique<juce::ComboBox>();
        
        // Dynamically populate ComboBox from APVTS parameter choices
        // This ensures UI matches the processor's engine order exactly
        auto* engineParam = dynamic_cast<juce::AudioParameterChoice*>(
            audioProcessor.getValueTreeState().getParameter("slot" + slotStr + "_engine"));
        
        if (engineParam) {
            // Add all engine choices with 1-based IDs (ComboBox requirement)
            for (int i = 0; i < engineParam->choices.size(); ++i) {
                slotUI.engineSelector->addItem(engineParam->choices[i], i + 1);
            }
        }
        slotUI.slotPanel.addAndMakeVisible(slotUI.engineSelector.get());
        
        slotUI.engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getValueTreeState(), "slot" + slotStr + "_engine", *slotUI.engineSelector);
        
        // Bypass button
        slotUI.bypassButton = std::make_unique<juce::ToggleButton>("Bypass");
        slotUI.slotPanel.addAndMakeVisible(slotUI.bypassButton.get());
        
        slotUI.bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getValueTreeState(), "slot" + slotStr + "_bypass", *slotUI.bypassButton);
        
        // Create parameter sliders
        for (int i = 0; i < 15; ++i) {
            auto slider = std::make_unique<juce::Slider>();
            slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
            slotUI.slotPanel.addAndMakeVisible(slider.get());
            
            auto label = std::make_unique<juce::Label>();
            label->setJustificationType(juce::Justification::centred);
            label->setFont(juce::Font(10.0f));
            slotUI.slotPanel.addAndMakeVisible(label.get());
            
            auto attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.getValueTreeState(), 
                "slot" + slotStr + "_param" + juce::String(i + 1), 
                *slider);
            
            slotUI.paramSliders.push_back(std::move(slider));
            slotUI.paramLabels.push_back(std::move(label));
            slotUI.sliderAttachments.push_back(std::move(attachment));
        }
    }
    
    // Add parameter listeners for all slots
    for (int slot = 1; slot <= NUM_SLOTS; ++slot) {
        audioProcessor.getValueTreeState().addParameterListener("slot" + juce::String(slot) + "_engine", this);
    }
    
    // Apply styling
    applyRetrofuturistStyling();
    
    // Initial parameter update for all slots
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        updateSlotParameters(slot);
    }
    
    // Start timer for network requests
    startTimer(100);
    
    // Larger window to accommodate 6 slots in 3x2 grid
    setSize(1200, 800);
}

ChimeraAudioProcessorEditor::~ChimeraAudioProcessorEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
    
    // Remove parameter listeners for all slots
    for (int slot = 1; slot <= NUM_SLOTS; ++slot) {
        audioProcessor.getValueTreeState().removeParameterListener("slot" + juce::String(slot) + "_engine", this);
    }
}

void ChimeraAudioProcessorEditor::paint(juce::Graphics& g) {
    g.fillAll(lookAndFeel.findColour(juce::ResizableWindow::backgroundColourId));
    
    // Draw top control panel background
    auto bounds = getLocalBounds();
    auto topPanel = bounds.removeFromTop(200).reduced(5);
    
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(topPanel.toFloat(), 10.0f);
    
    // Draw panel border
    g.setColour(lookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.3f));
    g.drawRoundedRectangle(topPanel.toFloat(), 10.0f, 1.0f);
    
    // Draw slot backgrounds
    auto slotSection = bounds.reduced(15);
    int slotWidth = slotSection.getWidth() / 3;
    int slotHeight = slotSection.getHeight() / 2;
    
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        int col = slot % 3;
        int row = slot / 3;
        
        auto slotBounds = juce::Rectangle<int>(
            slotSection.getX() + col * slotWidth,
            slotSection.getY() + row * slotHeight,
            slotWidth, slotHeight
        ).reduced(5).toFloat();
        
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRoundedRectangle(slotBounds, 10.0f);
        
        g.setColour(lookAndFeel.findColour(juce::Label::textColourId).withAlpha(0.3f));
        g.drawRoundedRectangle(slotBounds, 10.0f, 1.0f);
    }
}

void ChimeraAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();
    
    // Top section - Command Center controls
    auto topSection = bounds.removeFromTop(200).reduced(15);
    
    // Title and preset name on same line
    auto titleRow = topSection.removeFromTop(35);
    titleLabel.setBounds(titleRow.removeFromLeft(300));
    
    // Preset name and controls
    presetNameLabel.setBounds(titleRow.removeFromLeft(200));
    titleRow.removeFromLeft(10);
    savePresetButton.setBounds(titleRow.removeFromLeft(50));
    loadPresetButton.setBounds(titleRow.removeFromLeft(50));
    detailsButton.setBounds(titleRow.removeFromLeft(60));
    
    // A/B comparison on right
    titleRow.removeFromLeft(20);
    compareAButton.setBounds(titleRow.removeFromLeft(30));
    compareBButton.setBounds(titleRow.removeFromLeft(30));
    copyABButton.setBounds(titleRow.removeFromLeft(50));
    
    // Master bypass and meter on far right
    titleRow.removeFromLeft(20);
    masterBypassButton.setBounds(titleRow.removeFromLeft(100));
    outputLevelMeter.setBounds(titleRow.removeFromRight(20));
    
    topSection.removeFromTop(10);
    
    // Three columns for prompt/generate, macros, and status
    auto promptColumn = topSection.removeFromLeft(topSection.getWidth() / 2);
    
    promptBox.setBounds(promptColumn.removeFromTop(100));
    promptColumn.removeFromTop(10);
    generateButton.setBounds(promptColumn.removeFromTop(35).reduced(50, 0));
    
    // Right side - macros and status
    auto rightControls = topSection;
    statusLabel.setBounds(rightControls.removeFromTop(25));
    rightControls.removeFromTop(10);
    
    // Macro controls
    auto macroArea = rightControls;
    int macroWidth = macroArea.getWidth() / 3;
    for (int i = 0; i < 3; ++i) {
        auto macroBounds = macroArea.removeFromLeft(macroWidth).reduced(10);
        macroControls[i].label->setBounds(macroBounds.removeFromTop(20));
        macroControls[i].slider->setBounds(macroBounds);
    }
    
    // Bottom section - 3x2 grid of slots
    auto slotSection = bounds.reduced(15);
    int slotWidth = slotSection.getWidth() / 3;
    int slotHeight = slotSection.getHeight() / 2;
    
    for (int slot = 0; slot < NUM_SLOTS; ++slot) {
        auto& slotUI = slotUIs[slot];
        
        // Calculate position in grid
        int col = slot % 3;
        int row = slot / 3;
        
        auto slotBounds = juce::Rectangle<int>(
            slotSection.getX() + col * slotWidth,
            slotSection.getY() + row * slotHeight,
            slotWidth, slotHeight
        ).reduced(5);
        
        slotUI.slotPanel.setBounds(slotBounds);
        
        // Layout within slot panel
        auto panelBounds = slotUI.slotPanel.getLocalBounds();
        slotUI.slotLabel.setBounds(panelBounds.removeFromTop(25));
        
        auto controlRow = panelBounds.removeFromTop(30).reduced(5, 0);
        slotUI.engineSelector->setBounds(controlRow.removeFromLeft(180));
        controlRow.removeFromLeft(10);
        slotUI.bypassButton->setBounds(controlRow.removeFromLeft(60));
        
        // Parameter sliders in 3 rows of 5
        auto paramArea = panelBounds.reduced(5);
        int paramWidth = paramArea.getWidth() / 5;
        int paramHeight = paramArea.getHeight() / 3;
        
        for (int i = 0; i < 15; ++i) {
            int pRow = i / 5;
            int pCol = i % 5;
            
            auto paramBounds = juce::Rectangle<int>(
                paramArea.getX() + pCol * paramWidth,
                paramArea.getY() + pRow * paramHeight,
                paramWidth, paramHeight
            ).reduced(3);
            
            slotUI.paramLabels[i]->setBounds(paramBounds.removeFromTop(12));
            slotUI.paramSliders[i]->setBounds(paramBounds);
        }
    }
}

void ChimeraAudioProcessorEditor::timerCallback() {
    // Update level meter with current output level
    float currentLevel = audioProcessor.getCurrentOutputLevel();
    outputLevelMeter.setLevel(currentLevel);
    
    // Handle async network responses if needed
}

void ChimeraAudioProcessorEditor::generateButtonClicked() {
    auto prompt = promptBox.getText();
    if (prompt.isEmpty()) {
        setStatus("Please enter a prompt", true);
        return;
    }
    
    setStatus("Generating...");
    generateButton.setEnabled(false);
    
    // Create JSON request
    juce::DynamicObject::Ptr requestObj = new juce::DynamicObject();
    requestObj->setProperty("prompt", prompt);
    
    juce::var requestData(requestObj.get());
    juce::String jsonString = juce::JSON::toString(requestData);
    
    // Send request to AI server
    juce::URL url("http://localhost:8000/generate");
    url = url.withPOSTData(jsonString);
    
    // Create a thread to handle the request
    juce::Thread::launch([this, url, jsonString]() {
        juce::StringPairArray headers;
        headers.set("Content-Type", "application/json");
        
        juce::String headerString;
        for (int i = 0; i < headers.size(); ++i) {
            headerString += headers.getAllKeys()[i] + ": " + headers.getAllValues()[i] + "\n";
        }
        
        if (auto stream = url.createInputStream(juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                                                  .withConnectionTimeoutMs(10000)
                                                  .withExtraHeaders(headerString)
                                                  .withHttpRequestCmd("POST"))) {
            auto response = stream->readEntireStreamAsString();
            juce::MessageManager::callAsync([this, response] {
                handleAIResponse(response);
            });
        } else {
            juce::MessageManager::callAsync([this] {
                setStatus("Failed to connect to AI server", true);
                generateButton.setEnabled(true);
            });
        }
    });
}

void ChimeraAudioProcessorEditor::handleAIResponse(const juce::String& response) {
    // Log the response for debugging
    juce::Logger::writeToLog("AI Response: " + response.substring(0, 200));
    
    auto jsonResult = juce::JSON::parse(response);
    
    if (response.isEmpty()) {
        // No response - use fallback
        setStatus("No response from AI server - using fallback", true);
        currentPresetName = "Fallback Preset";
        presetNameLabel.setText(currentPresetName, juce::sendNotification);
        generateButton.setEnabled(true);
        return;
    }
    
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
        } else {
            // Generate a default description based on the prompt
            presetDescription = "This preset was created by the Trinity AI pipeline:\n\n"
                               "• Oracle: Analyzed your prompt and found similar presets\n"
                               "• Calculator: Applied intelligent parameter adjustments\n" 
                               "• Alchemist: Validated and optimized all parameters\n"
                               "• Visionary: Created the unique preset name\n\n"
                               "The result combines boutique analog warmth with modern precision.";
        }
        
        loadPresetFromJSON(preset);
        setStatus("Generated: " + currentPresetName);
    } else {
        // Log error
        juce::String errorMsg = jsonResult.hasProperty("message") ? 
                                jsonResult["message"].toString() : "Unknown error";
        juce::Logger::writeToLog("Generation failed: " + errorMsg);
        setStatus("Generation failed: " + errorMsg, true);
        
        // Use fallback preset
        currentPresetName = "Fallback Preset " + juce::String(juce::Random::getSystemRandom().nextInt(1000));
        presetNameLabel.setText(currentPresetName, juce::sendNotification);
    }
    
    generateButton.setEnabled(true);
}

void ChimeraAudioProcessorEditor::loadPresetFromJSON(const juce::var& preset) {
    if (!preset.hasProperty("parameters")) return;
    
    auto params = preset["parameters"];
    auto& valueTree = audioProcessor.getValueTreeState();
    
    // Load all parameters
    if (auto* dynObj = params.getDynamicObject()) {
        auto& properties = dynObj->getProperties();
        
        // Iterate through all properties
        for (auto& prop : properties) {
            auto paramID = prop.name.toString();
            auto value = prop.value;
            
            if (auto* param = valueTree.getParameter(paramID)) {
                float floatValue = 0.0f;
                if (value.isDouble()) floatValue = static_cast<float>(value);
                else if (value.isInt()) floatValue = static_cast<float>(static_cast<int>(value));
                else if (value.isInt64()) floatValue = static_cast<float>(static_cast<juce::int64>(value));
                
                param->setValueNotifyingHost(param->convertTo0to1(floatValue));
            }
        }
    }
    
    // Update macro controls if provided
    if (preset.hasProperty("macro_controls")) {
        updateMacroControls(preset["macro_controls"]);
    }
}

void ChimeraAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue) {
    // Check if it's an engine selector parameter
    for (int slot = 1; slot <= NUM_SLOTS; ++slot) {
        if (parameterID == "slot" + juce::String(slot) + "_engine") {
            juce::MessageManager::callAsync([this, slot] { updateSlotParameters(slot - 1); });
            break;
        }
    }
}

void ChimeraAudioProcessorEditor::updateSlotParameters(int slot) {
    auto& slotUI = slotUIs[slot];
    auto& engine = audioProcessor.getEngine(slot);
    
    if (!engine) return;
    
    int numParams = engine->getNumParameters();
    
    // Update parameter visibility and labels
    for (int i = 0; i < slotUI.paramSliders.size(); ++i) {
        bool visible = i < numParams;
        slotUI.paramSliders[i]->setVisible(visible);
        slotUI.paramLabels[i]->setVisible(visible);
        
        if (visible) {
            slotUI.paramLabels[i]->setText(engine->getParameterName(i), juce::dontSendNotification);
            
            // Special handling for specific engines
            int engineIndex = slotUI.engineSelector->getSelectedId() - 1;
            
            // Special handling for IntelligentHarmonizer (engine 33)
            if (engineIndex == 33) { // IntelligentHarmonizer
                // Get the actual engine to access its display methods
                auto* harmonizer = dynamic_cast<IntelligentHarmonizer*>(engine.get());
                if (harmonizer) {
                    // Set custom text from value function using engine's display strings
                    slotUI.paramSliders[i]->textFromValueFunction = [harmonizer, i](double value) {
                        return harmonizer->getParameterDisplayString(i, static_cast<float>(value));
                    };
                    
                    // Set value from text function for proper parsing
                    slotUI.paramSliders[i]->valueFromTextFunction = [](const juce::String& text) {
                        // For now, return 0.5 for any text - could be enhanced
                        return 0.5;
                    };
                    
                    // No decimal places for discrete parameters
                    slotUI.paramSliders[i]->setNumDecimalPlacesToDisplay(0);
                }
            }
            else if (engineIndex == 31) { // PitchShifter (Vocal Destroyer)
                // Get the actual engine to access its display methods
                auto* pitchShifter = dynamic_cast<PitchShifter*>(engine.get());
                if (pitchShifter) {
                    // Update parameter label based on current mode
                    slotUI.paramLabels[i]->setText(pitchShifter->getParameterName(i), 
                                                   juce::dontSendNotification);
                    
                    // Set custom text from value function using engine's display strings
                    slotUI.paramSliders[i]->textFromValueFunction = [pitchShifter, i](double value) {
                        return pitchShifter->getParameterDisplayString(i, static_cast<float>(value));
                    };
                    
                    // Special handling for mode selector (parameter 0)
                    if (i == 0) {
                        // Make mode selector snap to 3 positions
                        slotUI.paramSliders[i]->setRange(0.0, 1.0, 0.5);  // 3 positions: 0, 0.5, 1
                    }
                    
                    // Special handling for freeze in Glitch mode (parameter 3 when in mode 1)
                    // This is handled dynamically based on mode
                    
                    // No decimal places for any parameters
                    slotUI.paramSliders[i]->setNumDecimalPlacesToDisplay(0);
                }
            }
            else if (false && engineIndex == 31 && i == 0) { // Old PitchShifter code - disabled
                // Set 3 decimal places for pitch parameter
                slotUI.paramSliders[i]->setNumDecimalPlacesToDisplay(3);
                
                // Custom text from value function to show snapped values
                slotUI.paramSliders[i]->textFromValueFunction = [](double value) {
                    // Snap points for musical intervals
                    const float snapPoints[] = {
                        0.250f, 0.354f, 0.396f, 0.417f, 0.438f, 0.479f,
                        0.500f, 0.521f, 0.563f, 0.583f, 0.604f, 0.646f, 0.750f
                    };
                    
                    // Find closest snap point
                    float snappedValue = 0.5f;
                    float minDistance = 1.0f;
                    for (float snapPoint : snapPoints) {
                        float distance = std::abs(static_cast<float>(value) - snapPoint);
                        if (distance < minDistance) {
                            minDistance = distance;
                            snappedValue = snapPoint;
                        }
                    }
                    
                    return juce::String(snappedValue, 3);
                };
                
                // Also set the value from text function
                slotUI.paramSliders[i]->valueFromTextFunction = [](const juce::String& text) {
                    return text.getDoubleValue();
                };
            } else {
                // Default 2 decimal places for other parameters
                slotUI.paramSliders[i]->setNumDecimalPlacesToDisplay(2);
                slotUI.paramSliders[i]->textFromValueFunction = nullptr;
                slotUI.paramSliders[i]->valueFromTextFunction = nullptr;
            }
        }
    }
}

void ChimeraAudioProcessorEditor::updateMacroControls(const juce::var& macroData) {
    // Update macro control bindings based on AI response
    // This would map the macro controls to specific parameters
    // For now, just update the labels
    if (macroData.isArray()) {
        for (int i = 0; i < juce::jmin(3, macroData.size()); ++i) {
            auto macro = macroData[i];
            if (macro.hasProperty("name")) {
                macroControls[i].label->setText(macro["name"], juce::dontSendNotification);
            }
        }
    }
}

void ChimeraAudioProcessorEditor::setStatus(const juce::String& message, bool isError) {
    statusLabel.setText(message, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, 
                         isError ? juce::Colours::red : lookAndFeel.findColour(juce::Label::textColourId));
}

void ChimeraAudioProcessorEditor::applyRetrofuturistStyling() {
    // Apply the dark, high-tech aesthetic
    getLookAndFeel().setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff0a0a0a));
    
    // Style the generate button specially
    generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00d4ff).withAlpha(0.2f));
    generateButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00d4ff));
    
    // Style the title
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00d4ff));
    
    // Style the status label
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00d4ff).withAlpha(0.7f));
}

// Preset Management Methods
void ChimeraAudioProcessorEditor::showDetails() {
    auto* window = new DetailsWindow(currentPresetName, presetDescription);
    window->enterModalState(true, nullptr, true);
}

void ChimeraAudioProcessorEditor::savePreset() {
    juce::FileChooser chooser("Save Preset", 
                             juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                             "*.chimera");
    
    chooser.launchAsync(juce::FileBrowserComponent::saveMode, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file == juce::File{}) return;
        // TODO: Implement actual preset saving to file
        setStatus("Preset saved: " + file.getFileName());
    });
}

void ChimeraAudioProcessorEditor::loadPreset() {
    juce::FileChooser chooser("Load Preset",
                             juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                             "*.chimera");
    
    chooser.launchAsync(juce::FileBrowserComponent::openMode, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();
        if (file == juce::File{}) return;
        // TODO: Implement actual preset loading from file
        setStatus("Preset loaded: " + file.getFileName());
    });
}

void ChimeraAudioProcessorEditor::selectPresetA() {
    isPresetA = true;
    // TODO: Implement preset A recall in processor
    setStatus("Preset A selected");
}

void ChimeraAudioProcessorEditor::selectPresetB() {
    isPresetA = false;
    // TODO: Implement preset B recall in processor
    setStatus("Preset B selected");
}

void ChimeraAudioProcessorEditor::copyAtoB() {
    // TODO: Implement copy A to B in processor
    setStatus("Copied A → B");
}

// Details Window Implementation
ChimeraAudioProcessorEditor::DetailsWindow::DetailsWindow(const juce::String& presetName, 
                                                          const juce::String& description)
    : DocumentWindow("Preset Details: " + presetName, 
                    juce::Colour(0xff1a1a1a), 
                    DocumentWindow::closeButton) {
    
    auto* content = new juce::Component();
    content->setSize(500, 400);
    
    // Title label
    auto* titleLabel = new juce::Label("title", presetName);
    titleLabel->setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel->setColour(juce::Label::textColourId, juce::Colour(0xff00ff88));
    titleLabel->setJustificationType(juce::Justification::centred);
    titleLabel->setBounds(10, 10, 480, 40);
    content->addAndMakeVisible(titleLabel);
    
    // Section label
    auto* sectionLabel = new juce::Label("section", "AI Thought Process:");
    sectionLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    sectionLabel->setColour(juce::Label::textColourId, juce::Colour(0xff00d4ff));
    sectionLabel->setBounds(10, 60, 480, 25);
    content->addAndMakeVisible(sectionLabel);
    
    // Description text
    auto* textEditor = new juce::TextEditor();
    textEditor->setMultiLine(true);
    textEditor->setReadOnly(true);
    textEditor->setCaretVisible(false);
    textEditor->setText(description);
    textEditor->setFont(juce::Font(14.0f));
    textEditor->setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff0a0a0a));
    textEditor->setColour(juce::TextEditor::textColourId, juce::Colour(0xffcccccc));
    textEditor->setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff00d4ff).withAlpha(0.3f));
    textEditor->setBounds(10, 95, 480, 295);
    content->addAndMakeVisible(textEditor);
    
    setContentOwned(content, true);
    centreWithSize(500, 400);
    setVisible(true);
    setResizable(false, false);
}