#include "PluginEditorSkunkworks.h"

//==============================================================================
// Main Editor Implementation
//==============================================================================

ChimeraAudioProcessorEditorSkunkworks::ChimeraAudioProcessorEditorSkunkworks(ChimeraAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&skunkworksLookAndFeel);
    
    // Header
    addAndMakeVisible(headerPanel);
    
    // Control panel
    controlPanel = std::make_unique<ControlPanel>(audioProcessor.getValueTreeState());
    addAndMakeVisible(controlPanel.get());
    
    // Rack panel with slots
    rackPanel = std::make_unique<RackPanel>(audioProcessor, audioProcessor.getValueTreeState());
    addAndMakeVisible(rackPanel.get());
    
    // Command terminal (initially hidden)
    commandTerminal = std::make_unique<CommandTerminal>();
    commandTerminal->onCommandExecute = [this](const juce::String& cmd) {
        handleAIGenerate(cmd);
    };
    commandTerminal->setVisible(false);
    addAndMakeVisible(commandTerminal.get());
    
    // Meters panel
    metersPanel = std::make_unique<MetersPanel>();
    addAndMakeVisible(metersPanel.get());
    
    // Terminal toggle button
    terminalToggleButton.setColour(juce::TextButton::buttonColourId, 
                                  juce::Colour(SkunkworksLookAndFeel::ColorScheme::panelMetal));
    terminalToggleButton.onClick = [this] {
        isCommandTerminalVisible = !isCommandTerminalVisible;
        commandTerminal->setVisible(isCommandTerminalVisible);
        resized();
    };
    addAndMakeVisible(terminalToggleButton);
    
    // Initialize star field for background
    juce::Random rand;
    for (int i = 0; i < 50; ++i)
    {
        starField.push_back(juce::Point<float>(
            rand.nextFloat() * 1400,
            rand.nextFloat() * 900
        ));
    }
    
    // Start animation timer
    startTimerHz(30);
    
    // Check AI server connection on startup
    checkAIServerConnection();
    
    // Set initial size
    setSize(1400, 900);
    setResizable(true, true);
    setResizeLimits(1200, 800, 2000, 1200);
}

ChimeraAudioProcessorEditorSkunkworks::~ChimeraAudioProcessorEditorSkunkworks()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void ChimeraAudioProcessorEditorSkunkworks::paint(juce::Graphics& g)
{
    drawBackground(g);
    
    // Main frame
    auto bounds = getLocalBounds().toFloat().reduced(10);
    drawMetalFrame(g, bounds, "CHIMERA COMMAND CENTER");
}

void ChimeraAudioProcessorEditorSkunkworks::resized()
{
    auto bounds = getLocalBounds().reduced(15);
    
    // Header strip
    headerPanel.setBounds(bounds.removeFromTop(60));
    bounds.removeFromTop(10);
    
    // Terminal toggle in corner
    terminalToggleButton.setBounds(bounds.removeFromTop(25).removeFromRight(100));
    
    if (isCommandTerminalVisible)
    {
        // Split view when terminal is visible
        auto leftPanel = bounds.removeFromLeft(bounds.getWidth() * 0.4);
        commandTerminal->setBounds(leftPanel);
        bounds.removeFromLeft(10);
    }
    
    // Control panel
    auto controlBounds = bounds.removeFromTop(120);
    controlPanel->setBounds(controlBounds.removeFromLeft(controlBounds.getWidth() * 0.7));
    controlBounds.removeFromLeft(10);
    metersPanel->setBounds(controlBounds);
    
    bounds.removeFromTop(10);
    
    // Rack panel fills remaining space
    rackPanel->setBounds(bounds);
}

void ChimeraAudioProcessorEditorSkunkworks::timerCallback()
{
    // Update meters
    float inputL = audioProcessor.getCurrentInputLevel();
    float inputR = inputL; // Mono for now
    float outputL = audioProcessor.getCurrentOutputLevel();
    float outputR = outputL;
    
    metersPanel->setLevels(inputL, inputR, outputL, outputR);
    
    // Update slot activity
    for (int i = 0; i < 6; ++i)
    {
        float activity = audioProcessor.getSlotActivity(i);
        rackPanel->updateSlotActivity(i, activity);
    }
    
    // Background animation
    backgroundPulse += 0.02f;
    if (backgroundPulse > juce::MathConstants<float>::twoPi)
        backgroundPulse -= juce::MathConstants<float>::twoPi;
    
    // Update status LEDs
    headerPanel.powerLED.setState(true);
    headerPanel.audioLED.setState(audioProcessor.getCurrentOutputLevel() > 0.01f);
    headerPanel.aiLED.setState(isAIServerConnected, 
        isAIServerConnected ? juce::Colour(0xff00ff44) : juce::Colour(0xffff2222));
    
    // Periodically check server connection (every 2 seconds)
    static int connectionCheckCounter = 0;
    if (++connectionCheckCounter > 60) // 30Hz * 2 seconds
    {
        connectionCheckCounter = 0;
        checkAIServerConnection();
    }
}

void ChimeraAudioProcessorEditorSkunkworks::handleAIGenerate(const juce::String& prompt)
{
    commandTerminal->setStatus("CONNECTING TO AI SERVER...", false);
    
    // Create JSON request
    juce::DynamicObject::Ptr requestObj = new juce::DynamicObject();
    requestObj->setProperty("prompt", prompt);
    
    juce::var requestData(requestObj.get());
    juce::String jsonString = juce::JSON::toString(requestData);
    
    // Send to AI server (check port 8001 first, then 8000)
    juce::URL url("http://localhost:8001/generate");
    url = url.withPOSTData(jsonString);
    
    juce::Thread::launch([this, url, jsonString]() {
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
        } 
        else 
        {
            juce::MessageManager::callAsync([this] {
                commandTerminal->setStatus("CONNECTION FAILED", true);
                commandTerminal->addOutput("Error: Could not connect to AI server", true);
                commandTerminal->showTypingAnimation(false);
            });
        }
    });
    
    commandTerminal->showTypingAnimation(true);
}

void ChimeraAudioProcessorEditorSkunkworks::handleAIResponse(const juce::String& response)
{
    commandTerminal->showTypingAnimation(false);
    
    auto jsonResult = juce::JSON::parse(response);
    
    if (jsonResult.hasProperty("success") && jsonResult["success"])
    {
        auto preset = jsonResult["preset"];
        
        commandTerminal->setStatus("PRESET GENERATED", false);
        commandTerminal->addOutput("Success: " + preset["name"].toString());
        commandTerminal->addOutput(preset["description"].toString());
        
        // Load the preset into the processor
        if (preset.hasProperty("parameters"))
        {
            auto params = preset["parameters"];
            auto& valueTree = audioProcessor.getValueTreeState();
            
            if (auto* dynObj = params.getDynamicObject())
            {
                for (auto& prop : dynObj->getProperties())
                {
                    auto paramID = prop.name.toString();
                    auto value = prop.value;
                    
                    if (auto* param = valueTree.getParameter(paramID))
                    {
                        float floatValue = 0.0f;
                        if (value.isDouble()) floatValue = static_cast<float>(value);
                        else if (value.isInt()) floatValue = static_cast<float>(static_cast<int>(value));
                        
                        param->setValueNotifyingHost(param->convertTo0to1(floatValue));
                    }
                }
            }
        }
    }
    else
    {
        commandTerminal->setStatus("GENERATION FAILED", true);
        commandTerminal->addOutput("Error: " + 
            (jsonResult.hasProperty("message") ? jsonResult["message"].toString() : "Unknown error"), 
            true);
    }
}

void ChimeraAudioProcessorEditorSkunkworks::checkAIServerConnection()
{
    // Check server health asynchronously
    juce::Thread::launch([this]() {
        // Try port 8001 first, then 8000
        for (int port : {8001, 8000})
        {
            juce::URL healthUrl("http://localhost:" + juce::String(port) + "/health");
            
            if (auto stream = healthUrl.createInputStream(
                juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs(500)))
            {
                auto response = stream->readEntireStreamAsString();
                auto json = juce::JSON::parse(response);
                
                if (json.hasProperty("status") && json["status"].toString() == "healthy")
                {
                    juce::MessageManager::callAsync([this, port]() {
                        isAIServerConnected = true;
                        commandTerminal->setStatus("AI SERVER ONLINE [PORT " + juce::String(port) + "]", false);
                        commandTerminal->addOutput("Trinity Pipeline connected and ready");
                    });
                    return;
                }
            }
        }
        
        // No server found
        juce::MessageManager::callAsync([this]() {
            isAIServerConnected = false;
            commandTerminal->setStatus("AI SERVER OFFLINE", true);
            commandTerminal->addOutput("Warning: Trinity AI server not detected", true);
            commandTerminal->addOutput("Start server with: python3 Trinity_AI_Pipeline/main.py");
        });
    });
}

void ChimeraAudioProcessorEditorSkunkworks::drawBackground(juce::Graphics& g)
{
    // Dark military background
    g.fillAll(juce::Colour(SkunkworksLookAndFeel::ColorScheme::panelBackground));
    
    // Subtle grid pattern
    g.setColour(juce::Colour(0xff0a0a0a));
    for (int x = 0; x < getWidth(); x += 50)
    {
        g.drawVerticalLine(x, 0, getHeight());
    }
    for (int y = 0; y < getHeight(); y += 50)
    {
        g.drawHorizontalLine(y, 0, getWidth());
    }
    
    // Animated star field for depth
    g.setColour(juce::Colours::white.withAlpha(0.3f));
    for (auto& star : starField)
    {
        float twinkle = std::sin(backgroundPulse + star.x * 0.01f) * 0.5f + 0.5f;
        g.setOpacity(twinkle * 0.3f);
        g.fillEllipse(star.x, star.y, 2, 2);
    }
}

void ChimeraAudioProcessorEditorSkunkworks::drawMetalFrame(juce::Graphics& g, 
                                                           juce::Rectangle<float> bounds,
                                                           const juce::String& label)
{
    // Metal frame with rivets
    skunkworksLookAndFeel.drawMetalPanel(g, bounds, false);
    
    // Label plate if provided
    if (label.isNotEmpty())
    {
        auto labelBounds = bounds.removeFromTop(25).reduced(bounds.getWidth() * 0.3f, 0);
        skunkworksLookAndFeel.drawMetalPanel(g, labelBounds, true);
        
        g.setFont(skunkworksLookAndFeel.getStencilFont(14.0f));
        g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::textStencil));
        g.drawText(label, labelBounds, juce::Justification::centred);
    }
}

//==============================================================================
// HeaderPanel Implementation
//==============================================================================

ChimeraAudioProcessorEditorSkunkworks::HeaderPanel::HeaderPanel()
{
    titleLabel.setFont(juce::Font("Arial Black", 28.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);
    
    subtitleLabel.setFont(juce::Font("Arial", 12.0f, juce::Font::plain));
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(subtitleLabel);
    
    versionLabel.setFont(juce::Font("Courier New", 10.0f, juce::Font::plain));
    versionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(versionLabel);
    
    addAndMakeVisible(powerLED);
    addAndMakeVisible(aiLED);
    addAndMakeVisible(audioLED);
}

void ChimeraAudioProcessorEditorSkunkworks::HeaderPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Header panel background
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawMetalPanel(g, bounds, false);
    }
    
    // Warning stripes
    g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed).withAlpha(0.3f));
    for (int i = 0; i < 3; ++i)
    {
        auto stripe = bounds.removeFromBottom(2);
        bounds.removeFromBottom(2);
        g.fillRect(stripe);
    }
}

void ChimeraAudioProcessorEditorSkunkworks::HeaderPanel::resized()
{
    auto bounds = getLocalBounds().reduced(15, 5);
    
    auto leftSection = bounds.removeFromLeft(bounds.getWidth() * 0.6);
    titleLabel.setBounds(leftSection.removeFromTop(30));
    subtitleLabel.setBounds(leftSection);
    
    auto rightSection = bounds;
    versionLabel.setBounds(rightSection.removeFromTop(20));
    
    // Status LEDs
    rightSection.removeFromTop(5);
    auto ledRow = rightSection.removeFromTop(20).removeFromRight(100);
    
    powerLED.setBounds(ledRow.removeFromLeft(20));
    ledRow.removeFromLeft(10);
    aiLED.setBounds(ledRow.removeFromLeft(20));
    ledRow.removeFromLeft(10);
    audioLED.setBounds(ledRow.removeFromLeft(20));
}

void ChimeraAudioProcessorEditorSkunkworks::HeaderPanel::StatusLED::paint(juce::Graphics& g)
{
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawLEDIndicator(g, getLocalBounds().toFloat(), isActive, ledColor);
    }
}

void ChimeraAudioProcessorEditorSkunkworks::HeaderPanel::StatusLED::setState(bool active, juce::Colour color)
{
    isActive = active;
    ledColor = color;
    repaint();
}

//==============================================================================
// ControlPanel Implementation
//==============================================================================

ChimeraAudioProcessorEditorSkunkworks::ControlPanel::ControlPanel(juce::AudioProcessorValueTreeState& apvts)
{
    addAndMakeVisible(inputGainKnob);
    addAndMakeVisible(outputGainKnob);
    addAndMakeVisible(mixKnob);
    
    bypassButton.setColour(juce::ToggleButton::textColourId, 
                          juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed));
    addAndMakeVisible(bypassButton);
    
    panicButton.setColour(juce::ToggleButton::textColourId, 
                         juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed));
    addAndMakeVisible(panicButton);
    
    // Preset controls
    presetSelector.addItem("Init", 1);
    presetSelector.addItem("Warm Vintage", 2);
    presetSelector.addItem("Modern Crush", 3);
    presetSelector.setSelectedId(1);
    addAndMakeVisible(presetSelector);
    
    addAndMakeVisible(saveButton);
    addAndMakeVisible(loadButton);
    
    // A/B comparison
    compareAButton.setRadioGroupId(1001);
    compareAButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(compareAButton);
    
    compareBButton.setRadioGroupId(1001);
    addAndMakeVisible(compareBButton);
    
    addAndMakeVisible(copyButton);
}

void ChimeraAudioProcessorEditorSkunkworks::ControlPanel::paint(juce::Graphics& g)
{
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawMetalPanel(g, getLocalBounds().toFloat(), false);
    }
}

void ChimeraAudioProcessorEditorSkunkworks::ControlPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Master controls
    auto knobSection = bounds.removeFromLeft(250);
    auto knobRow = knobSection.removeFromTop(80);
    
    inputGainKnob.setBounds(knobRow.removeFromLeft(80));
    outputGainKnob.setBounds(knobRow.removeFromLeft(80));
    mixKnob.setBounds(knobRow.removeFromLeft(80));
    
    // Bypass buttons
    auto buttonRow = knobSection;
    bypassButton.setBounds(buttonRow.removeFromLeft(80));
    panicButton.setBounds(buttonRow.removeFromLeft(80));
    
    bounds.removeFromLeft(20);
    
    // Preset section
    auto presetSection = bounds.removeFromLeft(300);
    presetSelector.setBounds(presetSection.removeFromTop(30));
    presetSection.removeFromTop(5);
    
    auto presetButtons = presetSection.removeFromTop(30);
    saveButton.setBounds(presetButtons.removeFromLeft(70));
    presetButtons.removeFromLeft(5);
    loadButton.setBounds(presetButtons.removeFromLeft(70));
    
    bounds.removeFromLeft(20);
    
    // A/B section
    auto abSection = bounds.removeFromLeft(200);
    auto abButtons = abSection.removeFromTop(30);
    
    compareAButton.setBounds(abButtons.removeFromLeft(40));
    compareBButton.setBounds(abButtons.removeFromLeft(40));
    abButtons.removeFromLeft(10);
    copyButton.setBounds(abButtons.removeFromLeft(60));
}

//==============================================================================
// RackPanel Implementation
//==============================================================================

ChimeraAudioProcessorEditorSkunkworks::RackPanel::RackPanel(ChimeraAudioProcessor& processor,
                                                           juce::AudioProcessorValueTreeState& apvts)
{
    for (int i = 0; i < 6; ++i)
    {
        auto slot = std::make_unique<ChimeraSlotComponent>(i, apvts, 
            [&processor, i](int engineIndex) {
                // Engine change callback - convert choice index to engine ID
                int engineID = processor.choiceIndexToEngineID(engineIndex);
                processor.setSlotEngine(i, engineID);
            });
        addAndMakeVisible(slot.get());
        slots.push_back(std::move(slot));
    }
}

void ChimeraAudioProcessorEditorSkunkworks::RackPanel::paint(juce::Graphics& g)
{
    drawRackFrame(g);
}

void ChimeraAudioProcessorEditorSkunkworks::RackPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // 3x2 grid layout
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
        
        slots[i]->setBounds(slotBounds);
    }
}

void ChimeraAudioProcessorEditorSkunkworks::RackPanel::updateSlotActivity(int slot, float level)
{
    if (slot >= 0 && slot < slots.size())
    {
        slots[slot]->setProcessingLevel(level);
    }
}

void ChimeraAudioProcessorEditorSkunkworks::RackPanel::drawRackFrame(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Rack mounting rails
    g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::screwMetal));
    g.fillRect(bounds.removeFromLeft(5));
    g.fillRect(bounds.removeFromRight(5));
    
    // Rack screws
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        for (int y = 20; y < getHeight(); y += 40)
        {
            lnf->drawScrew(g, 2, y, 6);
            lnf->drawScrew(g, getWidth() - 8, y, 6);
        }
    }
}

//==============================================================================
// MetersPanel Implementation
//==============================================================================

ChimeraAudioProcessorEditorSkunkworks::MetersPanel::MetersPanel()
{
    startTimerHz(60);
}

ChimeraAudioProcessorEditorSkunkworks::MetersPanel::~MetersPanel()
{
    stopTimer();
}

void ChimeraAudioProcessorEditorSkunkworks::MetersPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    if (auto* lnf = dynamic_cast<SkunkworksLookAndFeel*>(&getLookAndFeel()))
    {
        lnf->drawMetalPanel(g, bounds, true);
    }
    
    bounds.reduce(10, 10);
    
    // Draw meters
    auto meterWidth = bounds.getWidth() / 2 - 5;
    
    auto inputBounds = bounds.removeFromLeft(meterWidth);
    inputMeter.paint(g, inputBounds, "INPUT");
    
    bounds.removeFromLeft(10);
    
    auto outputBounds = bounds.removeFromLeft(meterWidth);
    outputMeter.paint(g, outputBounds, "OUTPUT");
}

void ChimeraAudioProcessorEditorSkunkworks::MetersPanel::setLevels(float inputL, float inputR, 
                                                                  float outputL, float outputR)
{
    inputMeter.leftLevel = inputL;
    inputMeter.rightLevel = inputR;
    outputMeter.leftLevel = outputL;
    outputMeter.rightLevel = outputR;
}

void ChimeraAudioProcessorEditorSkunkworks::MetersPanel::timerCallback()
{
    inputMeter.update();
    outputMeter.update();
    repaint();
}

void ChimeraAudioProcessorEditorSkunkworks::MetersPanel::StereoMeter::update()
{
    // Smooth decay
    float decay = 0.92f;
    float attack = 1.0f;
    
    float targetL = leftLevel.load();
    float targetR = rightLevel.load();
    
    leftDisplay = targetL > leftDisplay ? targetL : leftDisplay * decay;
    rightDisplay = targetR > rightDisplay ? targetR : rightDisplay * decay;
}

void ChimeraAudioProcessorEditorSkunkworks::MetersPanel::StereoMeter::paint(juce::Graphics& g, 
                                                                           juce::Rectangle<float> bounds,
                                                                           const juce::String& label,
                                                                           bool showPeak)
{
    // Label
    g.setFont(juce::Font("Arial Black", 10.0f, juce::Font::plain));
    g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::textStencil));
    g.drawText(label, bounds.removeFromTop(15), juce::Justification::centred);
    
    bounds.removeFromTop(5);
    
    // Meter backgrounds
    auto leftBounds = bounds.removeFromLeft(bounds.getWidth() / 2 - 2).reduced(2);
    auto rightBounds = bounds.removeFromLeft(bounds.getWidth() - 2).reduced(2);
    
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(leftBounds, 2.0f);
    g.fillRoundedRectangle(rightBounds, 2.0f);
    
    // Draw levels
    auto drawLevel = [&g](juce::Rectangle<float> meterBounds, float level) {
        float dbLevel = juce::Decibels::gainToDecibels(level);
        float normalized = juce::jmap(dbLevel, -60.0f, 0.0f, 0.0f, 1.0f);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        
        if (normalized > 0.01f)
        {
            auto levelBounds = meterBounds.removeFromBottom(meterBounds.getHeight() * normalized);
            
            if (dbLevel > -3.0f)
                g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::warningRed));
            else if (dbLevel > -12.0f)
                g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::amberLED));
            else
                g.setColour(juce::Colour(SkunkworksLookAndFeel::ColorScheme::greenLED));
            
            g.fillRoundedRectangle(levelBounds, 1.0f);
        }
    };
    
    drawLevel(leftBounds, leftDisplay);
    drawLevel(rightBounds, rightDisplay);
}