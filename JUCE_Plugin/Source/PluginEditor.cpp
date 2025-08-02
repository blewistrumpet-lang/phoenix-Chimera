#include "PluginProcessor.h"
#include "PluginEditor.h"

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
        slotUI.engineSelector->addItem("Bypass", ENGINE_BYPASS + 2);
        slotUI.engineSelector->addItem("K-Style Overdrive", ENGINE_K_STYLE + 2);
        slotUI.engineSelector->addItem("Tape Echo", ENGINE_TAPE_ECHO + 2);
        slotUI.engineSelector->addItem("Plate Reverb", ENGINE_PLATE_REVERB + 2);
        slotUI.engineSelector->addItem("Rodent Distortion", ENGINE_RODENT_DISTORTION + 2);
        slotUI.engineSelector->addItem("Muff Fuzz", ENGINE_MUFF_FUZZ + 2);
        slotUI.engineSelector->addItem("Classic Tremolo", ENGINE_CLASSIC_TREMOLO + 2);
        slotUI.engineSelector->addItem("Magnetic Drum Echo", ENGINE_MAGNETIC_DRUM_ECHO + 2);
        slotUI.engineSelector->addItem("Bucket Brigade Delay", ENGINE_BUCKET_BRIGADE_DELAY + 2);
        slotUI.engineSelector->addItem("Digital Delay", ENGINE_DIGITAL_DELAY + 2);
        slotUI.engineSelector->addItem("Harmonic Tremolo", ENGINE_HARMONIC_TREMOLO + 2);
        slotUI.engineSelector->addItem("Rotary Speaker", ENGINE_ROTARY_SPEAKER + 2);
        slotUI.engineSelector->addItem("Detune Doubler", ENGINE_DETUNE_DOUBLER + 2);
        slotUI.engineSelector->addItem("Ladder Filter", ENGINE_LADDER_FILTER + 2);
        slotUI.engineSelector->addItem("Formant Filter", ENGINE_FORMANT_FILTER + 2);
        slotUI.engineSelector->addItem("Classic Compressor", ENGINE_CLASSIC_COMPRESSOR + 2);
        slotUI.engineSelector->addItem("State Variable Filter", ENGINE_STATE_VARIABLE_FILTER + 2);
        slotUI.engineSelector->addItem("Stereo Chorus", ENGINE_STEREO_CHORUS + 2);
        slotUI.engineSelector->addItem("Spectral Freeze", ENGINE_SPECTRAL_FREEZE + 2);
        slotUI.engineSelector->addItem("Granular Cloud", ENGINE_GRANULAR_CLOUD + 2);
        slotUI.engineSelector->addItem("Analog Ring Modulator", ENGINE_ANALOG_RING_MODULATOR + 2);
        slotUI.engineSelector->addItem("Multiband Saturator", ENGINE_MULTIBAND_SATURATOR + 2);
        slotUI.engineSelector->addItem("Comb Resonator", ENGINE_COMB_RESONATOR + 2);
        slotUI.engineSelector->addItem("Pitch Shifter", ENGINE_PITCH_SHIFTER + 2);
        slotUI.engineSelector->addItem("Phased Vocoder", ENGINE_PHASED_VOCODER + 2);
        slotUI.engineSelector->addItem("Convolution Reverb", ENGINE_CONVOLUTION_REVERB + 2);
        slotUI.engineSelector->addItem("Bit Crusher", ENGINE_BIT_CRUSHER + 2);
        slotUI.engineSelector->addItem("Frequency Shifter", ENGINE_FREQUENCY_SHIFTER + 2);
        slotUI.engineSelector->addItem("Wave Folder", ENGINE_WAVE_FOLDER + 2);
        slotUI.engineSelector->addItem("Shimmer Reverb", ENGINE_SHIMMER_REVERB + 2);
        slotUI.engineSelector->addItem("Vocal Formant Filter", ENGINE_VOCAL_FORMANT_FILTER + 2);
        slotUI.engineSelector->addItem("Transient Shaper", ENGINE_TRANSIENT_SHAPER + 2);
        slotUI.engineSelector->addItem("Dimension Expander", ENGINE_DIMENSION_EXPANDER + 2);
        slotUI.engineSelector->addItem("Analog Phaser", ENGINE_ANALOG_PHASER + 2);
        slotUI.engineSelector->addItem("Envelope Filter", ENGINE_ENVELOPE_FILTER + 2);
        slotUI.engineSelector->addItem("Gated Reverb", ENGINE_GATED_REVERB + 2);
        slotUI.engineSelector->addItem("Harmonic Exciter", ENGINE_HARMONIC_EXCITER + 2);
        slotUI.engineSelector->addItem("Feedback Network", ENGINE_FEEDBACK_NETWORK + 2);
        slotUI.engineSelector->addItem("Intelligent Harmonizer", ENGINE_INTELLIGENT_HARMONIZER + 2);
        slotUI.engineSelector->addItem("Parametric EQ", ENGINE_PARAMETRIC_EQ + 2);
        slotUI.engineSelector->addItem("Mastering Limiter", ENGINE_MASTERING_LIMITER + 2);
        slotUI.engineSelector->addItem("Noise Gate", ENGINE_NOISE_GATE + 2);
        slotUI.engineSelector->addItem("Vintage Opto", ENGINE_VINTAGE_OPTO_COMPRESSOR + 2);
        slotUI.engineSelector->addItem("Spectral Gate", ENGINE_SPECTRAL_GATE + 2);
        slotUI.engineSelector->addItem("Chaos Generator", ENGINE_CHAOS_GENERATOR + 2);
        slotUI.engineSelector->addItem("Buffer Repeat", ENGINE_BUFFER_REPEAT + 2);
        slotUI.engineSelector->addItem("Vintage Console EQ", ENGINE_VINTAGE_CONSOLE_EQ + 2);
        slotUI.engineSelector->addItem("Mid/Side Processor", ENGINE_MID_SIDE_PROCESSOR + 2);
        slotUI.engineSelector->addItem("Vintage Tube Preamp", ENGINE_VINTAGE_TUBE_PREAMP + 2);
        slotUI.engineSelector->addItem("Spring Reverb", ENGINE_SPRING_REVERB + 2);
        slotUI.engineSelector->addItem("Resonant Chorus", ENGINE_RESONANT_CHORUS + 2);
        slotUI.slotPanel.addAndMakeVisible(slotUI.engineSelector.get());
        
        slotUI.engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
            audioProcessor.getValueTreeState(), "slot" + slotStr + "_engine", *slotUI.engineSelector);
        
        // Bypass button
        slotUI.bypassButton = std::make_unique<juce::ToggleButton>("Bypass");
        slotUI.slotPanel.addAndMakeVisible(slotUI.bypassButton.get());
        
        slotUI.bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getValueTreeState(), "slot" + slotStr + "_bypass", *slotUI.bypassButton);
        
        // Create parameter sliders
        for (int i = 0; i < 10; ++i) {
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
    
    // Title across the top
    titleLabel.setBounds(topSection.removeFromTop(40));
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
        
        // Parameter sliders in 2 rows of 5
        auto paramArea = panelBounds.reduced(5);
        int paramWidth = paramArea.getWidth() / 5;
        int paramHeight = paramArea.getHeight() / 2;
        
        for (int i = 0; i < 10; ++i) {
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
    auto jsonResult = juce::JSON::parse(response);
    
    if (jsonResult.hasProperty("success") && jsonResult["success"]) {
        auto preset = jsonResult["preset"];
        loadPresetFromJSON(preset);
        setStatus("Preset generated successfully!");
    } else {
        setStatus("Generation failed: " + jsonResult["message"].toString(), true);
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