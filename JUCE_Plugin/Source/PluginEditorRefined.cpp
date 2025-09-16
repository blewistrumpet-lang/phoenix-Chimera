#include "PluginEditorRefined.h"
#include "UnifiedDefaultParameters.h"
#include "ParameterFormatter.h"

//==============================================================================
// Main Editor Implementation
//==============================================================================

ChimeraAudioProcessorEditorRefined::ChimeraAudioProcessorEditorRefined(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&artisticLookAndFeel);
    
    // Create sections
    headerSection = std::make_unique<HeaderSection>();
    addAndMakeVisible(headerSection.get());
    
    aiPromptSection = std::make_unique<AIPromptSection>();
    aiPromptSection->onGenerate = [this](const juce::String& prompt) {
        handleAIPrompt(prompt);
    };
    addAndMakeVisible(aiPromptSection.get());
    
    // Create 6 slot components in 2x3 grid for compactness
    for (int i = 0; i < 6; ++i)
    {
        auto slot = std::make_unique<RefinedSlotComponent>(i, audioProcessor.getValueTreeState());
        addAndMakeVisible(slot.get());
        slotComponents.push_back(std::move(slot));
    }
    
    masterSection = std::make_unique<MasterSection>(audioProcessor.getValueTreeState());
    addAndMakeVisible(masterSection.get());
    
    // Start timer for updates
    startTimerHz(30);
    
    // Check server connection
    checkServerConnection();
    
    // Set compact size - fits on standard screens
    setSize(900, 650);
    setResizable(false, false);
}

ChimeraAudioProcessorEditorRefined::~ChimeraAudioProcessorEditorRefined()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ChimeraAudioProcessorEditorRefined::paint(juce::Graphics& g)
{
    drawBackground(g);
}

void ChimeraAudioProcessorEditorRefined::resized()
{
    auto bounds = getLocalBounds();
    
    // Header (compact - 50px)
    headerSection->setBounds(bounds.removeFromTop(50));
    
    // AI Prompt section (60px)
    aiPromptSection->setBounds(bounds.removeFromTop(60).reduced(10, 5));
    
    bounds.removeFromTop(5);
    
    // Master controls on left (150px wide)
    masterSection->setBounds(bounds.removeFromLeft(150));
    
    bounds.removeFromLeft(10);
    
    // Slots in 2x3 grid (remaining space)
    int slotWidth = bounds.getWidth() / 3;
    int slotHeight = bounds.getHeight() / 2;
    
    for (int i = 0; i < 6; ++i)
    {
        int col = i % 3;
        int row = i / 3;
        
        auto slotBounds = juce::Rectangle<int>(
            bounds.getX() + col * slotWidth,
            bounds.getY() + row * slotHeight,
            slotWidth, slotHeight
        ).reduced(5);
        
        slotComponents[i]->setBounds(slotBounds);
    }
}

void ChimeraAudioProcessorEditorRefined::timerCallback()
{
    // Update meters
    masterSection->setInputLevel(audioProcessor.getCurrentInputLevel());
    masterSection->setOutputLevel(audioProcessor.getCurrentOutputLevel());
    
    // Update slot activity
    for (int i = 0; i < 6; ++i)
    {
        slotComponents[i]->setProcessingLevel(audioProcessor.getSlotActivity(i));
    }
    
    // Periodic server check (every 2 seconds)
    static int checkCounter = 0;
    if (++checkCounter > 60)
    {
        checkCounter = 0;
        checkServerConnection();
    }
}

void ChimeraAudioProcessorEditorRefined::handleAIPrompt(const juce::String& prompt)
{
    if (prompt.isEmpty()) return;
    
    aiPromptSection->statusLabel.setText("Generating...", juce::dontSendNotification);
    
    // Create request
    juce::DynamicObject::Ptr requestObj = new juce::DynamicObject();
    requestObj->setProperty("prompt", prompt);
    
    juce::var requestData(requestObj.get());
    juce::String jsonString = juce::JSON::toString(requestData);
    
    // Try ports 8001 then 8000
    juce::Thread::launch([this, jsonString]() {
        for (int port : {8001, 8000})
        {
            juce::URL url("http://localhost:" + juce::String(port) + "/generate");
            url = url.withPOSTData(jsonString);
            
            juce::StringPairArray headers;
            headers.set("Content-Type", "application/json");
            
            juce::String headerString;
            for (int i = 0; i < headers.size(); ++i) {
                headerString += headers.getAllKeys()[i] + ": " + headers.getAllValues()[i] + "\n";
            }
            
            if (auto stream = url.createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(10000)
                    .withExtraHeaders(headerString)
                    .withHttpRequestCmd("POST")))
            {
                auto response = stream->readEntireStreamAsString();
                juce::MessageManager::callAsync([this, response] {
                    handleAIResponse(response);
                });
                return;
            }
        }
        
        // Connection failed
        juce::MessageManager::callAsync([this] {
            aiPromptSection->statusLabel.setText("Connection failed", juce::dontSendNotification);
        });
    });
}

void ChimeraAudioProcessorEditorRefined::handleAIResponse(const juce::String& response)
{
    auto json = juce::JSON::parse(response);
    
    if (json.hasProperty("success") && json["success"])
    {
        auto preset = json["preset"];
        
        // Update status
        aiPromptSection->statusLabel.setText("✓ " + preset["name"].toString(), 
                                            juce::dontSendNotification);
        
        // Load parameters
        if (preset.hasProperty("parameters"))
        {
            auto params = preset["parameters"];
            auto& valueTree = audioProcessor.getValueTreeState();
            
            if (auto* dynObj = params.getDynamicObject())
            {
                for (auto& prop : dynObj->getProperties())
                {
                    if (auto* param = valueTree.getParameter(prop.name.toString()))
                    {
                        float value = 0.0f;
                        if (prop.value.isDouble()) value = static_cast<float>(prop.value);
                        else if (prop.value.isInt()) value = static_cast<float>(static_cast<int>(prop.value));
                        
                        param->setValueNotifyingHost(param->convertTo0to1(value));
                    }
                }
            }
        }
        
        // Clear input after success
        aiPromptSection->promptInput.clear();
    }
    else
    {
        aiPromptSection->statusLabel.setText("Generation failed", juce::dontSendNotification);
    }
}

void ChimeraAudioProcessorEditorRefined::checkServerConnection()
{
    juce::Thread::launch([this]() {
        for (int port : {8001, 8000})
        {
            juce::URL url("http://localhost:" + juce::String(port) + "/health");
            
            if (auto stream = url.createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(500)))
            {
                auto response = stream->readEntireStreamAsString();
                auto json = juce::JSON::parse(response);
                
                if (json.hasProperty("status") && json["status"].toString() == "healthy")
                {
                    juce::MessageManager::callAsync([this] {
                        isServerConnected = true;
                        headerSection->aiStatusLED.setState(true, juce::Colour(0xff10b981));
                        aiPromptSection->statusLabel.setText("AI Ready", juce::dontSendNotification);
                    });
                    return;
                }
            }
        }
        
        juce::MessageManager::callAsync([this] {
            isServerConnected = false;
            headerSection->aiStatusLED.setState(false, juce::Colour(0xffef4444));
            aiPromptSection->statusLabel.setText("AI Offline", juce::dontSendNotification);
        });
    });
}

void ChimeraAudioProcessorEditorRefined::drawBackground(juce::Graphics& g)
{
    // Elegant gradient background
    juce::ColourGradient bgGradient(
        juce::Colour(0xff1a1a1f),
        0, 0,
        juce::Colour(0xff252530),
        getWidth(), getHeight(),
        false
    );
    g.setGradientFill(bgGradient);
    g.fillAll();
}

//==============================================================================
// HeaderSection Implementation
//==============================================================================

ChimeraAudioProcessorEditorRefined::HeaderSection::HeaderSection()
{
    logoLabel.setFont(juce::Font(juce::FontOptions().withHeight(24.0f)));
    logoLabel.setFont(logoLabel.getFont().boldened());
    logoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff6366f1));
    addAndMakeVisible(logoLabel);
    
    versionLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    versionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    addAndMakeVisible(versionLabel);
    
    aiStatusLabel.setFont(juce::Font(juce::FontOptions().withHeight(11.0f)));
    aiStatusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    addAndMakeVisible(aiStatusLabel);
    
    addAndMakeVisible(aiStatusLED);
}

void ChimeraAudioProcessorEditorRefined::HeaderSection::paint(juce::Graphics& g)
{
    // Subtle separator line
    g.setColour(juce::Colour(0xff2a2a35));
    g.drawHorizontalLine(getHeight() - 1, 0, getWidth());
}

void ChimeraAudioProcessorEditorRefined::HeaderSection::resized()
{
    auto bounds = getLocalBounds().reduced(15, 5);
    
    logoLabel.setBounds(bounds.removeFromLeft(150));
    versionLabel.setBounds(bounds.removeFromLeft(100));
    
    // AI status on right
    auto aiArea = bounds.removeFromRight(60);
    aiStatusLED.setBounds(aiArea.removeFromLeft(16).withSizeKeepingCentre(12, 12));
    aiStatusLabel.setBounds(aiArea);
}

void ChimeraAudioProcessorEditorRefined::HeaderSection::StatusLED::paint(juce::Graphics& g)
{
    if (auto* lnf = dynamic_cast<ArtisticLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawModernLED(g, getLocalBounds().toFloat(), isActive, ledColor);
    }
}

//==============================================================================
// AIPromptSection Implementation
//==============================================================================

ChimeraAudioProcessorEditorRefined::AIPromptSection::AIPromptSection()
{
    promptInput.setTextToShowWhenEmpty("Enter sound design prompt...", 
                                      juce::Colour(0xff9ca3af));
    promptInput.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    promptInput.setColour(juce::TextEditor::textColourId, juce::Colour(0xfff3f4f6));
    promptInput.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff252530));
    promptInput.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff6366f1).withAlpha(0.3f));
    promptInput.setReturnKeyStartsNewLine(false);
    promptInput.onReturnKey = [this] {
        if (onGenerate) onGenerate(promptInput.getText());
    };
    addAndMakeVisible(promptInput);
    
    generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff6366f1));
    generateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    generateButton.onClick = [this] {
        if (onGenerate) onGenerate(promptInput.getText());
    };
    addAndMakeVisible(generateButton);
    
    statusLabel.setFont(juce::Font(juce::FontOptions().withHeight(12.0f)));
    statusLabel.setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
    statusLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(statusLabel);
}

void ChimeraAudioProcessorEditorRefined::AIPromptSection::paint(juce::Graphics& g)
{
    // Glass panel background
    if (auto* lnf = dynamic_cast<ArtisticLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawGlassPanel(g, getLocalBounds().toFloat(), 8.0f, 0.03f);
    }
}

void ChimeraAudioProcessorEditorRefined::AIPromptSection::resized()
{
    auto bounds = getLocalBounds().reduced(10, 8);
    
    generateButton.setBounds(bounds.removeFromRight(80));
    bounds.removeFromRight(10);
    statusLabel.setBounds(bounds.removeFromRight(120));
    bounds.removeFromRight(10);
    promptInput.setBounds(bounds);
}

//==============================================================================
// RefinedSlotComponent Implementation
//==============================================================================

ChimeraAudioProcessorEditorRefined::RefinedSlotComponent::RefinedSlotComponent(
    int slotNumber, juce::AudioProcessorValueTreeState& apvts)
    : slotNum(slotNumber), valueTreeState(apvts)
{
    juce::String slotStr = juce::String(slotNum + 1);
    
    // Slot label
    slotLabel.setText("Slot " + slotStr, juce::dontSendNotification);
    slotLabel.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)));
    slotLabel.setFont(slotLabel.getFont().boldened());
    slotLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa78bfa));
    addAndMakeVisible(slotLabel);
    
    // Engine selector
    if (auto* engineParam = dynamic_cast<juce::AudioParameterChoice*>(
        valueTreeState.getParameter("slot" + slotStr + "_engine")))
    {
        for (int i = 0; i < engineParam->choices.size(); ++i)
        {
            engineSelector.addItem(engineParam->choices[i], i + 1);
        }
    }
    engineSelector.onChange = [this] { updateParameters(); };
    addAndMakeVisible(engineSelector);
    
    engineAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        valueTreeState, "slot" + slotStr + "_engine", engineSelector);
    
    // Bypass button
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffef4444));
    addAndMakeVisible(bypassButton);
    
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        valueTreeState, "slot" + slotStr + "_bypass", bypassButton);
    
    // Parameters will be created dynamically based on engine selection
    updateParameters();
}

ChimeraAudioProcessorEditorRefined::RefinedSlotComponent::~RefinedSlotComponent() = default;

void ChimeraAudioProcessorEditorRefined::RefinedSlotComponent::paint(juce::Graphics& g)
{
    // Glass panel background
    if (auto* lnf = dynamic_cast<ArtisticLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawGlassPanel(g, getLocalBounds().toFloat(), 8.0f, 0.03f);
    }
    
    // Activity indicator
    if (processingLevel > 0.01f)
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(juce::Colour(0xff6366f1).withAlpha(processingLevel * 0.2f));
        g.drawRoundedRectangle(bounds.reduced(1), 8.0f, 2.0f);
    }
}

void ChimeraAudioProcessorEditorRefined::RefinedSlotComponent::resized()
{
    layoutParameters();
}

void ChimeraAudioProcessorEditorRefined::RefinedSlotComponent::updateParameters()
{
    // Clear existing controls
    for (auto& control : paramControls)
    {
        if (control.slider) removeChildComponent(control.slider.get());
        if (control.label) removeChildComponent(control.label.get());
        if (control.toggleButton) removeChildComponent(control.toggleButton.get());
    }
    paramControls.clear();
    
    // Get selected engine
    int engineId = engineSelector.getSelectedId() - 2; // -1 for empty, 0+ for engines
    
    if (engineId < 0) // Empty slot
    {
        resized();
        repaint();
        return;
    }
    
    // Get parameter count for this engine
    int paramCount = UnifiedDefaultParameters::getParameterCount(engineId);
    paramCount = juce::jmin(paramCount, 15); // Max 15 params per slot
    
    juce::String slotStr = juce::String(slotNum + 1);
    
    for (int i = 0; i < paramCount; ++i)
    {
        ParamControl control;
        
        // Get parameter name
        juce::String paramName = juce::String(UnifiedDefaultParameters::getParameterName(engineId, i));
        
        // Determine if this should be a toggle button
        bool isToggle = paramName.containsIgnoreCase("enable") || 
                       paramName.containsIgnoreCase("bypass") ||
                       paramName.containsIgnoreCase("on/off") ||
                       paramName.containsIgnoreCase("freeze") ||
                       paramName.containsIgnoreCase("gate") ||
                       paramName.containsIgnoreCase("sync") ||
                       paramName.containsIgnoreCase("stereo") ||
                       paramName.containsIgnoreCase("mono");
        
        juce::String paramId = "slot" + slotStr + "_param" + juce::String(i + 1);
        
        if (isToggle)
        {
            // Create toggle button
            control.toggleButton = std::make_unique<juce::ToggleButton>(paramName);
            control.toggleButton->setColour(juce::ToggleButton::textColourId, juce::Colour(0xff9ca3af));
            addAndMakeVisible(control.toggleButton.get());
            
            // Attach to parameter
            if (valueTreeState.getParameter(paramId))
            {
                control.buttonAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
                    valueTreeState, paramId, *control.toggleButton);
            }
        }
        else
        {
            // Create slider
            control.slider = std::make_unique<juce::Slider>();
            control.slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
            control.slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            control.slider->setPopupDisplayEnabled(true, true, this);
            addAndMakeVisible(control.slider.get());
            
            // Create label
            control.label = std::make_unique<juce::Label>();
            control.label->setText(paramName, juce::dontSendNotification);
            control.label->setFont(juce::Font(juce::FontOptions().withHeight(10.0f)));
            control.label->setColour(juce::Label::textColourId, juce::Colour(0xff9ca3af));
            control.label->setJustificationType(juce::Justification::centred);
            addAndMakeVisible(control.label.get());
            
            // Attach to parameter
            if (valueTreeState.getParameter(paramId))
            {
                control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    valueTreeState, paramId, *control.slider);
            }
        }
        
        paramControls.push_back(std::move(control));
    }
    
    layoutParameters();
}

void ChimeraAudioProcessorEditorRefined::RefinedSlotComponent::layoutParameters()
{
    auto bounds = getLocalBounds().reduced(8);
    
    // Header row
    auto headerRow = bounds.removeFromTop(25);
    slotLabel.setBounds(headerRow.removeFromLeft(60));
    bypassButton.setBounds(headerRow.removeFromRight(60));
    headerRow.removeFromRight(5);
    engineSelector.setBounds(headerRow);
    
    bounds.removeFromTop(5);
    
    if (paramControls.empty())
        return;
    
    // Dynamic grid layout based on parameter count
    int numParams = static_cast<int>(paramControls.size());
    
    // Calculate optimal grid dimensions
    int cols, rows;
    if (numParams <= 3)
    {
        cols = numParams;
        rows = 1;
    }
    else if (numParams <= 6)
    {
        cols = 3;
        rows = 2;
    }
    else if (numParams <= 9)
    {
        cols = 3;
        rows = 3;
    }
    else if (numParams <= 12)
    {
        cols = 4;
        rows = 3;
    }
    else // 13-15 parameters
    {
        cols = 5;
        rows = 3;
    }
    
    int knobSize = juce::jmin(40, bounds.getWidth() / (cols + 1));
    int labelHeight = 12;
    int totalHeight = rows * (knobSize + labelHeight) + (rows - 1) * 5;
    int totalWidth = cols * knobSize + (cols - 1) * 5;
    
    // Center the grid
    int startX = bounds.getX() + (bounds.getWidth() - totalWidth) / 2;
    int startY = bounds.getY() + juce::jmin(5, (bounds.getHeight() - totalHeight) / 2);
    
    for (int i = 0; i < numParams; ++i)
    {
        int col = i % cols;
        int row = i / cols;
        
        int x = startX + col * (knobSize + 5);
        int y = startY + row * (knobSize + labelHeight + 5);
        
        if (paramControls[i].toggleButton)
        {
            // Toggle buttons get centered in their cell
            paramControls[i].toggleButton->setBounds(x, y + knobSize/4, knobSize, knobSize/2);
        }
        else
        {
            if (paramControls[i].slider)
                paramControls[i].slider->setBounds(x, y, knobSize, knobSize);
            if (paramControls[i].label)
                paramControls[i].label->setBounds(x, y + knobSize, knobSize, labelHeight);
        }
    }
}

//==============================================================================
// MasterSection Implementation
//==============================================================================

ChimeraAudioProcessorEditorRefined::MasterSection::MasterSection(juce::AudioProcessorValueTreeState& apvts)
{
    // Input gain
    inputGain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    inputGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    addAndMakeVisible(inputGain);
    addAndMakeVisible(inputLabel);
    
    // Output gain
    outputGain.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    outputGain.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    addAndMakeVisible(outputGain);
    addAndMakeVisible(outputLabel);
    
    // Mix
    mixKnob.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    mixKnob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    addAndMakeVisible(mixKnob);
    addAndMakeVisible(mixLabel);
    
    // Create attachments (would connect to actual parameters)
    // For now using placeholder parameter names
}

void ChimeraAudioProcessorEditorRefined::MasterSection::paint(juce::Graphics& g)
{
    // Glass panel
    if (auto* lnf = dynamic_cast<ArtisticLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawGlassPanel(g, getLocalBounds().toFloat(), 8.0f, 0.03f);
    }
    
    // Title
    g.setFont(juce::Font(juce::FontOptions().withHeight(14.0f)).boldened());
    g.setColour(juce::Colour(0xffa78bfa));
    g.drawText("Master", getLocalBounds().removeFromTop(25), juce::Justification::centred);
    
    // Draw meters
    auto meterBounds = getLocalBounds().removeFromBottom(80).reduced(10, 5);
    auto inputMeterBounds = meterBounds.removeFromLeft(meterBounds.getWidth() / 2 - 5);
    auto outputMeterBounds = meterBounds.removeFromLeft(meterBounds.getWidth());
    
    drawMeter(g, inputMeterBounds.toFloat(), inputMeter, true);
    drawMeter(g, outputMeterBounds.toFloat(), outputMeter, false);
}

void ChimeraAudioProcessorEditorRefined::MasterSection::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(30); // Title space
    
    // Knobs
    auto knobSection = bounds.removeFromTop(200);
    int knobSize = 60;
    int spacing = 15;
    
    auto knobBounds = knobSection.withSizeKeepingCentre(knobSize, knobSize * 3 + spacing * 2);
    
    inputLabel.setBounds(knobBounds.removeFromTop(15));
    inputGain.setBounds(knobBounds.removeFromTop(knobSize));
    knobBounds.removeFromTop(spacing);
    
    outputLabel.setBounds(knobBounds.removeFromTop(15));
    outputGain.setBounds(knobBounds.removeFromTop(knobSize));
    knobBounds.removeFromTop(spacing);
    
    mixLabel.setBounds(knobBounds.removeFromTop(15));
    mixKnob.setBounds(knobBounds.removeFromTop(knobSize));
}

void ChimeraAudioProcessorEditorRefined::MasterSection::drawMeter(juce::Graphics& g, 
                                                                 juce::Rectangle<float> bounds,
                                                                 float level, bool isInput)
{
    // Background
    g.setColour(juce::Colour(0xff1a1a1f));
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Level
    if (level > 0.01f)
    {
        float db = juce::Decibels::gainToDecibels(level);
        float normalized = juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        
        auto levelBounds = bounds.removeFromBottom(bounds.getHeight() * normalized);
        
        juce::Colour meterColor = db > -3.0f ? juce::Colour(0xffef4444) :
                                  db > -12.0f ? juce::Colour(0xfff59e0b) :
                                  juce::Colour(0xff10b981);
        
        g.setColour(meterColor);
        g.fillRoundedRectangle(levelBounds, 2.0f);
    }
    
    // Label
    g.setFont(juce::Font(juce::FontOptions().withHeight(9.0f)));
    g.setColour(juce::Colour(0xff9ca3af));
    g.drawText(isInput ? "IN" : "OUT", bounds, juce::Justification::centredBottom);
}